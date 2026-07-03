# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Build & Test Commands

CMake Presets v6, generator **Ninja**, output `out/build/${presetName}`. Preset family: `base` (hidden) → platform-specific configure presets → corresponding `build-*` / `test-*` presets.

### Common workflows

```bash
# Configure
cmake --preset <preset-name>

# Build
cmake --build --preset build-<preset-name>

# Run all tests
ctest --preset test-<preset-name> --output-on-failure

# Build a single test binary
cmake --build --preset build-darwin-debug --target shape_data_tests
./out/build/darwin-debug/shape_data_tests       # or just run the binary
```

### Available presets

| Configure preset       | Build preset            | Test preset            | Notes |
| ---------------------- | ----------------------- | ---------------------- | ----- |
| `windows-ucrt64-debug` | `build-ucrt64-debug`    | `test-ucrt64-debug`    | MSVC or MSYS2 UCRT64 g++; preset injects `C:/msys64/ucrt64/bin` into `PATH` and sets `Qt6_DIR` |
| `windows-ucrt64-release` | `build-ucrt64-release`  | —                      | Same as above, `CMAKE_BUILD_TYPE=Release` |
| `linux-debug`          | `build-linux-debug`     | `test-linux-debug`     | Uses `/usr` for `CMAKE_PREFIX_PATH` |
| `linux-release`        | `build-linux-release`   | —                      | `CMAKE_BUILD_TYPE=Release` |
| `darwin-debug`         | `build-darwin-debug`    | `test-darwin-debug`    | Searches Homebrew Qt at `/opt/homebrew/opt/qt` and `/usr/local/opt/qt`; override with `CMAKE_PREFIX_PATH` env var for Qt-installer installs |
| `darwin-release`       | `build-darwin-release`  | —                      | `CMAKE_BUILD_TYPE=Release` |

### Quality gates

| Target           | What it does                                                       | Requires                  |
| ---------------- | ------------------------------------------------------------------ | ------------------------- |
| `typecheck`      | Full compile (C++ type checking happens at compile time)           | compiler                  |
| `format`         | `clang-format -i` over all source files                            | `clang-format` on PATH    |
| `format-check`   | `clang-format --dry-run --Werror` (CI gate)                        | `clang-format` on PATH    |
| `lint`           | `clang-tidy -p <build-dir>` per `.clang-tidy` config               | `clang-tidy` on PATH      |

```bash
cmake --build --preset build-darwin-debug --target typecheck
cmake --build --preset build-darwin-debug --target format
cmake --build --preset build-darwin-debug --target format-check
cmake --build --preset build-darwin-debug --target lint
```

### macOS Qt installation tips

- Homebrew: `brew install qt cmake ninja` → presets find Qt automatically.
- Qt installer: install to `~/Qt/6.x.x/macos`, then `export CMAKE_PREFIX_PATH="$HOME/Qt/6.x.x/macos:$CMAKE_PREFIX_PATH"` before `cmake --preset`.
- Run unsigned `.app` from terminal to bypass Gatekeeper: `./out/build/darwin-debug/SVG_Editor.app/Contents/MacOS/SVG_Editor`.

---

## High-Level Architecture

The codebase is a C++20 / Qt 6 (Widgets + Graphics View) vector editor. Four strict layers, lower layers never include upper ones:

```
app        app/main.cpp                       — QApplication, QSettings org/app name, event loop
core       core/ShapeData                     — pure data + JSON (de)serialization (only struct crossing every layer)
  ↳        core/FileManager                   — document-level JSON I/O (versioned .vgjson, see below)
  ↳        core/AppLanguage + core/I18n       — language enum and localized strings
  ↳        core/CanvasGeometry                — transform helpers shared by canvas interaction/tests
  ↳        core/SelectionFrame                — orthogonal interaction frame math
canvas     canvas/CanvasView*                 — tool state machine, three interactive workflows, doc I/O, PNG export
  ↳        canvas/DragCreationStrategy        — drag-created shapes
  ↳        canvas/PathCreationStrategy        — polyline/polygon creation
  ↳        canvas/MultiShapeSession           — multi-select snapshot/cancel support
  ↳        canvas/SelectionTransformOverlayItem — dashed bbox + 4 corner resize handles
graphics   graphics/ShapeItem                 — paint + hit-test for one shape
  ↳        graphics/ShapeFactory              — create/clone helpers bridging ShapeData to ShapeItem
ui         ui/MainWindow                      — menus, toolbar, QActionGroup, signal bridging, theme application
  ↳        ui/PropertyPanel                   — reactive editor for selected shape (single-selection only)
  ↳        ui/TutorialDialog                  — bilingual HTML help dialog
  ↳        ui/ThemeMode / ui/ThemeUtils       — light / dark / system theme (QPalette + Fusion style)
```

**`core` is a static library** (`svg_editor_core`) linked by both the app and the tests. `ShapeData` is the only struct that crosses every layer boundary; it must remain free of `QGraphicsItem`/Widgets dependencies.

### Why this matters when editing

