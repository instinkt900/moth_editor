cmake_minimum_required(VERSION 3.10)

project(moth_editor VERSION 1.0 DESCRIPTION "An editor for Moth UI data")

message("Building with CMake version: ${CMAKE_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_subdirectory(../external/nativefiledialog nativefiledialog)

set(SOURCES ${SOURCES}
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
	src/editor/imgui_ext.cpp
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

find_package(canyon REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} nativefiledialog)
target_link_libraries(${PROJECT_NAME} canyon::canyon)

if (MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W3 /WX")
	target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Wall -Werror")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
