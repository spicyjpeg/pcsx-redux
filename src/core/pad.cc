/***************************************************************************
 *   Copyright (C) 2019 PCSX-Redux authors                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#define GLFW_INCLUDE_NONE
#define _USE_MATH_DEFINES
#include "core/pad.h"

#include <GLFW/glfw3.h>
#include <memory.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <magic_enum_all.hpp>

#include "core/psxemulator.h"
#include "core/system.h"
#include "fmt/format.h"
#include "gui/gui.h"
#include "imgui.h"
#include "support/file.h"
#include "support/imgui-helpers.h"

class PadsImpl : public PCSX::Pads {
  public:
    enum class InputType { Auto, Controller, Keyboard };
    enum class PadType { Digital = 0, Analog, Mouse, Negcon, Gun, Guncon };

    PadsImpl();
    void init() override;
    void shutdown() override;
    uint8_t startPoll(Port port) override;
    uint8_t poll(uint8_t value, Port port, uint32_t& padState) override;

    json getCfg() override;
    void setCfg(const json& j) override;
    void setDefaults() override;
    bool configure(PCSX::GUI* gui) override;

    void scanGamepads();
    void reset() override;
    void map();

    void setLua(PCSX::Lua L) override;

    bool isPadConnected(int pad) override {
        if (pad > m_pads.size()) {
            return false;
        } else {
            return m_pads[pad - 1].isControllerConnected();
        }
    }

  private:
    PCSX::EventBus::Listener m_listener;
    // This is a list of all of the valid GLFW gamepad IDs that we have found querying GLFW.
    // A value of -1 means that there is no gamepad at that index.
    int m_gamepadsMap[16] = {0};

    static constexpr int GLFW_GAMEPAD_BUTTON_LEFT_TRIGGER = GLFW_GAMEPAD_BUTTON_LAST + 1;
    static constexpr int GLFW_GAMEPAD_BUTTON_RIGHT_TRIGGER = GLFW_GAMEPAD_BUTTON_LAST + 2;
    static constexpr int GLFW_GAMEPAD_BUTTON_INVALID = GLFW_GAMEPAD_BUTTON_LAST + 3;

    // settings block
    // Pad keyboard bindings
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadUp"), GLFW_KEY_UP> Keyboard_PadUp;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadRight"), GLFW_KEY_RIGHT> Keyboard_PadRight;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadDown"), GLFW_KEY_DOWN> Keyboard_PadDown;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadLeft"), GLFW_KEY_LEFT> Keyboard_PadLeft;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadCross"), GLFW_KEY_X> Keyboard_PadCross;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadTriangle"), GLFW_KEY_S> Keyboard_PadTriangle;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadSquare"), GLFW_KEY_Z> Keyboard_PadSquare;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadCircle"), GLFW_KEY_D> Keyboard_PadCircle;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadSelect"), GLFW_KEY_BACKSPACE> Keyboard_PadSelect;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadSstart"), GLFW_KEY_ENTER> Keyboard_PadStart;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadL1"), GLFW_KEY_Q> Keyboard_PadL1;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadL2"), GLFW_KEY_A> Keyboard_PadL2;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadL3"), GLFW_KEY_W> Keyboard_PadL3;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadR1"), GLFW_KEY_R> Keyboard_PadR1;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadR2"), GLFW_KEY_F> Keyboard_PadR2;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_PadR3"), GLFW_KEY_T> Keyboard_PadR3;
    typedef PCSX::Setting<int, TYPESTRING("Keyboard_AnalogMode"), GLFW_KEY_UNKNOWN> Keyboard_AnalogMode;

    // Pad controller bindings
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadUp"), GLFW_GAMEPAD_BUTTON_DPAD_UP> Controller_PadUp;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadRight"), GLFW_GAMEPAD_BUTTON_DPAD_RIGHT> Controller_PadRight;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadDown"), GLFW_GAMEPAD_BUTTON_DPAD_DOWN> Controller_PadDown;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadLeft"), GLFW_GAMEPAD_BUTTON_DPAD_LEFT> Controller_PadLeft;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadCross"), GLFW_GAMEPAD_BUTTON_CROSS> Controller_PadCross;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadTriangle"), GLFW_GAMEPAD_BUTTON_TRIANGLE>
        Controller_PadTriangle;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadSquare"), GLFW_GAMEPAD_BUTTON_SQUARE> Controller_PadSquare;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadCircle"), GLFW_GAMEPAD_BUTTON_CIRCLE> Controller_PadCircle;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadSelect"), GLFW_GAMEPAD_BUTTON_BACK> Controller_PadSelect;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadSstart"), GLFW_GAMEPAD_BUTTON_START> Controller_PadStart;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadL1"), GLFW_GAMEPAD_BUTTON_LEFT_BUMPER> Controller_PadL1;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadL2"), GLFW_GAMEPAD_BUTTON_LEFT_TRIGGER> Controller_PadL2;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadL3"), GLFW_GAMEPAD_BUTTON_LEFT_THUMB> Controller_PadL3;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadR1"), GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER> Controller_PadR1;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadR2"), GLFW_GAMEPAD_BUTTON_RIGHT_TRIGGER> Controller_PadR2;
    typedef PCSX::Setting<int, TYPESTRING("Controller_PadR3"), GLFW_GAMEPAD_BUTTON_RIGHT_THUMB> Controller_PadR3;

    typedef PCSX::Setting<InputType, TYPESTRING("PadType"), InputType::Auto> SettingInputType;
    // These typestrings are kind of odd, but it's best not to change so as not to break old config files
    typedef PCSX::Setting<PadType, TYPESTRING("DeviceType"), PadType::Digital> SettingDeviceType;
    typedef PCSX::Setting<int, TYPESTRING("ID")> SettingControllerID;

    typedef PCSX::Setting<bool, TYPESTRING("Connected")> SettingConnected;
    // Default sensitivity = 5/10 = 0.5
    typedef PCSX::SettingFloat<TYPESTRING("MouseSensitivityX"), 5, 10> SettingMouseSensitivityX;
    typedef PCSX::SettingFloat<TYPESTRING("MouseSensitivityY"), 5, 10> SettingMouseSensitivityY;

    typedef PCSX::Settings<
        Keyboard_PadUp, Keyboard_PadRight, Keyboard_PadDown, Keyboard_PadLeft, Keyboard_PadCross, Keyboard_PadTriangle,
        Keyboard_PadSquare, Keyboard_PadCircle, Keyboard_PadSelect, Keyboard_PadStart, Keyboard_PadL1, Keyboard_PadL2,
        Keyboard_PadL3, Keyboard_PadR1, Keyboard_PadR2, Keyboard_PadR3, Keyboard_AnalogMode, Controller_PadUp,
        Controller_PadRight, Controller_PadDown, Controller_PadLeft, Controller_PadCross, Controller_PadTriangle,
        Controller_PadSquare, Controller_PadCircle, Controller_PadSelect, Controller_PadStart, Controller_PadL1,
        Controller_PadL2, Controller_PadL3, Controller_PadR1, Controller_PadR2, Controller_PadR3, SettingInputType,
        SettingDeviceType, SettingControllerID, SettingConnected, SettingMouseSensitivityX, SettingMouseSensitivityY>
        PadSettings;

    struct PadData {
        // status of buttons - every controller fills this field
        uint16_t buttonStatus;

        // overriding from Lua
        uint16_t overrides = 0xffff;

        // Analog stick values in range (0 - 255) where 128 = center
        uint8_t rightJoyX, rightJoyY, leftJoyX, leftJoyY;
    };

    enum class PadCommands : uint8_t {
        Idle = 0x00,
        Read = 0x42,
        SetConfigMode = 0x43,
        SetAnalogMode = 0x44,
        GetAnalogMode = 0x45,
        Unknown46 = 0x46,
        Unknown47 = 0x47,
        Unknown4C = 0x4C,
        UnlockRumble = 0x4D
    };

    struct Pad {
        uint8_t startPoll();
        uint8_t read();
        uint8_t poll(uint8_t value, uint32_t& padState);
        uint8_t doDualshockCommand(uint32_t& padState);
        void getButtons();
        bool isControllerButtonPressed(int button, GLFWgamepadstate* state);
        bool isControllerConnected() { return m_settings.get<SettingConnected>(); }

        json getCfg();
        void setCfg(const json& j);
        void setDefaults(bool firstController);
        void map();
        void reset();

        bool configure();
        void keyboardEvent(const PCSX::Events::Keyboard&);
        int& getButtonFromGUIIndex(int buttonIndex);

        int m_scancodes[16];
        int m_padMapping[16];
        PadType m_type;
        PadData m_data;

        int m_padID = -1;
        int m_buttonToWait = -1;
        bool m_changed = false;

        bool m_configMode = false;
        bool m_analogMode = false;

        PadSettings m_settings;

        uint8_t m_buf[256];
        int m_bufferLen = 0, m_currentByte = 0;
        uint8_t m_cmd = magic_enum::enum_integer(PadCommands::Idle);

        uint8_t m_stdpar[8] = {0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        uint8_t m_mousepar[6] = {0x12, 0x5a, 0xff, 0xff, 0xff, 0xff};
        uint8_t m_analogpar[8] = {0x73, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    };

    std::array<Pad, 2> m_pads;
    unsigned m_selectedPadForConfig = 0;
};

static PadsImpl* s_pads = nullptr;

static ImGuiKey GlfwKeyToImGuiKey(int key) {
    switch (key) {
        case GLFW_KEY_TAB:
            return ImGuiKey_Tab;
        case GLFW_KEY_LEFT:
            return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT:
            return ImGuiKey_RightArrow;
        case GLFW_KEY_UP:
            return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN:
            return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP:
            return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return ImGuiKey_PageDown;
        case GLFW_KEY_HOME:
            return ImGuiKey_Home;
        case GLFW_KEY_END:
            return ImGuiKey_End;
        case GLFW_KEY_INSERT:
            return ImGuiKey_Insert;
        case GLFW_KEY_DELETE:
            return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE:
            return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE:
            return ImGuiKey_Space;
        case GLFW_KEY_ENTER:
            return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE:
            return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE:
            return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA:
            return ImGuiKey_Comma;
        case GLFW_KEY_MINUS:
            return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD:
            return ImGuiKey_Period;
        case GLFW_KEY_SLASH:
            return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON:
            return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL:
            return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET:
            return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK:
            return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE:
            return ImGuiKey_Pause;
        case GLFW_KEY_KP_0:
            return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1:
            return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2:
            return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3:
            return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4:
            return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5:
            return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6:
            return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7:
            return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8:
            return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9:
            return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL:
            return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD:
            return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL:
            return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT:
            return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT:
            return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU:
            return ImGuiKey_Menu;
        case GLFW_KEY_0:
            return ImGuiKey_0;
        case GLFW_KEY_1:
            return ImGuiKey_1;
        case GLFW_KEY_2:
            return ImGuiKey_2;
        case GLFW_KEY_3:
            return ImGuiKey_3;
        case GLFW_KEY_4:
            return ImGuiKey_4;
        case GLFW_KEY_5:
            return ImGuiKey_5;
        case GLFW_KEY_6:
            return ImGuiKey_6;
        case GLFW_KEY_7:
            return ImGuiKey_7;
        case GLFW_KEY_8:
            return ImGuiKey_8;
        case GLFW_KEY_9:
            return ImGuiKey_9;
        case GLFW_KEY_A:
            return ImGuiKey_A;
        case GLFW_KEY_B:
            return ImGuiKey_B;
        case GLFW_KEY_C:
            return ImGuiKey_C;
        case GLFW_KEY_D:
            return ImGuiKey_D;
        case GLFW_KEY_E:
            return ImGuiKey_E;
        case GLFW_KEY_F:
            return ImGuiKey_F;
        case GLFW_KEY_G:
            return ImGuiKey_G;
        case GLFW_KEY_H:
            return ImGuiKey_H;
        case GLFW_KEY_I:
            return ImGuiKey_I;
        case GLFW_KEY_J:
            return ImGuiKey_J;
        case GLFW_KEY_K:
            return ImGuiKey_K;
        case GLFW_KEY_L:
            return ImGuiKey_L;
        case GLFW_KEY_M:
            return ImGuiKey_M;
        case GLFW_KEY_N:
            return ImGuiKey_N;
        case GLFW_KEY_O:
            return ImGuiKey_O;
        case GLFW_KEY_P:
            return ImGuiKey_P;
        case GLFW_KEY_Q:
            return ImGuiKey_Q;
        case GLFW_KEY_R:
            return ImGuiKey_R;
        case GLFW_KEY_S:
            return ImGuiKey_S;
        case GLFW_KEY_T:
            return ImGuiKey_T;
        case GLFW_KEY_U:
            return ImGuiKey_U;
        case GLFW_KEY_V:
            return ImGuiKey_V;
        case GLFW_KEY_W:
            return ImGuiKey_W;
        case GLFW_KEY_X:
            return ImGuiKey_X;
        case GLFW_KEY_Y:
            return ImGuiKey_Y;
        case GLFW_KEY_Z:
            return ImGuiKey_Z;
        case GLFW_KEY_F1:
            return ImGuiKey_F1;
        case GLFW_KEY_F2:
            return ImGuiKey_F2;
        case GLFW_KEY_F3:
            return ImGuiKey_F3;
        case GLFW_KEY_F4:
            return ImGuiKey_F4;
        case GLFW_KEY_F5:
            return ImGuiKey_F5;
        case GLFW_KEY_F6:
            return ImGuiKey_F6;
        case GLFW_KEY_F7:
            return ImGuiKey_F7;
        case GLFW_KEY_F8:
            return ImGuiKey_F8;
        case GLFW_KEY_F9:
            return ImGuiKey_F9;
        case GLFW_KEY_F10:
            return ImGuiKey_F10;
        case GLFW_KEY_F11:
            return ImGuiKey_F11;
        case GLFW_KEY_F12:
            return ImGuiKey_F12;
        default:
            return ImGuiKey_None;
    }
}

void PadsImpl::init() {
    s_pads = this;
    scanGamepads();
    glfwSetJoystickCallback([](int jid, int event) {
        s_pads->scanGamepads();
        s_pads->map();
    });
    PCSX::g_system->findResource(
        [](const std::filesystem::path& filename) -> bool {
            PCSX::IO<PCSX::File> database(new PCSX::PosixFile(filename));
            if (database->failed()) {
                return false;
            }

            size_t dbsize = database->size();
            auto dbStr = database->readString(dbsize);

            int ret = glfwUpdateGamepadMappings(dbStr.c_str());

            return ret;
        },
        "gamecontrollerdb.txt", "resources", std::filesystem::path("third_party") / "SDL_GameControllerDB");
    reset();
    map();
}

void PadsImpl::shutdown() {
    glfwSetJoystickCallback(nullptr);
    s_pads = nullptr;
}

PadsImpl::PadsImpl() : m_listener(PCSX::g_system->m_eventBus) {
    m_listener.listen<PCSX::Events::Keyboard>([this](const auto& event) {
        if (m_showCfg) {
            m_pads[m_selectedPadForConfig].keyboardEvent(event);
        }
    });
}

void PadsImpl::scanGamepads() {
    static_assert((1 + GLFW_JOYSTICK_LAST - GLFW_JOYSTICK_1) <= sizeof(m_gamepadsMap) / sizeof(m_gamepadsMap[0]));
    for (auto& m : m_gamepadsMap) {
        m = -1;
    }
    unsigned index = 0;
    for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; i++) {
        if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i)) {
            m_gamepadsMap[index++] = i;
        }
    }
}

void PadsImpl::reset() {
    m_pads[0].reset();
    m_pads[1].reset();
}

void PadsImpl::Pad::reset() {
    // m_analogMode = false;
    m_configMode = false;
    m_cmd = magic_enum::enum_integer(PadCommands::Idle);
    m_bufferLen = 0;
    m_currentByte = 0;
    m_data.buttonStatus = 0xffff;
    m_data.overrides = 0xffff;
}

void PadsImpl::map() {
    m_pads[0].map();
    m_pads[1].map();
}

void PadsImpl::Pad::map() {
    m_padID = s_pads->m_gamepadsMap[m_settings.get<SettingControllerID>()];
    m_type = m_settings.get<SettingDeviceType>();

    // L3/R3 are only avalable on analog controllers
    if (m_type == PadType::Analog) {
        m_scancodes[1] = m_settings.get<Keyboard_PadL3>();     // L3
        m_scancodes[2] = m_settings.get<Keyboard_PadR3>();     // R3
        m_padMapping[1] = m_settings.get<Controller_PadL3>();  // L3
        m_padMapping[2] = m_settings.get<Controller_PadR3>();  // R3
    } else {
        m_scancodes[1] = 255;
        m_scancodes[2] = 255;
        m_padMapping[1] = GLFW_GAMEPAD_BUTTON_INVALID;
        m_padMapping[2] = GLFW_GAMEPAD_BUTTON_INVALID;
    }

    // keyboard mappings
    m_scancodes[0] = m_settings.get<Keyboard_PadSelect>();     // SELECT
    m_scancodes[3] = m_settings.get<Keyboard_PadStart>();      // START
    m_scancodes[4] = m_settings.get<Keyboard_PadUp>();         // UP
    m_scancodes[5] = m_settings.get<Keyboard_PadRight>();      // RIGHT
    m_scancodes[6] = m_settings.get<Keyboard_PadDown>();       // DOWN
    m_scancodes[7] = m_settings.get<Keyboard_PadLeft>();       // LEFT
    m_scancodes[8] = m_settings.get<Keyboard_PadL2>();         // L2
    m_scancodes[9] = m_settings.get<Keyboard_PadR2>();         // R2
    m_scancodes[10] = m_settings.get<Keyboard_PadL1>();        // L1
    m_scancodes[11] = m_settings.get<Keyboard_PadR1>();        // R1
    m_scancodes[12] = m_settings.get<Keyboard_PadTriangle>();  // TRIANGLE
    m_scancodes[13] = m_settings.get<Keyboard_PadCircle>();    // CIRCLE
    m_scancodes[14] = m_settings.get<Keyboard_PadCross>();     // CROSS
    m_scancodes[15] = m_settings.get<Keyboard_PadSquare>();    // SQUARE

    // gamepad mappings
    m_padMapping[0] = m_settings.get<Controller_PadSelect>();     // SELECT
    m_padMapping[3] = m_settings.get<Controller_PadStart>();      // START
    m_padMapping[4] = m_settings.get<Controller_PadUp>();         // UP
    m_padMapping[5] = m_settings.get<Controller_PadRight>();      // RIGHT
    m_padMapping[6] = m_settings.get<Controller_PadDown>();       // DOWN
    m_padMapping[7] = m_settings.get<Controller_PadLeft>();       // LEFT
    m_padMapping[8] = m_settings.get<Controller_PadL2>();         // L2
    m_padMapping[9] = m_settings.get<Controller_PadR2>();         // R2
    m_padMapping[10] = m_settings.get<Controller_PadL1>();        // L1
    m_padMapping[11] = m_settings.get<Controller_PadR1>();        // R1
    m_padMapping[12] = m_settings.get<Controller_PadTriangle>();  // TRIANGLE
    m_padMapping[13] = m_settings.get<Controller_PadCircle>();    // CIRCLE
    m_padMapping[14] = m_settings.get<Controller_PadCross>();     // CROSS
    m_padMapping[15] = m_settings.get<Controller_PadSquare>();    // SQUARE
}

static constexpr float THRESHOLD = 0.85f;

// Certain buttons on controllers are actually axis that can be pressed, half-pressed, etc.
bool PadsImpl::Pad::isControllerButtonPressed(int button, GLFWgamepadstate* state) {
    int mapped = m_padMapping[button];
    switch (mapped) {
        case GLFW_GAMEPAD_BUTTON_LEFT_TRIGGER:
            return state->axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] >= THRESHOLD;
        case GLFW_GAMEPAD_BUTTON_RIGHT_TRIGGER:
            return state->axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] >= THRESHOLD;
        case GLFW_GAMEPAD_BUTTON_INVALID:
            return false;
        default:
            return state->buttons[mapped];
            break;
    }
}

static constexpr float π(float fraction = 1.0f) { return fraction * M_PI; }

void PadsImpl::Pad::getButtons() {
    PadData& pad = m_data;
    if (!m_settings.get<SettingConnected>()) {
        pad.buttonStatus = 0xffff;
        pad.leftJoyX = pad.rightJoyX = pad.leftJoyY = pad.rightJoyY = 0x80;
        return;
    }

    GLFWgamepadstate state;
    int hasPad = GLFW_FALSE;
    const auto& inputType = m_settings.get<SettingInputType>();

    auto getKeyboardButtons = [this]() -> uint16_t {
        uint16_t result = 0;
        for (unsigned i = 0; i < 16; i++) {
            auto key = GlfwKeyToImGuiKey(m_scancodes[i]);
            if (key == ImGuiKey_None) continue;
            result |= (ImGui::IsKeyDown(key)) << i;
        }
        return result ^ 0xffff;  // Controls are inverted, so 0 = pressed
    };

    if (inputType == InputType::Keyboard) {
        pad.buttonStatus = getKeyboardButtons();
        pad.leftJoyX = pad.rightJoyX = pad.leftJoyY = pad.rightJoyY = 0x80;
        return;
    }

    if ((m_padID >= GLFW_JOYSTICK_1) && (m_padID <= GLFW_JOYSTICK_LAST)) {
        hasPad = glfwGetGamepadState(m_padID, &state);
        if (!hasPad) {
            const char* guid = glfwGetJoystickGUID(m_padID);
            PCSX::g_system->printf("Gamepad error: GUID %s likely has no database mapping, disabling pad\n", guid);
            m_padID = -1;
        }
    }

    if (!hasPad) {
        if (inputType == InputType::Auto) {
            pad.buttonStatus = getKeyboardButtons();
        } else {
            pad.buttonStatus = 0xffff;
        }

        pad.leftJoyX = pad.rightJoyX = pad.leftJoyY = pad.rightJoyY = 0x80;
        return;
    }

    bool buttons[16];
    for (unsigned i = 0; i < 16; i++) {
        buttons[i] = isControllerButtonPressed(i, &state);
    }

    // For digital gamepads, make the PS1 dpad controllable with our gamepad's left analog stick
    if (m_type == PadType::Digital) {
        float x = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        float y = -state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        float ds = x * x + y * y;
        if (ds >= THRESHOLD * THRESHOLD) {
            float d = std::sqrt(ds);
            x /= d;
            y /= d;
            float a = 0;
            if ((x * x) > (y * y)) {
                a = std::acos(x);
                if (y < 0) {
                    a = π(2.0f) - a;
                }
            } else {
                a = std::asin(y);
                if (x < 0) {
                    a = π() - a;
                } else if (y < 0) {
                    a = π(2.0f) + a;
                }
            }
            if ((a < π(2.5f / 8.0f)) || (a >= π(13.5f / 8.0f))) {
                // right
                buttons[5] = true;
            }
            if ((π(1.5f / 8.0f) <= a) && (a < π(6.5f / 8.0f))) {
                // up
                buttons[4] = true;
            }
            if ((π(5.5f / 8.0f) <= a) && (a < π(10.5f / 8.0f))) {
                // left
                buttons[7] = true;
            }
            if ((π(9.5f / 8.0f) <= a) && (a < π(14.5f / 8.0f))) {
                // down
                buttons[6] = true;
            }
        }
    } else if (m_type == PadType::Analog) {
        // Normalize an axis from (-1, 1) to (0, 255) with 128 = center
        const auto axisToUint8 = [](float axis) {
            constexpr float scale = 1.3f;
            const float scaledValue = std::clamp<float>(axis * scale, -1.0f, 1.0f);
            return (uint8_t)(std::clamp<float>(std::round(((scaledValue + 1.0f) / 2.0f) * 255.0f), 0.0f, 255.0f));
        };
        pad.leftJoyX = axisToUint8(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
        pad.leftJoyY = axisToUint8(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
        pad.rightJoyX = axisToUint8(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
        pad.rightJoyY = axisToUint8(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
    }

    uint16_t result = 0;
    for (unsigned i = 0; i < 16; i++) result |= buttons[i] << i;

    pad.buttonStatus = result ^ 0xffff;  // Controls are inverted, so 0 = pressed
}

uint8_t PadsImpl::startPoll(Port port) {
    int index = magic_enum::enum_integer(port);
    m_pads[index].getButtons();
    return m_pads[index].startPoll();
}

uint8_t PadsImpl::poll(uint8_t value, Port port, uint32_t& padState) {
    int index = magic_enum::enum_integer(port);
    return m_pads[index].poll(value, padState);
}

uint8_t PadsImpl::Pad::poll(uint8_t value, uint32_t& padState) {
    if (m_currentByte == 0) {
        m_cmd = value;
        m_currentByte = 1;

        if (m_cmd == magic_enum::enum_integer(PadCommands::Read)) {
            return read();
        } else if (m_type == PadType::Analog) {
            return doDualshockCommand(padState);
        } else {
            PCSX::g_system->log(PCSX::LogClass::SIO0, _("Unknown command for pad: %02X\n"), value);
            m_cmd = magic_enum::enum_integer(PadCommands::Idle);
            m_bufferLen = 0;
            padState = PAD_STATE_BAD_COMMAND;  // Tell the SIO class we're in an invalid state
            return 0xff;
        }
    } else if (m_currentByte >= m_bufferLen) {
        return 0xff;
    } else if (m_currentByte == 2 && m_type == PadType::Analog) {
        switch (m_cmd) {
            case magic_enum::enum_integer(PadCommands::SetConfigMode):
                m_configMode = value == 1;
                break;
            case magic_enum::enum_integer(PadCommands::SetAnalogMode):
                m_analogMode = value == 1;
                break;
            case magic_enum::enum_integer(PadCommands::Unknown46):
                if (value == 0) {
                    m_buf[4] = 0x01;
                    m_buf[5] = 0x02;
                    m_buf[6] = 0x00;
                    m_buf[7] = 0x0A;
                } else if (value == 1) {
                    m_buf[4] = 0x01;
                    m_buf[5] = 0x01;
                    m_buf[6] = 0x01;
                    m_buf[7] = 0x14;
                }
                break;
            case magic_enum::enum_integer(PadCommands::Unknown47):
                if (value != 0) {
                    m_buf[4] = 0;
                    m_buf[5] = 0;
                    m_buf[6] = 0;
                    m_buf[7] = 0;
                }
                break;
            case magic_enum::enum_integer(PadCommands::Unknown4C):
                if (value == 0) {
                    m_buf[5] = 0x04;
                } else if (value == 1) {
                    m_buf[5] = 0x07;
                }
                break;
        }
    }

    return m_buf[m_currentByte++];
}

uint8_t PadsImpl::Pad::doDualshockCommand(uint32_t& padState) {
    m_bufferLen = 8;

    if (m_cmd == magic_enum::enum_integer(PadCommands::SetConfigMode)) {
        if (m_configMode) {  // The config mode version of this command does not reply with pad data
            static constexpr uint8_t reply[] = {0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            std::memcpy(m_buf, reply, 8);
            return 0xf3;
        } else {
            return read();
        }
    } else if (m_cmd == magic_enum::enum_integer(PadCommands::GetAnalogMode) && m_configMode) {
        static uint8_t reply[] = {0x00, 0x5a, 0x01, 0x02, 0x00, 0x02, 0x01, 0x00};

        reply[4] = m_analogMode ? 1 : 0;
        std::memcpy(m_buf, reply, 8);
        return 0xf3;
    } else if (m_cmd == magic_enum::enum_integer(PadCommands::UnlockRumble) && m_configMode) {
        static uint8_t reply[] = {0x00, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

        std::memcpy(m_buf, reply, 8);
        return 0xf3;
    } else if (m_cmd == magic_enum::enum_integer(PadCommands::SetAnalogMode) && m_configMode) {
        static uint8_t reply[] = {0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        std::memcpy(m_buf, reply, 8);
        return 0xf3;
    } else if (m_cmd == magic_enum::enum_integer(PadCommands::Unknown46) && m_configMode) {
        static uint8_t reply[] = {0x00, 0x5a, 0x00, 0x00, 0x01, 0x02, 0x00, 0x0a};

        std::memcpy(m_buf, reply, 8);
        return 0xf3;
    } else if (m_cmd == magic_enum::enum_integer(PadCommands::Unknown47) && m_configMode) {
        static uint8_t reply[] = {0x00, 0x5a, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00};

        std::memcpy(m_buf, reply, 8);
        return 0xf3;
    } else if (m_cmd == magic_enum::enum_integer(PadCommands::Unknown4C) && m_configMode) {
        static uint8_t reply[] = {0x00, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        std::memcpy(m_buf, reply, 8);
        return 0xf3;
    } else {
        PCSX::g_system->log(PCSX::LogClass::SIO0, _("Unknown command for pad: %02X\n"), static_cast<uint8_t>(m_cmd));
        m_cmd = magic_enum::enum_integer(PadCommands::Idle);
        m_bufferLen = 0;
        padState = PAD_STATE_BAD_COMMAND;  // Tell the SIO class we're in an invalid state
        return 0xff;
    }
}

uint8_t PadsImpl::Pad::startPoll() {
    m_currentByte = 0;
    return 0xff;
}

uint8_t PadsImpl::Pad::read() {
    const PadData& pad = m_data;
    uint16_t buttonStatus = pad.buttonStatus & pad.overrides;
    if (!m_settings.get<SettingConnected>()) {
        m_bufferLen = 0;
        return 0xff;
    }

    switch (m_type) {
        case PadType::Mouse: {
            const int leftClick = ImGui::IsMouseDown(ImGuiMouseButton_Left) ? 0 : 1;
            const int rightClick = ImGui::IsMouseDown(ImGuiMouseButton_Right) ? 0 : 1;
            const auto& io = ImGui::GetIO();
            const float scaleX = m_settings.get<SettingMouseSensitivityX>();
            const float scaleY = m_settings.get<SettingMouseSensitivityY>();

            const float deltaX = io.MouseDelta.x * scaleX;
            const float deltaY = io.MouseDelta.y * scaleY;

            // The top 4 bits are always set to 1, the low 2 bits seem to always be set to 0.
            // Left/right click are inverted in the response byte, ie 0 = pressed
            m_mousepar[3] = 0xf0 | (leftClick << 3) | (rightClick << 2);
            m_mousepar[4] = (int8_t)std::clamp<float>(deltaX, -128.f, 127.f);
            m_mousepar[5] = (int8_t)std::clamp<float>(deltaY, -128.f, 127.f);

            memcpy(m_buf, m_mousepar, 6);
            m_bufferLen = 6;
            return 0x12;
            break;
        }

        case PadType::Negcon:  // npc101/npc104(slph00001/slph00069)
            m_analogpar[0] = 0x23;
            m_analogpar[2] = buttonStatus & 0xff;
            m_analogpar[3] = buttonStatus >> 8;
            m_analogpar[4] = pad.rightJoyX;
            m_analogpar[5] = pad.rightJoyY;
            m_analogpar[6] = pad.leftJoyX;
            m_analogpar[7] = pad.leftJoyY;

            memcpy(m_buf, m_analogpar, 8);
            m_bufferLen = 8;
            return 0x23;
            break;
        case PadType::Analog:  // scph1110, scph1150
            if (m_analogMode || m_configMode) {
                m_analogpar[0] = 0x73;
                m_analogpar[2] = buttonStatus & 0xff;
                m_analogpar[3] = buttonStatus >> 8;
                m_analogpar[4] = pad.rightJoyX;
                m_analogpar[5] = pad.rightJoyY;
                m_analogpar[6] = pad.leftJoyX;
                m_analogpar[7] = pad.leftJoyY;

                memcpy(m_buf, m_analogpar, 8);
                m_bufferLen = 8;
                return m_configMode ? 0xf3 : 0x73;
            }
            [[fallthrough]];
        case PadType::Digital:
        default:
            m_stdpar[2] = buttonStatus & 0xff;
            m_stdpar[3] = buttonStatus >> 8;

            memcpy(m_buf, m_stdpar, 4);
            m_bufferLen = 4;
            return 0x41;
            break;
    }
}

bool PadsImpl::configure(PCSX::GUI* gui) {
    // Check for analog mode toggle key
    for (auto& pad : m_pads) {
        if (pad.m_type == PadType::Analog && pad.m_settings.get<Keyboard_AnalogMode>() != GLFW_KEY_UNKNOWN) {
            const int key = pad.m_settings.get<Keyboard_AnalogMode>();

            if ((key != ImGuiKey_None) && ImGui::IsKeyReleased(GlfwKeyToImGuiKey(key))) {
                pad.m_analogMode = !pad.m_analogMode;
            }
        }
    }

    if (!m_showCfg) {
        return false;
    }

    ImGui::SetNextWindowPos(ImVec2(70, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_("Pad configuration"), &m_showCfg)) {
        ImGui::End();
        return false;
    }

    static std::function<const char*()> const c_padNames[] = {
        l_("Pad 1"),
        l_("Pad 2"),
    };

    if (ImGui::Button(_("Rescan gamepads and re-read game controllers database"))) {
        shutdown();
        init();
    }

    bool changed = false;
    changed |= ImGui::Checkbox(_("Use raw input for mouse"), &gui->isRawMouseMotionEnabled());
    PCSX::ImGuiHelpers::ShowHelpMarker(
        _("When enabled, the cursor will be hidden and captured when the emulator is running. This is useful for games "
          "that require mouse input."));
    changed |= ImGui::Checkbox(_("Allow mouse capture toggle"), &gui->allowMouseCaptureToggle());
    PCSX::ImGuiHelpers::ShowHelpMarker(
        _("When enabled, pressing CTRL and ALT will toggle the setting above, raw input"));

    if (ImGui::BeginCombo(_("Pad"), c_padNames[m_selectedPadForConfig]())) {
        for (unsigned i = 0; i < 2; i++) {
            if (ImGui::Selectable(c_padNames[i](), m_selectedPadForConfig == i)) {
                m_selectedPadForConfig = i;
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button(_("Set defaults"))) {
        changed = true;
        m_pads[m_selectedPadForConfig].setDefaults(m_selectedPadForConfig == 0);
    }

    changed |= m_pads[m_selectedPadForConfig].configure();
    ImGui::End();
    return changed;
}

// GLFW doesn't support converting some of the most common keys to strings
static std::string glfwKeyToString(int key) {
    // define strings for some common keys that are not supported by glfwGetKeyName
    switch (key) {
        case GLFW_KEY_UP:
            return _("Keyboard Up");
        case GLFW_KEY_RIGHT:
            return _("Keyboard Right");
        case GLFW_KEY_DOWN:
            return _("Keyboard Down");
        case GLFW_KEY_LEFT:
            return _("Keyboard Left");
        case GLFW_KEY_BACKSPACE:
            return _("Keyboard Backspace");
        case GLFW_KEY_ENTER:
            return _("Keyboard Enter");
        case GLFW_KEY_SPACE:
            return _("Keyboard Space");
        case GLFW_KEY_ESCAPE:
            return _("Keyboard Escape");
        case GLFW_KEY_UNKNOWN:
            return _("Unbound");
    };

    auto keyName = glfwGetKeyName(key, 0);
    if (keyName == nullptr) {
        return fmt::format(f_("Unknown keyboard key {}"), key);
    }

    auto str = std::string(keyName);
    str[0] = toupper(str[0]);
    return fmt::format(f_("Keyboard {}"), str);
}

void PadsImpl::Pad::keyboardEvent(const PCSX::Events::Keyboard& event) {
    if (m_buttonToWait == -1) {
        return;
    }
    getButtonFromGUIIndex(m_buttonToWait) = event.key;
    m_buttonToWait = -1;
    m_changed = true;
    map();
}

bool PadsImpl::Pad::configure() {
    static std::function<const char*()> const c_inputDevices[] = {
        l_("Auto"),
        l_("Controller"),
        l_("Keyboard"),
    };
    static std::function<const char*()> const c_buttonNames[] = {
        l_("╳"),  l_("□"),  l_("△"),  l_("◯"),  l_("Select"), l_("Start"),       l_("L1"),
        l_("R1"), l_("L2"), l_("R2"), l_("L3"), l_("R3"),     l_("Analog Mode"),
    };
    static std::function<const char*()> const c_dpadDirections[] = {
        l_("↑"),
        l_("→"),
        l_("↓"),
        l_("←"),
    };
    static std::function<const char*()> const c_controllerTypes[] = {
        l_("Digital"),
        l_("Analog"),
        l_("Mouse"),
        l_("Negcon (Unimplemented)"),
        l_("Gun (Unimplemented)"),
        l_("Guncon (Unimplemented)"),
    };

    bool changed = false;
    if (ImGui::Checkbox(_("Connected"), &m_settings.get<SettingConnected>().value)) {
        changed = true;
        reset();  // Reset pad state when unplugging/replugging pad
    }

    if (m_type != PadType::Analog) {
        ImGui::BeginDisabled();
    }

    ImGui::Checkbox(_("Analog mode"), &m_analogMode);

    if (m_type != PadType::Analog) {
        ImGui::EndDisabled();
    }

    {
        const char* currentType = c_controllerTypes[static_cast<int>(m_type)]();
        if (ImGui::BeginCombo(_("Controller Type"), currentType)) {
            for (int i = 0; i < 3; i++) {
                if (ImGui::Selectable(c_controllerTypes[i]())) {
                    changed = true;
                    m_type = static_cast<PadType>(i);
                    m_settings.get<SettingDeviceType>().value = m_type;
                    reset();  // Reset pad state when changing pad type
                    map();
                }
            }
            ImGui::EndCombo();
        }
    }

    auto& inputDevice = m_settings.get<SettingInputType>().value;

    if (ImGui::BeginCombo(_("Device type"), c_inputDevices[magic_enum::enum_integer<InputType>(inputDevice)]())) {
        for (auto i : magic_enum::enum_values<InputType>()) {
            if (ImGui::Selectable(c_inputDevices[magic_enum::enum_integer<InputType>(i)](), i == inputDevice)) {
                changed = true;
                inputDevice = i;
            }
        }

        ImGui::EndCombo();
    }
    changed |=
        ImGui::SliderFloat(_("Mouse sensitivity X"), &m_settings.get<SettingMouseSensitivityX>().value, 0.f, 10.f);
    changed |=
        ImGui::SliderFloat(_("Mouse sensitivity Y"), &m_settings.get<SettingMouseSensitivityY>().value, 0.f, 10.f);

    ImGui::TextUnformatted(_("Keyboard mapping"));
    if (ImGui::BeginTable("Mapping", 2, ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn(_("Computer button mapping"));
        ImGui::TableSetupColumn(_("Gamepad button"));
        ImGui::TableHeadersRow();
        for (auto i = 0; i < 13; i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(c_buttonNames[i]());
            ImGui::TableSetColumnIndex(0);
            bool hasToPop = false;
            if (m_buttonToWait == i) {
                const ImVec4 highlight = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
                ImGui::PushStyleColor(ImGuiCol_Button, highlight);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, highlight);
                hasToPop = true;
            }

            // The name of the mapped key
            const auto keyName = fmt::format("{}##{}", glfwKeyToString(getButtonFromGUIIndex(i)), i);
            if (ImGui::Button(keyName.c_str(), ImVec2{-1, 0})) {
                m_buttonToWait = i;
            }
            if (hasToPop) {
                ImGui::PopStyleColor(2);
            }
        }
        for (auto i = 0; i < 4; i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(c_dpadDirections[i]());
            ImGui::TableSetColumnIndex(0);
            bool hasToPop = false;
            const auto absI = i + 13;
            if (m_buttonToWait == absI) {
                const ImVec4 highlight = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
                ImGui::PushStyleColor(ImGuiCol_Button, highlight);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, highlight);
                hasToPop = true;
            }

            // The name of the mapped key
            const auto keyName = fmt::format("{}##{}", glfwKeyToString(getButtonFromGUIIndex(absI)), absI);
            if (ImGui::Button(keyName.c_str(), ImVec2{-1, 0})) {
                m_buttonToWait = absI;
            }
            if (hasToPop) {
                ImGui::PopStyleColor(2);
            }
        }
        ImGui::EndTable();
    }

    const char* preview = _("No gamepad selected or connected");
    auto& id = m_settings.get<SettingControllerID>().value;
    int glfwjid = id >= 0 ? s_pads->m_gamepadsMap[id] : -1;

    std::vector<const char*> gamepadsNames;

    for (auto& m : s_pads->m_gamepadsMap) {
        if (m == -1) {
            continue;
        }
        const char* name = glfwGetGamepadName(m);
        gamepadsNames.push_back(name);
        if (m == glfwjid) {
            preview = name;
        }
    }

    if (ImGui::BeginCombo(_("Gamepad"), preview)) {
        for (int i = 0; i < gamepadsNames.size(); i++) {
            const auto gamepadName = fmt::format("{}##{}", gamepadsNames[i], i);
            if (ImGui::Selectable(gamepadName.c_str())) {
                changed = true;
                id = i;
                map();
            }
        }

        ImGui::EndCombo();
    }

    if (m_changed) {
        changed = true;
        m_changed = false;
    }

    return changed;
}

int& PadsImpl::Pad::getButtonFromGUIIndex(int buttonIndex) {
    switch (buttonIndex) {
        case 0:
            return m_settings.get<Keyboard_PadCross>().value;
        case 1:
            return m_settings.get<Keyboard_PadSquare>().value;
        case 2:
            return m_settings.get<Keyboard_PadTriangle>().value;
        case 3:
            return m_settings.get<Keyboard_PadCircle>().value;
        case 4:
            return m_settings.get<Keyboard_PadSelect>().value;
        case 5:
            return m_settings.get<Keyboard_PadStart>().value;
        case 6:
            return m_settings.get<Keyboard_PadL1>().value;
        case 7:
            return m_settings.get<Keyboard_PadR1>().value;
        case 8:
            return m_settings.get<Keyboard_PadL2>().value;
        case 9:
            return m_settings.get<Keyboard_PadR2>().value;
        case 10:
            return m_settings.get<Keyboard_PadL3>().value;
        case 11:
            return m_settings.get<Keyboard_PadR3>().value;
        case 12:
            return m_settings.get<Keyboard_AnalogMode>().value;
        case 13:
            return m_settings.get<Keyboard_PadUp>().value;
        case 14:
            return m_settings.get<Keyboard_PadRight>().value;
        case 15:
            return m_settings.get<Keyboard_PadDown>().value;
        case 16:
            return m_settings.get<Keyboard_PadLeft>().value;
        default:
            abort();
            break;
    }
}

json PadsImpl::getCfg() {
    json ret;
    ret[0] = m_pads[0].getCfg();
    ret[1] = m_pads[1].getCfg();
    return ret;
}

json PadsImpl::Pad::getCfg() { return m_settings.serialize(); }

void PadsImpl::setCfg(const json& j) {
    if (j.count("pads") && j["pads"].is_array()) {
        auto padsCfg = j["pads"];
        if (padsCfg.size() >= 1) {
            m_pads[0].setCfg(padsCfg[0]);
        } else {
            m_pads[0].setDefaults(true);
        }
        if (padsCfg.size() >= 2) {
            m_pads[1].setCfg(padsCfg[1]);
        } else {
            m_pads[1].setDefaults(false);
        }
    } else {
        setDefaults();
    }
}

void PadsImpl::Pad::setCfg(const json& j) {
    m_settings.deserialize(j);
    map();
}

void PadsImpl::setDefaults() {
    m_pads[0].setDefaults(true);
    m_pads[1].setDefaults(false);
}

void PadsImpl::Pad::setDefaults(bool firstController) {
    m_settings.reset();
    if (firstController) {
        m_settings.get<SettingConnected>() = true;
    }
    map();
}

void PadsImpl::setLua(PCSX::Lua L) {
    L.getfieldtable("PCSX", LUA_GLOBALSINDEX);

    // setting constants
    L.getfieldtable("CONSTS");
    L.getfieldtable("PAD");
    L.getfieldtable("BUTTON");

    L.push(lua_Number(0));
    L.setfield("SELECT");
    L.push(lua_Number(3));
    L.setfield("START");
    L.push(lua_Number(4));
    L.setfield("UP");
    L.push(lua_Number(5));
    L.setfield("RIGHT");
    L.push(lua_Number(6));
    L.setfield("DOWN");
    L.push(lua_Number(7));
    L.setfield("LEFT");
    L.push(lua_Number(8));
    L.setfield("L2");
    L.push(lua_Number(9));
    L.setfield("R2");
    L.push(lua_Number(10));
    L.setfield("L1");
    L.push(lua_Number(11));
    L.setfield("R1");
    L.push(lua_Number(12));
    L.setfield("TRIANGLE");
    L.push(lua_Number(13));
    L.setfield("CIRCLE");
    L.push(lua_Number(14));
    L.setfield("CROSS");
    L.push(lua_Number(15));
    L.setfield("SQUARE");

    L.pop();
    L.pop();
    L.pop();

    // pushing settings

    L.getfieldtable("settings");
    L.getfieldtable("pads");
    auto pushSettings = [this, L](unsigned pad) mutable {
        L.push(lua_Number(pad + 1));
        m_pads[pad].m_settings.pushValue(L);
        L.settable();
    };
    pushSettings(0);
    pushSettings(1);
    L.pop();
    L.pop();

    L.getfieldtable("SIO0");
    L.getfieldtable("slots");

    // pads callbacks

    auto setCallbacks = [this, L](unsigned pad) mutable {
        L.getfieldtable(pad + 1);
        L.getfieldtable("pads");
        L.getfieldtable(1);

        L.declareFunc(
            "getButton",
            [this, pad](PCSX::Lua L) -> int {
                int n = L.gettop();
                if (n == 0) {
                    return L.error("Not enough arguments to getButton");
                }
                if (!L.isnumber(1)) {
                    return L.error("Invalid argument to getButton");
                }
                auto buttons = m_pads[pad].m_data.buttonStatus;
                auto overrides = m_pads[pad].m_data.overrides;
                unsigned button = L.checknumber(1);
                L.push(((overrides & buttons) & (1 << button)) == 0);
                return 1;
            },
            -1);
        L.declareFunc(
            "setOverride",
            [this, pad](PCSX::Lua L) -> int {
                int n = L.gettop();
                if (n == 0) {
                    return L.error("Not enough arguments to setOverride");
                }
                if (!L.isnumber(1)) {
                    return L.error("Invalid argument to setOverride");
                }
                auto& overrides = m_pads[pad].m_data.overrides;
                unsigned button = L.checknumber(1);
                button = 1 << button;
                overrides &= ~button;
                return 0;
            },
            -1);
        L.declareFunc(
            "clearOverride",
            [this, pad](PCSX::Lua L) -> int {
                int n = L.gettop();
                if (n == 0) {
                    return L.error("Not enough arguments to clearOverride");
                }
                if (!L.isnumber(1)) {
                    return L.error("Invalid argument to clearOverride");
                }
                auto& overrides = m_pads[pad].m_data.overrides;
                unsigned button = L.checknumber(1);
                button = 1 << button;
                overrides |= button;
                return 0;
            },
            -1);
        L.declareFunc(
            "setAnalogMode",
            [this, pad](PCSX::Lua L) -> int {
                int n = L.gettop();
                if (n == 0) {
                    m_pads[pad].m_analogMode = false;
                } else {
                    m_pads[pad].m_analogMode = L.toboolean();
                }
                return 0;
            },
            -1);
        L.declareFunc(
            "map",
            [this, pad](PCSX::Lua L) -> int {
                m_pads[pad].map();
                return 0;
            },
            -1);

        L.pop();
        L.pop();
        L.pop();
    };

    setCallbacks(0);
    setCallbacks(1);

    L.pop();
    L.pop();
    L.pop();

    assert(L.gettop() == 0);
}

PCSX::Pads* PCSX::Pads::factory() { return s_pads = new PadsImpl(); }
