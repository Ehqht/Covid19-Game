#pragma once
#ifndef __SPHERE_H__
#define __SPHERE_H__
#include "cgmath.h"

struct sphere_t
{
	vec3	center = vec3(0);		// 2D position for translation
	float	radius = 1.0f;		// radius
	float	theta = 0.0f;			// revolution rotation angle
	vec4	color;				// RGBA color in [0,1]
	mat4	model_matrix;		// modeling transformation

	// public functions
	void	update(float t);
};

inline sphere_t create_sphere()
{
	
	sphere_t s;
	
	s = { vec3(0,0,0),15.0f, 0.0f, vec4(1.0f,0.5f,0.5f,1.0f) };
	
	return s;
}

inline void sphere_t::update(float t)
{
	theta = t * 0.0f;

	float c = cos(theta), s = sin(theta);

	mat4 scale_matrix =
	{
		radius, 0, 0, 0,
		0, radius, 0, 0,
		0, 0, radius, 0,
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


	model_matrix =  translate_matrix * scale_matrix * rotation_matrix;
}

#endif
