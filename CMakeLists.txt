cmake_minimum_required(VERSION 3.10)

project(moth_editor VERSION 1.0 DESCRIPTION "An editor implementation for uilib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _ITERATOR_DEBUG_LEVEL=0 IMGUI_DEFINE_MATH_OPERATORS)
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
	src/editor/actions/add_clip_action.cpp
	src/editor/actions/add_keyframe_action.cpp
	src/editor/actions/change_index_action.cpp
	src/editor/actions/composite_action.cpp
	src/editor/actions/delete_action.cpp
	src/editor/actions/delete_clip_action.cpp
	src/editor/actions/delete_keyframe_action.cpp
	src/editor/actions/editor_action.cpp
	src/editor/actions/modify_clip_action.cpp
	src/editor/actions/modify_keyframe_action.cpp
	src/editor/actions/move_keyframe_action.cpp
	src/editor/panels/editor_panel.cpp
	src/editor/panels/editor_panel_animation.cpp
	src/editor/panels/editor_panel_asset_list.cpp
	src/editor/panels/editor_panel_canvas.cpp
	src/editor/panels/editor_panel_canvas_properties.cpp
	src/editor/panels/editor_panel_elements.cpp
	src/editor/panels/editor_panel_keyframes.cpp
	src/editor/panels/editor_panel_preview.cpp
	src/editor/panels/editor_panel_properties.cpp
	src/editor/panels/editor_panel_undo_stack.cpp
	src/editor/anchor_bounds_handle.cpp
	src/editor/bounds_handle.cpp
	src/editor/bounds_widget.cpp
	src/editor/confirm_prompt.cpp
	src/editor/content_list.cpp
	src/editor/editor_layer.cpp
	src/editor/imgui_ext.cpp
	src/editor/imgui_ext_focus.cpp
	src/editor/offset_bounds_handle.cpp
	src/editor/properties_elements.cpp
	src/layers/layer.cpp
	src/layers/layer_stack.cpp
	src/app.cpp
	src/event_factory.cpp
	src/font.cpp
	src/font_factory.cpp
	src/image.cpp
	src/image_factory.cpp
	src/main.cpp
	src/SDL_FontCache.c
	src/ui_renderer.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} moth_ui ${CONAN_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W3 /WX")
set_source_files_properties(src/SDL_FontCache.c PROPERTIES COMPILE_FLAGS "/W0")
