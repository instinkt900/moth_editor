cmake_minimum_required(VERSION 3.10)

project(moth_editor VERSION 1.0 DESCRIPTION "An editor for Moth UI data")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _ITERATOR_DEBUG_LEVEL=0 IMGUI_DEFINE_MATH_OPERATORS)

add_subdirectory(../external/harfbuzz-icu-freetype harfbuzz-icu-freetype)
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

find_package(fmt REQUIRED)
#find_package(freetype REQUIRED)
find_package(glfw3 REQUIRED)
find_package(magic_enum REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(range-v3 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(spdlog REQUIRED)
find_package(Vulkan REQUIRED)
find_package(vulkan-memory-allocator REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} moth_ui)
target_link_libraries(${PROJECT_NAME} moth_ui_backends)
target_link_libraries(${PROJECT_NAME} fmt::fmt)
#target_link_libraries(${PROJECT_NAME} freetype)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} range-v3::range-v3)
target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
target_link_libraries(${PROJECT_NAME} SDL2_image::SDL2_image)
target_link_libraries(${PROJECT_NAME} SDL2_ttf::SDL2_ttf-static)
target_link_libraries(${PROJECT_NAME} spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} vulkan-memory-allocator::vulkan-memory-allocator)
target_link_libraries(${PROJECT_NAME} harfbuzz)

if (MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W3 /WX")
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Wall -Werror")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
