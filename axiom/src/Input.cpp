#include "Core/Input.hpp"
bool axm::Input::IsKeyDown(const Keycode& keycode) const {
    auto pressed = std::ranges::find(p_PressedKeyboardButtons.begin(), p_PressedKeyboardButtons.end(), keycode);
    auto justPressed
            = std::ranges::find(p_JustPressedKeyboardButtons.begin(), p_JustPressedKeyboardButtons.end(), keycode);

    return pressed != p_PressedKeyboardButtons.end() || justPressed != p_JustPressedKeyboardButtons.end();
}
bool axm::Input::IsKeyJustPressed(const Keycode& keycode) const {
    return std::ranges::find(p_JustPressedKeyboardButtons.begin(), p_JustPressedKeyboardButtons.end(), keycode)
           != p_JustPressedKeyboardButtons.end();
}

bool axm::Input::IsGamepadButtonDown(u8 index, const GamepadButton& keycode) const {
    AXM_ASSERT(index < kNumGamepads, "Gamepad index out of range");
    const auto pressed = std::ranges::find(
            p_GamepadsState[index].m_PressedButtons.begin(), p_GamepadsState[index].m_PressedButtons.end(), keycode);
    const auto justPressed = std::ranges::find(p_GamepadsState[index].m_JustPressedButtons.begin(),
                                               p_GamepadsState[index].m_JustPressedButtons.end(),
                                               keycode);

    return pressed != p_GamepadsState[index].m_PressedButtons.end()
           || justPressed != p_GamepadsState[index].m_JustPressedButtons.end();
}

bool axm::Input::IsGamepadButtonJustPressed(u8 index, const GamepadButton& button) const {
    AXM_ASSERT(index < kNumGamepads, "Gamepad index out of range");
    auto justPressed = std::ranges::find(p_GamepadsState[index].m_JustPressedButtons.begin(),
                                         p_GamepadsState[index].m_JustPressedButtons.end(),
                                         button);
    return justPressed != p_GamepadsState[index].m_JustPressedButtons.end();
}

bool axm::Input::IsMouseButtonDown(const MouseButton& button) const {
    auto pressed     = std::ranges::find(p_PressedMouseButtons.begin(), p_PressedMouseButtons.end(), button);
    auto justPressed = std::ranges::find(p_JustPressedMouseButtons.begin(), p_JustPressedMouseButtons.end(), button);

    return pressed != p_PressedMouseButtons.end() || justPressed != p_JustPressedMouseButtons.end();
}
bool axm::Input::IsMouseButtonJustPressed(const MouseButton& button) const {
    return std::ranges::find(p_JustPressedMouseButtons.begin(), p_JustPressedMouseButtons.end(), button)
           != p_JustPressedMouseButtons.end();
}

axm::aml::Float2 axm::Input::GetMousePosition() const { return p_MouseState.m_Pos; }
axm::aml::Float2 axm::Input::GetMousePositionLastFrame() const { return p_MouseState.m_LastPos; }
f32              axm::Input::GetGamepadAxis(u8 index, const GamepadAxis& axis) const {
    AXM_ASSERT(index < kNumGamepads, "Gamepad index out of range");
    return p_GamepadsState[index].m_AxisState.m_AxisValues[static_cast<i32>(axis)];
}


void axm::Input::ClearInputs() {
    p_JustPressedKeyboardButtons.clear();

    for (auto i = 0; i < kNumGamepads; i++) {
        p_GamepadsState[i].m_JustPressedButtons.clear();
    }

    p_JustPressedMouseButtons.clear();

    p_MouseState.m_LastPos = p_MouseState.m_Pos;
}

void axm::Input::HandleFrameInputEvent(SDL_Event& e) {

    if (e.type == SDL_EVENT_KEY_DOWN) {
        const auto k = static_cast<Keycode>(e.key.key);

        if (std::ranges::find(p_PressedKeyboardButtons.begin(), p_PressedKeyboardButtons.end(), k)
            == p_PressedKeyboardButtons.end()) {
            p_PressedKeyboardButtons.push_back(k);
            p_JustPressedKeyboardButtons.push_back(k);
        }
    }

    if (e.type == SDL_EVENT_KEY_UP) {
        const auto k  = static_cast<Keycode>(e.key.key);
        const auto it = std::ranges::find(p_PressedKeyboardButtons.begin(), p_PressedKeyboardButtons.end(), k);

        if (it != p_PressedKeyboardButtons.end()) {
            p_PressedKeyboardButtons.erase(it);
        }
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        const auto mb = static_cast<MouseButton>(e.button.button);

        if (std::ranges::find(p_PressedMouseButtons.begin(), p_PressedMouseButtons.end(), mb)
            == p_PressedMouseButtons.end()) {
            p_PressedMouseButtons.push_back(mb);
            p_JustPressedMouseButtons.push_back(mb);
        }
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        const auto mb = static_cast<MouseButton>(e.button.button);
        const auto it = std::ranges::find(p_PressedMouseButtons.begin(), p_PressedMouseButtons.end(), mb);

        if (it != p_PressedMouseButtons.end()) {
            p_PressedMouseButtons.erase(it);
        }
    }

    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        const auto gb        = static_cast<GamepadButton>(e.button.button);
        const auto index     = GetGamepadIndex(e.gbutton.which);

        const auto pressedIt = std::ranges::find(
                p_GamepadsState[index].m_PressedButtons.begin(), p_GamepadsState[index].m_PressedButtons.end(), gb);

        if (pressedIt == p_GamepadsState[index].m_PressedButtons.end()) {
            p_GamepadsState[index].m_PressedButtons.push_back(gb);
            p_GamepadsState[index].m_JustPressedButtons.push_back(gb);
        }
    }

    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        const auto gb        = static_cast<GamepadButton>(e.button.button);
        const auto index     = GetGamepadIndex(e.gbutton.which);

        const auto pressedIt = std::ranges::find(
                p_GamepadsState[index].m_PressedButtons.begin(), p_GamepadsState[index].m_PressedButtons.end(), gb);

        if (pressedIt != p_GamepadsState[index].m_PressedButtons.end()) {
            p_GamepadsState[index].m_PressedButtons.erase(pressedIt);
        }
    }

    if (e.type == SDL_EVENT_MOUSE_MOTION) {
        p_MouseState.m_Pos = aml::Float2(e.motion.x, e.motion.y);
    }

    if (e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        const auto axis                                               = static_cast<GamepadAxis>(e.gaxis.axis);
        const auto index                                              = GetGamepadIndex(e.gaxis.which);

        p_GamepadsState[index].m_AxisState.m_AxisValues[e.gaxis.axis] = static_cast<float>(e.gaxis.value);
    }
}
u8 axm::Input::GetGamepadIndex(const SDL_JoystickID& id) {
    for (u8 i = 0; i < kNumGamepads; i++) {
        if (p_GamepadsState[i].m_ID == id) {
            return i;
        }
    }

    for (u8 i = 0; i < kNumGamepads; i++) {
        if (p_GamepadsState[i].m_ID == GamepadState::kUnknown) {
            p_GamepadsState[i].m_ID = id;
            return i;
        }
    }

    return UINT8_MAX;
}