- **Data/view separation**: `ShapeItem::shapeData()` pulls data out for serialization; `CanvasView` never reads internal `ShapeItem` state. To change what gets rendered, edit `ShapeItem::buildPath`. To change what gets saved, edit `shapeDataToJson` / `shapeDataFromJson` in `ShapeData.cpp`.
- **JSON field names are a stable contract**. Renaming a field in `ShapeData.cpp` breaks every `.vgjson` on disk — coordinate with `ShapeData.h` and `FileManager.cpp` if you must.
- **Three interactive workflows live in `CanvasView`** and are mutually exclusive. `setTool()` and `cancelDrawing()` must keep them clean:
  1. **Drag workflow** (`beginDragShape` / `updateDragPreview` / `finishDragShape`) — used by `Line` / `Rectangle` / `Circle` / `Ellipse`. Shares `m_previewItem`.
  2. **Path workflow** (`beginPathPoint` / `updatePathPreview` / `finishPathShape`) — used by `Polyline` / `Polygon`. Also shares `m_previewItem`.
  3. **Transform session** (`beginTransformSession` / `updateTransformSession` / `finishTransformSession` / `cancelTransformSession`) — multi-select resize via 4 corner handles, driven by `SelectionTransformOverlayItem`. Uses `m_transformSession` (snapshot + restore on cancel), **not** `m_previewItem`. Shift-modifier keeps aspect ratio.
  Both the drag and path workflows free `m_previewItem` on `cancelDrawing()`. The transform session is independent and is cancelled by `cancelTransformSession()` (called on tool switch and `Esc`).
- **In-place move of selected items** uses `ShapeItem::setPendingMoveOffset` / `commitPendingMoveOffset` / `hasPendingMoveOffset`. The bbox display is updated immediately; the underlying `ShapeData` is only rewritten on `mouseReleaseEvent` to avoid noisy `shapeChanged` signals during drag.
- **`CanvasView::nextZValue()`** is the only source of new z values; never hardcode `zValue` in scene code. `ShapeItem` adds a small per-item z delta on top to disambiguate hit-tests for same-z shapes.
- **`PropertyPanel::m_updatingWidgets` is a reentrancy guard**, not a config flag. When the panel writes back to `ShapeData`, the model's signals would otherwise re-trigger the panel and re-enter `setShapeData`.
- **Theme is applied at startup and re-applied on change**. `MainWindow` reads the persisted `ThemeMode` from `QSettings` in its constructor and calls `applyApplicationTheme(QApplication, mode)` immediately. User-initiated switches via the `Tutorial → Theme → System/Light/Dark` actions also re-apply on click. `System` mode additionally listens to `QStyleHints::colorSchemeChanged` so the palette follows OS dark-mode toggles. `m_selectionOverlay` colors are not theme-aware (always `#2d7ff9`).

### File format

`.vgjson` (JSON, also accepts `.json`). Root: `{ "version": 2, "canvas": {width, height}, "shapes": [...] }`. **The load path requires `version == 2`** — older v1 files are rejected with `errorMessage = "Unsupported document version. Expected version 2."`. See `FileManager.cpp` and the schema table in `ShapeData.h` for per-shape field semantics.

Key point: `points` and `rect` have **type-dependent semantics** — for `point`/`line`/`polyline`/`polygon` only `points` is meaningful; for `circle`/`ellipse`/`rectangle` only `rect` is. The single-shape deserialization failure path now surfaces the offending index via `errorMessage` (was previously silent — see commit history if you need to reintroduce tolerance).

### One FIXME currently in the code (do not silently remove)

1. `ShapeData.cpp:116` — `shapeTypeToString` falls through to `"unknown"`. If you add a `ShapeType` value, you must add a case here (GCC `-Wswitch` will warn).

(`FileManager.cpp` previously had a FIXME for silently-skipped bad shapes; that has since been fixed and the marker removed. If you re-introduce silent tolerance, the original rationale is in the file header's `@details` block.)

### Tooling config files

- `.clang-format` — LLVM base, 4-space indent, 120-col limit, `SortIncludes: Never` (project enforces include order manually).
- `.clang-tidy` — `bugprone-*`, `clang-analyzer-*`, `modernize-use-nullptr/override`, `performance-*`, `readability-duplicate-include`. Header filter restricts it to `SVG_Editor/` and `tests/`.
- `.editorconfig` — UTF-8, LF, final newline, trim trailing whitespace.
- `CMakePresets.json` — v6 schema. `base` is portable (no compiler/PATH); Windows-specific bits live in `windows-ucrt64` (hidden); `base-unix` (hidden) is the parent of `linux-*` and `darwin-*`. To add a new platform, inherit from `base-unix` (or create a new hidden platform-specific intermediate if you need hardcoded paths).

### Tests

Test sources now mirror production modules under `tests/core/` and `tests/graphics/`. Binaries remain linked against `svg_editor_core` + `Qt6::Test`:

- `shape_data_tests` — `tests/core/ShapeDataTests.cpp`
- `file_manager_tests` — `tests/core/FileManagerTests.cpp`
- `canvas_geometry_tests` — `tests/core/CanvasGeometryTests.cpp`
- `shape_item_tests` — `tests/graphics/ShapeItemTests.cpp`

Both register as CTest cases with the same names. Filter with `ctest -R <name>` or run the binary directly for debugger attachment.
