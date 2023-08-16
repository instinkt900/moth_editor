#include "common.h"
#include "sdl_events.h"

#include "moth_ui/events/event_key.h"
#include "moth_ui/events/event_mouse.h"
#include "events/event.h"

#include <SDL.h>

namespace {
    moth_ui::MouseButton FromSDLMouse(uint8_t button) {
        switch (button) {
        case 1:
            return moth_ui::MouseButton::Left;
        case 2:
            return moth_ui::MouseButton::Middle;
        case 3:
            return moth_ui::MouseButton::Right;
        default:
            return moth_ui::MouseButton::Unknown;
        }
    }

    moth_ui::Key FromSDLKey(SDL_Keycode const& code) {
        switch (code) {
        case SDLK_RETURN:
            return moth_ui::Key::Return;
        case SDLK_ESCAPE:
            return moth_ui::Key::Escape;
        case SDLK_BACKSPACE:
            return moth_ui::Key::Backspace;
        case SDLK_TAB:
            return moth_ui::Key::Tab;
        case SDLK_SPACE:
            return moth_ui::Key::Space;
        case SDLK_EXCLAIM:
            return moth_ui::Key::Exclaim;
        case SDLK_QUOTEDBL:
            return moth_ui::Key::Quotedbl;
        case SDLK_HASH:
            return moth_ui::Key::Hash;
        case SDLK_PERCENT:
            return moth_ui::Key::Percent;
        case SDLK_DOLLAR:
            return moth_ui::Key::Dollar;
        case SDLK_AMPERSAND:
            return moth_ui::Key::Ampersand;
        case SDLK_QUOTE:
            return moth_ui::Key::Quote;
        case SDLK_LEFTPAREN:
            return moth_ui::Key::Leftparen;
        case SDLK_RIGHTPAREN:
            return moth_ui::Key::Rightparen;
        case SDLK_ASTERISK:
            return moth_ui::Key::Asterisk;
        case SDLK_PLUS:
            return moth_ui::Key::Plus;
        case SDLK_COMMA:
            return moth_ui::Key::Comma;
        case SDLK_MINUS:
            return moth_ui::Key::Minus;
        case SDLK_PERIOD:
            return moth_ui::Key::Period;
        case SDLK_SLASH:
            return moth_ui::Key::Slash;
        case SDLK_0:
            return moth_ui::Key::N0;
        case SDLK_1:
            return moth_ui::Key::N1;
        case SDLK_2:
            return moth_ui::Key::N2;
        case SDLK_3:
            return moth_ui::Key::N3;
        case SDLK_4:
            return moth_ui::Key::N4;
        case SDLK_5:
            return moth_ui::Key::N5;
        case SDLK_6:
            return moth_ui::Key::N6;
        case SDLK_7:
            return moth_ui::Key::N7;
        case SDLK_8:
            return moth_ui::Key::N8;
        case SDLK_9:
            return moth_ui::Key::N9;
        case SDLK_COLON:
            return moth_ui::Key::Colon;
        case SDLK_SEMICOLON:
            return moth_ui::Key::Semicolon;
        case SDLK_LESS:
            return moth_ui::Key::Less;
        case SDLK_EQUALS:
            return moth_ui::Key::Equals;
        case SDLK_GREATER:
            return moth_ui::Key::Greater;
        case SDLK_QUESTION:
            return moth_ui::Key::Question;
        case SDLK_AT:
            return moth_ui::Key::At;

        case SDLK_LEFTBRACKET:
            return moth_ui::Key::Leftbracket;
        case SDLK_BACKSLASH:
            return moth_ui::Key::Backslash;
        case SDLK_RIGHTBRACKET:
            return moth_ui::Key::Rightbracket;
        case SDLK_CARET:
            return moth_ui::Key::Caret;
        case SDLK_UNDERSCORE:
            return moth_ui::Key::Underscore;
        case SDLK_BACKQUOTE:
            return moth_ui::Key::Backquote;
        case SDLK_a:
            return moth_ui::Key::A;
        case SDLK_b:
            return moth_ui::Key::B;
        case SDLK_c:
            return moth_ui::Key::C;
        case SDLK_d:
            return moth_ui::Key::D;
        case SDLK_e:
            return moth_ui::Key::E;
        case SDLK_f:
            return moth_ui::Key::F;
        case SDLK_g:
            return moth_ui::Key::G;
        case SDLK_h:
            return moth_ui::Key::H;
        case SDLK_i:
            return moth_ui::Key::I;
        case SDLK_j:
            return moth_ui::Key::J;
        case SDLK_k:
            return moth_ui::Key::K;
        case SDLK_l:
            return moth_ui::Key::L;
        case SDLK_m:
            return moth_ui::Key::M;
        case SDLK_n:
            return moth_ui::Key::N;
        case SDLK_o:
            return moth_ui::Key::O;
        case SDLK_p:
            return moth_ui::Key::P;
        case SDLK_q:
            return moth_ui::Key::Q;
        case SDLK_r:
            return moth_ui::Key::R;
        case SDLK_s:
            return moth_ui::Key::S;
        case SDLK_t:
            return moth_ui::Key::T;
        case SDLK_u:
            return moth_ui::Key::U;
        case SDLK_v:
            return moth_ui::Key::V;
        case SDLK_w:
            return moth_ui::Key::W;
        case SDLK_x:
            return moth_ui::Key::X;
        case SDLK_y:
            return moth_ui::Key::Y;
        case SDLK_z:
            return moth_ui::Key::Z;

        case SDLK_CAPSLOCK:
            return moth_ui::Key::Capslock;

        case SDLK_F1:
            return moth_ui::Key::F1;
        case SDLK_F2:
            return moth_ui::Key::F2;
        case SDLK_F3:
            return moth_ui::Key::F3;
        case SDLK_F4:
            return moth_ui::Key::F4;
        case SDLK_F5:
            return moth_ui::Key::F5;
        case SDLK_F6:
            return moth_ui::Key::F6;
        case SDLK_F7:
            return moth_ui::Key::F7;
        case SDLK_F8:
            return moth_ui::Key::F8;
        case SDLK_F9:
            return moth_ui::Key::F9;
        case SDLK_F10:
            return moth_ui::Key::F10;
        case SDLK_F11:
            return moth_ui::Key::F11;
        case SDLK_F12:
            return moth_ui::Key::F12;

        case SDLK_PRINTSCREEN:
            return moth_ui::Key::Printscreen;
        case SDLK_SCROLLLOCK:
            return moth_ui::Key::Scrolllock;
        case SDLK_PAUSE:
            return moth_ui::Key::Pause;
        case SDLK_INSERT:
            return moth_ui::Key::Insert;
        case SDLK_HOME:
            return moth_ui::Key::Home;
        case SDLK_PAGEUP:
            return moth_ui::Key::Pageup;
        case SDLK_DELETE:
            return moth_ui::Key::Delete;
        case SDLK_END:
            return moth_ui::Key::End;
        case SDLK_PAGEDOWN:
            return moth_ui::Key::Pagedown;
        case SDLK_RIGHT:
            return moth_ui::Key::Right;
        case SDLK_LEFT:
            return moth_ui::Key::Left;
        case SDLK_DOWN:
            return moth_ui::Key::Down;
        case SDLK_UP:
            return moth_ui::Key::Up;

        case SDLK_NUMLOCKCLEAR:
            return moth_ui::Key::Numlockclear;
        case SDLK_KP_DIVIDE:
            return moth_ui::Key::KP_Divide;
        case SDLK_KP_MULTIPLY:
            return moth_ui::Key::KP_Multiply;
        case SDLK_KP_MINUS:
            return moth_ui::Key::KP_Minus;
        case SDLK_KP_PLUS:
            return moth_ui::Key::KP_Plus;
        case SDLK_KP_ENTER:
            return moth_ui::Key::KP_Enter;
        case SDLK_KP_1:
            return moth_ui::Key::KP_1;
        case SDLK_KP_2:
            return moth_ui::Key::KP_2;
        case SDLK_KP_3:
            return moth_ui::Key::KP_3;
        case SDLK_KP_4:
            return moth_ui::Key::KP_4;
        case SDLK_KP_5:
            return moth_ui::Key::KP_5;
        case SDLK_KP_6:
            return moth_ui::Key::KP_6;
        case SDLK_KP_7:
            return moth_ui::Key::KP_7;
        case SDLK_KP_8:
            return moth_ui::Key::KP_8;
        case SDLK_KP_9:
            return moth_ui::Key::KP_9;
        case SDLK_KP_0:
            return moth_ui::Key::KP_0;
        case SDLK_KP_PERIOD:
            return moth_ui::Key::KP_Period;

        case SDLK_LCTRL:
            return moth_ui::Key::Lctrl;
        case SDLK_LSHIFT:
            return moth_ui::Key::Lshift;
        case SDLK_LALT:
            return moth_ui::Key::Lalt;
        case SDLK_RCTRL:
            return moth_ui::Key::Rctrl;
        case SDLK_RSHIFT:
            return moth_ui::Key::Rshift;
        case SDLK_RALT:
            return moth_ui::Key::Ralt;

        default:
            return moth_ui::Key::Unknown;
        }
    }
}

