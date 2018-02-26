#include "Input.h"

#ifdef IS_WINDOWS
#include "SDL.h"
#endif

#ifdef IS_OSX
#include <SDL2/SDL.h>
#endif

bool keys[11] = { false };

void setKeyState() {
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);
	keys[KEY_LEFT]  = keystate[SDL_SCANCODE_LEFT];
	keys[KEY_RIGHT]  = keystate[SDL_SCANCODE_RIGHT];
	keys[KEY_UP]  = keystate[SDL_SCANCODE_UP];
	keys[KEY_DOWN]  = keystate[SDL_SCANCODE_DOWN];
	keys[KEY_W]  = keystate[SDL_SCANCODE_W];
	keys[KEY_S]  = keystate[SDL_SCANCODE_S];
	keys[KEY_A]  = keystate[SDL_SCANCODE_A];
	keys[KEY_D]  = keystate[SDL_SCANCODE_D];
	keys[KEY_C]  = keystate[SDL_SCANCODE_C];
	keys[KEY_SPACE]  = keystate[SDL_SCANCODE_SPACE];
	keys[KEY_ESC]  = keystate[SDL_SCANCODE_ESCAPE];
}

const bool * getKeys(void) {
	return keys;
}
