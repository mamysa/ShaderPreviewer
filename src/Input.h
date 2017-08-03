#pragma once
#include <Windows.h>

enum KeyboardMap {
	KEY_LEFT = 0,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_SPACE,
	KEY_C,
	KEY_W, 
	KEY_S,
	KEY_A,
	KEY_D,
	KEY_ESC,
};

const bool * getKeys(void);
void setKeyState(WPARAM, bool);