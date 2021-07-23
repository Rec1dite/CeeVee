#pragma once
#include "olcPixelGameEngine.h"
#include <random>

using namespace olc;

struct Particle
{
	int posx, posy;
};

//Iterates through all particles in the array, updating their positions
void UpdateParticles(Particle* parts, int size, int spawnX, int spawnY)
{
	for (int i = 0; i < size; i++)
	{
		parts[i].posx += rand() % 3 - 1;
		parts[i].posy -= rand() % 2;

		if (parts[i].posy < 0) //Reset particle to spawn if off-screen
		{
			parts[i].posx = spawnX;
			parts[i].posy = spawnY;
		}
	};
}

void DrawParticles(PixelGameEngine* ge, Particle* parts, int size, Pixel color, bool inverted)
{
	if (inverted)
	{
		int screenHeight = ge->ScreenHeight();
		int screenWidth = ge->ScreenWidth();

		for (int i = 0; i < size; i++)
		{
			ge->Draw(screenWidth-parts[i].posx, screenHeight-parts[i].posy, color);
		}
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			ge->Draw(parts[i].posx, parts[i].posy, color);
		}
	}
}
