#pragma once

#include "moth_ui/events/event.h"

#include <SDL.h>

namespace backend::sdl {
    std::unique_ptr<moth_ui::Event> FromSDL(SDL_Event const& event);
}

