
#ifdef IS_WINDOWS
#include "GL/glew.h"
#include <GL/gl.h>
#include "SDL.h"
#include <windows.h>
#endif

#ifdef IS_OSX
//#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <SDL2/SDL.h>
#endif


#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "ResourceManager.h"
#include "utils/Math.h"
#include "Input.h"
#include "Renderer.h"
#include <chrono>
#include <iostream>
#include <cassert>
#include <string>
#include "imgui.h"
#include "ui/UI.h"
#include "ui/UIRender.h"
#include "Logger.h"

#define W 1280
#define H 720 

static bool RUNNING = true;
static int w, h;

static std::string getGLVersion(void) {
	int major = 0, minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	return "OpenGL " + std::to_string(major) + "." + std::to_string(minor);
}

int main(int argc, char **argv) {
	w = W;
	h = H;
#ifdef IS_WINDOWS
	FreeConsole();
#endif
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
#ifdef IS_WINDOWS
		MessageBox(NULL, "Error initializing SDL", NULL, MB_OK);
#endif
#ifdef IS_OSX
		std::cout << "Error initializing SDL\n";
#endif
		return 1;
	}
		
	SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (!window) {
#ifdef IS_WINDOWS
		MessageBox(NULL, "Error initializing SDL window", NULL, MB_OK);
#endif
#ifdef IS_OSX
		std::cout << "Error initializing SDL window\n";
#endif
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);
	

#ifdef IS_WINDOWS
	if (glewInit() != GLEW_OK) {
		MessageBox(NULL, "Error initializing glew", NULL, MB_OK);
		return 1;
	}
#endif

	auto version = getGLVersion();	
	SDL_SetWindowTitle(window, version.c_str());

	SDL_ShowWindow(window);
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); 
	ImGui_ImplSdlGL3_Init(window);
	ImGui::StyleColorsDark();
	initialize();

	SDL_Event event;
	//wglSwapIntervalEXT(0);
	while (RUNNING) {
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT) RUNNING = false;
		}

		SDL_GetWindowSize(window, &w, &h);
		ImGui_ImplSdlGL3_NewFrame(window);
		makeUI(w, h);

		glClearColor(0.05, 0.05, 0.05, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		setKeyState();
		handleInput();
		ResourceManager::getInstance().tryUpdate();
		if (ResourceManager::getInstance().resourcesAreOK()) {
			drawFrame();	
		}
		glFinish();
		ImGui::Render();
		SDL_GL_SwapWindow(window);	
	}

	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	cleanup();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
