#pragma once

#include "layers/layer_stack.h"
#include "events/event.h"

class App : public moth_ui::EventListener {
public:
    App();
    virtual ~App();

    int Run();
    void Stop() { m_running = false; }

    bool OnEvent(moth_ui::Event const& event) override;

    auto GetRenderer() const { return m_renderer; }

    void SetWindowTitle(std::string const& title);

    nlohmann::json& GetPersistentState() { return m_persistentState; }

protected:
    bool Initialise();
    
    void Update();
    void Draw();
    void Shutdown();

private:
    static int constexpr INIT_WINDOW_WIDTH = 1280;
    static int constexpr INIT_WINDOW_HEIGHT = 960;

    int m_windowWidth = 0;
    int m_windowHeight = 0;

    bool m_running = false;
    bool m_paused = false;
    bool m_editorMode = false;
    uint32_t m_updateTicks = 0;
    uint32_t m_lastUpdateTicks = 0;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    moth_ui::IntVec2 m_gameWindowPos;

    std::unique_ptr<LayerStack> m_layerStack;

    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const PERSISTENCE_FILE;

    bool OnWindowSizeEvent(EventWindowSize const& event);
    bool OnQuitEvent(EventQuit const& event);
};
