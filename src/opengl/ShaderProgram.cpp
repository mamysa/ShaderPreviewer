#include "ShaderProgram.h"
#include <iostream>
#define ERRORMESG_BUF_LENGTH 1024
#include "utils/Math.h"
#include <cassert>

//=============================================
// GLShader class implementation
//=============================================
GLShader::GLShader(GLenum shaderType) :
	m_shaderID(0),
	m_shaderType(shaderType) { }

GLShader::~GLShader(void) {
	if (m_shaderID)  
		glDeleteShader(m_shaderID);
}

void GLShader::compile(const char *src) {
	m_shaderID = (m_shaderID == 0) ? glCreateShader(m_shaderType) : m_shaderID;
	glShaderSource(m_shaderID, 1, &src, NULL);
	glCompileShader(m_shaderID);

	m_errorOccured = false;
	GLint status; 
	glGetShaderiv(m_shaderID, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		m_errorOccured = true;
	}
}

std::string GLShader::getErrorMessage(void) {
	if (m_errorOccured) {
		char log_string[ERRORMESG_BUF_LENGTH];
		glGetShaderInfoLog(m_shaderID, ERRORMESG_BUF_LENGTH, 0, log_string);
		return std::string(log_string);
	}
	
	return "";
}

GLuint GLShader::getID(void) const {
	return m_shaderID;
}

GLenum GLShader::getType(void) const {
	return m_shaderType;
}

//=============================================
// GLShaderProgram class implementation
//=============================================
GLShaderProgram::GLShaderProgram(void) :
m_programID(0),
m_vertShader(nullptr),
m_geomShader(nullptr),
m_fragShader(nullptr) { }

GLShaderProgram::~GLShaderProgram(void) {
	if (m_programID)
		glDeleteProgram(m_programID); 
}

void GLShaderProgram::add(const GLShader *shader) { 
	if (m_linked) {
		std::cout << "Attempting to modify immutable shader\n";
		return;
	}

	GLenum type = shader->getType();
	if (type == GL_VERTEX_SHADER)   { m_vertShader = shader; }
	if (type == GL_GEOMETRY_SHADER) { m_geomShader = shader; }
	if (type == GL_FRAGMENT_SHADER) { m_fragShader = shader; }
}

void GLShaderProgram::link(void) {
	if (m_programID == 0) {
		m_programID = glCreateProgram();
		if (m_vertShader) { glAttachShader(m_programID, m_vertShader->getID()); }
		if (m_geomShader) { glAttachShader(m_programID, m_geomShader->getID()); }
		if (m_fragShader) { glAttachShader(m_programID, m_fragShader->getID()); }
	}

	glLinkProgram(m_programID);
	m_errorOccured = false;
	m_linked = true;

	GLint status;
	glGetProgramiv(m_programID, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		m_errorOccured = true;
	}
}

void GLShaderProgram::use(int id) const {
	GLuint p = (id == 0) ? 0 : m_programID;
	glUseProgram(p);
}

void GLShaderProgram::uniform(const char *str, const Matrix3& n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	glProgramUniformMatrix3fv(m_programID, uniform, 1, GL_FALSE, n.getBuffer());
}

void GLShaderProgram::uniform(const char *str, const Vector3& n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	float buf[3] = {n.x, n.y, n.z};
	glProgramUniform3fv(m_programID, uniform, 1, buf);
}

void GLShaderProgram::uniform(const char *str, float n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	glProgramUniform1f(m_programID, uniform, n);
}

void GLShaderProgram::uniform(const char *str, int n) const {
	GLuint uniform = glGetUniformLocation(m_programID, str);
	glProgramUniform1i(m_programID, uniform, n);
}

void GLShaderProgram::enableAttribPointer(const char *str, GLint size, GLint offset, GLsizei stride) const {
	GLint attrib = glGetAttribLocation(m_programID, str);
	if (attrib != -1) {
		glVertexAttribPointer(attrib,size,GL_FLOAT,GL_FALSE,sizeof(GLfloat)*stride,(GLvoid *)(sizeof(GLfloat)*offset));
		glEnableVertexAttribArray(attrib);
	}
}