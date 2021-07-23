#pragma once

#include "olcPixelGameEngine.h"
#include "constants.h"
#include <cmath>

using namespace olc;

class SpinCube
{
private:
	float rot;
	float corns[4];
	float cornhs[4];
	float cornskews[4];

public:
	float rotSpeed;
	float perspectiveSkew;
	float size;
	float xDisplace, yDisplace;

	SpinCube();
	SpinCube(float, float, float);
	void Update();
	void Draw(PixelGameEngine*);
	vi2d GetPoint(PixelGameEngine*, int, bool);
};
