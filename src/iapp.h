#pragma once

#include "backends/igraphics.h"

class IApp {
public:
    virtual ~IApp() = default;

    virtual int Run() = 0;
    virtual void Stop() = 0;

    virtual void SetWindowTitle(std::string const& title) = 0;
    virtual nlohmann::json& GetPersistentState() = 0;

    virtual backend::IGraphics& GetGraphics() = 0;
};

extern IApp* g_App;
