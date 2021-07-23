#pragma once

class Ball
{
public:
	float x, y, rad;
	float vx, vy, ax, ay;
	float mass;

	float groundH;
	float groundFriction;
	float bounciness;

	float rot, vrot;
public:
	Ball();
	Ball(float, float, float);
	void Update(float);
};
