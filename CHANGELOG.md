# Changelog

All notable changes to this project will be documented in this file.
Entries are generated automatically from git history using [git-cliff](https://github.com/orhun/git-cliff).

## [0.3.0] - 2026-03-21
### Bug Fixes
- Initialise atlases as empty JSON array instead of null

### Changes
- Updating ci to be more simplified
- Fixing action typo
- Fixing up upload action
- Fixing more upload actions

### Documentation
- Rewrite README to focus on user-facing features
- Updating docs to reflect removed profiles

### Features
- Updating texture packing to the new format

### Miscellaneous
- Bumped version to 0.3.0

## [0.2.0] - 2026-03-19
### Bug Fixes
- Resolve clang-tidy findings in action files
- Resolve remaining clang-tidy findings
- Resolve all clang-tidy warnings across moth_editor sources
- Address review findings — safety guards, robustness, nitpicks
- Guard against null/OOB in change_index, move_keyframe, animation, fonts
- Validate all preconditions before mutating trees in change_index, fix fonts OOB

### Changes
- Align workflows, build config, and README with canyon/moth_ui structure
- Correct capitalization of project name in README
- Generate release notes before CHANGELOG commit to fix git-cliff --current

### Features
- Enable precompiled headers for common.h

### Miscellaneous
- Bump canyon to 0.5.0 and replace deprecated ui_fwd.h includes
- Bumping canyon dependency version
- Bump version to 0.2.0

## [0.1.0] - 2026-03-16
### Bug Fixes
- Fixing conan deps issue. conan2 continues to fail me. i need a new package manager
- Fixing the usual conan bullshit
- Fixing more conan bullshit
- Fixing missing build tools
- Fixing linux workflow
- Update example app includes and conan dep to build against canyon 0.3.0
- Update canyon dependency to 0.4.0 and adapt to API changes
- Vendor stb_rect_pack.h directly instead of relying on canyon internals
- Update TexturePacker to match new DrawToPNG signature
- Extract MousePosToFrame helper, fix keyframe right-click offset, and extend box selection to clips and events
- Guard pending popup edits against use-after-free
- Remove duplicate m_actionIndex assignment in ClearEditActions
- Guard ModifyClipAction and ModifyEventAction against end-iterator dereference
- Correct entity tree index in ChangeIndexAction::Undo
- Guard BoundsWidget mouse handlers against null m_node
- Enum/combo properties never committed their selected value
- Guard filesystem::relative with error_code in properties panel
- Guard image preview against zero dimensions in properties panel
- Skip enum selectable update when value is unchanged
- Several defensive fixes across editor
- Fixing upload action

### Changes
- Initial commit of moth_ui separated from previous project
- Renaming uilib to moth_ui
- Removing SDL dependency from ui lib
- Fixing up namespaces
- Making the editor a bit more like a native tool
- Adding source rects to both the base image class but also the node image class
- Adding todo to track needs.
- Adding a close button to the keyframe editor window
- Adding interp types to keyframes.
- Adding exit menu item. App is now an event listener
- Added a preview window
- Rewrote vector class to be more generic.
- Fixing previoew window not getting correct keyframe timings.
- Updating testing.
- Addign basic font support. still wip
- Tweaking font handling. Fonts no longer take paths and instead are referred to by name.
- Changing order of nodes via pgup and pgdown
- Fixing loading layouts with text elements
- Adding image tiling to the image node
- Updating the properties panel to be supported by the undo/redo system.
- Working with possible imgui fix for inputint asserts
- Cleaning up layout entity structure. favoring public members.
- Adding some generic editor actions for the undo stack and removing a lot of specialized versions
- Exit confirm prompt
- Allowing the addition of user events
- Cleaning up how layouts and sublayouts are loaded/saved
- Adding layout versioning to handle old version loading
- Adding clip rect support
- Fixing sublayout loading on editor
- Updating the editor to use projects. Layout path and image path. The idea being that layouts and images will just be referred to by name rather than path.
- Changed layouts to be able to convert paths to relative paths so saved image and sublayout references are relative to the layout theyre saved into.
- Adding support for 9slicing
- Nine slice is no longer its own entity and is now just a type of image scaling.
- Fixing 9slice loading.
- Just cleaning up the headers in editor layer
- Should have built the project before my last commit. Here's the runon changes
- Added key modifiers to key events.
- Adding source rect and 9 slice indication to the image preview
- Drawing 9slice guides on the bounds widget
- Cleaning up menus. Adding keyboard shortcuts to things. Added a canvas reset option
- Adding a proper extension to the layouts.
- Changing the dockspace over viewport setup in favor of using a canvas imgui window
- Changing the image and layout lists to be a single asset list file browser. Still need to sort out what to do with project settings
- No more delete acttion on the asset list. I'm thinking it doesnt need to be an explorer replacement, it's just a shortcut.
- Slight modification to the canvas input handling. Only take inputs when focused for a start
- Adding multi selection behaviour.
- Cleaning up selection logic. Can select and deselect items using ctrl.
- Added text drop shadow support.
- Added the ability to alter/override some sublayout data
- Adding the ability to add a class to layouts so they can be hooked into implementation details in code
- Updated how the context works. Now can set and replace contexts so theres no longer a single static context.
- Removed time from animations and work purely on frames. Much simpler
- Adding alt drag on keyframes to duplicate them
- Adding box select to keyframe window
- Added actions for adding and removing clips so now they should undo/redo properly.
- Adding layout extension when saving
- Adding persistence to current directory, window size, canvas settings etc.
- Drag and drop elements will now be placed where the mouse drops them.
- Adding anchor preset buttons to the top of the bounds widget. Topleft and Fill.
- Fixing image and sublayout element buttons opening file windows not at cwd.
- 9 slice target rects are now anchor based. probably has limited use but it gives more options
- Added visibility to the serialized data.
- Just committing this to clean up
- Removed ImGui from moth_ui
- Removed ImGui from moth_ui
- Fixing drag select in animation panel only starting when clicking in the region
- Allowing drag cloning 0th keyframe.
- Added a menu option to set an animation event name.
- Fixing id change causing entities to be positioned incorrectly.
- Added self registering widget classes
- Fixing imgui.ini location
- Added animation stopped event that should only be sent once per event. Not super stoked about its implementation but it works for now.
- Fixed starting up in non existent directory.
- Adding some color customisation to the editor
- Adding texture packing tool.
- Sorting out texture packing alpha
- Changing how the texture packer decides the pack dimensions
- Bit of an overhaul of the animation panel to allow for different handling of events.
- Moving draw calls into an interface in prep for vulkan.
- Fixing imgui image display with new texture wrapper stuff
- Moving the graphics calls into its own interface
- Recreated vulkan branch.
- Adding some window management
- Moving some things to the backend namespace
- Fixing up changing to a directory that doesnt exist.
- Implemented set window title for vulkan
- Added validation layers.
- Reasonably close to working. Just need to hook in imgui rendering again and clean up some small things. But graphics is reasonably ready
- Small changes
- Custom drawing and drawing of ImGui working. Just need to clean it up and put in some real world usages
- Removed the second renderpass for render targets. Now properly transitions. (Properly?)
- Cleaning up the app class
- Cleaning up the context class
- Cleaning up graphics class
- Added imgui support to images. Not a nice implementation since we need access to the graphics shader but it works for now
- Adding some rendering into the vulkan ui_renderer
- Fixing set logical size for vulkan
- Removing the need for caching draw lists and instead opting to directly use the command buffer
- Cleaning up semaphore usage with framebuffers
- Adding the starting font changes
- Vulkan fonts are building but go not run since the libs clash with sdl ttf (as far as i can tell) so the backends need to be separated into libraries and configurations or something
- Added a TTF shim to allow dynamic loading of sdl_ttf libs while still linking to freetype. Might be a better idea to work the other way? either way this compiles for now.
- Updating to vs 2022
- Fixed the rendering of the canvas.
- Basic bitmap font rendering working. Still need to deal with positioning etc. but its visible progress
- Working text alignment with vulkan. Needs a lot of cleanup. Also need some kind of font cache at some point
- Adding a font cache used for reusing font objects of the same face and size
- Fixing widget rendering when canvas tab bar is visible
- Updating to latest vs 2022
- Resolving conflicts with main
- Adding resize support with vulkan swapchain recreation
- Adding keyboard events to vulkan implementation
- Fixing up conan cmake process
- Adding clip rect support in vulkan
- Updating readme to have some basic info. Also adding install step to cmake
- Fixing up conan/cmake build on linux
- Fixing up editor build. warnings etc
- Adding back accidental removal of animation panel
- Fixed the preview window crashing the editor. Just needed to make sure we weren't trying to render a negative image size
- Allowing the delete key to be used on the canvas or the animation panel. Added focused property to panels and added ToggleEntityVisibility to the layer
- Moved the rendering code out to its own location that could act as its own library
- Fixing up cmake build
- Few small fixes
- Moved application out of the backend folder
- Moved the font loading from font factory to the application(s). moved the example layouts to its own base folder. fixed the location of imgui.ini
- Adding a font panel with the ability to add/remove fonts from the library and also load or save the library
- Cleaning up font panel to be a bit nicer
- Updating cmake build
- Font factory is now no longer an interface and instead handles a bunch of generic stuff
- Adding editor build actions for windows and linux
- Removing the now unneeded ttf shim
- Removing dead code
- Moved application back to backend and made it more generic
- Updating cmake file
- Forgot to remove old files
- Fixing some small build issues
- Made backends its own library. Added an example application. Needs work
- Fixing linux build
- Fixing example build
- Cleaning up cmake files
- Added mouse events to application. Some window sizing tweaks. Added a button to the example layout
- Started to fill out the example layouts but this can be completed in another branch
- Fixing issues with clip name editing
- Fixed a small issue with vulkan colour tinting not working
- Fixing the animation cursor to properly be positioned when the track is scrolled
- Updating cmake build
- Forked the harfbuzz build so i could disable the freetype build. Since it's already supplied elsewhere it just causes issues
- Fixing package conflicts
- Fixing vulkan font rendering vertical positioning
- Tweaked font rendering to fix some black line issues. Added a bunch of extra fonts for testing. Theres still some hard edges on some fonts. Might need to add some internal glyph border pixels
- Cleaning up and tweaking font rendering still
- Fixing linux build
- Adding a write to png function similar to sdl
- Moved stb files around and made it more obvious where the actual implementation is
- Not a rewrite but a lot of cleanups. Large change to how horizontal scrolling works
- Cleaning up a bunch more stuff. Better use of clip rects when drawing clips events and keyframes
- Fixing selecting keyframes not deselecting clips/events. fixes #40
- Holy shit conan is bad
- Conan continues to cause issues
- Moved the frame ribbon outside of the scrolling panel. Fixes #53 Also fixed some keyframe issues where sometimes keyframes werent being created when moving elements around
- Fixed unused variable. It was an error that we weren't using the drawList but when I changed it to use the draw list the cursor rendered under the scroll panel for some reason. The cursor has to be part of the scrolling panel draw list for some reason
- Changing props file to only hold paths to things that might want to be customized. ie. things that need to be built might have a specific output directory to point to
- Whats the fucking point in having version numbers if the contents under that fucking version number changes
- Embedding the vulkan shaders into the source code with a batch script to generate the source
- Adding frame info to layout data so animation panel setup can be based on layouts loaded
- Adding box selection to the keyframe window
- Adding alt drag duplication to the animation panel. fixes #62
- Fixing copy/paste behaviour. Now copies will make a hard copy of the
- Adding checks that the mouse is in the scrolling viewport/frame. Fixes #70
- Updating the example with some new behaviour
- Bit of an overhaul on how property edits happen re. focus changes etc
- Updating cmake build
- Update conanfile.py
- Fixed small issue with incorrect horizontal scrolling on the animation
- Updating lockfile. Might be better?
- Reverted lockfile
- Adding lockfile from linux
- Replacing imgui filebrowser with nativefiledialog.
- Created a fork for nativefiledialog so we can use cmake
- Fixing string buffer sizes
- Fixing extensions in native file dialog
- Tweaking to better support vscode
- Updating github actions
- Update CMakeLists.txt
- Updating github actions to better utilize conan cmake presets
- Added click to drag the keyframe window using the middle mouse button
- Update editor_panel_animation.cpp
- Building with harfbuzz and conan working. time to try linux
- Linux appears to be building now?
- Updating windows build. Conditionally added harfbuzz to conan install. Linking harfbuzz when using windows
- Using libgettext override because the configure of the latest version triggers a windows CRT popup that hard locks github runners
- Updated the visual studio solution to use the conan generated props files to handle the dependencies. This means most of the external submodules can now go away. Also cleaned up the github action slightly
- Shared linking with libpng. this should fix the crash with file dialogs
- Libpng shared only on linux. rebuilt lock
- Updated lock for linux
- Just testing commit signing etc
- Updating the docs a bit
- Adding clangd support. Fixing conan config so moth_ui links properly
- Updates for canyon
- Add [skip ci] to CHANGELOG commit to prevent re-triggering upload-lib
- Fix image preview aspect ratio and merge duplicate GetImage blocks
- Show image path relative to layout file in properties panel
- Add drag-to-reorder on animation panel node labels
- Improve unsaved-changes prompts with three-button dialogs
- Size confirm prompt buttons to fit their text content
- Fix tag glob pattern in release workflow
- Fix version.txt change detection to cover full push range
- Clear anchor press state on null-node early returns in BoundsWidget
- Fix scrollbar handle crossing and framePixelWidth zero-division
- Fix scrollbar handle crossing due to float-to-int rounding
- Fix scrollbar handle separation factor and tighten cliff.toml
- Prevent negative frames when dragging multi-selection items
- Guard MousePosToFrame against zero m_framePixelWidth

### Documentation
- Revamp README with features, updated badges, and build instructions

### Features
- Adding initial docked layout when imgui.ini doesnt exist.
- Moved the example from moth_ui into this repo.
- Add binary archive upload and GitHub Release creation to CI
- Only create GitHub Release when version.txt changes
- Draw animation track rows in reverse child order

### Miscellaneous
- Adding actions for building the editor
- Adding nativefiledialog external submodule
- Updating CMake and CI build with correct paths.
- Small updates to workflows.
- Updating build action to utilize a matrix
- Still trying to fix github actions
- Still fixing
- Almost there
- Updating both actions to utilize a matrix
- Testing if specific package configs are still needed.
- Removing unneeded windows define for example project.
- Updating build action to use official conan action.
- Adding private artifactory to before create step.
- Updated package upload name to match the built package name
- Added back libgtk-dev install step for linux.
- Updating example build to use conan action and matricies
- Updating the actions to be inline with other moth packages.
- Updating for the changes to moth and canyon
- Removing debug build from upload action
- This will work
- Adapt to updated canyon API and switch platform backend to SDL
- Add version.txt and read version from file in conan and cmake
- Move Artifactory URL to repository secret
- Add git-cliff changelog automation
- Updating example layout with proper 9slice setup

### Refactoring
- Updating editor to use canyon as a backend.
- Replace implicit clip drag handle arithmetic with named constants
- Clarify m_clickConsumed initialisation by extracting named condition
- Standardise selection lookups to use ranges::find_if
- Extract hardcoded colour literals to named constants
- Simplify and clarify animation panel
- Replace local moth_ui fmt formatters with canyon/utils/moth_ui_format.h


