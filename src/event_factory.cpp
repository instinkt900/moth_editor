#include "common.h"
#include "event_factory.h"

#include "moth_ui/events/event_window_size.h"
#include "moth_ui/events/event_quit.h"
#include "moth_ui/events/event_key.h"
#include "moth_ui/events/event_device.h"
#include "moth_ui/events/event_mouse.h"

MouseButton FromSDLMouse(uint8_t button);
Key FromSDLKey(SDL_Keycode const& code);

std::unique_ptr<Event> EventFactory::FromSDL(SDL_Event const& event) {
    switch (event.type) {
    case SDL_WINDOWEVENT: {
        switch (event.window.type) {
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
            return std::make_unique<EventWindowSize>(event.window.data1, event.window.data2);
        default:
            return nullptr;
        }
        }
    }
    case SDL_QUIT: {
        return std::make_unique<EventQuit>();
    }
    case SDL_KEYUP: {
        return std::make_unique<EventKey>(KeyAction::Up, FromSDLKey(event.key.keysym.sym));
    }
    case SDL_KEYDOWN: {
        return std::make_unique<EventKey>(KeyAction::Down, FromSDLKey(event.key.keysym.sym));
    }
    case SDL_RENDER_DEVICE_RESET: {
        return std::make_unique<EventRenderDeviceReset>();
    }
    case SDL_RENDER_TARGETS_RESET: {
        return std::make_unique<EventRenderTargetReset>();
    }
    case SDL_MOUSEBUTTONDOWN: {
        return std::make_unique<EventMouseDown>(FromSDLMouse(event.button.button), IntVec2{ event.button.x, event.button.y });
    }
    case SDL_MOUSEBUTTONUP: {
        return std::make_unique<EventMouseUp>(FromSDLMouse(event.button.button), IntVec2{ event.button.x, event.button.y });
    }
    case SDL_MOUSEMOTION: {
        return std::make_unique<EventMouseMove>(IntVec2{ event.motion.x, event.motion.y }, FloatVec2{ static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel) });
    }
    case SDL_MOUSEWHEEL: {
        return std::make_unique<EventMouseWheel>(IntVec2{ event.wheel.x, event.wheel.y });
    }
    }
    return nullptr;
}

MouseButton FromSDLMouse(uint8_t button) {
    switch (button) {
    case 1:
        return MouseButton::Left;
    case 2:
        return MouseButton::Middle;
    case 3:
        return MouseButton::Right;
    default:
        return MouseButton::Unknown;
    }
}

