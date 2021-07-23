#pragma once
#include "olcPixelGameEngine.h"
#include "constants.h"
#include "spinCube.h"
#include "particle.h"

using namespace olc;
using namespace cv;

namespace titleScreen
{

    int frameCount;
    float speed = 1;
    int halfWidth, halfHeight;
    int skyGradientSteps = 10;
    SpinCube titleCube;

    bool cubeClicked = false;
    float welcomeTextTransparency = 1.0;

    //const int numParticles = 100;
    //Particle parts[numParticles];

    bool Create(PixelGameEngine* ge)
    {

        frameCount = 0;
        halfWidth = ge->ScreenWidth() / 2;
        halfHeight = ge->ScreenHeight() / 2;

        titleCube = SpinCube(20, 0.02, -20);

        /*
        for (int i = 0; i < numParticles; i++)
        {
            parts[i] = { halfWidth, rand()%halfHeight };
        }
        */

        return true;
    }

    bool Update(PixelGameEngine* ge)
    {
        //INPUT
        float mouseX = ge->GetMouseX();
		float mouseY = ge->GetMouseY();
		float mouseDX = halfWidth - mouseX;
        float mouseDY = halfHeight - mouseY;

        if (ge->GetMouse(0).bReleased)
        {
            if (mouseX < halfWidth + titleCube.size && mouseX > halfWidth - titleCube.size &&
                mouseY < halfHeight + titleCube.size && mouseY > halfHeight - titleCube.size)
            {
                cubeClicked = true;
            }
        }

        //UPDATE
        if (cubeClicked)
        {
            if (titleCube.size < 500)
            {
				titleCube.size *= 1.1;
				welcomeTextTransparency *= 0.9;
            }
        }
        else
        {
            float a = (mouseDX * mouseDX + mouseDY * mouseDY);
            titleCube.size = 10/(0.001*a+1) + 20;
        }
        titleCube.perspectiveSkew = mouseDY;
        titleCube.perspectiveSkew *= 0.1;
        titleCube.xDisplace = 10*(1-mouseX/halfWidth);
        titleCube.Update();

        //UpdateParticles(parts, numParticles, halfWidth + titleCube.xDisplace + rand()%51 - 25, halfHeight + titleCube.yDisplace + rand()%51 - 50);

        //DRAWING
        ge->Clear(c_DarkBackground);

        //Sky
        for (float f = 0; f < 1.0; f += 1.0 / skyGradientSteps)
        {
            ge->FillRect(0, f * (halfHeight), ge->ScreenWidth(), ge->ScreenHeight() / skyGradientSteps, Pixel(255 * (1 - f), 100 * (1 - f), 100 * (1 - f) + 50));
        }

        //Ground perspective
        for (int i = -32; i <= 32; i++)
        {
            ge->DrawLine(halfWidth, halfHeight, halfWidth + 64 * i, ge->ScreenHeight(), c_MagentaPink * ((32 - abs(i)) / 32.0));
        }
        //Ground horizontal
        for (float f = 64 + fmod(frameCount * speed, 64.0); f > 0.1; f /= 2.0)
        {
            ge->DrawLine(0, ge->ScreenHeight() / 2 + f, ge->ScreenWidth(), ge->ScreenHeight() / 2 + f, c_MagentaPink);
        }

        //Particles
        //DrawParticles(ge, parts, numParticles, BLACK, false);
        //DrawParticles(ge, parts, numParticles, WHITE, true);

        //Spin Cube
        titleCube.Draw(ge);

        //Title
        ge->SetPixelMode(Pixel::ALPHA);
        ge->DrawString(ge->ScreenWidth() / 5, 3 * ge->ScreenHeight() / 4, "WELCOME", RED * welcomeTextTransparency, 3);
        ge->DrawString(ge->ScreenWidth() / 5 - 1, 3 * ge->ScreenHeight() / 4 - 1, "WELCOME", WHITE * welcomeTextTransparency, 3);
        ge->SetPixelMode(Pixel::NORMAL);

        frameCount++;
        return true;
    }

}
