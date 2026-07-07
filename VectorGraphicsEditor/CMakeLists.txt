find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(VECTOR_GRAPHICS_EDITOR_CORE_SOURCES
  core/AppLanguage.h
  core/AppLanguage.cpp
  core/CanvasGeometry.h
  core/CanvasGeometry.cpp
  core/CanvasViewConstants.h
  core/I18n.h
  core/I18n.cpp
  core/SelectionFrame.cpp
  core/SelectionFrame.h
  core/ShapeData.cpp
  core/ShapeData.h
  core/FileManager.cpp
  core/FileManager.h
)

set(VECTOR_GRAPHICS_EDITOR_APP_SOURCES
  app/main.cpp
  canvas/CanvasView.h
  canvas/CanvasViewCore.cpp
  canvas/CanvasViewInput.cpp
  canvas/CanvasViewSelection.cpp
  canvas/CreationStrategy.h
  canvas/DragCreationStrategy.h
  canvas/DragCreationStrategy.cpp
  canvas/PathCreationStrategy.h
  canvas/PathCreationStrategy.cpp
  canvas/MultiShapeSession.h
  canvas/MultiShapeSession.cpp
  canvas/SelectionTransformOverlayItem.h
  canvas/SelectionTransformOverlayItem.cpp
  graphics/ShapeItem.h
  graphics/ShapeItem.cpp
  graphics/ShapeFactory.h
  graphics/ShapeFactory.cpp
  ui/MainWindow.h
  ui/MainWindow.cpp
  ui/MainWindowActions.h
  ui/MainWindowActions.cpp
  ui/PropertyPanel.h
  ui/PropertyPanel.cpp
  ui/PropertyPanelGeometryFields.h
  ui/ThemeMode.h
  ui/ThemeUtils.h
  ui/ThemeUtils.cpp
  ui/TutorialDialog.h
  ui/TutorialDialog.cpp
  ui/TutorialManualContent.h
)

# 全项目源文件清单（format / lint target 使用），单一来源避免双份维护。
# 路径用绝对路径：format-check 的 cwd 是根 ${CMAKE_SOURCE_DIR}，而这些源在子目录 VectorGraphicsEditor/ 中。
set_property(GLOBAL PROPERTY
  VECTOR_GRAPHICS_EDITOR_SOURCE_FILES
  ${VECTOR_GRAPHICS_EDITOR_CORE_SOURCES} ${VECTOR_GRAPHICS_EDITOR_APP_SOURCES})
foreach(_src IN LISTS VECTOR_GRAPHICS_EDITOR_SOURCE_FILES)
  if(NOT IS_ABSOLUTE "${_src}")
    list(REMOVE_ITEM VECTOR_GRAPHICS_EDITOR_SOURCE_FILES "${_src}")
    list(APPEND VECTOR_GRAPHICS_EDITOR_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
  endif()
endforeach()
set_property(GLOBAL PROPERTY VECTOR_GRAPHICS_EDITOR_SOURCE_FILES "${VECTOR_GRAPHICS_EDITOR_SOURCE_FILES}")

add_library(vector_graphics_editor_warnings INTERFACE)

if(VECTOR_GRAPHICS_EDITOR_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(vector_graphics_editor_warnings INTERFACE /W4 /permissive- /Zc:__cplusplus)
  else()
    target_compile_options(vector_graphics_editor_warnings INTERFACE -Wall -Wextra -Wpedantic)
  endif()
endif()

add_library(vector_graphics_editor_core STATIC ${VECTOR_GRAPHICS_EDITOR_CORE_SOURCES})
target_include_directories(vector_graphics_editor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vector_graphics_editor_core PUBLIC Qt6::Core Qt6::Gui vector_graphics_editor_warnings)

add_executable(VectorGraphicsEditor WIN32 MACOSX_BUNDLE ${VECTOR_GRAPHICS_EDITOR_APP_SOURCES})
set_target_properties(VectorGraphicsEditor PROPERTIES
  MACOSX_BUNDLE_BUNDLE_NAME "VectorGraphicsEditor"
  MACOSX_BUNDLE_GUI_IDENTIFIER "com.vector.graphics.editor"
  MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
)
target_include_directories(VectorGraphicsEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(VectorGraphicsEditor PRIVATE vector_graphics_editor_core Qt6::Widgets vector_graphics_editor_warnings)

if(COMMAND qt_finalize_executable)
  qt_finalize_executable(VectorGraphicsEditor)
endif()

install(TARGETS VectorGraphicsEditor RUNTIME DESTINATION . BUNDLE DESTINATION .)
