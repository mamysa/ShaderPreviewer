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
		char log_string[ERRORMESG_BUF_LENGTH];
		glGetShaderInfoLog(m_shaderID, ERRORMESG_BUF_LENGTH, 0, log_string);
		std::cout << "Shader Error: \n" << log_string << "\n";
		m_errorOccured = true;
	}
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
ShaderProgram::ShaderProgram(void) :
m_programID(0),
m_vertShader(nullptr),
m_geomShader(nullptr),
m_fragShader(nullptr) { }

ShaderProgram::~ShaderProgram(void) {
	if (m_programID)
		glDeleteProgram(m_programID); 
}

void ShaderProgram::add(const GLShader *shader) { 
	GLenum type = shader->getType();
	if (type == GL_VERTEX_SHADER   && !m_vertShader) { m_vertShader = shader; }
	if (type == GL_GEOMETRY_SHADER && !m_geomShader) { m_geomShader = shader; }
	if (type == GL_FRAGMENT_SHADER && !m_fragShader) { m_fragShader = shader; }
}

void ShaderProgram::link(void) {
	if (m_programID == 0) {
		m_programID = glCreateProgram();
		if (m_vertShader) { glAttachShader(m_programID, m_vertShader->getID()); }
		if (m_geomShader) { glAttachShader(m_programID, m_geomShader->getID()); }
		if (m_fragShader) { glAttachShader(m_programID, m_fragShader->getID()); }
	}

	glLinkProgram(m_programID);
	m_errorOccured = false;

	GLint status;
	glGetProgramiv(m_programID, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		m_errorOccured = true;
	}
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