#pragma once
#include "Core/Maths.hpp"
#include "Core/STL.hpp"
#include "SDL3/SDL.h"
namespace axm {

    enum class Keycode {
        UNKNOWN       = SDLK_UNKNOWN,
        RETURN        = SDLK_RETURN,
        ESCAPE        = SDLK_ESCAPE,
        BACKSPACE     = SDLK_BACKSPACE,
        TAB           = SDLK_TAB,
        SPACE         = SDLK_SPACE,
        EXCLAIM       = SDLK_EXCLAIM,
        DBLAPOSTROPHE = SDLK_DBLAPOSTROPHE,
        HASH          = SDLK_HASH,
        DOLLAR        = SDLK_DOLLAR,
        PERCENT       = SDLK_PERCENT,
        AMPERSAND     = SDLK_AMPERSAND,
        APOSTROPHE    = SDLK_APOSTROPHE,
        LEFTPAREN     = SDLK_LEFTPAREN,
        RIGHTPAREN    = SDLK_RIGHTPAREN,
        ASTERISK      = SDLK_ASTERISK,
        PLUS          = SDLK_PLUS,
        COMMA         = SDLK_COMMA,
        MINUS         = SDLK_MINUS,
        PERIOD        = SDLK_PERIOD,
        SLASH         = SDLK_SLASH,
        Num0          = SDLK_0,
        Num1          = SDLK_1,
        Num2          = SDLK_2,
        Num3          = SDLK_3,
        Num4          = SDLK_4,
        Num5          = SDLK_5,
        Num6          = SDLK_6,
        Num7          = SDLK_7,
        Num8          = SDLK_8,
        Num9          = SDLK_9,
        COLON         = SDLK_COLON,
        SEMICOLON     = SDLK_SEMICOLON,
        LESS          = SDLK_LESS,
        EQUALS        = SDLK_EQUALS,
        GREATER       = SDLK_GREATER,
        QUESTION      = SDLK_QUESTION,
        AT            = SDLK_AT,
        LEFTBRACKET   = SDLK_LEFTBRACKET,
        BACKSLASH     = SDLK_BACKSLASH,
        RIGHTBRACKET  = SDLK_RIGHTBRACKET,
        CARET         = SDLK_CARET,
        UNDERSCORE    = SDLK_UNDERSCORE,
        GRAVE         = SDLK_GRAVE,
        A             = SDLK_A,
        B             = SDLK_B,
        C             = SDLK_C,
        D             = SDLK_D,
        E             = SDLK_E,
        F             = SDLK_F,
        G             = SDLK_G,
        H             = SDLK_H,
        I             = SDLK_I,
        J             = SDLK_J,
        K             = SDLK_K,
        L             = SDLK_L,
        M             = SDLK_M,
        N             = SDLK_N,
        O             = SDLK_O,
        P             = SDLK_P,
        Q             = SDLK_Q,
        R             = SDLK_R,
        S             = SDLK_S,
        T             = SDLK_T,
        U             = SDLK_U,
        V             = SDLK_V,
        W             = SDLK_W,
        X             = SDLK_X,
        Y             = SDLK_Y,
        Z             = SDLK_Z,
        LEFTBRACE     = SDLK_LEFTBRACE,
        PIPE          = SDLK_PIPE,
        RIGHTBRACE    = SDLK_RIGHTBRACE,
        TILDE         = SDLK_TILDE,
        DELETE        = SDLK_DELETE,
        PLUSMINUS     = SDLK_PLUSMINUS,
        CAPSLOCK      = SDLK_CAPSLOCK,
        F1            = SDLK_F1,
        F2            = SDLK_F2,
        F3            = SDLK_F3,
        F4            = SDLK_F4,
        F5            = SDLK_F5,
        F6            = SDLK_F6,
        F7            = SDLK_F7,
        F8            = SDLK_F8,
        F9            = SDLK_F9,
        F10           = SDLK_F10,
        F11           = SDLK_F11,
        F12           = SDLK_F12,
        PRINTSCREEN   = SDLK_PRINTSCREEN,
        SCROLLLOCK    = SDLK_SCROLLLOCK,
        PAUSE         = SDLK_PAUSE,
        INSERT        = SDLK_INSERT,
        HOME          = SDLK_HOME,
        PAGEUP        = SDLK_PAGEUP,
        END           = SDLK_END,
        PAGEDOWN      = SDLK_PAGEDOWN,
        RIGHT         = SDLK_RIGHT,
        LEFT          = SDLK_LEFT,
        DOWN          = SDLK_DOWN,
        UP            = SDLK_UP,
        NUMLOCKCLEAR  = SDLK_NUMLOCKCLEAR,
        KP_DIVIDE     = SDLK_KP_DIVIDE,
        KP_MULTIPLY   = SDLK_KP_MULTIPLY,
        KP_MINUS      = SDLK_KP_MINUS,
        KP_PLUS       = SDLK_KP_PLUS,
        KP_ENTER      = SDLK_KP_ENTER,
        KP_1          = SDLK_KP_1,
        KP_2          = SDLK_KP_2,
        KP_3          = SDLK_KP_3,
        KP_4          = SDLK_KP_4,
        KP_5          = SDLK_KP_5,
        KP_6          = SDLK_KP_6,
        KP_7          = SDLK_KP_7,
        KP_8          = SDLK_KP_8,
        KP_9          = SDLK_KP_9,
        KP_0          = SDLK_KP_0,
        KP_PERIOD     = SDLK_KP_PERIOD,
        APPLICATION   = SDLK_APPLICATION,
        POWER         = SDLK_POWER,
        KP_EQUALS     = SDLK_KP_EQUALS,
    };

    enum class MouseButton {
        Right,
        Left,
        Middle,
        Extra1,
        Extra2,
        Extra3,
        Extra4,
        Extra5,
    };

    struct MouseState
    {
        aml::Float2 m_Pos     = { 0.0f, 0.0f };
        aml::Float2 m_LastPos = { 0.0f, 0.0f };
    };

    enum class GamepadButton {
        FaceNorth,
        FaceSouth,
        FaceEast,
        FaceWest,
        DpadNorth,
        DpadSouth,
        DpadEast,
        DpadWest,
        RightBumper,
        LeftBumper,
        Start,
        Back,
        Home,
    };

    enum class GamepadAxis : i32 {
        LeftStick    = 0,
        RightStick   = 1,
        RightTrigger = 2,
        LeftTrigger  = 3,
    };

    struct GamepadAxisState
    {
        Array<f32, 4> m_AxisValues = { };
    };

    class Input
    {
    public:
        bool        IsKeyDown(const Keycode& keycode);
        bool        IsKeyJustPressed(const Keycode& keycode);

        bool        IsGamepadButtonDown(const GamepadButton& button);
        bool        IsGamepadButtonJustPressed(const GamepadButton& button);

        bool        IsMouseButtonDown(const MouseButton& button);
        bool        IsMouseButtonJustPressed(const MouseButton& button);

        aml::Float2 GetMousePosition();
        aml::Float2 GetMousePositionLastFrame();

    private:
        DynArray<Keycode>          p_PressedKeyboardButtons;
        DynArray<Keycode>          p_JustPressedKeyboardButtons;

        DynArray<GamepadButton>    p_PressedGamepadButtons;
        DynArray<GamepadButton>    p_JustPressedGamepadButtons;

        DynArray<MouseButton>      p_PressedMouseButtons;
        DynArray<MouseButton>      p_JustPressedMouseButtons;

        MouseState                 p_MouseState;

        Array<GamepadAxisState, 4> p_GamepadsState;
    };
}
