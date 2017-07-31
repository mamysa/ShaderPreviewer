#pragma once 
#include "GL/glew.h"
#include <GL/gl.h>
#include "utils/Math.h"

class ShaderProgram {
private:
	GLuint m_vertshaderID;
	GLuint m_geomshaderID;
	GLuint m_fragshaderID;
	GLuint m_programID;
	bool m_errorOccured = false; 
public:
	ShaderProgram(void);
	~ShaderProgram(void); 
	bool addShader(const char *, GLenum);
	bool link(void);
	bool errorOccured(void) const { return m_errorOccured; }
	void use(int = 1) const;
	// various uniform setters
	void uniform(const char *, const Matrix3&) const;
	void uniform(const char *, const Vector3&) const;
	void uniform(const char *, float) const; 
	void uniform(const char *, int) const;
	void enableAttribPointer(const char *, GLint, GLint, GLsizei) const;
};
