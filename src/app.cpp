#include "common.h"
#include "app.h"
#include "editor/editor_layer.h"
#include "event_factory.h"
#include "moth_ui/context.h"
#include "image_factory.h"
#include "ui_renderer.h"
#include "moth_ui/event_dispatch.h"

#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_sdlrenderer.h>

App::App()
    : m_windowWidth(INIT_WINDOW_WIDTH)
    , m_windowHeight(INIT_WINDOW_HEIGHT) {
    m_updateTicks = 1000 / 60;

    std::ifstream ifile("window_config.json");
    if (ifile.is_open()) {
        nlohmann::json json;
        ifile >> json;
        json.at("window_width").get_to(m_windowWidth);
        json.at("window_height").get_to(m_windowHeight);
    }
}

App::~App() {
    std::ofstream ofile("window_config.json");
    if (ofile.is_open()) {
        nlohmann::json json;
        json["window_width"] = m_windowWidth;
        json["window_height"] = m_windowHeight;
        ofile << json;
    }
}

int App::Run() {
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
            if (ImGui::GetIO().WantCaptureMouse && (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL)) {
                continue;
            }
            if (auto const translatedEvent = EventFactory::FromSDL(event)) {
                OnEvent(*translatedEvent);
            }
        }

        Update();
        Draw();
    }

    Shutdown();

    return 0;
}

bool App::Initialise() {
    if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
        return false;
    }

    if (nullptr == (m_window = SDL_CreateWindow("UI Tool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_windowWidth, m_windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE))) {
        return false;
    }

    if (nullptr == (m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED))) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer_Init(m_renderer);

    int const imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
    if (imgFlags != IMG_Init(imgFlags)) {
        return false;
    }

    auto imageFactory = std::make_unique<ImageFactory>(*m_renderer);
    auto uiRenderer = std::make_unique<UIRenderer>(*m_renderer);
    moth_ui::Context::Init(std::move(imageFactory), std::move(uiRenderer));

    m_layerStack = std::make_unique<LayerStack>(m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight);
    m_layerStack->SetEventListener(this);
    m_layerStack->PushLayer(std::make_unique<EditorLayer>());

    return true;
}

bool App::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &App::OnWindowSizeEvent);
    dispatch.Dispatch(this, &App::OnQuitEvent);
    dispatch.Dispatch(m_layerStack.get());
    return dispatch.GetHandled();
}

void App::Update() {
    uint32_t const nowTicks = SDL_GetTicks();
    uint32_t deltaTicks = nowTicks - m_lastUpdateTicks;
    while (deltaTicks > m_updateTicks) {
        if (!m_paused) {
            m_layerStack->Update(m_updateTicks);
        }
        m_lastUpdateTicks += m_updateTicks;
        deltaTicks -= m_updateTicks;
    }
}

void App::Draw() {
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window);
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(m_renderer);

    m_layerStack->Draw(*m_renderer);

    ImGui::Render();
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(m_renderer);
}

void App::Shutdown() {
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    IMG_Quit();
    SDL_Quit();
}

bool App::OnWindowSizeEvent(moth_ui::EventWindowSize const& event) {
    m_windowWidth = event.GetWidth();
    m_windowHeight = event.GetHeight();
    m_layerStack->SetWindowSize({ m_windowWidth, m_windowHeight });
    return true;
}

bool App::OnQuitEvent(moth_ui::EventQuit const& event) {
    m_running = false;
    return true;
}
