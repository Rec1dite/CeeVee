#pragma once
#include "olcPixelGameEngine.h"

using namespace olc;

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

struct Edge
{
	float sx, sy; //start
	float ex, ey; //end
};

struct Cell
{
	int edge_id[4];
	bool edge_exist[4];
	bool exist = false;
};

class ShadowCaster
{
	private:
		PixelGameEngine* ge;
		Cell* world;
		int worldW = 64, worldH = 64;

		std::vector<Edge> edges;
		
		void ConvertTileMapToPolyMap(int, int, int, int, float, int);

	public:
		bool Create(PixelGameEngine*);
		bool Update();
		int CursorToBlock(float, float, float);
	
};
