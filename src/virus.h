#pragma once
#ifndef __VIRUS_H__
#define __VIRUS_H__
#include "cgmath.h"

struct virus_t
{
	vec3	center = vec3(0);		
	float	scale = 1.0f;		// 사실상 반지름
	float	theta = 0.0f;			
	//vec4	color;				
	mat4	model_matrix;		

	// public functions
	void	update(float t);
};
/* 
inline std::vector<virus_t> create_viruses()
{
	std::vector<virus_t> viruses;
	virus_t vi;
	for (int i = 0;i < 10;i++) {
		float rdm_theta = (float)rand();
		float rdm_phi = (float)rand();
		float rdm_r = 0.01f + (rand() / (float)RAND_MAX) * 0.22f;	//radius range: 0.01 ~ 0.23
		//float rdm_r = 0.2f;
		float r = 15.0f + 5*rdm_r;

		vi = { vec3(r*sin(rdm_phi)*cos(rdm_theta), r*sin(rdm_phi)*sin(rdm_theta), r*cos(rdm_phi)), rdm_r, 0.0f};
		viruses.emplace_back(vi);
	}

	return viruses;
}*/

inline virus_t add_viruses(float player_size) {	//주인공 크기에 비례한 바이러스 크기
	virus_t vi;
	float a = 0.0f;
	if (player_size > 0.3f)
		a = 0.2f;
	else if (player_size > 0.6f)
		a = 0.6f;	
	else if (player_size > 0.9f)
		a = 1.0f;

		float rdm_theta = (float)rand();
		float rdm_phi = (float)rand();
		float rdm_r = a + 0.01f + (rand() / (float)RAND_MAX) * 0.22f;	//radius range: 0.01 ~ 0.23
		float r = 15.0f + 5 * rdm_r;
		
		vi = { vec3(r * sin(rdm_phi) * cos(rdm_theta), r * sin(rdm_phi) * sin(rdm_theta), r * cos(rdm_phi)), rdm_r, 0.0f };

		return vi;
}
inline void virus_t::update(float t)
{
	//theta = t * 0.01f;
	//float c = cos(theta), s = sin(theta);

	mat4 scale_matrix =
	{
		scale, 0, 0, 0,
		0, scale, 0, 0,
		0, 0, scale, 0,
		0, 0, 0, 1
	};


	mat4 translate_matrix =
	{
		1, 0, 0, center.x,
		0, 1, 0, center.y,
		0, 0, 1, center.z,
		0, 0, 0, 1
	};


	model_matrix =  translate_matrix * scale_matrix;
}

#endif
