#include "common.h"
#include "sdl_app.h"
#include "editor/editor_layer.h"
#include "editor/texture_packer.h"
#include "editor/actions/editor_action.h"
#include "editor/panels/editor_panel.h"

#include "sdl/sdl_ui_renderer.h"
#include "sdl/sdl_events.h"
#include "sdl/sdl_image_factory.h"
#include "sdl/sdl_font_factory.h"
#include "sdl/sdl_graphics.h"

#include "moth_ui/context.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"

#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_sdlrenderer.h>

namespace backend::sdl {
    char const* const Application::IMGUI_FILE = "imgui.ini";
    char const* const Application::PERSISTENCE_FILE = "editor.json";

    std::unique_ptr<IApplication> CreateApplication() {
        return std::make_unique<Application>();
    }

    Application::Application()
        : m_windowWidth(INIT_WINDOW_WIDTH)
        , m_windowHeight(INIT_WINDOW_HEIGHT) {

        m_imguiSettingsPath = (std::filesystem::current_path() / IMGUI_FILE).string();
        m_persistentFilePath = std::filesystem::current_path() / PERSISTENCE_FILE;
        std::ifstream persistenceFile(m_persistentFilePath.string());
        if (persistenceFile.is_open()) {
            try {
                persistenceFile >> m_persistentState;
            } catch (std::exception&) {
            }

            m_windowWidth = m_persistentState.value("window_width", m_windowWidth);
            m_windowHeight = m_persistentState.value("window_height", m_windowHeight);
        }

        g_App = this;
    }

    Application::~Application() {
        g_App = nullptr;
    }

    int Application::Run() {
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
                if (auto const translatedEvent = FromSDL(event)) {
                    OnEvent(*translatedEvent);
                }
            }

            Update();
            Draw();
        }

        Shutdown();

        return 0;
    }

    bool Application::Initialise() {
        if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
            return false;
        }

        if (nullptr == (m_window = SDL_CreateWindow("UI Tool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_windowWidth, m_windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE))) {
            return false;
        }

        if (nullptr == (m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC))) {
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = m_imguiSettingsPath.c_str();
        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer_Init(m_renderer);

        int const imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
        if (imgFlags != IMG_Init(imgFlags)) {
            return false;
        }

#ifdef _WIN32
        load_sdl_ttf_shim();
#endif

        m_graphics = std::make_unique<backend::sdl::SDLGraphics>(m_renderer);
        m_imageFactory = std::make_unique<ImageFactory>(*m_renderer);
        m_fontFactory = std::make_unique<FontFactory>(*m_renderer);
        m_uiRenderer = std::make_unique<UIRenderer>(*m_renderer);
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

    bool Application::OnEvent(moth_ui::Event const& event) {
        moth_ui::EventDispatch dispatch(event);
        dispatch.Dispatch(this, &Application::OnWindowSizeEvent);
        dispatch.Dispatch(this, &Application::OnQuitEvent);
        dispatch.Dispatch(m_layerStack.get());
        return dispatch.GetHandled();
    }

    void Application::SetWindowTitle(std::string const& title) {
        SDL_SetWindowTitle(m_window, title.c_str());
    }

    void Application::Update() {
        auto const nowTicks = SDL_GetTicks();
        auto const deltaTicks = nowTicks - m_lastUpdateTicks;
        m_lastUpdateTicks = nowTicks;
        m_layerStack->Update(deltaTicks);
    }

    void Application::Draw() {
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame(m_window);
        ImGui::NewFrame();

        m_layerStack->Draw();

        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(m_renderer);
    }

    void Application::Shutdown() {
        std::ofstream ofile(m_persistentFilePath.string());
        if (ofile.is_open()) {
            m_persistentState["current_path"] = std::filesystem::current_path().string();
            m_persistentState["window_width"] = m_windowWidth;
            m_persistentState["window_height"] = m_windowHeight;
            ofile << m_persistentState;
        }

        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        IMG_Quit();
        SDL_Quit();
    }

    bool Application::OnWindowSizeEvent(EventWindowSize const& event) {
        m_windowWidth = event.GetWidth();
        m_windowHeight = event.GetHeight();
        m_layerStack->SetWindowSize({ m_windowWidth, m_windowHeight });
        return true;
    }

    bool Application::OnQuitEvent(EventQuit const& event) {
        m_running = false;
        return true;
    }
}
