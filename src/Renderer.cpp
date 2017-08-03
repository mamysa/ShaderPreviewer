#include "GL/glew.h"
#include <GL/gl.h>
#include "Renderer.h"
#include "ShaderWatcher.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "Input.h"
#include "utils/Math.h"
#include <cassert>

Vector3 gResolution;
Vector3 gCameraPosition;
Matrix3 gCameraOrientation;
Vector3 gMouseCoordinates;
unsigned ticks = 0;


ShaderProgram *myMainProgram = nullptr;
GLMesh *screenQuad = nullptr;

const char *DEFAULT_VERT_SHADER = R"(
	#version 450
	in vec4 position; in vec2 texCoord;
	out vec2 fragTexCoord;
	void main(void) { gl_Position = position; fragTexCoord = texCoord; }
)";

bool initialize(ShaderWatcher& shaderWatcher) {
	bool initSuccessful = true;

	const char *shaderPath = "D://Projects/ShaderPreviewV2/shaders/sdf1.frag";

	// initialize shader progs.
	myMainProgram = new ShaderProgram();
	myMainProgram->addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	myMainProgram->addShader(readTextFile(shaderPath).c_str(), GL_FRAGMENT_SHADER);
	shaderWatcher.add(shaderPath, myMainProgram, GL_FRAGMENT_SHADER);
	initSuccessful = initSuccessful && myMainProgram->link();


	screenQuad = createQuad();


	assert(glGetError() == GL_NO_ERROR);
	return initSuccessful;
}

void resizeTextures(int width, int height) {
	gResolution.x = (float)width;
	gResolution.y = (float)height;

	// resize colorbuffers and stuff....
}

void drawFrame() {
	glViewport(0,0,gResolution.x,gResolution.y);


	myMainProgram->use();
	screenQuad->setupAttributes(*myMainProgram);
	float tickf = ((float)ticks) * 0.1;
	myMainProgram->uniform("resolution", gResolution);
	myMainProgram->uniform("iGlobalTime", tickf);
	myMainProgram->uniform("viewMatrix", gCameraOrientation);
	myMainProgram->uniform("viewPosition", gCameraPosition);
	screenQuad->bind();
	screenQuad->draw();
	screenQuad->bind(0);
	myMainProgram->use(0);


	ticks++;
}

void cleanup(ShaderWatcher& shaderWatcher) {
	shaderWatcher.clear();
	delete myMainProgram;
	delete screenQuad;
}

void handleInput(void) {
	const float rotXStep = 0.01f;
	const float rotYStep = 0.01f;
	const float movStep  = 0.06f;

	float movingForward  = 0.0f;
	float movingSideways = 0.0f;
	float movingUp       = 0.0f;

	const bool *keys = getKeys();

	if(keys[KEY_LEFT])  { gMouseCoordinates.x -= rotXStep; }
	if(keys[KEY_RIGHT]) { gMouseCoordinates.x += rotXStep; }
	if(keys[KEY_UP])    { gMouseCoordinates.y -= rotYStep; }
	if(keys[KEY_DOWN])  { gMouseCoordinates.y += rotYStep; }
	if(keys[KEY_W])     { movingForward  += movStep;  }
	if(keys[KEY_S])     { movingForward  -= movStep;  }
	if(keys[KEY_D])     { movingSideways += movStep;  }
	if(keys[KEY_A])     { movingSideways -= movStep;  }
	if(keys[KEY_C])     { movingUp -= movStep; }
	if(keys[KEY_SPACE]) { movingUp += movStep; }
	if(keys[KEY_ESC]) {
		gCameraPosition = Vector3();
		gMouseCoordinates = Vector3();
		ticks = 0;
	}

	gCameraOrientation = rotateY(gMouseCoordinates.x)*rotateX(gMouseCoordinates.y);
	Vector3 rd    = (gCameraOrientation * Vector3(0.0, 0.0, 1.0)).normalize();
	Vector3 right = (gCameraOrientation * Vector3(1.0, 0.0, 0.0)).normalize();
	Vector3 up    = (gCameraOrientation * Vector3(0.0, 1.0, 0.0)).normalize();
	gCameraPosition = gCameraPosition + rd    * movingForward;
	gCameraPosition = gCameraPosition + right * movingSideways;
	gCameraPosition = gCameraPosition + up    * movingUp;
}