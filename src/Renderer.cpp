#include "GL/glew.h"
#include <GL/gl.h>
#include "Renderer.h"
#include "ShaderWatcher.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "Input.h"
#include "utils/Math.h"
#include <cassert>
#include <vector>
#include <iostream>

Vector3 gResolution;
Vector3 gCameraPosition;
Matrix3 gCameraOrientation;
Vector3 gMouseCoordinates;
unsigned ticks = 0;

GLuint  noiseTexture1ID;
GLuint  depthBuffer1ID; 

GLuint sceneTexture2ID;
GLuint depthBuffer2ID;

GLuint bloomTexture1ID;
GLuint bloomTexture2ID;

GLuint  framebuffer1ID;
GLuint  framebuffer2ID;
GLuint  bloomFramebuffer1ID;
GLuint  bloomFramebuffer2ID;

#define NOISETEXSIZE 256 
#define W 800 
#define H 600 
ShaderProgram *noiseGeneratorProgram = nullptr;
ShaderProgram *mainProgram = nullptr;
ShaderProgram *fxaaProgram = nullptr;
ShaderProgram *combinerProgram = nullptr;
GLMesh *screenQuad = nullptr;

const char *DEFAULT_VERT_SHADER = R"(
	#version 450
	in vec4 position; in vec2 texCoord;
	out vec2 fragTexCoord;
	void main(void) { gl_Position = position; fragTexCoord = texCoord; }
)";

void setupFrameBuffers();
		