namespace backend::sdl {
    std::unique_ptr<moth_ui::Event> FromSDL(SDL_Event const& event) {
        switch (event.type) {
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                return std::make_unique<EventWindowSize>(event.window.data1, event.window.data2);
            default:
                return nullptr;
            }
            }
        }
        case SDL_QUIT: {
            return std::make_unique<EventRequestQuit>();
        }
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            moth_ui::KeyAction action = event.type == SDL_KEYUP ? moth_ui::KeyAction::Up : moth_ui::KeyAction::Down;
            int mods = 0;
            if ((event.key.keysym.mod & KMOD_LSHIFT) != 0)
                mods |= moth_ui::KeyMod_LeftShift;
            if ((event.key.keysym.mod & KMOD_RSHIFT) != 0)
                mods |= moth_ui::KeyMod_RightShift;
            if ((event.key.keysym.mod & KMOD_LALT) != 0)
                mods |= moth_ui::KeyMod_LeftAlt;
            if ((event.key.keysym.mod & KMOD_RALT) != 0)
                mods |= moth_ui::KeyMod_RightAlt;
            if ((event.key.keysym.mod & KMOD_LCTRL) != 0)
                mods |= moth_ui::KeyMod_LeftCtrl;
            if ((event.key.keysym.mod & KMOD_RCTRL) != 0)
                mods |= moth_ui::KeyMod_RightCtrl;
            return std::make_unique<moth_ui::EventKey>(action, FromSDLKey(event.key.keysym.sym), mods);
        }
        case SDL_RENDER_DEVICE_RESET: {
            return std::make_unique<EventRenderDeviceReset>();
        }
        case SDL_RENDER_TARGETS_RESET: {
            return std::make_unique<EventRenderTargetReset>();
        }
        case SDL_MOUSEBUTTONDOWN: {
            return std::make_unique<moth_ui::EventMouseDown>(FromSDLMouse(event.button.button), moth_ui::IntVec2{ event.button.x, event.button.y });
        }
        case SDL_MOUSEBUTTONUP: {
            return std::make_unique<moth_ui::EventMouseUp>(FromSDLMouse(event.button.button), moth_ui::IntVec2{ event.button.x, event.button.y });
        }
        case SDL_MOUSEMOTION: {
            return std::make_unique<moth_ui::EventMouseMove>(moth_ui::IntVec2{ event.motion.x, event.motion.y }, moth_ui::FloatVec2{ static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel) });
        }
        case SDL_MOUSEWHEEL: {
            return std::make_unique<moth_ui::EventMouseWheel>(moth_ui::IntVec2{ event.wheel.x, event.wheel.y });
        }
        }
        return nullptr;
    }
}
