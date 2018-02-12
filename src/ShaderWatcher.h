#pragma once

#include "GL/glew.h"
#include <gl/GL.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <map>


class ShaderProgram;
class GLShader;
class ShaderProgramResource;

struct ShaderInfo {
#ifdef _WIN32
	HANDLE shaderHandle;
	FILETIME lastUpdateTime;
	const char *pathToShader;
#endif
	GLenum shaderType;
	ShaderProgram *shaderProgram;		
};

struct FileInfo {
	HANDLE  handle;
	FILETIME lastUpdateTime;
	const char *filename;
	FileInfo(const char *);

	bool isOK(void);
	~FileInfo(void);
};

class BaseResource {
public:
	virtual void tryUpdate(void) = 0;
	virtual bool isOK(void) = 0;
};

class ShaderResource: public BaseResource {
public: 
	FileInfo fileInfo;
	GLShader *shader;
	std::vector<ShaderProgramResource *> shaderProgramResources;

	ShaderResource(const char *, GLenum);

	void tryUpdate(void);
	bool isOK(void);
};

class ShaderProgramResource: public BaseResource {
public:
	const char *programIdentifier;
	ShaderProgram *program;
	bool requiresRelinking;

	ShaderProgramResource(const char *);
	~ShaderProgramResource(void);
	void tryUpdate(void);
	bool isOK(void);
};

// TODO this should not be here either, will be moved to parser module eventually.
// FIXME may potentially cause memleaks!
struct ASTNodeShaderProgram {
	const char *identifier;
	const char *vertShaderPath;	
	const char *fragShaderPath;
};


//=============================================
// ShaderWatcher class 
// Checks the last modified time of some file (shader) and reloads the shader 
// if necessary.
//=============================================
class ShaderWatcher {
private:
	std::map<std::string, BaseResource *> m_resourceList; 
	ShaderWatcher();
	~ShaderWatcher(void);
	void addShaderAsset(const char *, GLenum, ShaderProgramResource *);
public:
	static ShaderWatcher& getInstance();
	void addShaderProgramResource(const ASTNodeShaderProgram&);
	void tryUpdate(void);
	bool resourcesAreOK(void);
	void removeAllResources(void);
	BaseResource * lookupResource(std::string&);
};

std::string readTextFile(const char *);
