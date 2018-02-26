#pragma once

#include "GL/glew.h"
#ifdef IS_WINDOWS
#include <gl/GL.h>
#include <windows.h>
#endif

#ifdef IS_OSX
#include <OpenGL/gl.h>
#endif

#include <string>
#include <vector>
#include <map>


class GLShaderProgram;
class GLShader;
class ShaderProgramResource;
class Texture2D;

enum ResourceType {
	R_Base,
	R_Shader,
	R_ShaderProgram, 
	R_Texture2D
};

struct FileInfo {
#ifdef IS_WINDOWS
	HANDLE  handle;
	FILETIME lastUpdateTime;
#endif
	const char *filename;
	FileInfo(const char *);

	bool isOK(void);
	~FileInfo(void);
};

class BaseResource {
private:
	ResourceType m_resourceType;
public:
	BaseResource(ResourceType type): m_resourceType(type) { }
	virtual void tryUpdate(void) = 0;
	virtual bool isOK(void) = 0;
	ResourceType getType(void) { return m_resourceType; }
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
	GLShaderProgram *program;
	bool requiresRelinking;

	ShaderProgramResource(const char *);
	~ShaderProgramResource(void);
	void tryUpdate(void);
	bool isOK(void);
};

class Texture2DResource: public BaseResource {
public:
	std::string identifier;
	Texture2D *texture;
	Texture2DResource(std::string&, unsigned, unsigned);
	~Texture2DResource(void);

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

//FIXME this should not be here either
struct ASTNodeTexture2D {
	const char *identifier;
	unsigned width;
	unsigned height;
};

//=============================================
// ResourceManager class 
// Checks the last modified time of some file (shader) and reloads the shader 
// if necessary.
//=============================================
class ResourceManager {
private:
	std::map<std::string, BaseResource *> m_resourceList; 
	ResourceManager();
	~ResourceManager(void);
	void addShaderAsset(const char *, GLenum, ShaderProgramResource *);
public:
	static ResourceManager& getInstance();
	void addShaderProgramResource(const ASTNodeShaderProgram&);
	void addTextureResource(const ASTNodeTexture2D&);
	void tryUpdate(void);
	bool resourcesAreOK(void);
	void removeAllResources(void);
	BaseResource * lookupResource(std::string&);

	// FIXME quick hack as I don't feel like implementing the iterator for this atm
	const std::map<std::string, BaseResource *> getResources(void) { return m_resourceList; }

};

std::string readTextFile(const char *);
