# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when working in this repository.

Full documentation is in `docs/`. Read the relevant file before editing a layer.

---

## Quick Build

```bash
cmake --preset darwin-debug
cmake --build --preset build-darwin-debug
ctest --preset test-darwin-debug --output-on-failure
```

See [docs/build-and-run.md](docs/build-and-run.md) for all platforms and presets.

---

## Architecture at a Glance

Four strict layers, bottom-up: `core → graphics → canvas → ui → entry`

| Layer | Key Files | Docs |
|-------|-----------|------|
| core | `ShapeData`, `FileManager`, `CanvasGeometry`, `SelectionFrame`, `AppLanguage`, `I18n`, `CanvasViewConstants` | [docs/core-layer.md](docs/core-layer.md) |
| graphics | `ShapeItem`, `ShapeFactory` | [docs/graphics-layer.md](docs/graphics-layer.md) |
| canvas | `CanvasView` (3 .cpp), `CreationStrategy`, `DragCreationStrategy`, `PathCreationStrategy`, `MultiShapeSession`, `SelectionTransformOverlayItem` | [docs/canvas-layer.md](docs/canvas-layer.md) |
| ui | `MainWindow`, `MainWindowActions`, `PropertyPanel`, `TutorialDialog`, `ThemeMode`, `ThemeUtils` | [docs/ui-layer.md](docs/ui-layer.md) |
| entry | `app/main.cpp` | — |

---

## Critical Rules When Editing

1. **Data/view separation**: To change rendering → `ShapeItem::buildBasePath`. To change serialization → `ShapeData.cpp`. Never mix.
2. **JSON field names are a contract**. Renaming a field in `ShapeData.cpp` breaks every `.vgjson` on disk.
3. **Three mutually-exclusive workflows in CanvasView**: Drag creation, Path creation, Transform session. `setTool()` must cancel the previous.
4. **`CanvasView::nextZValue()`** is the only z-value source. Never hardcode z.
5. **`PropertyPanel::m_updatingWidgets`** is a reentrancy guard, not a config flag.
6. **`core` is a static library** (`vector_graphics_editor_core`). It must stay free of `QGraphicsItem`/Widgets.
7. **Theme is Fusion-only**. Selection overlay color (`#2d7ff9`) is not theme-aware.

See [docs/architecture.md](docs/architecture.md) and [docs/canvas-layer.md](docs/canvas-layer.md) for details.

---

## File Format

`.vgjson` v2. Root: `{version: 2, canvas: {width, height}, shapes: [...]}`. Version 1 is rejected. Full spec: [docs/file-format.md](docs/file-format.md).

## FIXME

`ShapeData.cpp:109` — `shapeTypeToString` default returns `"unknown"`. Add a case when adding `ShapeType` values.

## Tests

5 binaries, 39 cases. See [docs/testing.md](docs/testing.md).

## Adding Features

Step-by-step guides: [docs/extending.md](docs/extending.md).

## Coding Conventions

See [docs/dev-workflow.md](docs/dev-workflow.md) for naming, include order, Doxygen header format, and quality gates.

## Troubleshooting

See [docs/troubleshooting.md](docs/troubleshooting.md).
