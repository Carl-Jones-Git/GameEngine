#version 330

uniform sampler2D texture_diffuse;
uniform vec4 colour = vec4(1.0,0.0,0.0,1.0);
in vec3 normal;
in vec2 texCoord;

layout (location=0) out vec4 fragColour;

void main(void) 
{
	//hard coded light
	vec3 ambient=vec3(0.3,0.3,0.4);
	vec3 diffuse=vec3(1.0,1.0,0.9);
	vec3 lightDir=normalize(vec3(0.0,10,10));
	float lamb=dot(lightDir,normalize(normal));
	fragColour=vec4(colour.rgb*diffuse*lamb+colour.rgb*ambient,colour.a);
}
