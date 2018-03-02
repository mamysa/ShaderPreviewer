#version 410

in vec2 fragTexCoord;
out vec4 outColor;
uniform vec3 resolution;
uniform float iGlobalTime;

uniform mat3 viewMatrix;
uniform mat3 viewPosition;


void main(void) {
	outColor = vec4(clamp(fragTexCoord, 0.0, 1.0), 0.0, 1.0);	
}


