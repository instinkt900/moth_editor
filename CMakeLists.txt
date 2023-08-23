cmake_minimum_required(VERSION 3.10)

project(moth_editor VERSION 1.0 DESCRIPTION "An editor implementation for uilib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _ITERATOR_DEBUG_LEVEL=0 IMGUI_DEFINE_MATH_OPERATORS)
if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")
else()
endif()

add_subdirectory(.. moth_ui)
add_subdirectory(../backends moth_ui_backends)

include_directories(../backends/include)
include_directories(../external/imgui)
include_directories(../external/murmurhash.c)

set(SOURCES ${SOURCES}
	../external/imgui/imgui.cpp
	../external/imgui/imgui_draw.cpp
	../external/imgui/imgui_tables.cpp
	../external/imgui/imgui_widgets.cpp
	../external/imgui/backends/imgui_impl_sdlrenderer.cpp
	../external/imgui/backends/imgui_impl_sdl.cpp
	../external/imgui/backends/imgui_impl_vulkan.cpp
	../external/imgui/backends/imgui_impl_glfw.cpp
	src/editor/actions/add_action.cpp
	src/editor/actions/add_clip_action.cpp
	src/editor/actions/add_event_action.cpp
	src/editor/actions/add_keyframe_action.cpp
	src/editor/actions/change_index_action.cpp
	src/editor/actions/composite_action.cpp
	src/editor/actions/delete_action.cpp
	src/editor/actions/delete_clip_action.cpp
	src/editor/actions/delete_event_action.cpp
	src/editor/actions/delete_keyframe_action.cpp
	src/editor/actions/editor_action.cpp
	src/editor/actions/modify_clip_action.cpp
	src/editor/actions/modify_event_action.cpp
	src/editor/actions/modify_keyframe_action.cpp
	src/editor/actions/move_keyframe_action.cpp
	src/editor/anchor_bounds_handle.cpp
	src/editor/bounds_handle.cpp
	src/editor/bounds_widget.cpp
	src/editor/confirm_prompt.cpp
	src/editor/content_list.cpp
	src/editor/editor_layer.cpp
	src/editor/editor_layer_stack.cpp
	src/editor/imgui_ext.cpp
	src/editor/imgui_ext_focus.cpp
	src/editor/offset_bounds_handle.cpp
	src/editor/panels/editor_panel.cpp
	src/editor/panels/editor_panel_animation.cpp
	src/editor/panels/editor_panel_asset_list.cpp
	src/editor/panels/editor_panel_canvas.cpp
	src/editor/panels/editor_panel_canvas_properties.cpp
	src/editor/panels/editor_panel_config.cpp
	src/editor/panels/editor_panel_elements.cpp
	src/editor/panels/editor_panel_fonts.cpp
	src/editor/panels/editor_panel_preview.cpp
	src/editor/panels/editor_panel_properties.cpp
	src/editor/panels/editor_panel_undo_stack.cpp
	src/editor/properties_elements.cpp
	src/editor/texture_packer.cpp
	src/editor_application.cpp
	src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} moth_ui)
target_link_libraries(${PROJECT_NAME} moth_ui_backends ${CONAN_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
if (MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W3 /WX")
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Wall -Werror")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
