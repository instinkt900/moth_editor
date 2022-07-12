#pragma once

#include "igraphics_context.h"

namespace backend {
    class IApplication {
    public:
        virtual ~IApplication() = default;

        virtual int Run() = 0;
        virtual void Stop() = 0;

        virtual void SetWindowTitle(std::string const& title) = 0;
        virtual nlohmann::json& GetPersistentState() = 0;

        virtual backend::IGraphicsContext& GetGraphics() = 0;
    };
}

extern backend::IApplication* g_App;
