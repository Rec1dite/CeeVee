#pragma once
#include "olcPixelGameEngine.h"
#include <fstream>
#include <sstream>

using namespace std;
using namespace olc;

struct mat4x4
{
	float m[4][4] = { 0 }; //[row][col]

	mat4x4 operator*(const mat4x4& a) const
	{
		mat4x4 matrix;
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				matrix.m[r][c] = m[r][0] * a.m[0][c] + m[r][1] * a.m[1][c] + m[r][2] * a.m[2][c] + m[r][3] * a.m[3][c];
		return matrix;
	}

	mat4x4& operator*=(const mat4x4 rhs)
	{
		*this = *this * rhs;
		return *this;
	}
};

struct vec2d
{
	union
	{
		struct { float x, y, z; };
		struct { float u, v, w; };
		float n[3] = { 0.0f, 0.0f, 1.0f };
	};

	vec2d()
		: x(0), y(0), z(1)
	{}
	vec2d(float x, float y)
		: x(x), y(y), z(1)
	{}
	vec2d(float x, float y, float z)
		: x(x), y(y), z(z)
	{}

	float length() const
	{
		return sqrtf(x * x + y * y);
	}

	float dot(const vec2d& a) const
	{
		return (x * a.x + y * a.y);
	}

	float angle(const vec2d& a) const
	{
		return acosf( dot(a)/(length() * a.length()) );
	}

#pragma region Operator Overloads
	vec2d operator+(const vec2d& rhs) const
	{
		vec2d res = {
			this->x + rhs.x,
			this->y + rhs.y,
			1
		};
		return res;
	}
	vec2d operator-(const vec2d& rhs) const
	{
		vec2d res = {
			this->x - rhs.x,
			this->y - rhs.y,
			1
		};
		return res;
	}
	vec2d operator*(const float rhs) const
	{
		vec2d res = {
			this->x * rhs,
			this->y * rhs,
			1
		};
		return res;
}
	vec2d operator/(const float rhs) const
	{
		vec2d res = {
			this->x / rhs,
			this->y / rhs,
			1
		};
		return res;
	}
	vec2d& operator+=(const vec2d& rhs)
	{
		*this = *this + rhs;
		return *this;
	}
	vec2d& operator-=(const vec2d& rhs)
	{
		*this = *this - rhs;
		return *this;
	}
	vec2d& operator*=(const float rhs)
	{
		*this = *this * rhs;
		return *this;
	}
	vec2d& operator/=(const float rhs)
	{
		*this = *this / rhs;
		return *this;
	}
#pragma endregion
};

struct vec3d
{
	union
	{
		struct { float x, y, z, w; };
		struct { float r, g, b, a; };
		float n[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	vec3d()
		: x(0), y(0), z(0), w(1)
	{}
	
	vec3d(float x, float y, float z, float w)
		: x(x), y(y), z(z), w(w)
	{}

	vec3d(float x, float y, float z)
		: x(x), y(y), z(z), w(1)
	{}

	float length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	vec3d normalized() const
	{
		return *this / length();
	}

	float angle(const vec3d& a) const
	{
		return acosf( dot(a)/(length() * a.length()) );
	}

	float dot(const vec3d& a) const
	{
		return (x * a.x + y * a.y + z * a.z);
	}

	vec3d cross(const vec3d& a) const
	{
		return {
			n[1] * a.n[2] - n[2] * a.n[1],
			n[2] * a.n[0] - n[0] * a.n[2],
			n[0] * a.n[1] - n[1] * a.n[0],
			1
		};
	}

	//Returns the vector projected onto the horizontal plane
	vec2d as2d() const
	{
		return { x, z };
	}

#pragma region Operator Overloads
	//Returns the product of the current vector with the specified matrix
	vec3d operator*(const mat4x4& m) const
	{
		return vec3d(
						(x * m.m[0][0]) + (y * m.m[1][0]) + (z * m.m[2][0]) + (m.m[3][0]),
						(x * m.m[0][1]) + (y * m.m[1][1]) + (z * m.m[2][1]) + (m.m[3][1]),
						(x * m.m[0][2]) + (y * m.m[1][2]) + (z * m.m[2][2]) + (m.m[3][2]),
						(x * m.m[0][3]) + (y * m.m[1][3]) + (z * m.m[2][3]) + (m.m[3][3])
					);
	}

	vec3d operator+(const vec3d& rhs) const
	{
		vec3d res = {
			this->x + rhs.x,
			this->y + rhs.y,
			this->z + rhs.z,
			1
		};
		return res;
	}
	vec3d operator-(const vec3d& rhs) const
	{
		vec3d res = {
			this->x - rhs.x,
			this->y - rhs.y,
			this->z - rhs.z,
			1
		};
		return res;
	}
	vec3d operator*(const float rhs) const
	{
		vec3d res = {
			this->x * rhs,
			this->y * rhs,
			this->z * rhs,
			1
		};
		return res;
}
	vec3d operator/(const float rhs) const
	{
		vec3d res = {
			this->x / rhs,
			this->y / rhs,
			this->z / rhs,
			1
		};
		return res;
	}
	vec3d& operator+=(const vec3d& rhs)
	{
		*this = *this + rhs;
		return *this;
	}
	vec3d& operator-=(const vec3d& rhs)
	{
		*this = *this - rhs;
		return *this;
	}
	vec3d& operator*=(const float rhs)
	{
		*this = *this * rhs;
		return *this;
	}
	vec3d& operator/=(const float rhs)
	{
		*this = *this / rhs;
		return *this;
	}
#pragma endregion
};

struct triangle
{
	vec3d p[3]; //Points
	vec2d t[3]; //Texture coords
	vec3d vn[3];//Vertex normals	//TODO: copy between triangles in render pipeline; interpolate where the triangles are clipped
	int matIndex;
	
