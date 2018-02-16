#include "GL/glew.h"
#include <GL/gl.h>
#include "Renderer.h"
#include "ResourceManager.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "Input.h"
#include "utils/Math.h"
#include <cassert>
#include <vector>
#include <iostream>

#include "imgui.h"

Vector3 gResolution;
Vector3 gCameraPosition;
Matrix3 gCameraOrientation;
Vector3 gMouseCoordinates;
unsigned ticks = 0;

GLuint  noiseTexture1ID;
GLuint  depthBuffer1ID; 

GLuint sceneTexture2ID;

GLuint bloomTexture1ID;
GLuint bloomTexture2ID;
GLuint finalTextureID;

GLuint  framebuffer1ID;
GLuint  framebuffer2ID;
GLuint  bloomFramebuffer1ID;
GLuint  bloomFramebuffer2ID;
GLuint  finalFramebufferID;

#define NOISETEXSIZE 256 
#define W 800 
#define H 600 
GLShaderProgram *noiseGeneratorProgram = nullptr;
GLShaderProgram *mainProgram = nullptr;
GLShaderProgram *fxaaProgram = nullptr;
GLShaderProgram *combinerProgram = nullptr;
GLMesh *screenQuad = nullptr;

void setupFrameBuffers();
		
bool initialize() {
	ResourceManager& shaderWatcher = ResourceManager::getInstance();
	bool initSuccessful = true;

	const char *defaultVertexShader= "D://Projects/ShaderPreviewV2/shaders/default.vert";
	const char *shaderPath = "D://Projects/ShaderPreviewV2/shaders/noisetex.frag";
	const char *mainProgramPath = "D://Projects/ShaderPreviewV2/shaders/A/testificate.frag";
	const char *fxaaProgramPath = "D://Projects/ShaderPreviewV2/shaders/A/postprocess-bloom.frag";
	const char *combinerProgramPath = "D://Projects/ShaderPreviewV2/shaders/A/postprocess-combiner.frag";


	ASTNodeShaderProgram noiseGenProgram = {
		"noise_generator", 
		defaultVertexShader,
		shaderPath
	};

	ASTNodeShaderProgram mainProgramAST = {
		"main_scene_program",
		defaultVertexShader,
		mainProgramPath
	};

	ASTNodeShaderProgram fxaaProgramAST = {
		"fxaa_program",
		defaultVertexShader,
		fxaaProgramPath
	};

	ASTNodeShaderProgram combinerProgramAST = {
		"combiner_program",
		defaultVertexShader,
		combinerProgramPath
	};

	shaderWatcher.addShaderProgramResource(noiseGenProgram);
	shaderWatcher.addShaderProgramResource(mainProgramAST);
	shaderWatcher.addShaderProgramResource(fxaaProgramAST);
	shaderWatcher.addShaderProgramResource(combinerProgramAST);

	ShaderProgramResource *r1 = (ShaderProgramResource*)shaderWatcher.lookupResource(std::string(noiseGenProgram.identifier));
	ShaderProgramResource *r2 = (ShaderProgramResource*)shaderWatcher.lookupResource(std::string(mainProgramAST.identifier));
	ShaderProgramResource *r3 = (ShaderProgramResource*)shaderWatcher.lookupResource(std::string(fxaaProgramAST.identifier));
	ShaderProgramResource *r4 = (ShaderProgramResource*)shaderWatcher.lookupResource(std::string(combinerProgramAST.identifier));

	noiseGeneratorProgram = r1->program;
	mainProgram = r2->program;
	fxaaProgram = r3->program;
	combinerProgram = r4->program;
	

	screenQuad = createQuad();

	// initialize textures

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

	glGenTextures(1, &finalTextureID);
	glBindTexture(GL_TEXTURE_2D, finalTextureID);
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
	glGenFramebuffers(1, &finalFramebufferID);

	assert(glGetError() == GL_NO_ERROR);


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


	glBindFramebuffer(GL_FRAMEBUFFER, finalFramebufferID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalTextureID, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "finalFramebuffer incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeTextures(int width, int height) {
	gResolution.x = (float)width;
	gResolution.y = (float)height;

	// resize colorbuffers and stuff....
}

void drawFrame() {
	float tickf = ((float)ticks) * 0.1;
	screenQuad->setupAttributes(*mainProgram);

	//render main scene
	glViewport(0,0,W,H);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2ID);
	mainProgram->use();
		mainProgram->uniform("resolution", Vector3(W,H,0));
		mainProgram->uniform("iGlobalTime", tickf);
		mainProgram->uniform("viewMatrix", gCameraOrientation);
		mainProgram->uniform("viewPosition", gCameraPosition);
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
	mainProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// bloom pass 1  
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer1ID);
	fxaaProgram->use();
		fxaaProgram->uniform("iResolution", Vector3(W,H,0));
		fxaaProgram->uniform("u_texture", 1);
		fxaaProgram->uniform("blur_direction", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, sceneTexture2ID);
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	fxaaProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//bloom pass 2
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer2ID);
	fxaaProgram->use();
		fxaaProgram->uniform("iResolution", Vector3(W,H,0));
		fxaaProgram->uniform("u_texture", 2);
		fxaaProgram->uniform("blur_direction", 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, bloomTexture1ID);
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	fxaaProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, finalFramebufferID);
	combinerProgram->use();
		combinerProgram->uniform("u_texture2", 1);
		combinerProgram->uniform("u_texture1", 2);
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, sceneTexture2ID);
		glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, bloomTexture1ID);
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
	combinerProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0,1.0,1.0,1.0));
	//ImGui::PushStyleVar(ImGuiStyleVar, 0.0);
	ImGui::Begin("Viewport");
	ImGui::Image((ImTextureID)finalTextureID, ImVec2(W*0.5,H*0.5),ImVec2(0, 1), ImVec2(1, 0) );
	ImGui::End();
	//ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	ticks++;
}

void cleanup() {
	ResourceManager& shaderWatcher = ResourceManager::getInstance();
	shaderWatcher.removeAllResources();
	delete fxaaProgram;
	delete mainProgram;
	delete noiseGeneratorProgram;
	delete screenQuad;
	glDeleteTextures(1, &noiseTexture1ID);
	glDeleteTextures(1, &depthBuffer1ID);
	glDeleteTextures(1, &sceneTexture2ID);
	glDeleteTextures(1, &bloomTexture1ID);
	glDeleteTextures(1, &bloomTexture2ID);
	glDeleteTextures(1, &finalTextureID);
	glDeleteFramebuffers(1, &framebuffer1ID);
	glDeleteFramebuffers(1, &framebuffer2ID);
	glDeleteFramebuffers(1, &bloomFramebuffer1ID);
	glDeleteFramebuffers(1, &bloomFramebuffer2ID);
	glDeleteFramebuffers(1, &finalFramebufferID);
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