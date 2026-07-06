find_package(Qt6 REQUIRED COMPONENTS Test Widgets)
find_package(Threads REQUIRED)

# format / lint target 在根 CMake 中以 ${CMAKE_SOURCE_DIR} 为 cwd 调起；
# 暴露给它们的路径用绝对路径避免与子目录 cwd 错乱
set(VECTOR_GRAPHICS_EDITOR_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/core/CanvasGeometryTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/ShapeDataTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/FileManagerTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/ShapeItemTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/MainWindowTests.cpp
  ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor/graphics/ShapeItem.cpp)

set_property(GLOBAL APPEND PROPERTY VECTOR_GRAPHICS_EDITOR_SOURCE_FILES ${VECTOR_GRAPHICS_EDITOR_TEST_SOURCES})

add_executable(canvas_geometry_tests ${CMAKE_CURRENT_SOURCE_DIR}/core/CanvasGeometryTests.cpp)
target_include_directories(canvas_geometry_tests PRIVATE ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor)
target_link_libraries(canvas_geometry_tests PRIVATE vector_graphics_editor_core Qt6::Test vector_graphics_editor_warnings Threads::Threads)

add_executable(shape_data_tests ${CMAKE_CURRENT_SOURCE_DIR}/core/ShapeDataTests.cpp)
target_include_directories(shape_data_tests PRIVATE ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor)
target_link_libraries(shape_data_tests PRIVATE vector_graphics_editor_core Qt6::Test vector_graphics_editor_warnings Threads::Threads)

add_executable(file_manager_tests ${CMAKE_CURRENT_SOURCE_DIR}/core/FileManagerTests.cpp)
target_include_directories(file_manager_tests PRIVATE ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor)
target_link_libraries(file_manager_tests PRIVATE vector_graphics_editor_core Qt6::Test vector_graphics_editor_warnings Threads::Threads)

add_executable(shape_item_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/graphics/ShapeItemTests.cpp
  ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor/graphics/ShapeItem.cpp)
target_include_directories(shape_item_tests PRIVATE ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor)
target_link_libraries(shape_item_tests PRIVATE vector_graphics_editor_core Qt6::Test Qt6::Widgets vector_graphics_editor_warnings Threads::Threads)

get_target_property(VECTOR_GRAPHICS_EDITOR_APP_TEST_SOURCES VectorGraphicsEditor SOURCES)
list(FILTER VECTOR_GRAPHICS_EDITOR_APP_TEST_SOURCES EXCLUDE REGEX [[(^|.*/)app/main\.cpp$]])
foreach(_src IN LISTS VECTOR_GRAPHICS_EDITOR_APP_TEST_SOURCES)
  if(NOT IS_ABSOLUTE "${_src}")
    list(REMOVE_ITEM VECTOR_GRAPHICS_EDITOR_APP_TEST_SOURCES "${_src}")
    list(APPEND VECTOR_GRAPHICS_EDITOR_APP_TEST_SOURCES "${CMAKE_SOURCE_DIR}/VectorGraphicsEditor/${_src}")
  endif()
endforeach()

add_executable(main_window_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/ui/MainWindowTests.cpp
  ${VECTOR_GRAPHICS_EDITOR_APP_TEST_SOURCES})
target_include_directories(main_window_tests PRIVATE ${CMAKE_SOURCE_DIR}/VectorGraphicsEditor)
target_link_libraries(main_window_tests PRIVATE vector_graphics_editor_core Qt6::Test Qt6::Widgets vector_graphics_editor_warnings Threads::Threads)

add_custom_target(vector_graphics_editor_tests DEPENDS canvas_geometry_tests shape_data_tests file_manager_tests shape_item_tests main_window_tests)

add_test(NAME canvas_geometry_tests COMMAND canvas_geometry_tests)
add_test(NAME shape_data_tests COMMAND shape_data_tests)
add_test(NAME file_manager_tests COMMAND file_manager_tests)
add_test(NAME shape_item_tests COMMAND shape_item_tests)
add_test(NAME main_window_tests COMMAND main_window_tests)
