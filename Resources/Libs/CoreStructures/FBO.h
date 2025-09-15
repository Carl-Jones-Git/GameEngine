#ifndef FBO_H
#define FBO_H

#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>


class FBO{

private:

	//
	// Framebuffer Object (FBO) variables
	//
	// Actual FBO
	GLuint							demoFBO;
	// Colour texture to render into
	GLuint							fboColourTexture;
	// Depth texture to render into
	GLuint							fboDepthTexture;
	// Flag to indicate that the FBO is valid
	bool							fboOkay;
	GLuint width;
	GLuint height;
	//
	// Private API
	//

	//void loadShader();
	void setupFBO();


	//
	// Public API
	//

public:
	FBO(GLuint width=1024,GLuint height=1024);

	~FBO();
	GLint getFBO();
	bool getFBOOK();

	GLuint getTexture() {

		return fboColourTexture;
	}
};

#endif