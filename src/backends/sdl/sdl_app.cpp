#include "common.h"
#include "sdl_app.h"
#include "editor/editor_layer.h"

#include "sdl_event_factory.h"
#include "sdl_image_factory.h"
#include "sdl_font_factory.h"
#include "sdl_ui_renderer.h"

#include "moth_ui/context.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"

#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_sdlrenderer.h>

SDLApp* g_sdlApp = nullptr;
extern IApp* g_App;

char const* const SDLApp::PERSISTENCE_FILE = "editor.json";

SDLApp::SDLApp()
    : m_windowWidth(INIT_WINDOW_WIDTH)
    , m_windowHeight(INIT_WINDOW_HEIGHT) {

    m_persistentFilePath = std::filesystem::current_path() / PERSISTENCE_FILE;
    std::ifstream persistenceFile(m_persistentFilePath.string());
    if (persistenceFile.is_open()) {
        try {
            persistenceFile >> m_persistentState;
        } catch (std::exception) {
        }

        m_windowWidth = m_persistentState.value("window_width", m_windowWidth);
        m_windowHeight = m_persistentState.value("window_height", m_windowHeight);
    }

    g_sdlApp = this;
    g_App = this;
}

SDLApp::~SDLApp() {
    g_sdlApp = nullptr;
    g_App = nullptr;
}

int SDLApp::Run() {
    if (!Initialise()) {
        return 1;
    }

    m_running = true;
    m_lastUpdateTicks = SDL_GetTicks();

    while (m_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (ImGui::GetIO().WantCaptureKeyboard && (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)) {
                continue;
            }
            if (auto const translatedEvent = SDLEventFactory::FromSDL(event)) {
                OnEvent(*translatedEvent);
            }
        }

        Update();
        Draw();
    }

    Shutdown();

    return 0;
}

bool SDLApp::Initialise() {
    if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
        return false;
    }

    if (nullptr == (m_window = SDL_CreateWindow("UI Tool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_windowWidth, m_windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE))) {
        return false;
    }

    if (nullptr == (m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC))) {
        return false;
    }

    m_originalCwd = std::filesystem::current_path();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::LoadIniSettingsFromDisk((m_originalCwd / "imgui.ini").string().c_str());

    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer_Init(m_renderer);

    int const imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
    if (imgFlags != IMG_Init(imgFlags)) {
        return false;
    }

    m_imageFactory = std::make_unique<SDLImageFactory>(*m_renderer);
    m_fontFactory = std::make_unique<SDLFontFactory>(*m_renderer);
    m_uiRenderer = std::make_unique<SDLUIRenderer>(*m_renderer);
    auto uiContext = std::make_shared<moth_ui::Context>(m_imageFactory.get(), m_fontFactory.get(), m_uiRenderer.get());
    moth_ui::Context::SetCurrentContext(uiContext);

    
    if (m_persistentState.contains("current_path")) {
        std::string const currentPath = m_persistentState["current_path"];
        if (std::filesystem::exists(currentPath)) {
            std::filesystem::current_path(currentPath);
        }
    }

    m_layerStack = std::make_unique<LayerStack>(m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight);
    m_layerStack->SetEventListener(this);
    m_layerStack->PushLayer(std::make_unique<EditorLayer>());

    return true;
}

bool SDLApp::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &SDLApp::OnWindowSizeEvent);
    dispatch.Dispatch(this, &SDLApp::OnQuitEvent);
    dispatch.Dispatch(m_layerStack.get());
    return dispatch.GetHandled();
}

void SDLApp::SetWindowTitle(std::string const& title) {
    SDL_SetWindowTitle(m_window, title.c_str());
}

void SDLApp::Update() {
    auto const nowTicks = SDL_GetTicks();
    auto const deltaTicks = nowTicks - m_lastUpdateTicks;
    m_lastUpdateTicks = nowTicks;
    m_layerStack->Update(deltaTicks);
}

void SDLApp::Draw() {
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window);
    ImGui::NewFrame();

    m_layerStack->Draw(*m_renderer);

    ImGui::Render();
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(m_renderer);
}

void SDLApp::Shutdown() {
    std::ofstream ofile(m_persistentFilePath.string());
    if (ofile.is_open()) {
        m_persistentState["current_path"] = std::filesystem::current_path().string();
        m_persistentState["window_width"] = m_windowWidth;
        m_persistentState["window_height"] = m_windowHeight;
        ofile << m_persistentState;
    }
    ImGui::SaveIniSettingsToDisk((m_originalCwd / "imgui.ini").string().c_str());
    std::filesystem::current_path(m_originalCwd);

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    IMG_Quit();
    SDL_Quit();
}

bool SDLApp::OnWindowSizeEvent(EventWindowSize const& event) {
    m_windowWidth = event.GetWidth();
    m_windowHeight = event.GetHeight();
    m_layerStack->SetWindowSize({ m_windowWidth, m_windowHeight });
    return true;
}

bool SDLApp::OnQuitEvent(EventQuit const& event) {
    m_running = false;
    return true;
}
