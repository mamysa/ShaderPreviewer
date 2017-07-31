#include "ShaderProgram.h"
#include <iostream>
#define ERRORMESG_BUF_LENGTH 1024
#include "utils/Math.h"
#include <cassert>

ShaderProgram::ShaderProgram(void) :
m_vertshaderID(0),
m_geomshaderID(0),
m_fragshaderID(0),
m_programID(0) { }

ShaderProgram::~ShaderProgram(void) {
	if (m_programID) { glDeleteProgram(m_programID); }
	if (m_vertshaderID) { glDeleteShader(m_vertshaderID); }
	if (m_geomshaderID) { glDeleteShader(m_geomshaderID); }
	if (m_fragshaderID) { glDeleteShader(m_fragshaderID); }
}

bool ShaderProgram::addShader(const char *source, GLenum type) { 
	GLuint *shaderID = nullptr;
	if (type == GL_VERTEX_SHADER)   { shaderID = &m_vertshaderID; }
	if (type == GL_GEOMETRY_SHADER) { shaderID = &m_geomshaderID; }
	if (type == GL_FRAGMENT_SHADER) { shaderID = &m_fragshaderID; }
	assert(shaderID != nullptr);

	*shaderID = (*shaderID == 0) ? glCreateShader(type) : *shaderID;
	glShaderSource(*shaderID, 1, &source, NULL);
	glCompileShader(*shaderID);

	// check status 
	GLint status;
	glGetShaderiv(*shaderID, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
#if 0
		char log_string[ERRORMESG_BUF_LENGTH];
		glGetShaderInfoLog(*shaderID, ERRORMESG_BUF_LENGTH, 0, log_string);
		std::cout << "Shader Error: \n" << log_string << "\n";
#endif
		return false;
	}


	return true;
}

bool ShaderProgram::link(void) {
	if (m_programID == 0) {
		m_programID = glCreateProgram();
		if (m_vertshaderID != 0) { glAttachShader(m_programID, m_vertshaderID); }
		if (m_geomshaderID != 0) { glAttachShader(m_programID, m_geomshaderID); }
		if (m_fragshaderID != 0) { glAttachShader(m_programID, m_fragshaderID); }
	}
	glLinkProgram(m_programID);

	GLint status;
	glGetProgramiv(m_programID, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char log_string[ERRORMESG_BUF_LENGTH];
		glGetProgramInfoLog(m_programID, ERRORMESG_BUF_LENGTH, 0, log_string);
		std::cout << log_string << "\n\n";
		m_errorOccured = true;
		return false;
	}

	m_errorOccured = false;
	return true;
}

void ShaderProgram::use(int id) const {
	GLuint p = (id == 0) ? 0 : m_programID;
	glUseProgram(p);
}

void ShaderProgram::uniform(const char *str, const Matrix3& n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	glProgramUniformMatrix3fv(m_programID, uniform, 1, GL_FALSE, n.getBuffer());
}

void ShaderProgram::uniform(const char *str, const Vector3& n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	float buf[3] = {n.x, n.y, n.z};
	glProgramUniform3fv(m_programID, uniform, 1, buf);
}

void ShaderProgram::uniform(const char *str, float n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	glProgramUniform1f(m_programID, uniform, n);
}

void ShaderProgram::uniform(const char *str, int n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	glProgramUniform1i(m_programID, uniform, n);
}

void ShaderProgram::enableAttribPointer(const char *str, GLint size, GLint offset, GLsizei stride) const {
	GLint attrib = glGetAttribLocation(m_programID, str);
	if (attrib != -1) {
		glVertexAttribPointer(attrib,size,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*stride,(GLvoid *)(sizeof(GLfloat)*offset));
		glEnableVertexAttribArray(attrib);
	}
}