#pragma once
#ifdef IS_WINDOWS
#include "GL/glew.h"
#include <GL/gl.h>
#include <windows.h>
#endif

#ifdef IS_OSX
#include <OpenGL/gl3.h>
#endif

class GLShaderProgram;

//=============================================
// Mesh layout constants 
//=============================================
static const int ELEMENTS_PER_VERTEX    = 4;
static const int ELEMENTS_PER_NORMAL    = 4;
static const int ELEMENTS_PER_TANGENT   = 4;
static const int ELEMENTS_PER_BITANGENT = 4;
static const int ELEMENTS_PER_TEXCOORD  = 2;
static const int VERTEX_ATTRIB_STRIDE   = ELEMENTS_PER_VERTEX +
										  ELEMENTS_PER_NORMAL    + 
										  ELEMENTS_PER_TANGENT   + 
										  ELEMENTS_PER_BITANGENT +
										  ELEMENTS_PER_TEXCOORD;
static const int    VERTEX_OFFSET = 0;
static const int    NORMAL_OFFSET = 4;
static const int  TEXCOORD_OFFSET = 8;
static const int   TANGENT_OFFSET = 10;
static const int BITANGENT_OFFSET = 14;

//=============================================
// GLMesh class 
//=============================================
class GLMesh {
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ebo;
	GLfloat *m_vbobuf = nullptr;
	GLuint  *m_ebobuf = nullptr;
	int m_vbobufLength;
	int m_ebobufLength;
	void makeGLResource(void);
public:
	GLMesh(GLfloat *, GLuint *, int, int);
	~GLMesh(void);
	void setupAttributes(const GLShaderProgram&) const;
	void draw(void) const;
	void bind(int=1) const;
};

GLMesh * createQuad(void); 
