#include <cassert>
#include "Mesh.h"
#include "ShaderProgram.h"
#include <Windows.h>


//=============================================
// GLMesh class implementation 
//=============================================

GLMesh::GLMesh(GLfloat *vbobuf, GLuint *ebobuf, int vbobufLength, int ebobufLength) :
m_vbobuf(vbobuf),
m_ebobuf(ebobuf),
m_vbobufLength(vbobufLength),
m_ebobufLength(ebobufLength) {
	makeGLResource();
}

GLMesh::~GLMesh(void) { 
	delete[] m_vbobuf; 
	delete[] m_ebobuf; 
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ebo);
	glDeleteVertexArrays(1, &m_vao);
}

void GLMesh::makeGLResource(void) {
	//========================
	// create vao 
	//========================
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	//========================
	// bind mesh vertex data
	//========================
	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER,
				 m_vbobufLength * sizeof(float),
				 m_vbobuf, GL_STATIC_DRAW);

	//========================
	// bind mesh element data
	//========================
	glGenBuffers(1, &m_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				 m_ebobufLength * sizeof(GLuint),
				 m_ebobuf, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	assert(glGetError() == GL_NO_ERROR);
}

void GLMesh::setupAttributes(const GLShaderProgram& program) const {
	assert(glGetError() == GL_NO_ERROR);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBindVertexArray(m_vao);

	program.enableAttribPointer("position", ELEMENTS_PER_VERTEX, VERTEX_OFFSET, VERTEX_ATTRIB_STRIDE);
	program.enableAttribPointer("normal",   ELEMENTS_PER_NORMAL, NORMAL_OFFSET, VERTEX_ATTRIB_STRIDE);
	program.enableAttribPointer("texCoord", ELEMENTS_PER_TEXCOORD, TEXCOORD_OFFSET, VERTEX_ATTRIB_STRIDE);
	program.enableAttribPointer("tangent", ELEMENTS_PER_TANGENT, TANGENT_OFFSET, VERTEX_ATTRIB_STRIDE);
	program.enableAttribPointer("bitangent", ELEMENTS_PER_BITANGENT, BITANGENT_OFFSET, VERTEX_ATTRIB_STRIDE);

	assert(glGetError() == GL_NO_ERROR);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLMesh::bind(int n) const {
	glBindVertexArray((n == 0) ? 0 : m_vao);
}

void GLMesh::draw(void) const {
	glDrawElements(GL_TRIANGLES, m_ebobufLength, GL_UNSIGNED_INT, 0);
}

//=============================================
// Quad object builtin. 
//=============================================
const int QUAD_VBOLENGTH = VERTEX_ATTRIB_STRIDE * 4;

const GLfloat QUAD_VBO[QUAD_VBOLENGTH] = {
	-1.0f,  1.0f,  0.0f,  1.0f,  // top left position
	0.0f,  0.0f, -1.0f,  0.0f,  // top left normal
	0.0f,  1.0f,				// top left textcoord 
	1.0f,  0.0f,  0.0f,  0.0f,  // top left tangent 
	0.0f,  1.0f,  0.0f,  0.0f,  // top left bitanget 

	1.0f,  1.0f,  0.0f,  1.0f,  // top right position
	0.0f,  0.0f, -1.0f,  0.0f,  // top right normal
	1.0f,  1.0f,				// top right textcoord 
	1.0f,  0.0f,  0.0f,  0.0f,  // top right tangent 
	0.0f,  1.0f,  0.0f,  0.0f,  // top right bitanget 


	-1.0f, -1.0f,  0.0f,  1.0f, // bottom left position
	0.0f,  0.0f, -1.0f,  0.0f,  // bottom left normal
	0.0f,  0.0f,				// bottom left textcoord 
	1.0f,  0.0f,  0.0f,  0.0f,  // bottom left tangent 
	0.0f,  1.0f,  0.0f,  0.0f,  // bottom left bitanget 


	1.0f, -1.0f,  0.0f,  1.0f,  // bottom right position
	0.0f,  0.0f, -1.0f,  0.0f,  // bottom right normal
	1.0f,  0.0f,				// bottom right textcoord 
	1.0f,  0.0f,  0.0f,  0.0f,  // bottom right tangent 
	0.0f,  1.0f,  0.0f,  0.0f,  // bottom right bitang
};

const int QUAD_EBOLENGTH = 6;
const GLuint QUAD_EBO[QUAD_EBOLENGTH] = { 0, 1, 3, 0, 3, 2 };

GLMesh * createQuad(void) {
	GLfloat *vbo = new GLfloat[QUAD_VBOLENGTH];
	GLuint  *ebo = new  GLuint[QUAD_EBOLENGTH];
	CopyMemory(vbo, QUAD_VBO, QUAD_VBOLENGTH*sizeof(GLfloat));
	CopyMemory(ebo, QUAD_EBO, QUAD_EBOLENGTH*sizeof(GLuint));
	return new GLMesh(vbo, ebo, QUAD_VBOLENGTH, QUAD_EBOLENGTH);
}