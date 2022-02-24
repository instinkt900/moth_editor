cmake_minimum_required(VERSION 3.10)

project(moth_editor VERSION 1.0 DESCRIPTION "An editor implementation for uilib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _ITERATOR_DEBUG_LEVEL=0)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")

add_subdirectory(.. moth_ui)

#include_directories(../include)
include_directories(../external/imgui)

set(SOURCES ${SOURCES}
	../external/imgui/imgui.cpp
	../external/imgui/imgui_draw.cpp
	../external/imgui/imgui_tables.cpp
	../external/imgui/imgui_widgets.cpp
	../external/imgui/backends/imgui_impl_sdlrenderer.cpp
	../external/imgui/backends/imgui_impl_sdl.cpp
	src/editor/actions/add_action.cpp
	src/editor/actions/add_keyframe_action.cpp
	src/editor/actions/composite_action.cpp
	src/editor/actions/delete_action.cpp
	src/editor/actions/delete_keyframe_action.cpp
	src/editor/actions/modify_clip_action.cpp
	src/editor/actions/modify_keyframe_action.cpp
	src/editor/actions/move_keyframe_action.cpp
	src/editor/anchor_bounds_handle.cpp
	src/editor/animation_widget.cpp
	src/editor/bounds_handle.cpp
	src/editor/bounds_widget.cpp
	src/editor/editor_layer.cpp
	src/editor/keyframe_widget.cpp
	src/editor/offset_bounds_handle.cpp
	src/editor/properties_editor.cpp
	src/game.cpp
	src/layers/layer.cpp
	src/layers/layer_stack.cpp
	src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} moth_ui ${CONAN_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
