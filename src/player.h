#pragma once
#ifndef __PLAYER_H__
#define __PLAYER_H__
#include "cgmath.h"

struct movement {
	bool right = false;
	bool left = false;
	bool up = false;
	bool down = false;
};

struct player_t
{
	vec3   center = vec3(0);      // 2D position for translation
	float   scale = 0.3f;      // radius
	float   theta = 0.0f;
	float	phi = 0.0f;

	float	r = 15.0f + 3 * scale;
	mat4   model_matrix;      // modeling transformation

	int score = 0;

	float minus = 1.0f;
	bool upsidedown = false;
	movement mv;	// to move the player
	// public functions
	void   update(movement mv, float t);
	int		collision(std::vector<virus_t>& viruses, std::vector<medicine_t>& medicines);
};

inline player_t create_player()
{
	player_t p;

	float theta = 0.0f;
	float phi = 1.0f;
	p = { vec3(p.r * sin(phi) * cos(theta), p.r * sin(phi) * sin(theta), p.r * cos(phi)), 0.15f, theta, phi };

	return p;
}

inline int player_t::collision(std::vector<virus_t>& viruses, std::vector<medicine_t>& medicines) {

	for (std::vector<virus_t>::iterator it = viruses.begin();it != viruses.end();it++) {
		float dist = (center - it->center).length();
		if (dist <= 3.0f * scale + it->scale * 5.0f) { // 충돌
			if (scale <= it->scale)
			{
				//printf("game over\n\n");
				return 3;	//그냥 종료하는거 말고 멈추는 방법 없나?
			}// game over

			scale += it->scale * 0.1f;	//더하기로 하는게 맞나?
			r = 15.0f + 3 * scale;
			it = viruses.erase(it);	// 잡아먹힌 바이러스 삭제욤
			score++;
			return 1;
		}
	}

	for (std::vector<medicine_t>::iterator it = medicines.begin();it != medicines.end();it++) {
		float dist = (center - it->center).length();
		if (dist <= 3.0f * scale + it->scale * 5.0f) { // 충돌
			it = medicines.erase(it);	// 잡아먹힌 바이러스 삭제욤
			return 4;
		}
	}

	return 0;
}

inline void player_t::update(movement mv, float t)
{
	float c = cos(PI), s = sin(PI);

	if (mv.right == true) {
		theta += 0.001f * minus;
	}
	if (mv.left == true) {
		theta -= 0.001f * minus;
	}
	if (mv.up == true) {
		phi -= 0.001f * minus;
	}
	if (mv.down == true) {
		phi += 0.001f * minus;
	}

	if (phi <= 0) {
		theta = PI + theta;
		phi = -phi;	//부호 바꿔줌
		minus = -minus;
		upsidedown = !upsidedown;
	}
	else if (phi >= PI) {
		theta = PI + theta;
		phi = 2 * PI - phi;
		minus = -minus;
		upsidedown = !upsidedown;
	}

	if (theta >= 2 * PI) {
		theta = theta - 2 * PI;
	}
	else if (theta <= 0) {
		theta = 2 * PI - theta;
	}

	center = vec3(r * sin(phi) * cos(theta), r * sin(phi) * sin(theta), r * cos(phi));

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

	model_matrix = translate_matrix * scale_matrix;
}

#endif