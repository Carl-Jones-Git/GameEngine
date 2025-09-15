#version 330

uniform mat4 mvpMatrix;
uniform float tscale;
uniform float tyScale;

layout (location=0) in vec4 vertexPos;
layout (location=1) in vec3 vertexNormal;

out vec3 normal;
out vec3 pos;

void main(void) {
	pos=vertexPos.xyz;
	vec4 vPos = vec4(vertexPos.x * tscale, vertexPos.y * tyScale, vertexPos.z * tscale, 1.0);
	normal = vertexNormal;//normalize(vec3(vertexNormal.x,-vertexNormal.z*(0.3-tyScale*0.005),vertexNormal.y));
	gl_Position = mvpMatrix * vPos;
}
