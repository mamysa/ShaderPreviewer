
#include <iostream>

#include "GL/glew.h"
#include "GL/wglew.h"
#include <GL/gl.h>
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "ShaderWatcher.h"
#include "Input.h"
#include <chrono>
#include <cassert>
#include <windows.h>

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

Vector3 mouseCoordinates;
Vector3 cameraPosition; 
Vector3 windowSize = Vector3(800, 600, 0);
Vector3 windowResizeEvent = Vector3(800, 600, 0);
unsigned ticks = 0;

static void handleInput(void) {
	const float rotXStep = 0.01f;
	const float rotYStep = 0.01f;
	const float movStep  = 0.06f;

	float movingForward  = 0.0f;
	float movingSideways = 0.0f;
	float movingUp       = 0.0f;

	if(keys[KEY_LEFT])  { mouseCoordinates.x -= rotXStep; }
	if(keys[KEY_RIGHT]) { mouseCoordinates.x += rotXStep; }
	if(keys[KEY_UP])    { mouseCoordinates.y -= rotYStep; }
	if(keys[KEY_DOWN])  { mouseCoordinates.y += rotYStep; }
	if(keys[KEY_W])     { movingForward  += movStep;  }
	if(keys[KEY_S])     { movingForward  -= movStep;  }
	if(keys[KEY_D])     { movingSideways += movStep;  }
	if(keys[KEY_A])     { movingSideways -= movStep;  }
	if(keys[KEY_C])     { movingUp -= movStep; }
	if(keys[KEY_SPACE]) { movingUp += movStep; }
	if(keys[KEY_ESC]) {
		cameraPosition = Vector3();
		mouseCoordinates = Vector3();
		ticks = 0;
	}

	if (g_VALIDGLSTATE) {
		Matrix3 a = rotateY(mouseCoordinates.x) * rotateX(mouseCoordinates.y);
		Vector3 rd = (a * Vector3(0.0, 0.0, 1.0)).normalize();
		Vector3 right = (a * Vector3(1.0, 0.0, 0.0)).normalize();
		Vector3 up = (a * Vector3(0.0, 1.0, 0.0)).normalize();
		cameraPosition = cameraPosition + rd * movingForward;
		cameraPosition = cameraPosition + right * movingSideways;
		cameraPosition = cameraPosition + up * movingUp;

		currentProgram->uniform("viewMatrix", a);
		currentProgram->uniform("viewPosition", cameraPosition);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HGLRC glContext;
bool RUNNING = true;
bool STAGE_ONE_SUCCESS = false;
bool STAGE2_SUCCESS = false;

void drawQuad(void) {
	quad->setupAttributes(*currentProgram);
	float tickf = ((float)ticks) * 0.1;
	currentProgram->uniform("resolution", windowSize);
	currentProgram->uniform("iGlobalTime", tickf);
	quad->bind();
	quad->draw();
	quad->bind(0);
}

#include "utils/Math.h"

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

	const char *shaderPath = "D://Projects/ShaderPreviewV2/shaders/sdf1.frag";
	ShaderProgram myShaderProgram;
	myShaderProgram.addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	myShaderProgram.addShader(readTextFile(shaderPath).c_str(), GL_FRAGMENT_SHADER);
        
	g_VALIDGLSTATE = myShaderProgram.link();
	shaderWatcher.add(shaderPath, &myShaderProgram, GL_FRAGMENT_SHADER);
	currentProgram = &myShaderProgram;
	assert(glGetError() == GL_NO_ERROR);


	quad = createQuad();

	BOOL console = AllocConsole();
	freopen("CONOUT$", "w", stdout);

	ShowWindow(hwnd, nCmdShow);
	
	// Run the message loop.
	RUNNING =true;
	MSG msg = { };
	HDC deviceContext = GetDC(hwnd);
	assert(glGetError() == GL_NO_ERROR);
	for (;;) {
		if(!RUNNING) {
			break;
		}
		assert(glGetError() == GL_NO_ERROR);
		auto frameStart = std::chrono::high_resolution_clock::now();

		glClearColor(1.0, 1.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		if (RUNNING) {
			g_VALIDGLSTATE = shaderWatcher.watch();
			handleInput();

			if (windowSize.x != windowResizeEvent.x || windowSize.y != windowResizeEvent.y) {
				std::cout << "We should resize textures...!\n";
				windowSize = windowResizeEvent;
			}

			if (g_VALIDGLSTATE) {
				glViewport(0,0,windowSize.x,windowSize.y);
				currentProgram->use();
				drawQuad();
				currentProgram->use(0);
			}

			SwapBuffers(deviceContext);	
			glFinish();
		}

		auto frameEnd = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd- frameStart);
		float fps = 1.0f / ((float)ms.count() * 0.001f);
		std::string timestr = std::to_string((int)floor(fps)) + " FPS\n";

		SetWindowText(hwnd, timestr.c_str());
		ticks++;

		assert(glGetError() == GL_NO_ERROR);

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//currentProgram->use(0);
	//currentProgram->~ShaderProgram();
	
	shaderWatcher.clear();
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