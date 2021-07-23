#include "shadowCast.h"

bool ShadowCaster::Create(PixelGameEngine* ge)
{
	world = new Cell[worldW * worldH];
	this->ge = ge;

	return true;
}

bool ShadowCaster::Update()
{
	float blockWidth = 4.0f;

	//INPUT
	if (ge->GetMouse(0).bReleased)
	{
		int i = CursorToBlock(ge->GetMouseX(), ge->GetMouseY(), blockWidth);
		world[i].exist = !world[i].exist;
	}

	//DISPLAY
	ge->Clear(olc::BLACK);

	//Draw blocks
	for (int x = 0; x < worldW; x++)
	{
		for (int y = 0; y < worldH; y++)
		{
			if (world[y * worldW + x].exist)
			{
				ge->FillRect(x * blockWidth, y * blockWidth, blockWidth, blockWidth, BLUE);
			}
		}
	}

	return true;
}

int ShadowCaster::CursorToBlock(float mouseX, float mouseY, float blockWidth)
{
	//i = y*width + x
	return ((int)mouseY / (int)blockWidth) * worldW + ((int)mouseX/(int)blockWidth);
}

void ShadowCaster::ConvertTileMapToPolyMap(int sx, int sy, int w, int h, float blockWidth, int pitch)
{
	//Clear PolyMap
	edges.clear();

	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			for (int j = 0; j < 4; j++)
			{
				world[(y + sy) * pitch + (x + sx)].edge_exist[j] = false;
				world[(y + sy) * pitch + (x + sx)].edge_id[j] = 0;
			}
		}
	}

	//Iterate from top left to bottom right
	for (int x = 1; x < w - 1; x++)
	{
		for (int y = 1; y < h - 1; y++)
		{
			int i = (y + sy) * pitch + (x + sx);	//Current cell
			int n = (y + sy - 1) * pitch + (x + sx);//Cell to North
			int s = (y + sy + 1) * pitch + (x + sx);//Cell to South
			int e = (y + sy) * pitch + (x + sx + 1);//Cell to East
			int w = (y + sy) * pitch + (x + sx - 1);//Cell to West

			//If cell exists, check if it needs edges
			if (world[i].exist)
			{
				//if the cell has no western neighbour, it needs a western edge
				if (!world[w].exist)
				{
					//Either extend an existing edge, or start a new one
					if (world[n].edge_exist[WEST])
					{

					}

				}
			}
		}
	}
}