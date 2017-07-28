
#include <iostream>
#if SDL_ENABLED
#include "GL/glew.h"
#include <GL/gl.h>
#include "SDL.h"
#else
#include "GL/glew.h"
#include "GL/wglew.h"
#include <GL/gl.h>
#endif
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

#ifdef SDL_ENABLED
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
#else

#include <windows.h>


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HGLRC glContext;
bool RUNNING = true;
bool STAGE_ONE_SUCCESS = false;
bool STAGE2_SUCCESS = false;



int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {

	// Register the window class.
	const char *CLASS_NAME = "Sample Window Class";

	WNDCLASS wc = { };
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hwnd;

	// Create the window and dummy context
	hwnd = CreateWindow(CLASS_NAME, 0, WS_OVERLAPPEDWINDOW, 0, 0, 800, 600,  NULL, NULL, hInstance, NULL);
	if (hwnd == NULL || !STAGE_ONE_SUCCESS) { 
		return 0; 
	}
	glContext = NULL;
	DestroyWindow(hwnd);


	hwnd = CreateWindow(CLASS_NAME, 0, WS_OVERLAPPEDWINDOW, 0, 0, 800, 600,  NULL, NULL, hInstance, NULL);
	if (hwnd == NULL || !STAGE2_SUCCESS) { 
		return 2;	
	}



	ShowWindow(hwnd, nCmdShow);

	
	

	// Run the message loop.
	RUNNING =true;
	MSG msg = { };
	HDC deviceContext = GetDC(hwnd);
	for (;;) {
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		glClearColor(1.0, 1.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		SwapBuffers(deviceContext);	
		if(!RUNNING) break;
		
	}

	DestroyWindow(hwnd);
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_CREATE: { 
			HDC deviceContext = GetDC(hwnd);

			// create opengl4.0 context
			if (STAGE_ONE_SUCCESS) {
				int  pixel_format_arb;
				UINT pixel_formats_found;

				const int pixel_attributes[] = {
					WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
					WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
					WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
					WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB,     32,
					WGL_DEPTH_BITS_ARB,     24,
					WGL_STENCIL_BITS_ARB,   8,	
					0
				};

				const int contextAttribs[] = {
					WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
					WGL_CONTEXT_MINOR_VERSION_ARB, 4,
					WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
					0
				};

				BOOL result = wglChoosePixelFormatARB(deviceContext, pixel_attributes, NULL, 1, &pixel_format_arb, &pixel_formats_found);
				bool status = SetPixelFormat(deviceContext, pixel_format_arb, NULL);
				assert(result && status);
				glContext = wglCreateContextAttribsARB(deviceContext, NULL, contextAttribs);

				if (!glContext) {
					return 0;	
				}

				wglMakeCurrent(deviceContext, glContext);
				STAGE2_SUCCESS = true;
				return 0;
			}


			// Initialize the basic OpenGL context and get function pointers...
			if (!STAGE_ONE_SUCCESS) {
				PIXELFORMATDESCRIPTOR descriptor;
				ZeroMemory(&descriptor, sizeof(PIXELFORMATDESCRIPTOR));
				descriptor.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
				descriptor.nVersion     = 1;
				descriptor.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
				descriptor.iPixelType   = PFD_TYPE_RGBA;
				descriptor.cColorBits   = 32;
				descriptor.cDepthBits   = 24;
				descriptor.cStencilBits = 8;
				descriptor.iLayerType   = PFD_MAIN_PLANE;

				int pixelFormat = ChoosePixelFormat(deviceContext, &descriptor);
				bool status = SetPixelFormat(deviceContext, pixelFormat, &descriptor);
				if (!status) { return 0 ; }

				glContext = wglCreateContext(deviceContext);
				wglMakeCurrent(deviceContext, glContext);
				STAGE_ONE_SUCCESS = (glewInit() == GLEW_OK); 
				return 0;
			}

			return 0;
		}
		case WM_DESTROY: {
			RUNNING = false;
			wglMakeCurrent(GetDC(hwnd), NULL);
			wglDeleteContext(glContext);
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif