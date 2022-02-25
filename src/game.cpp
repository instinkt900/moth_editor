#include "common.h"
#include "game.h"
#include "moth_ui/events/event_window_size.h"
#include "moth_ui/events/event_key.h"
#include "moth_ui/events/event_quit.h"
#include "moth_ui/events/event_mouse.h"
#include "editor/editor_layer.h"
#include "event_factory.h"
#include "moth_ui/context.h"
#include "image_factory.h"
#include "ui_renderer.h"

#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_sdlrenderer.h>

#include <SDL_image.h>
#include <SDL_ttf.h>

Game::Game()
    : m_windowWidth(INIT_WINDOW_WIDTH)
    , m_windowHeight(INIT_WINDOW_HEIGHT) {
    m_updateTicks = 1000 / 60;
}

Game::~Game() {
}

int Game::Run() {
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

bool Game::Initialise() {
    if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
        return false;
    }

    if (nullptr == (m_window = SDL_CreateWindow("UI Tool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_windowWidth, m_windowHeight, SDL_WINDOW_SHOWN))) {
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

    if (0 > TTF_Init()) {
        return false;
    }

    // if (0 > Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)) {
    //     return false;
    // }

    auto imageFactory = std::make_unique<ImageFactory>(*m_renderer);
    auto uiRenderer = std::make_unique<UIRenderer>(*m_renderer);
    ui::Context::Init(std::move(imageFactory), std::move(uiRenderer));

    m_layerStack = std::make_unique<LayerStack>(m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight);
    m_layerStack->PushLayer(std::make_unique<ui::EditorLayer>());

    return true;
}

void Game::OnEvent(Event const& event) {
    if (auto windowEvent = event_cast<EventWindowSize>(event)) {
        m_windowWidth = windowEvent->GetWidth();
        m_windowHeight = windowEvent->GetHeight();
    } else if (auto quitEvent = event_cast<EventQuit>(event)) {
        m_running = false;
    } else if (auto keyEvent = event_cast<EventKey>(event)) {
    }

    m_layerStack->OnEvent(event);
}

void Game::Update() {
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

void Game::Draw() {
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

void Game::Shutdown() {
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    IMG_Quit();
    SDL_Quit();
}
