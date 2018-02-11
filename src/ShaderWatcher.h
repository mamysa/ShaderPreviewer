#pragma once

#include "GL/glew.h"
#include <gl/GL.h>
#include <Windows.h>
#include <string>
#include <vector>


class ShaderProgram;

struct ShaderInfo {
#ifdef _WIN32
	HANDLE shaderHandle;
	FILETIME lastUpdateTime;
	const char *pathToShader;
#endif
	GLenum shaderType;
	ShaderProgram *shaderProgram;		
};

//=============================================
// ShaderWatcher class 
// Checks the last modified time of some file (shader) and reloads the shader 
// if necessary.
//=============================================
class ShaderWatcher {
private:
	std::vector<ShaderInfo> m_shaderList;
	ShaderWatcher();
	~ShaderWatcher(void);
public:
	static ShaderWatcher& getInstance();
#ifdef _WIN32
	bool add(const char *, ShaderProgram *, GLenum);
#endif
	void clear(void);
	bool tryUpdateAssets(void);
};

std::string readTextFile(const char *);