Key FromSDLKey(SDL_Keycode const& code) {
    switch (code) {
    case SDLK_RETURN:
        return Key::Return;
    case SDLK_ESCAPE:
        return Key::Escape;
    case SDLK_BACKSPACE:
        return Key::Backspace;
    case SDLK_TAB:
        return Key::Tab;
    case SDLK_SPACE:
        return Key::Space;
    case SDLK_EXCLAIM:
        return Key::Exclaim;
    case SDLK_QUOTEDBL:
        return Key::Quotedbl;
    case SDLK_HASH:
        return Key::Hash;
    case SDLK_PERCENT:
        return Key::Percent;
    case SDLK_DOLLAR:
        return Key::Dollar;
    case SDLK_AMPERSAND:
        return Key::Ampersand;
    case SDLK_QUOTE:
        return Key::Quote;
    case SDLK_LEFTPAREN:
        return Key::Leftparen;
    case SDLK_RIGHTPAREN:
        return Key::Rightparen;
    case SDLK_ASTERISK:
        return Key::Asterisk;
    case SDLK_PLUS:
        return Key::Plus;
    case SDLK_COMMA:
        return Key::Comma;
    case SDLK_MINUS:
        return Key::Minus;
    case SDLK_PERIOD:
        return Key::Period;
    case SDLK_SLASH:
        return Key::Slash;
    case SDLK_0:
        return Key::N0;
    case SDLK_1:
        return Key::N1;
    case SDLK_2:
        return Key::N2;
    case SDLK_3:
        return Key::N3;
    case SDLK_4:
        return Key::N4;
    case SDLK_5:
        return Key::N5;
    case SDLK_6:
        return Key::N6;
    case SDLK_7:
        return Key::N7;
    case SDLK_8:
        return Key::N8;
    case SDLK_9:
        return Key::N9;
    case SDLK_COLON:
        return Key::Colon;
    case SDLK_SEMICOLON:
        return Key::Semicolon;
    case SDLK_LESS:
        return Key::Less;
    case SDLK_EQUALS:
        return Key::Equals;
    case SDLK_GREATER:
        return Key::Greater;
    case SDLK_QUESTION:
        return Key::Question;
    case SDLK_AT:
        return Key::At;

    case SDLK_LEFTBRACKET:
        return Key::Leftbracket;
    case SDLK_BACKSLASH:
        return Key::Backslash;
    case SDLK_RIGHTBRACKET:
        return Key::Rightbracket;
    case SDLK_CARET:
        return Key::Caret;
    case SDLK_UNDERSCORE:
        return Key::Underscore;
    case SDLK_BACKQUOTE:
        return Key::Backquote;
    case SDLK_a:
        return Key::A;
    case SDLK_b:
        return Key::B;
    case SDLK_c:
        return Key::C;
    case SDLK_d:
        return Key::D;
    case SDLK_e:
        return Key::E;
    case SDLK_f:
        return Key::F;
    case SDLK_g:
        return Key::G;
    case SDLK_h:
        return Key::H;
    case SDLK_i:
        return Key::I;
    case SDLK_j:
        return Key::J;
    case SDLK_k:
        return Key::K;
    case SDLK_l:
        return Key::L;
    case SDLK_m:
        return Key::M;
    case SDLK_n:
        return Key::N;
    case SDLK_o:
        return Key::O;
    case SDLK_p:
        return Key::P;
    case SDLK_q:
        return Key::Q;
    case SDLK_r:
        return Key::R;
    case SDLK_s:
        return Key::S;
    case SDLK_t:
        return Key::T;
    case SDLK_u:
        return Key::U;
    case SDLK_v:
        return Key::V;
    case SDLK_w:
        return Key::W;
    case SDLK_x:
        return Key::X;
    case SDLK_y:
        return Key::Y;
    case SDLK_z:
        return Key::Z;

    case SDLK_CAPSLOCK:
        return Key::Capslock;

    case SDLK_F1:
        return Key::F1;
    case SDLK_F2:
        return Key::F2;
    case SDLK_F3:
        return Key::F3;
    case SDLK_F4:
        return Key::F4;
    case SDLK_F5:
        return Key::F5;
    case SDLK_F6:
        return Key::F6;
    case SDLK_F7:
        return Key::F7;
    case SDLK_F8:
        return Key::F8;
    case SDLK_F9:
        return Key::F9;
    case SDLK_F10:
        return Key::F10;
    case SDLK_F11:
        return Key::F11;
    case SDLK_F12:
        return Key::F12;

    case SDLK_PRINTSCREEN:
        return Key::Printscreen;
    case SDLK_SCROLLLOCK:
        return Key::Scrolllock;
    case SDLK_PAUSE:
        return Key::Pause;
    case SDLK_INSERT:
        return Key::Insert;
    case SDLK_HOME:
        return Key::Home;
    case SDLK_PAGEUP:
        return Key::Pageup;
    case SDLK_DELETE:
        return Key::Delete;
    case SDLK_END:
        return Key::End;
    case SDLK_PAGEDOWN:
        return Key::Pagedown;
    case SDLK_RIGHT:
        return Key::Right;
    case SDLK_LEFT:
        return Key::Left;
    case SDLK_DOWN:
        return Key::Down;
    case SDLK_UP:
        return Key::Up;

    case SDLK_NUMLOCKCLEAR:
        return Key::Numlockclear;
    case SDLK_KP_DIVIDE:
        return Key::KP_Divide;
    case SDLK_KP_MULTIPLY:
        return Key::KP_Multiply;
    case SDLK_KP_MINUS:
        return Key::KP_Minus;
    case SDLK_KP_PLUS:
        return Key::KP_Plus;
    case SDLK_KP_ENTER:
        return Key::KP_Enter;
    case SDLK_KP_1:
        return Key::KP_1;
    case SDLK_KP_2:
        return Key::KP_2;
    case SDLK_KP_3:
        return Key::KP_3;
    case SDLK_KP_4:
        return Key::KP_4;
    case SDLK_KP_5:
        return Key::KP_5;
    case SDLK_KP_6:
        return Key::KP_6;
    case SDLK_KP_7:
        return Key::KP_7;
    case SDLK_KP_8:
        return Key::KP_8;
    case SDLK_KP_9:
        return Key::KP_9;
    case SDLK_KP_0:
        return Key::KP_0;
    case SDLK_KP_PERIOD:
        return Key::KP_Period;

    case SDLK_LCTRL:
        return Key::Lctrl;
    case SDLK_LSHIFT:
        return Key::Lshift;
    case SDLK_LALT:
        return Key::Lalt;
    case SDLK_RCTRL:
        return Key::Rctrl;
    case SDLK_RSHIFT:
        return Key::Rshift;
    case SDLK_RALT:
        return Key::Ralt;

    default:
        return Key::Unknown;
    }
}
