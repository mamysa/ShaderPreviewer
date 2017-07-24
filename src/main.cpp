
#include <iostream>
#include "GL/glew.h"
#include <GL/gl.h>
#include "SDL.h"
#include "glm.hpp"
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "ShaderWatcher.h"
#include <chrono>

#define W 800
#define H 600 
ShaderWatcher shaderWatcher;
ShaderProgram *currentProgram = nullptr;
GLMesh *quad = nullptr;
bool g_VALIDGLSTATE = true;

const char *DEFAULT_VERT_SHADER = R"(
	#version 450
	in vec4 position; in vec2 texCoord;
	out vec2 fragTexCoord;
	void main(void) { gl_Position = position; fragTexCoord = texCoord; }
)";


glm::vec2 mouse = glm::vec2(0.0f);
float forward =  0.0f;
float up = 0.0;
unsigned ticks = 0;
static void handleInput(void) {
	const float rotXStep = 0.01f;
	const float rotYStep = 0.01f;
	const float movStep  = 0.11f;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if(keys[SDL_SCANCODE_LEFT])  { mouse.x -= rotXStep; }
	if(keys[SDL_SCANCODE_RIGHT]) { mouse.x += rotXStep; }
	if(keys[SDL_SCANCODE_DOWN])  { mouse.y -= rotYStep; }
	if(keys[SDL_SCANCODE_UP])    { mouse.y += rotYStep; }
	if(keys[SDL_SCANCODE_W])     { forward += movStep;  }
	if(keys[SDL_SCANCODE_S])     { forward -= movStep;  }
	if(keys[SDL_SCANCODE_Z])     { up += movStep;  }
	if(keys[SDL_SCANCODE_X])     { up -= movStep;  }
	if(keys[SDL_SCANCODE_SPACE]) {
		mouse = glm::vec2(0.0f);
		forward = 0.0f;
		up = 2.0f;
		ticks = 0;
	}

	if (g_VALIDGLSTATE) {
		currentProgram->uniform("mouse", mouse);
		currentProgram->uniform("forward", forward);
		currentProgram->uniform("up", up);
	}
}

void drawQuad(void) {
	glm::vec2 resolution = glm::vec2((float)W, (float)H);
	quad->setupAttributes(*currentProgram);
	float tickf = ((float)ticks) * 0.1;
	currentProgram->uniform("resolution", resolution);
	currentProgram->uniform("iGlobalTime", tickf);
	quad->bind();
	quad->draw();
	quad->bind(0);
}

int main(int argc, char **argv) {
	assert(argc == 2);

	SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_OPENGL);
	if (!window) {
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	glewInit();


	ShaderProgram myShaderProgram;
	myShaderProgram.addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	myShaderProgram.addShader(readTextFile(argv[1]).c_str(), GL_FRAGMENT_SHADER);
	g_VALIDGLSTATE = myShaderProgram.link();
	shaderWatcher.add(argv[1], &myShaderProgram, GL_FRAGMENT_SHADER);
	currentProgram = &myShaderProgram;

	SDL_Event event;
	quad = createQuad();

	for (;;) {
		handleInput();
		assert(glGetError() == GL_NO_ERROR);
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) { goto exit; }
		}

		auto frameStart = std::chrono::high_resolution_clock::now(); 
		glClearColor(1.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		g_VALIDGLSTATE = shaderWatcher.watch();

		if (g_VALIDGLSTATE) {
			currentProgram->use();
			drawQuad();
			currentProgram->use(0);
		}
		SDL_GL_SwapWindow(window);
		glFinish();

		// compute fps
		auto frameEnd = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd- frameStart);
		float fps = 1.0f / ((float)ms.count() * 0.001f);
		std::string timestr = std::to_string((int)floor(fps)) + " FPS\n";
		SDL_SetWindowTitle(window, timestr.c_str());

		assert(glGetError() == GL_NO_ERROR);
		ticks++;
	}

exit:
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	delete quad;
	return 0;
}

