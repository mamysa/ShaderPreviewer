#pragma once 

#ifdef IS_WINDOWS
#include "GL/glew.h"
#include <GL/gl.h>
#endif

#ifdef IS_OSX
#include <OpenGL/gl3.h>
#endif

#include "utils/Math.h"
#include "string"

class GLShader {
private:
	GLuint m_shaderID;
	GLenum m_shaderType;
	bool m_errorOccured = true;
public:
	GLShader(GLenum);
	~GLShader(void);
	void compile(const char *);
	std::string getErrorMessage(void);
	GLuint getID(void) const;
	GLenum getType(void) const;
	bool errorOccured(void) const { return m_errorOccured; }
};

class GLShaderProgram {
private:
	GLuint m_programID;
	const GLShader *m_vertShader;
	const GLShader *m_geomShader;
	const GLShader *m_fragShader;
	bool m_errorOccured = false; 
	bool m_linked = false; //ensure shaders cannot be reattached without resetting the thing first.
public:
	GLShaderProgram(void);
	~GLShaderProgram(void); 
	void add(const GLShader *);
	void link(void);
	bool errorOccured(void) const { return m_errorOccured; }
	void use(int = 1) const;
	// various uniform setters
	void uniform(const char *, const Matrix3&) const;
	void uniform(const char *, const Vector3&) const;
	void uniform(const char *, float) const; 
	void uniform(const char *, int) const;
	void enableAttribPointer(const char *, GLint, GLint, GLsizei) const;
};
