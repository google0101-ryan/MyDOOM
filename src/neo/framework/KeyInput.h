#pragma once

#include <string>

enum keyNum_t {
	K_NONE,

	K_ESCAPE,
	K_1,
	K_2,
	K_3,
	K_4,
	K_5,
	K_6,
	K_7,
	K_8,
	K_9,
	K_0,
	K_MINUS,
	K_EQUALS,
	K_BACKSPACE,
	K_TAB,
	K_Q,
	K_W,
	K_E,
	K_R,
	K_T,
	K_Y,
	K_U,
	K_I,
	K_O,
	K_P,
	K_LBRACKET,
	K_RBRACKET,
	K_ENTER,
	K_LCTRL,
	K_A,
	K_S,
	K_D,
	K_F,
	K_G,
	K_H,
	K_J,
	K_K,
	K_L,
	K_SEMICOLON,
	K_APOSTROPHE,
	K_GRAVE,
	K_LSHIFT,
	K_BACKSLASH,
	K_Z,
	K_X,
	K_C,
	K_V,
	K_B,
	K_N,
	K_M,
	K_COMMA,
	K_PERIOD,
	K_SLASH,
	K_RSHIFT,
	K_KP_STAR,
	K_LALT,
	K_SPACE,
	K_CAPSLOCK,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_NUMLOCK,
	K_SCROLL,
	K_KP_7,
	K_KP_8,
	K_KP_9,
	K_KP_MINUS,
	K_KP_4,
	K_KP_5,
	K_KP_6,
	K_KP_PLUS,
	K_KP_1,
	K_KP_2,
	K_KP_3,
	K_KP_0,
	K_KP_DOT,
	K_F11			= 0x57,
	K_F12			= 0x58,
	K_F13			= 0x64,
	K_F14			= 0x65,
	K_F15			= 0x66,
	K_KANA			= 0x70,
	K_CONVERT		= 0x79,
	K_NOCONVERT		= 0x7B,
	K_YEN			= 0x7D,
	K_KP_EQUALS		= 0x8D,
	K_CIRCUMFLEX	= 0x90,
	K_AT			= 0x91,
	K_COLON			= 0x92,
	K_UNDERLINE		= 0x93,
	K_KANJI			= 0x94,
	K_STOP			= 0x95,
	K_AX			= 0x96,
	K_UNLABELED		= 0x97,
	K_KP_ENTER		= 0x9C,
	K_RCTRL			= 0x9D,
	K_KP_COMMA		= 0xB3,
	K_KP_SLASH		= 0xB5,
	K_PRINTSCREEN	= 0xB7,
	K_RALT			= 0xB8,
	K_PAUSE			= 0xC5,
	K_HOME			= 0xC7,
	K_UPARROW		= 0xC8,
	K_PGUP			= 0xC9,
	K_LEFTARROW		= 0xCB,
	K_RIGHTARROW	= 0xCD,
	K_END			= 0xCF,
	K_DOWNARROW		= 0xD0,
	K_PGDN			= 0xD1,
	K_INS			= 0xD2,
	K_DEL			= 0xD3,
	K_LWIN			= 0xDB,
	K_RWIN			= 0xDC,
	K_APPS			= 0xDD,
	K_POWER			= 0xDE,
	K_SLEEP			= 0xDF,

	//------------------------
	// K_JOY codes must be contiguous, too
	//------------------------

	K_JOY1 = 256,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,

	K_JOY_STICK1_UP,
	K_JOY_STICK1_DOWN,
	K_JOY_STICK1_LEFT,
	K_JOY_STICK1_RIGHT,

	K_JOY_STICK2_UP,
	K_JOY_STICK2_DOWN,
	K_JOY_STICK2_LEFT,
	K_JOY_STICK2_RIGHT,

	K_JOY_TRIGGER1,
	K_JOY_TRIGGER2,

	K_JOY_DPAD_UP,
	K_JOY_DPAD_DOWN,
	K_JOY_DPAD_LEFT,
	K_JOY_DPAD_RIGHT,

	//------------------------
	// K_MOUSE enums must be contiguous (no char codes in the middle)
	//------------------------

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,

	K_MWHEELDOWN,
	K_MWHEELUP,

	K_LAST_KEY
};

struct keyBindings_t
{
    std::string keyboard;
    std::string mouse;
    std::string gamepad;
};

int Key_ConvertHIDCode(int hid);

class idKeyInput
{
public:
    static void Init();
    static void SetBinding(int keynum, const char* binding);
};