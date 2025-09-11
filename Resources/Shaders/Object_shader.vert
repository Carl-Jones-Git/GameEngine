#version 330

uniform mat4 model;
uniform mat4 viewProjection;



layout (location=0) in vec3 vertexPos;
layout (location=1) in vec3 vertexNormal;
layout (location=2) in vec2 vertexTexCoord;

out vec3 normal;
out vec2 texCoord;

void main(void) 
{
	texCoord = vertexTexCoord;
	normal= (transpose(inverse(model)) *vec4( normalize(vertexNormal),1.0)).xyz;
	gl_Position = viewProjection * model * vec4(vertexPos, 1.0);
}
