#pragma once

class ShaderWatcher;
bool initialize(ShaderWatcher&);
void resizeTextures(int, int);
void drawFrame(void);
void handleInput(void);
void cleanup(ShaderWatcher&);
