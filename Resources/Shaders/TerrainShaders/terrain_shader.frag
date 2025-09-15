#version 330

in vec3 normal;
in vec3 pos;

layout (location=0) out vec4 fragColour;

void main(void) {

	vec3 ambient=vec3(0.1,0.1,0.3);
	vec3 diffuse=vec3(1.0,1.0,0.9);
	vec3 lightDir=normalize(vec3(0.0,10,10));
	float lamb=dot(lightDir,normal);

	vec3 snow_colour=vec3(1.0,1.0,0.9);
	vec3 ground_colour=vec3(0.4,0.3,0.0)*pos.y;
	
	vec3 colour=snow_colour;
	if(normal.y<0.5)
		colour=ground_colour;
	else if(normal.y>=0.5 && normal.y<=0.6)
		colour=mix(ground_colour,snow_colour,(normal.y-0.5)*10.0);

	colour=colour*diffuse*lamb+colour*ambient;

	//fragColour = vec4(lamb,lamb,lamb, 1.0);
	fragColour = vec4(colour, 1.0);
	//fragColour = vec4(normalize(normal), 1.0);
	//fragColour = vec4(pos.y,pos.y,pos.y, 1.0);
}
