#include "Input.h"

bool keys[11] = { false };

void setKeyState(WPARAM wParam, bool state) {
	if (wParam == VK_LEFT)   { keys[KEY_LEFT]  = state; }
	if (wParam == VK_RIGHT)  { keys[KEY_RIGHT] = state; }
	if (wParam == VK_UP)     { keys[KEY_UP] = state; }
	if (wParam == VK_DOWN)   { keys[KEY_DOWN] = state; }
	if (wParam == 'W')  { keys[KEY_W] = state; }
	if (wParam == 'S')  { keys[KEY_S] = state; }
	if (wParam == 'A')  { keys[KEY_A] = state; }
	if (wParam == 'D')  { keys[KEY_D] = state; }
	if (wParam == 'C')  { keys[KEY_C] = state; }
	if (wParam == VK_SPACE)  { keys[KEY_SPACE] = state; }
	if (wParam == VK_ESCAPE) { keys[KEY_ESC] = state; }
}

const bool * getKeys(void) {
	return keys;
}
