#pragma once

#ifdef IS_WINDOWS
#include "GL/glew.h"
#include <GL/gl.h>
#endif

#ifdef IS_OSX
#include <OpenGL/gl3.h>
#endif

class Texture2D {
private:
	GLuint m_textureID;
	GLuint m_width;
	GLuint m_height;
public:
	Texture2D(unsigned, unsigned);
	~Texture2D(void);
	void bind(int=1) const;
	GLuint getID(void) const; 
	GLuint getW(void) const { return m_width; }
	GLuint getH(void) const { return m_height; }
};
