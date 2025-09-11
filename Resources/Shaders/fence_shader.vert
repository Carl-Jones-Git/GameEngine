#version 330

uniform mat4 mvpMatrix;
uniform float tscale;
uniform float tyScale;

layout (location=0) in vec4 vertexPos;
layout (location=1) in vec3 vertexNormal;
layout (location=2) in vec4 vertexColour;
layout (location=3) in vec2 tex;

out vec3 normal;
out vec3 pos;
out vec4 colour;
out vec2 texCoord;

void main(void) {
	pos=vertexPos.xyz;
	texCoord.x=tex.x;
	texCoord.y=tex.y;
	vec4 vPos = vec4(vertexPos.x * tscale, vertexPos.y * tyScale, vertexPos.z * tscale, 1.0);
	normal = vertexNormal;//normalize(vec3(vertexNormal.x,-vertexNormal.z*(0.3-tyScale*0.005),vertexNormal.y));
	colour=vertexColour;
	gl_Position = mvpMatrix * vPos;
}
