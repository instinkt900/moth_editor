#pragma once

class IApp {
public:
    virtual ~IApp() = default;

    virtual int Run() = 0;
    virtual void Stop() = 0;

    virtual void SetWindowTitle(std::string const& title) = 0;
    virtual nlohmann::json& GetPersistentState() = 0;
};
