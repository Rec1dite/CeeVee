#include "spinCube.h"

SpinCube::SpinCube()
{
	SpinCube(40, 0.01, 0);
}

SpinCube::SpinCube(float size, float rotSpeed, float perspectiveSkew)
{
	this->size = size;
	this->rot = 0;
	this->rotSpeed = rotSpeed;
	this->perspectiveSkew = perspectiveSkew;
	this->xDisplace = 0;
	this->yDisplace = 0;
}

void SpinCube::Update()
{
	rot += rotSpeed;
	for (int i = 0; i < 4; i++)
	{
		corns[i] = size * std::sin(rot + i*cv::PI/2);
		float dist = std::cos(rot + i * cv::PI / 2);
		cornhs[i] = cv::lerp(dist, -1, 1, size/2, size);
		cornskews[i] = cv::lerp(dist, -1, 1, 0, perspectiveSkew);
	}
}

void SpinCube::Draw(PixelGameEngine* ge)
{
	//Fill
	for (int i = 0; i < 4; i++)
	{
		ge->FillTriangle(GetPoint(ge, i, false), GetPoint(ge, (i + 1) % 4, false), GetPoint(ge, i, true), BLACK);
		ge->FillTriangle(GetPoint(ge, (i+1)%4, true), GetPoint(ge, i, true), GetPoint(ge, (i+1)%4, false), BLACK);
	}

	//Top edges
	for (int i = 0; i < 4; i++)
	{
		ge->DrawLine(GetPoint(ge, i, true), GetPoint(ge, (i+1)%4, true));
	}
	//Vertical edges
	for (int i = 0; i < 4; i++)
	{
		ge->DrawLine(GetPoint(ge, i, true), GetPoint(ge, i, false));
	}
	//Bottom edges
	for (int i = 0; i < 4; i++)
	{
		ge->DrawLine(GetPoint(ge, i, false), GetPoint(ge, (i+1)%4, false));
	}

}

vi2d SpinCube::GetPoint(PixelGameEngine*ge, int cornerIndex, bool onTop)
{
	return vi2d(ge->ScreenWidth() / 2 + corns[cornerIndex] + xDisplace, ge->ScreenHeight() / 2 + (onTop ? -1 : 1) * cornhs[cornerIndex] + cornskews[cornerIndex] + yDisplace);
}
