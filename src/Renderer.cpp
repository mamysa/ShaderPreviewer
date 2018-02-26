#include "GL/glew.h"
#ifdef IS_WINDOWS
#include <GL/gl.h>
#endif

#ifdef IS_OSX
#include <OpenGL/gl.h>
#endif

#include "Renderer.h"
#include "ResourceManager.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Mesh.h"
#include "opengl/Texture.h"
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

// textures
Texture2D *sceneTexture2ID = nullptr;
Texture2D *bloomTexture1ID = nullptr;
Texture2D *bloomTexture2ID = nullptr;
Texture2D *finalTextureID = nullptr;

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


	std::string s1 = std::string(noiseGenProgram.identifier);
	std::string s2 = std::string(mainProgramAST.identifier);
	std::string s3 = std::string(fxaaProgramAST.identifier);
	std::string s4 = std::string(combinerProgramAST.identifier);

	ShaderProgramResource *r1 = (ShaderProgramResource*)shaderWatcher.lookupResource(s1);
	ShaderProgramResource *r2 = (ShaderProgramResource*)shaderWatcher.lookupResource(s2);
	ShaderProgramResource *r3 = (ShaderProgramResource*)shaderWatcher.lookupResource(s3);
	ShaderProgramResource *r4 = (ShaderProgramResource*)shaderWatcher.lookupResource(s4);

	noiseGeneratorProgram = r1->program;
	mainProgram = r2->program;
	fxaaProgram = r3->program;
	combinerProgram = r4->program;


	ASTNodeTexture2D sceneTextureAST = { "scene_texture", W, H };
	ASTNodeTexture2D bloomTexture1AST = { "bloom1_texture", W, H };
	ASTNodeTexture2D bloomTexture2AST = { "bloom2_texture", W, H };
	ASTNodeTexture2D finalTextureAST = { "final_texture", W, H };

	shaderWatcher.addTextureResource(sceneTextureAST);
	shaderWatcher.addTextureResource(bloomTexture1AST);
	shaderWatcher.addTextureResource(bloomTexture2AST);
	shaderWatcher.addTextureResource(finalTextureAST);

	std::string a1 = std::string(sceneTextureAST.identifier);
	std::string a2 = std::string(bloomTexture1AST.identifier);
	std::string a3 = std::string(bloomTexture2AST.identifier);
	std::string a4 = std::string(finalTextureAST.identifier);
	
	Texture2DResource *t1 = (Texture2DResource*)shaderWatcher.lookupResource(a1);
	Texture2DResource *t2 = (Texture2DResource*)shaderWatcher.lookupResource(a2);
	Texture2DResource *t3 = (Texture2DResource*)shaderWatcher.lookupResource(a3);
	Texture2DResource *t4 = (Texture2DResource*)shaderWatcher.lookupResource(a4);

	sceneTexture2ID = t1->texture;
	bloomTexture1ID = t2->texture;
	bloomTexture2ID = t3->texture;
	finalTextureID = t4->texture;
	

	screenQuad = createQuad();
	// initialize textures


	

	


	// initialize framebuffers
	glGenFramebuffers(1, &framebuffer1ID);
	glGenFramebuffers(1, &framebuffer2ID);
	glGenFramebuffers(1, &bloomFramebuffer1ID);
	glGenFramebuffers(1, &bloomFramebuffer2ID);
	glGenFramebuffers(1, &finalFramebufferID);


	setupFrameBuffers();
	assert(glGetError() == GL_NO_ERROR);



	assert(glGetError() == GL_NO_ERROR);
	return initSuccessful;
}

void setupFrameBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer2ID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture2ID->getID(), 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "framebuffer2 incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer1ID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture1ID->getID(), 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "bloomFramebuffer1 incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer2ID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture2ID->getID(), 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { std::cout << "bloomFramebuffer2 incomplete"; }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glBindFramebuffer(GL_FRAMEBUFFER, finalFramebufferID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalTextureID->getID(), 0);
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
	assert(glGetError() == GL_NO_ERROR);
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
		glActiveTexture(GL_TEXTURE1); sceneTexture2ID->bind();
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
		sceneTexture2ID->bind(0);
	fxaaProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//bloom pass 2
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffer2ID);
	fxaaProgram->use();
		fxaaProgram->uniform("iResolution", Vector3(W,H,0));
		fxaaProgram->uniform("u_texture", 2);
		fxaaProgram->uniform("blur_direction", 1);
		glActiveTexture(GL_TEXTURE2); bloomTexture1ID->bind();
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
		bloomTexture1ID->bind(0);
	fxaaProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	assert(glGetError() == GL_NO_ERROR);

	glBindFramebuffer(GL_FRAMEBUFFER, finalFramebufferID);
	combinerProgram->use();
		combinerProgram->uniform("u_texture2", 1);
		combinerProgram->uniform("u_texture1", 2);
		glActiveTexture(GL_TEXTURE1); sceneTexture2ID->bind(); 
		glActiveTexture(GL_TEXTURE2); bloomTexture2ID->bind(); 
		screenQuad->bind();
		screenQuad->draw();
		screenQuad->bind(0);
		glActiveTexture(GL_TEXTURE1); sceneTexture2ID->bind(0); 
		glActiveTexture(GL_TEXTURE2); bloomTexture2ID->bind(0); 
	combinerProgram->use(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ticks++;
}

void cleanup() {
	ResourceManager& shaderWatcher = ResourceManager::getInstance();
	shaderWatcher.removeAllResources();
	delete screenQuad;
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
