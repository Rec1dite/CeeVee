#include "Ball.h"
#include <cmath>

Ball::Ball()
{
	Ball(0, 0, 0);
}

Ball::Ball(float x, float y, float r)
{
	this->x = x;
	this->y = y;
	this->rad = r;

	this->mass = 1;

	this->vx = 0;
	this->vy = 0;
	this->ax = 0;
	this->ay = 0;
}

void Ball::Update(float fps)
{
	vx += ax/fps;
	vy += ay/fps;

	x += vx;
	y += vy;

	rot += vrot;

	//Bounce
	if (y + rad > groundH)
	{
		y = groundH - rad;
		vy *= -bounciness;

		vx *= groundFriction;

		vrot += vx/rad;
		
		if (abs(vy) < 0.01 && y + rad < groundH + 1);
		{
			//vy = 0;
		}
	}
}