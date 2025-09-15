#version 330
uniform sampler2D texture_diffuse;


in vec3 normal;
in vec3 pos;
in vec4 colour;
in vec2 texCoord;

layout (location=0) out vec4 fragColour;



void main(void) {

	//hard coded light
	vec3 ambient=vec3(0.3,0.3,0.3);
	vec3 diffuse=vec3(0.6,0.6,0.6);
	vec3 lightDir=normalize(vec3(0.0,10,10));
	float lamb=dot(lightDir,normalize(normal));
	vec4 tex = texture(texture_diffuse, texCoord);
	vec4 finalColour=vec4(diffuse*lamb*tex.rgb+ambient*tex.rgb,tex.a);
	fragColour = finalColour+colour;
	//fragColour = vec4(ambient,1.0);
	//	fragColour = tex;

}
