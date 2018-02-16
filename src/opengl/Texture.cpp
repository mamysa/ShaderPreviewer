#include "Texture.h"
#include "GL/glew.h"
#include <GL/gl.h>
#include <cassert>


Texture2D::Texture2D(unsigned width, unsigned height) : 
m_textureID(0),
m_width(width),
m_height(height) {
	glGenTextures(1, &m_textureID);
	assert(m_textureID != 0);
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::~Texture2D(void) {
	glDeleteTextures(1, &m_textureID);
}

GLuint Texture2D::getID(void) const { return m_textureID; }

void Texture2D::bind(int n) const {
	glBindTexture(GL_TEXTURE_2D, (n == 0) ? 0 : m_textureID);
}