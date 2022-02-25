#pragma once

#include "moth_ui/events/event.h"

class EventFactory {
public:
    static std::unique_ptr<moth_ui::Event> FromSDL(SDL_Event const& event);
};

