#pragma once

#include "moth_ui/events/event.h"

class EventFactory {
public:
    static std::unique_ptr<Event> FromSDL(SDL_Event const& event);
};

