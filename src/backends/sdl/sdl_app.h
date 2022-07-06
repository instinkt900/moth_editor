#pragma once

#include "../../iapp.h"
#include "layers/layer_stack.h"
#include "events/event.h"
#include "moth_ui/iimage_factory.h"
#include "moth_ui/ifont_factory.h"
#include "moth_ui/irenderer.h"

class SDLApp : public IApp, public moth_ui::EventListener {
public:
    SDLApp();
    virtual ~SDLApp();

    int Run() override;
    void Stop() override { m_running = false; }

    bool OnEvent(moth_ui::Event const& event) override;

    void SetWindowTitle(std::string const& title) override;

    nlohmann::json& GetPersistentState() override { return m_persistentState; }

    SDL_Renderer* GetRenderer() const { return m_renderer; }

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
    uint32_t m_lastUpdateTicks;

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    moth_ui::IntVec2 m_gameWindowPos;

    std::unique_ptr<LayerStack> m_layerStack;

    std::filesystem::path m_originalCwd;
    std::filesystem::path m_persistentFilePath;
    nlohmann::json m_persistentState;
    static char const* const PERSISTENCE_FILE;

    std::unique_ptr<moth_ui::IImageFactory> m_imageFactory;
    std::unique_ptr<moth_ui::IFontFactory> m_fontFactory;
    std::unique_ptr<moth_ui::IRenderer> m_uiRenderer;

    bool OnWindowSizeEvent(EventWindowSize const& event);
    bool OnQuitEvent(EventQuit const& event);
};

extern SDLApp* g_sdlApp;
