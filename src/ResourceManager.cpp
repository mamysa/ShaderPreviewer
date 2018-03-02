#include <cassert>
#include <iostream>
#include <fstream>
#include "ResourceManager.h"
#include "opengl/ShaderProgram.h"
#include "opengl/Texture.h"
#include "Logger.h"


#ifdef IS_WINDOWS 
static HANDLE initializeFileHandle(const char *path) {
	HANDLE handle = CreateFile( path, 
		GENERIC_READ,  
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL );
	if (handle == INVALID_HANDLE_VALUE) {
		Logger::add("Unable to open file " + std::string(path) + "\n", LogType::FAILURE);	
	}
	return handle;
}
#endif


#ifdef IS_OSX
static bool fileExists(const char *path) {
	struct stat buf; 
	bool status = stat(path, &buf);

	if (status != 0) {
		Logger::add("Unable to open file " + std::string(path) + "\n", LogType::FAILURE);	
		return false;
	}

	return true;
}
#endif

// Query timestamp and return true if lastModified time is newer than the one
// stored in the info struct.
static bool checkandUpdateTimestamp(FileInfo& info) {
#ifdef IS_WINDOWS 
	FILETIME time; 
	bool status = GetFileTime(info.handle, NULL, NULL, &time);
	FILETIME *t = &info.lastUpdateTime;
	if (status && ( time.dwHighDateTime > t->dwHighDateTime || 
		            time.dwLowDateTime  > t->dwLowDateTime ) ) {
		info.lastUpdateTime = time;
		return true;
	}

	return false;
#endif

#ifdef IS_OSX
	struct stat buf;
	stat(info.filename, &buf);
	return false;
#endif
}

// IO function. This should not be here at all!  @FIXME
std::string readTextFile(const char *path) {
	std::ifstream stream(path);
	if (!stream.is_open()) {
		std::cout << "Error opening file " << path << "\r\n";
		exit(1);
	}

	std::string src;
	std::string temp;
	while (std::getline(stream, temp)) {
		src += (temp + "\n");
	}

	return src;
}

//=============================================
// FileInfo struct implementation 
//=============================================
FileInfo::FileInfo(const char *f) :
#ifdef IS_WINDOWS
	handle(initializeFileHandle(f)), 
	lastUpdateTime({0, 0}),
#endif
#ifdef IS_OSX
	exists(fileExists(f)), 
	lastUpdateTime(0),
#endif
	filename(f)
{ }

bool FileInfo::isOK(void) {
#ifdef IS_WINDOWS
	return handle != INVALID_HANDLE_VALUE;	
#endif

#ifdef IS_OSX
	return false;
#endif
}

FileInfo::~FileInfo(void) {
#if IS_WINDOWS
	if (isOK())
		CloseHandle(handle);		
#endif
}

//=============================================
// ShaderProgramResource implementation
//=============================================
ShaderProgramResource::ShaderProgramResource(const char *ident) :
	BaseResource(R_ShaderProgram),
	programIdentifier(ident),
	program(new GLShaderProgram()), 
	requiresRelinking(false)
{ }

ShaderProgramResource::~ShaderProgramResource(void) {
	// FIXME we should also be deleting program identifier but ATM we do not own it...
	delete program;
}

void ShaderProgramResource::tryUpdate(void) {
	if (requiresRelinking) {
		program->link(); 
		if (program->errorOccured()) {
			Logger::add("Error linking shader program " + std::string(programIdentifier) + "\n", LogType::FAILURE);	
		}

		requiresRelinking = false;
	}
}

bool ShaderProgramResource ::isOK(void) {
	return !program->errorOccured();
}

//=============================================
// ShaderResouce struct implementation 
//=============================================

ShaderResource::ShaderResource(const char *filename, GLenum type): 
	BaseResource(R_Shader),
	fileInfo(filename), 
	shader(new GLShader(type))   { }

