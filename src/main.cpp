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




Vector3 windowSize = Vector3(800, 600, 0);
Vector3 windowResizeEvent = Vector3(800, 600, 0);

bool RUNNING = true;


#define W 800
#define H 600


int main(int argc, char **argv) {
	FreeConsole();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		MessageBox(NULL, "Error initializing SDL", NULL, MB_OK);
		return 1;
	}
		
	SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_OPENGL);
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
	while (RUNNING) {
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			if (event.type == SDL_QUIT) RUNNING = false;
		}

		//mystr += "yay\n";


		ImGui_ImplSdlGL3_NewFrame(window);

		ImGui::Begin("Output");
		//ImVec2 vec = ImGui::GetWindowSize();

		const std::list<std::string>& messages = Logger::getBuf();
		for (auto it = messages.begin(); it != messages.end(); it++) {
			ImGui::Text(it->c_str());
		}
		//ImGui::SetScrollFromPosY(ImGui::GetScrollMaxY());

		//ImGui::SetWindowSize(ImVec2(600, 300));
		//std::string s = std::to_string(vec.x) + " " + std::to_string(vec.y);
		
		//ImGui::Text(s.c_str());
		ImGui::End();

		glClearColor(1.0, 1.0, 0.0, 1.0);
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