#pragma once
#ifndef __MEDICINE_H__
#define __MEDICINE_H__
#include "cgmath.h"

struct medicine_t
{
	vec3   center = vec3(0);      // 2D position for translation
	float   scale = 0.3f;      // radius
	vec4   color;
	mat4   model_matrix;      // modeling transformation
	
	// public functions
	void   update(float t);

};

inline medicine_t create_medicine()
{
	medicine_t m;
	float rdm_theta = (float)rand();
	float rdm_phi = (float)rand();
	float r = 15.0f + 3*0.2f;

	m = { vec3(r * sin(rdm_phi) * cos(rdm_theta), r * sin(rdm_phi) * sin(rdm_theta), r * cos(rdm_phi)), 0.3f, vec4(0, 1, 0, 1) };

	return m;
}




inline void medicine_t::update( float t)
{
	float c = cos(PI), s = sin(PI);


	mat4 scale_matrix =
	{
	   scale, 0, 0, 0,
	   0, scale, 0, 0,
	   0, 0, scale, 0,
	   0, 0, 0, 1
	};

	mat4 rotation_matrix =
	{
		c , -s, 0, 0,
		s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	mat4 translate_matrix =
	{
	   1, 0, 0, center.x,
	   0, 1, 0, center.y,
	   0, 0, 1, center.z,
	   0, 0, 0, 1
	};

	model_matrix = translate_matrix * scale_matrix * rotation_matrix;
}

#endif