void ShaderResource::tryUpdate() {
	if (!checkandUpdateTimestamp(fileInfo)) 
		return;
	
	std::string shadersrc = readTextFile(fileInfo.filename);
	shader->compile(shadersrc.c_str());
	if (shader->errorOccured()) {
		Logger::add("Error updating shader: " + std::string(fileInfo.filename) + "\n", LogType::FAILURE);
		Logger::add(shader->getErrorMessage(), LogType::INFO);
	}
	else {
		Logger::add("Shader " + std::string(fileInfo.filename) + " compiled successfully!\n", LogType::SUCCESS);
	}

	for (ShaderProgramResource *res: shaderProgramResources) {
		res->requiresRelinking = true;
	}
}

bool ShaderResource::isOK(void) {
	return fileInfo.isOK() && !shader->errorOccured();
}

//=============================================
// Texture2DResouce class implementation
//=============================================

Texture2DResource::Texture2DResource(std::string& ident, unsigned width, unsigned height) :
	BaseResource(R_Texture2D),
	identifier(ident),
	texture(new Texture2D(width, height))
{ }

Texture2DResource::~Texture2DResource(void) {
	delete texture;
}

void Texture2DResource::tryUpdate(void) {
// do not need to update it for now, until we start loading textures from images...
}

bool Texture2DResource::isOK(void) {
	return texture->getID() != 0;
}

//=============================================
// ResourceManager class implementation
//=============================================
ResourceManager::ResourceManager() { }

ResourceManager::~ResourceManager(void) {
	//TODO clear lists!
}

ResourceManager& ResourceManager::getInstance() {
	static ResourceManager singleton;
	return singleton;
}

void ResourceManager::addShaderAsset(const char *filename, GLenum type, ShaderProgramResource *prog) {
	ShaderResource *res; 
	auto iterator = m_resourceList.find(std::string(filename));
	if (iterator == m_resourceList.end()) {
		res = new ShaderResource(filename, type);
		m_resourceList.insert(std::pair<std::string, BaseResource *>(std::string(filename), res));
	}
	else {
		res = (ShaderResource *)iterator->second; // uh-oh!
	}
	
	res->shaderProgramResources.push_back(prog);
	prog->program->add(res->shader);
}

void ResourceManager::addShaderProgramResource(const ASTNodeShaderProgram& progInfo) {
	ShaderProgramResource *resource = new ShaderProgramResource(progInfo.identifier);
	this->addShaderAsset(progInfo.vertShaderPath, GL_VERTEX_SHADER,   resource);
	this->addShaderAsset(progInfo.fragShaderPath, GL_FRAGMENT_SHADER, resource);
	m_resourceList.insert(std::pair<std::string, BaseResource *>(std::string(progInfo.identifier), resource));
}

void ResourceManager::addTextureResource(const ASTNodeTexture2D& info) {
	std::string ident(info.identifier);
	Texture2DResource *resource = new Texture2DResource(ident, info.width, info.height);
	std::pair<std::string, BaseResource *> entry(std::string(info.identifier), resource);
	m_resourceList.insert(entry);
}

void ResourceManager::tryUpdate(void) {
	for (auto it = m_resourceList.begin(); it != m_resourceList.end(); it++)
		it->second->tryUpdate();
}

bool ResourceManager::resourcesAreOK(void) {
	for (auto it = m_resourceList.begin(); it != m_resourceList.end(); it++) {
		if (!it->second->isOK()) {
			return false;
		}
	}
	return true;
}

void ResourceManager::removeAllResources(void) {
	for (auto it = m_resourceList.begin(); it != m_resourceList.end(); it++)
		delete it->second;
	m_resourceList.clear();
}

BaseResource * ResourceManager::lookupResource(std::string& ident) {
	auto it = m_resourceList.find(ident);
	if (it != m_resourceList.end()) {
		return it->second;
	}
	return nullptr;
}
