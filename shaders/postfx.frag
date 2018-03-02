#version 410

in vec2 fragTexCoord;
out vec4 outColor;

uniform sampler2D u_texture1;

void main() {
	outColor = texture(u_texture1, fragTexCoord);
}

