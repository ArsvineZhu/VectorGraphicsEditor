find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(SVG_EDITOR_CORE_SOURCES
  AppLanguage.h
  SelectionFrame.cpp
  SelectionFrame.h
  ShapeData.cpp
  ShapeData.h
  FileManager.cpp
  FileManager.h
)

set(SVG_EDITOR_APP_SOURCES
  main.cpp
  ThemeMode.h
  ThemeUtils.cpp
  ThemeUtils.h
  MainWindow.cpp
  MainWindow.h
  TutorialDialog.cpp
  TutorialDialog.h
  CanvasView.cpp
  CanvasView.h
  SelectionTransformOverlayItem.cpp
  SelectionTransformOverlayItem.h
  ShapeItem.cpp
  ShapeItem.h
  ShapeFactory.cpp
  ShapeFactory.h
  PropertyPanel.cpp
  PropertyPanel.h
)

set_property(GLOBAL APPEND PROPERTY SVG_EDITOR_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ThemeMode.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ThemeUtils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ThemeUtils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/TutorialDialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/TutorialDialog.h
  ${CMAKE_CURRENT_SOURCE_DIR}/CanvasView.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CanvasView.h
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionTransformOverlayItem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionTransformOverlayItem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/AppLanguage.h
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionFrame.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SelectionFrame.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ShapeData.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShapeData.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ShapeItem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShapeItem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ShapeFactory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ShapeFactory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/FileManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FileManager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanel.h)

add_library(svg_editor_warnings INTERFACE)

if(SVG_EDITOR_ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(svg_editor_warnings INTERFACE /W4 /permissive- /Zc:__cplusplus)
  else()
    target_compile_options(svg_editor_warnings INTERFACE -Wall -Wextra -Wpedantic)
  endif()
endif()

add_library(svg_editor_core STATIC ${SVG_EDITOR_CORE_SOURCES})
target_include_directories(svg_editor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(svg_editor_core PUBLIC Qt6::Core Qt6::Gui svg_editor_warnings)

add_executable(SVG_Editor WIN32 ${SVG_EDITOR_APP_SOURCES})
target_include_directories(SVG_Editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SVG_Editor PRIVATE svg_editor_core Qt6::Widgets svg_editor_warnings)

if(COMMAND qt_finalize_executable)
  qt_finalize_executable(SVG_Editor)
endif()
