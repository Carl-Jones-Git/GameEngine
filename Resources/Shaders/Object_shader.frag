#version 330
uniform float selected;
uniform sampler2D texture_diffuse;

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

	vec4 colour = texture(texture_diffuse, texCoord);
	colour.rgb=colour.rgb*diffuse*lamb+colour.rgb*ambient;
	if (selected>0.5f)
	colour*=1.5;
	fragColour =colour;
}
