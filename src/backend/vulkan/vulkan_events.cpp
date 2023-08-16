#include "common.h"
#include "vulkan_events.h"

#include "moth_ui/events/event_key.h"
#include "moth_ui/events/event_mouse.h"
#include "events/event.h"

namespace {
    moth_ui::Key FromGLFWKey(int key) {
        switch (key) {
        case GLFW_KEY_ENTER:
            return moth_ui::Key::Return;
        case GLFW_KEY_ESCAPE:
            return moth_ui::Key::Escape;
        case GLFW_KEY_BACKSPACE:
            return moth_ui::Key::Backspace;
        case GLFW_KEY_TAB:
            return moth_ui::Key::Tab;
        case GLFW_KEY_SPACE:
            return moth_ui::Key::Space;
        case GLFW_KEY_COMMA:
            return moth_ui::Key::Comma;
        case GLFW_KEY_MINUS:
            return moth_ui::Key::Minus;
        case GLFW_KEY_PERIOD:
            return moth_ui::Key::Period;
        case GLFW_KEY_SLASH:
            return moth_ui::Key::Slash;
        case GLFW_KEY_0:
            return moth_ui::Key::N0;
        case GLFW_KEY_1:
            return moth_ui::Key::N1;
        case GLFW_KEY_2:
            return moth_ui::Key::N2;
        case GLFW_KEY_3:
            return moth_ui::Key::N3;
        case GLFW_KEY_4:
            return moth_ui::Key::N4;
        case GLFW_KEY_5:
            return moth_ui::Key::N5;
        case GLFW_KEY_6:
            return moth_ui::Key::N6;
        case GLFW_KEY_7:
            return moth_ui::Key::N7;
        case GLFW_KEY_8:
            return moth_ui::Key::N8;
        case GLFW_KEY_9:
            return moth_ui::Key::N9;
        case GLFW_KEY_SEMICOLON:
            return moth_ui::Key::Semicolon;

        case GLFW_KEY_LEFT_BRACKET:
            return moth_ui::Key::Leftbracket;
        case GLFW_KEY_BACKSLASH:
            return moth_ui::Key::Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return moth_ui::Key::Rightbracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return moth_ui::Key::Backquote;
        case GLFW_KEY_A:
            return moth_ui::Key::A;
        case GLFW_KEY_B:
            return moth_ui::Key::B;
        case GLFW_KEY_C:
            return moth_ui::Key::C;
        case GLFW_KEY_D:
            return moth_ui::Key::D;
        case GLFW_KEY_E:
            return moth_ui::Key::E;
        case GLFW_KEY_F:
            return moth_ui::Key::F;
        case GLFW_KEY_G:
            return moth_ui::Key::G;
        case GLFW_KEY_H:
            return moth_ui::Key::H;
        case GLFW_KEY_I:
            return moth_ui::Key::I;
        case GLFW_KEY_J:
            return moth_ui::Key::J;
        case GLFW_KEY_K:
            return moth_ui::Key::K;
        case GLFW_KEY_L:
            return moth_ui::Key::L;
        case GLFW_KEY_M:
            return moth_ui::Key::M;
        case GLFW_KEY_N:
            return moth_ui::Key::N;
        case GLFW_KEY_O:
            return moth_ui::Key::O;
        case GLFW_KEY_P:
            return moth_ui::Key::P;
        case GLFW_KEY_Q:
            return moth_ui::Key::Q;
        case GLFW_KEY_R:
            return moth_ui::Key::R;
        case GLFW_KEY_S:
            return moth_ui::Key::S;
        case GLFW_KEY_T:
            return moth_ui::Key::T;
        case GLFW_KEY_U:
            return moth_ui::Key::U;
        case GLFW_KEY_V:
            return moth_ui::Key::V;
        case GLFW_KEY_W:
            return moth_ui::Key::W;
        case GLFW_KEY_X:
            return moth_ui::Key::X;
        case GLFW_KEY_Y:
            return moth_ui::Key::Y;
        case GLFW_KEY_Z:
            return moth_ui::Key::Z;

        case GLFW_KEY_CAPS_LOCK:
            return moth_ui::Key::Capslock;

        case GLFW_KEY_F1:
            return moth_ui::Key::F1;
        case GLFW_KEY_F2:
            return moth_ui::Key::F2;
        case GLFW_KEY_F3:
            return moth_ui::Key::F3;
        case GLFW_KEY_F4:
            return moth_ui::Key::F4;
        case GLFW_KEY_F5:
            return moth_ui::Key::F5;
        case GLFW_KEY_F6:
            return moth_ui::Key::F6;
        case GLFW_KEY_F7:
            return moth_ui::Key::F7;
        case GLFW_KEY_F8:
            return moth_ui::Key::F8;
        case GLFW_KEY_F9:
            return moth_ui::Key::F9;
        case GLFW_KEY_F10:
            return moth_ui::Key::F10;
        case GLFW_KEY_F11:
            return moth_ui::Key::F11;
        case GLFW_KEY_F12:
            return moth_ui::Key::F12;

        case GLFW_KEY_PRINT_SCREEN:
            return moth_ui::Key::Printscreen;
        case GLFW_KEY_SCROLL_LOCK:
            return moth_ui::Key::Scrolllock;
        case GLFW_KEY_PAUSE:
            return moth_ui::Key::Pause;
        case GLFW_KEY_INSERT:
            return moth_ui::Key::Insert;
        case GLFW_KEY_HOME:
            return moth_ui::Key::Home;
        case GLFW_KEY_PAGE_UP:
            return moth_ui::Key::Pageup;
        case GLFW_KEY_DELETE:
            return moth_ui::Key::Delete;
        case GLFW_KEY_END:
            return moth_ui::Key::End;
        case GLFW_KEY_PAGE_DOWN:
            return moth_ui::Key::Pagedown;
        case GLFW_KEY_RIGHT:
            return moth_ui::Key::Right;
        case GLFW_KEY_LEFT:
            return moth_ui::Key::Left;
        case GLFW_KEY_DOWN:
            return moth_ui::Key::Down;
        case GLFW_KEY_UP:
            return moth_ui::Key::Up;

        case GLFW_KEY_KP_DIVIDE:
            return moth_ui::Key::KP_Divide;
        case GLFW_KEY_KP_MULTIPLY:
            return moth_ui::Key::KP_Multiply;
        case GLFW_KEY_KP_SUBTRACT:
            return moth_ui::Key::KP_Minus;
        case GLFW_KEY_KP_ADD:
            return moth_ui::Key::KP_Plus;
        case GLFW_KEY_KP_ENTER:
            return moth_ui::Key::KP_Enter;
        case GLFW_KEY_KP_1:
            return moth_ui::Key::KP_1;
        case GLFW_KEY_KP_2:
            return moth_ui::Key::KP_2;
        case GLFW_KEY_KP_3:
            return moth_ui::Key::KP_3;
        case GLFW_KEY_KP_4:
            return moth_ui::Key::KP_4;
        case GLFW_KEY_KP_5:
            return moth_ui::Key::KP_5;
        case GLFW_KEY_KP_6:
            return moth_ui::Key::KP_6;
        case GLFW_KEY_KP_7:
            return moth_ui::Key::KP_7;
        case GLFW_KEY_KP_8:
            return moth_ui::Key::KP_8;
        case GLFW_KEY_KP_9:
            return moth_ui::Key::KP_9;
        case GLFW_KEY_KP_0:
            return moth_ui::Key::KP_0;
        case GLFW_KEY_KP_DECIMAL:
            return moth_ui::Key::KP_Period;

        case GLFW_KEY_LEFT_CONTROL:
            return moth_ui::Key::Lctrl;
        case GLFW_KEY_LEFT_SHIFT:
            return moth_ui::Key::Lshift;
        case GLFW_KEY_LEFT_ALT:
            return moth_ui::Key::Lalt;
        case GLFW_KEY_RIGHT_CONTROL:
            return moth_ui::Key::Rctrl;
        case GLFW_KEY_RIGHT_SHIFT:
            return moth_ui::Key::Rshift;
        case GLFW_KEY_RIGHT_ALT:
            return moth_ui::Key::Ralt;

        default:
            return moth_ui::Key::Unknown;
        }
    }
}

namespace backend::vulkan {
    std::unique_ptr<moth_ui::Event> FromGLFW(int key, int scancode, int action, int mods) {
        moth_ui::KeyAction keyAction = action == GLFW_RELEASE ? moth_ui::KeyAction::Up : moth_ui::KeyAction::Down;
        int keyMods = 0;
        if ((mods & GLFW_MOD_SHIFT) != 0)
            keyMods |= moth_ui::KeyMod_LeftShift;
        if ((mods & GLFW_MOD_ALT) != 0)
            keyMods |= moth_ui::KeyMod_LeftAlt;
        if ((mods & GLFW_MOD_CONTROL) != 0)
            keyMods |= moth_ui::KeyMod_LeftCtrl;
        return std::make_unique<moth_ui::EventKey>(keyAction, FromGLFWKey(key), keyMods);
    }
}