bool initialize() {
	ShaderWatcher& shaderWatcher = ShaderWatcher::getInstance();
	bool initSuccessful = true;

	const char *shaderPath = "D://Projects/ShaderPreviewV2/shaders/noisetex.frag";
	const char *mainProgramPath = "D://Projects/ShaderPreviewV2/shaders/A/testificate.frag";
	const char *fxaaProgramPath = "D://Projects/ShaderPreviewV2/shaders/A/postprocess-bloom.frag";
	const char *combinerProgramPath = "D://Projects/ShaderPreviewV2/shaders/A/postprocess-combiner.frag";


	// initialize shader progs.
	noiseGeneratorProgram = new ShaderProgram();
	noiseGeneratorProgram->addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	noiseGeneratorProgram->addShader(readTextFile(shaderPath).c_str(), GL_FRAGMENT_SHADER);
	shaderWatcher.add(shaderPath, noiseGeneratorProgram, GL_FRAGMENT_SHADER);
	initSuccessful = initSuccessful && noiseGeneratorProgram->link();

	mainProgram = new ShaderProgram();
	mainProgram->addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	mainProgram->addShader(readTextFile(mainProgramPath).c_str(), GL_FRAGMENT_SHADER);
	shaderWatcher.add(mainProgramPath, mainProgram, GL_FRAGMENT_SHADER);
	initSuccessful = initSuccessful && mainProgram->link();


	fxaaProgram = new ShaderProgram();
	fxaaProgram->addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	fxaaProgram->addShader(readTextFile(fxaaProgramPath).c_str(), GL_FRAGMENT_SHADER);
	shaderWatcher.add(fxaaProgramPath, fxaaProgram, GL_FRAGMENT_SHADER);
	initSuccessful = initSuccessful && fxaaProgram->link();

	combinerProgram= new ShaderProgram();
	combinerProgram->addShader(DEFAULT_VERT_SHADER, GL_VERTEX_SHADER);
	combinerProgram->addShader(readTextFile(combinerProgramPath).c_str(), GL_FRAGMENT_SHADER);
	shaderWatcher.add(combinerProgramPath, combinerProgram, GL_FRAGMENT_SHADER);
	initSuccessful = initSuccessful && combinerProgram->link();

	screenQuad = createQuad();

	// initialize textures
	glGenTextures(1, &noiseTexture1ID);
	glBindTexture(GL_TEXTURE_2D, noiseTexture1ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, NOISETEXSIZE, NOISETEXSIZE, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &depthBuffer1ID);
	glBindTexture(GL_TEXTURE_2D, depthBuffer1ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, NOISETEXSIZE, NOISETEXSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	// textures for main scene
	glGenTextures(1, &sceneTexture2ID);
	glBindTexture(GL_TEXTURE_2D, sceneTexture2ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &depthBuffer2ID);
	glBindTexture(GL_TEXTURE_2D, depthBuffer2ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, W, H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	// textures for bloom pass 
	glGenTextures(1, &bloomTexture1ID);
	glBindTexture(GL_TEXTURE_2D, bloomTexture1ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &bloomTexture2ID);
	glBindTexture(GL_TEXTURE_2D, bloomTexture2ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	// initialize framebuffers
	glGenFramebuffers(1, &framebuffer1ID);
	glGenFramebuffers(1, &framebuffer2ID);
	glGenFramebuffers(1, &bloomFramebuffer1ID);
	glGenFramebuffers(1, &bloomFramebuffer2ID);



	setupFrameBuffers();


	assert(glGetError() == GL_NO_ERROR);
	return initSuccessful;
}

void setupFrameBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2ID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture2ID, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "framebuffer2 incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer1ID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture1ID, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "bloomFramebuffer1 incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer2ID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture2ID, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "bloomFramebuffer2 incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeTextures(int width, int height) {
	gResolution.x = (float)width;
	gResolution.y = (float)height;

	// resize colorbuffers and stuff....
}

void drawFrame() {
	//render main scene
	mainProgram->use();
	glViewport(0,0,W,H);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2ID);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return;
	}

	float tickf = ((float)ticks) * 0.1;


	mainProgram->uniform("resolution", Vector3(W,H,0));
	mainProgram->uniform("iGlobalTime", tickf);
	mainProgram->uniform("viewMatrix", gCameraOrientation);
	mainProgram->uniform("viewPosition", gCameraPosition);

	// bind noise texture
	glBindTexture(GL_TEXTURE_2D, noiseTexture1ID);
	glActiveTexture(GL_TEXTURE1);
	mainProgram->uniform("iChannel0", 1);
	screenQuad->setupAttributes(*mainProgram);

		
	screenQuad->bind();
	screenQuad->draw();
	screenQuad->bind(0);
	mainProgram->use(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// bloom pass 1  
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer1ID);
	fxaaProgram->use();
	fxaaProgram->uniform("iResolution", Vector3(W,H,0));
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sceneTexture2ID);
	fxaaProgram->uniform("u_texture", 1);
	fxaaProgram->uniform("blur_direction", 0);
	//screenQuad->setupAttributes(*fxaaProgram);
	screenQuad->bind();
	screenQuad->draw();
	screenQuad->bind(0);
	fxaaProgram->use(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);



	//bloom pass 2
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer2ID);
	fxaaProgram->use();
	fxaaProgram->uniform("iResolution", Vector3(W,H,0));
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bloomTexture1ID);
	fxaaProgram->uniform("u_texture", 2);
	fxaaProgram->uniform("blur_direction", 1);
	//screenQuad->setupAttributes(*fxaaProgram);
	screenQuad->bind();
	screenQuad->draw();
	screenQuad->bind(0);
	fxaaProgram->use(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	combinerProgram->use();
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bloomTexture1ID);
	combinerProgram->uniform("u_texture1", 2);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sceneTexture2ID);
	combinerProgram->uniform("u_texture2", 1);

	//screenQuad->setupAttributes(*combinerProgram);
	screenQuad->bind();
	screenQuad->draw();
	screenQuad->bind(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	combinerProgram->use(0);



	ticks++;
}

void cleanup() {
	ShaderWatcher& shaderWatcher = ShaderWatcher::getInstance();
	shaderWatcher.clear();
	delete fxaaProgram;
	delete mainProgram;
	delete noiseGeneratorProgram;
	delete screenQuad;
	glDeleteTextures(1, &noiseTexture1ID);
	glDeleteTextures(1, &depthBuffer1ID);
	glDeleteTextures(1, &sceneTexture2ID);
	glDeleteTextures(1, &depthBuffer2ID);
	glDeleteTextures(1, &bloomTexture1ID);
	glDeleteTextures(1, &bloomTexture2ID);
	glDeleteFramebuffers(1, &framebuffer1ID);
	glDeleteFramebuffers(1, &framebuffer2ID);
	glDeleteFramebuffers(1, &bloomFramebuffer1ID);
	glDeleteFramebuffers(1, &bloomFramebuffer2ID);
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