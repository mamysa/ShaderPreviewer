#version 450
in vec4 position; 
in vec2 texCoord;
out vec2 fragTexCoord;

void main(void) { 
    gl_Position = position; 
    fragTexCoord = texCoord; 
}