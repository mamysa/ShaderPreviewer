#include <cassert>
#include <iostream>
#include <fstream>
#include "ShaderWatcher.h"
#include "opengl/ShaderProgram.h"


#ifdef _WIN32
static HANDLE initializeFileHandle(const char *path) {
	HANDLE handle = CreateFile( path, 
		GENERIC_READ,  
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL );
	assert(handle != INVALID_HANDLE_VALUE); 
	return handle;
}
#endif

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
// ShaderWatcher class implementation
//=============================================
ShaderWatcher::~ShaderWatcher(void) {
	//TODO clear lists!
}


#ifdef _WIN32
bool ShaderWatcher::add(const char *filepath, ShaderProgram *program, GLenum type) {
	ShaderInfo shaderInfo; 
	shaderInfo.pathToShader = filepath;
	shaderInfo.shaderProgram = program;
	shaderInfo.shaderHandle = initializeFileHandle(filepath);
	shaderInfo.shaderType = type;
	bool b = GetFileTime(shaderInfo.shaderHandle, NULL, NULL, &shaderInfo.lastUpdateTime);
	assert(b);

	m_shaderList.push_back(shaderInfo);
	return true;
}
#endif


void ShaderWatcher::clear(void) {
	m_shaderList.clear();
}

bool ShaderWatcher::watch(void) {
	FILETIME currentTime;
	for (auto& n: m_shaderList) {
		bool b = GetFileTime(n.shaderHandle, NULL, NULL, &currentTime);
		if (currentTime.dwHighDateTime != n.lastUpdateTime.dwHighDateTime || 
			currentTime.dwLowDateTime  != n.lastUpdateTime.dwLowDateTime) {
			std::cout << "Updating " << n.pathToShader << "...\n";
			n.lastUpdateTime = currentTime;
			std::string shadersrc = readTextFile(n.pathToShader);
			bool a = n.shaderProgram->addShader(shadersrc.c_str(), n.shaderType);
			bool b = n.shaderProgram->link();
			if (!a||!b) {
				std::cout << "Error updating " << n.pathToShader << "\n";
				return false;
			}
		}

		if (n.shaderProgram->errorOccured()) { 
			return false;
		}
	}

	assert(glGetError() == GL_NO_ERROR);
	return true;
}

