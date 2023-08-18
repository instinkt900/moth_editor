cmake_minimum_required(VERSION 3.10)

project(moth_editor VERSION 1.0 DESCRIPTION "An editor implementation for uilib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _ITERATOR_DEBUG_LEVEL=0 IMGUI_DEFINE_MATH_OPERATORS)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")

find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)
find_package(range-v3 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(spdlog REQUIRED)
find_package(glfw3 REQUIRED)
find_package(vulkan-memory-allocator REQUIRED)
find_package(freetype REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(.. moth_ui)

#include_directories(../include)
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
	../external/murmurhash.c/murmurhash.c
	src/backend/sdl/sdl_app.cpp
	src/backend/sdl/sdl_events.cpp
	src/backend/sdl/sdl_font.cpp
	src/backend/sdl/sdl_font_factory.cpp
	src/backend/sdl/SDL_FontCache.c
	src/backend/sdl/sdl_graphics.cpp
	src/backend/sdl/sdl_image.cpp
	src/backend/sdl/sdl_image_factory.cpp
	src/backend/sdl/sdl_ttf_shim.c
	src/backend/sdl/sdl_ui_renderer.cpp
	src/backend/vulkan/vulkan_app.cpp
	src/backend/vulkan/vulkan_buffer.cpp
	src/backend/vulkan/vulkan_command_buffer.cpp
	src/backend/vulkan/vulkan_context.cpp
	src/backend/vulkan/vulkan_events.cpp
	src/backend/vulkan/vulkan_fence.cpp
	src/backend/vulkan/vulkan_font.cpp
	src/backend/vulkan/vulkan_font_cache.cpp
	src/backend/vulkan/vulkan_font_factory.cpp
	src/backend/vulkan/vulkan_framebuffer.cpp
	src/backend/vulkan/vulkan_graphics.cpp
	src/backend/vulkan/vulkan_image.cpp
	src/backend/vulkan/vulkan_image_factory.cpp
	src/backend/vulkan/vulkan_pipeline.cpp
	src/backend/vulkan/vulkan_renderpass.cpp
	src/backend/vulkan/vulkan_shader.cpp
	src/backend/vulkan/vulkan_subimage.cpp
	src/backend/vulkan/vulkan_swapchain.cpp
	src/backend/vulkan/vulkan_ui_renderer.cpp
	src/backend/vulkan/vulkan_vma.cpp
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
	src/editor/imgui_ext_focus.cpp
	src/editor/offset_bounds_handle.cpp
	src/editor/panels/editor_panel.cpp
	src/editor/panels/editor_panel_animation.cpp
	src/editor/panels/editor_panel_asset_list.cpp
	src/editor/panels/editor_panel_canvas.cpp
	src/editor/panels/editor_panel_canvas_properties.cpp
	src/editor/panels/editor_panel_config.cpp
	src/editor/panels/editor_panel_elements.cpp
	src/editor/panels/editor_panel_preview.cpp
	src/editor/panels/editor_panel_properties.cpp
	src/editor/panels/editor_panel_undo_stack.cpp
	src/editor/properties_elements.cpp
	src/editor/texture_packer.cpp
	src/layers/layer.cpp
	src/layers/layer_stack.cpp
	src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} moth_ui ${CONAN_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W3 /WX")
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} fmt::fmt)
target_link_libraries(${PROJECT_NAME} magic_enum::magic_enum)
target_link_libraries(${PROJECT_NAME} range-v3::range-v3)
target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
target_link_libraries(${PROJECT_NAME} SDL2_image::SDL2_image)
target_link_libraries(${PROJECT_NAME} SDL2_ttf::SDL2_ttf-static)
target_link_libraries(${PROJECT_NAME} vulkan-headers::vulkan-headers)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} vulkan-memory-allocator::vulkan-memory-allocator)
target_link_libraries(${PROJECT_NAME} freetype)
target_link_libraries(${PROJECT_NAME} glm::glm)
set_source_files_properties(src/backend/sdl/SDL_FontCache.c PROPERTIES COMPILE_FLAGS "/W0")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
