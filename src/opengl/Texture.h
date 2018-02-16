#pragma once
#include "GL/glew.h"
#include <GL/gl.h>

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
};
