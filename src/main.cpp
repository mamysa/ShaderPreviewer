#include "GL/glew.h"
#include "GL/wglew.h"
#include <GL/gl.h>
#include "SDL.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "ResourceManager.h"
#include "utils/Math.h"
#include "Input.h"
#include "Renderer.h"
#include <chrono>
#include <windows.h>
#include <iostream>
#include <cassert>
#include "imgui.h"
#include "ui/UIRender.h"
#include "Logger.h"

#include "ui/UI.h"




Vector3 windowSize = Vector3(800, 600, 0);
Vector3 windowResizeEvent = Vector3(800, 600, 0);

bool RUNNING = true;


#define W 1280
#define H 720 

int w, h;

ImVec2 outputsize;

int main(int argc, char **argv) {
	w = W;
	h = H;
	FreeConsole();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		MessageBox(NULL, "Error initializing SDL", NULL, MB_OK);
		return 1;
	}
		
	SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (!window) {
		MessageBox(NULL, "Error initializing SDL window", NULL, MB_OK);
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	
	const GLubyte *version = glGetString(GL_VERSION);

	if (glewInit() != GLEW_OK) {
		MessageBox(NULL, "Error initializing glew", NULL, MB_OK);
		return 1;
	}

	//initialize();
	SDL_ShowWindow(window);
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); 
	ImGui_ImplSdlGL3_Init(window);
	ImGui::StyleColorsDark();
	
	initialize();

	SDL_Event event;
	//wglSwapIntervalEXT(0);

	const ImVec4 ColSuccess(0.039, 0.901, 0.047, 1.0);
	const ImVec4 ColFailure(0.9, 0.05, 0.05, 1.0);
	const ImVec4 ColInfo(0.9, 0.9, 0.9, 1.0);
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

		//glViewport(0, 0, w, h);
		ImGui::Render();
		SDL_GL_SwapWindow(window);	

#if 0
		
		glClearColor(1.0, 1.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		auto frameStart = std::chrono::high_resolution_clock::now();

		if (ResourceManager::getInstance().resourcesAreOK()) {
			drawFrame();	
		}

		glFinish();
		SDL_GL_SwapWindow(window);	
		assert(glGetError() == GL_NO_ERROR);

		auto frameEnd = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd- frameStart);
		float fps = 1.0f / ((float)ms.count() * 0.001f);
		std::string timestr = std::to_string((int)floor(fps)) + " FPS\n";
		SDL_SetWindowTitle(window, timestr.c_str());


		
	#endif
	}

	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();


	cleanup();
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}