	triangle()
		: p{ vec3d(), vec3d(), vec3d() }, t{ vec2d(), vec2d(), vec2d() }, vn{ vec3d(), vec3d(), vec3d() }, matIndex(0)
	{}

	triangle(const vec3d &p1, const vec3d &p2, const vec3d &p3, int matIndex)
		: p{ p1, p2, p3 }, t{ vec2d(), vec2d(), vec2d() }, vn{ vec3d(), vec3d(), vec3d() }, matIndex(matIndex)
	{}

	triangle(const vec3d &p1, const vec3d &p2, const vec3d &p3,
			 const vec2d &t1, const vec2d &t2, const vec2d &t3,
			 int matIndex)
		: p{ p1, p2, p3 }, t{ t1, t2, t3 }, vn{ vec3d(), vec3d(), vec3d() }, matIndex(matIndex)
	{}

	triangle(const vec3d &p1, const vec3d &p2, const vec3d &p3,
			 const vec2d &t1, const vec2d &t2, const vec2d &t3,
			 const vec3d &n1, const vec3d &n2, const vec3d &n3,
			 int matIndex)
		: p{ p1, p2, p3 }, t{ t1, t2, t3 }, vn{ n1, n2, n3 }, matIndex(matIndex)
	{}

	Pixel SampleCol(float u, float v) //TODO ?
	{
	}
};

struct mesh
{
	string name;
	vector<triangle> tris;
	vec3d position;
	vec3d rotation;
	int modifier;
	//vec3d scale; //TODO

	mesh(string meshName)
		: name(meshName), tris{}, position(vec3d()), rotation(vec3d()), modifier(-1)
	{}
};

struct material
{
	int textureIndex;
	int numFrames; //For animated textures
	float animSpeed;
	int alphaIndex;
	Pixel col;
	Pixel emis;
	float metallic;

	material()
		: textureIndex(-1), numFrames(0), animSpeed(0.0f), alphaIndex(-1), col(WHITE), metallic(0) //Default material will just be a solid white, no texture
	{}

	material(int textureIndex)
		: textureIndex(textureIndex), numFrames(0), animSpeed(0.0f), alphaIndex(-1), col(WHITE), metallic(0)
	{}
	
	material(int textureIndex, int alphaIndex)
		: textureIndex(textureIndex), numFrames(0), animSpeed(0.0f), alphaIndex(alphaIndex), col(WHITE), metallic(0)
	{}
};


Pixel AveragePixels(Pixel a, Pixel b, Pixel c, Pixel d)
{
	return Pixel((a.r + b.r + c.r + d.r) / 4,
				 (a.g + b.g + c.g + d.g) / 4,
				 (a.b + b.b + c.b + d.b) / 4,
				 (a.a + b.a + c.a + d.a) / 4
				);
}

//Only downscales by half
void DownscaleSprite(Sprite* in, Sprite* out)
{
	//Pixel r = Pixel(rand() % 255, rand() % 255, rand() % 255);
	for (int y = 0; y < in->height-1; y+=2)
	{
		for (int x = 0; x < in->width-1; x+=2)
		{
			Pixel p = AveragePixels(in->GetPixel(x, y), in->GetPixel(x+1, y), in->GetPixel(x, y+1), in->GetPixel(x+1, y+1));
			out->SetPixel(x/2, y/2, p);
		}
	}
}

struct texture
{
	int numMips = 5;
	Sprite** mips;
	//const float mipDist;

	texture()
		: numMips(1), mips{ nullptr }//, mipDist()
	{}

	texture(int numMips)
		: numMips(numMips), mips( new Sprite*[numMips] )
	{}

	texture(int numMips, Sprite* mips[])
		: numMips(numMips), mips( new Sprite*[numMips] )
	{
		for (int m = 0; m < numMips; m++)
		{
			this->mips[m] = mips[m];
		}
	}

	//Generates mips automatically, expects a square texture
	texture(Sprite* sprite)
	{
		numMips = min(3, (int)ceil(log2(sprite->width))); //Number of mips is based on the texture size,
												 //the number of times the image can be halved

		mips = new Sprite*[numMips]; //If this broke there's probably something wrong with the file path in the .mtl file

		mips[0] = sprite;
		mips[0]->SetSampleMode(Sprite::Mode::PERIODIC);

		//Generate mips, each one is half the size of the previous
		for (int m = 1; m < numMips; m++)
		{
			mips[m] = new Sprite(mips[m-1]->width/2, mips[m-1]->height/2);
			mips[m]->SetSampleMode(Sprite::Mode::PERIODIC);

			DownscaleSprite(mips[m-1], mips[m]);
		}
	}

	~texture()
	{
		//delete mips;
	}

};

//Modifies properties of an entire mesh
struct modifier
{
	vec3d constantRotation;
	bool isBillboard;

	modifier()
		: isBillboard(false), constantRotation { 0.0f, 0.0f, 0.0f }
	{}

	modifier(vec3d constantRotation, bool isBillboard)
		: isBillboard(isBillboard), constantRotation(constantRotation)
	{}
};

struct path
{
	int currPoint;
	vec3d pts[];

	//Freezeframe slow-mo effect when at important points

	path()
		: currPoint(-1), pts{}
	{}
};
