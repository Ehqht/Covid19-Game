#ifdef GL_ES
	#ifndef GL_FRAGMENT_PRECISION_HIGH	// highp may not be defined
		#define highp mediump
	#endif
	precision highp float; // default precision needs to be defined
#endif

// input from vertex shader
in vec3 norm;
in vec2 tc;

// the only output variable
out vec4 fragColor;

uniform sampler2D	TEX0;
uniform sampler2D	TEX1;
uniform sampler2D	TEX2;
uniform int frag_type;
uniform vec4 medicine_color;


void main()
{
	if (frag_type == 0)	fragColor = texture(TEX0, tc);
	else if (frag_type == 1)	fragColor = texture(TEX1, tc);
	else if (frag_type == 2) fragColor = texture(TEX2, tc);
	else	fragColor = medicine_color;
}
