#include "GL/glew.h"
#include "GL/wglew.h"
#include <GL/gl.h>
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "ShaderWatcher.h"
#include "utils/Math.h"
#include "Input.h"
#include "Renderer.h"
#include <chrono>
#include <windows.h>
#include <iostream>
#include <cassert>

bool g_VALIDGLSTATE = true;


Vector3 windowSize = Vector3(800, 600, 0);
Vector3 windowResizeEvent = Vector3(800, 600, 0);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HGLRC glContext;
bool RUNNING = true;
bool STAGE_ONE_SUCCESS = false;
bool STAGE2_SUCCESS = false;

HWND hwnd;


bool setupBasicGLContext(void) {
	HDC deviceContext = GetDC(hwnd);
	PIXELFORMATDESCRIPTOR descriptor;


	descriptor = {
		sizeof(PIXELFORMATDESCRIPTOR),      // nsize
		1,                                  // nversion
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // dwflags
		PFD_TYPE_RGBA,                      // iPixelType
		32, // cColorBits
		0, 0, // cRedBits, cRedShift
		0, 0, // cGreenBits, cGreenShift
		0, 0, // cBlueBits, cBlueShift
		0, 0, // cAlphaBits, cAlphaShift
		0, 0, 0, 0, 0, // cAccumBits, Red, Green, blue, Alpha
		24, // cDepthBits
		8,  // cStencilBits
		0,  // cAuxBuffers
		PFD_MAIN_PLANE, // iLayerType
		0, // bReserved
		0, // dwLayerMask
		0, //dwVisibleMask
		0  // dwDamageMask
	};

	int pixelFormat = ChoosePixelFormat(deviceContext, &descriptor);
	bool status = SetPixelFormat(deviceContext, pixelFormat, &descriptor);
	if (!status) { return false; }

	glContext = wglCreateContext(deviceContext);
	if (glContext == NULL) {
		return false;
	}
	return true;
}

bool setupGL4Context() {
	HDC deviceContext = GetDC(hwnd);
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
		return false;	
	}

	return true;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
	// Register the window class.
	const char *CLASS_NAME = "Sample Window Class";

	WNDCLASS wc = { };
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	// Create the window and dummy context
	hwnd = CreateWindow(CLASS_NAME, 0, WS_OVERLAPPEDWINDOW, 0, 0, 800, 600,  NULL, NULL, hInstance, NULL);
	if (hwnd == NULL || !setupBasicGLContext()) { 
		return 1; 
	}

	wglMakeCurrent(GetDC(hwnd), glContext);
	const GLubyte *version = glGetString(GL_VERSION);

	if (glewInit() != GLEW_OK) {
		return 2;
	}

	wglDeleteContext(glContext);
	glContext = NULL;
	DestroyWindow(hwnd);


	hwnd = CreateWindow(CLASS_NAME, 0, WS_OVERLAPPEDWINDOW, 0, 0, 800, 600,  NULL, NULL, hInstance, NULL);
	if (hwnd == NULL || !setupGL4Context()) { 
		return 3;	
	}

	wglMakeCurrent(GetDC(hwnd), glContext);
	g_VALIDGLSTATE = initialize();

	BOOL console = AllocConsole();
	freopen("CONOUT$", "w", stdout); 

	ShowWindow(hwnd, nCmdShow);
	
	// Run the message loop.
	RUNNING =true;
	MSG msg = { };
	HDC deviceContext = GetDC(hwnd);


	//wglSwapIntervalEXT(0);
	for (;;) {
		if(!RUNNING) {
			break;
		}

		auto frameStart = std::chrono::high_resolution_clock::now();

		if (RUNNING) {
#if 0
			if (windowSize.x != windowResizeEvent.x || windowSize.y != windowResizeEvent.y) {
				std::cout << "We should resize textures...!\n";
				windowSize = windowResizeEvent;
				resizeTextures(windowResizeEvent.x, windowResizeEvent.y);
			}
#endif
			ShaderWatcher::getInstance().tryUpdate();

			glClearColor(1.0, 1.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);

			if (ShaderWatcher::getInstance().resourcesAreOK()) {
				handleInput();
				drawFrame();	
			}

			glFinish();
			SwapBuffers(deviceContext);	
			assert(glGetError() == GL_NO_ERROR);
		}

		auto frameEnd = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd- frameStart);
		float fps = 1.0f / ((float)ms.count() * 0.001f);
		std::string timestr = std::to_string((int)floor(fps)) + " FPS\n";

		SetWindowText(hwnd, timestr.c_str());


		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	cleanup();
	wglDeleteContext(glContext);
	DestroyWindow(hwnd);
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_DESTROY: {
			RUNNING = false;
			wglMakeCurrent(GetDC(hwnd), NULL);
			//wglDeleteContext(glContext);
			PostQuitMessage(0);
			return 0;
		}

		case WM_KEYDOWN: {
			setKeyState(wParam, true);
			return 0;
		}

		case WM_KEYUP: {
			setKeyState(wParam, false);
			return 0;
		}
		
		case WM_SIZE: {
			windowResizeEvent.x = LOWORD(lParam);
			windowResizeEvent.y = HIWORD(lParam);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}