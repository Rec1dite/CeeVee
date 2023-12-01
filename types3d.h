#pragma once
#include "olcPixelGameEngine.h"
#include <fstream>
#include <sstream>

using namespace std;
using namespace olc;

struct intPair
{
	intPair(int a = 0, int b = 0)
		: a(a), b(b)
	{}

	union
	{
		struct { int a, b; };
		struct { int x, y; };
		int n[2]{ 0, 0 };
	};
};

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

	vec2d lerp(const vec2d& a, float t)
	{
		return {
			x + (a.x-x)*t,
			y + (a.y-y)*t,
			1
		};
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

struct quaternion
{

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

	vec3d lerp(const vec3d& a, float t)
	{
		return {
			x + (a.x-x)*t,
			y + (a.y-y)*t,
			z + (a.z-z)*t,
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
	
	triangle(int matIndex = 0,
			 const vec3d &p1 = vec3d(), const vec3d &p2 = vec3d(), const vec3d &p3 = vec3d(),
			 const vec2d &t1 = vec2d(), const vec2d &t2 = vec2d(), const vec2d &t3 = vec2d(),
			 const vec3d &n1 = vec3d(), const vec3d &n2 = vec3d(), const vec3d &n3 = vec3d())
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

	void setPos(vec3d& pos)
	{
		this->position = pos;
	}

	void setRot(vec3d& rot)
	{
		this->rotation = rot;
	}

	mesh(string meshName)
		: name(meshName), tris{}, position(vec3d()), rotation(vec3d()), modifier(-1)
	{}
};

struct material
{
	int textureIndex;
	int alphaIndex;
	Pixel col;
	Pixel emis;
	float metallic;

	float mipScale = 1.0f;

	//Animation stuff
	int startIndex, endIndex; //Indexed from 0
	int xDivisions, yDivisions;
	float animSpeed;

	//Default material will just be a solid white, no texture
	material(int textureIndex = -1, int alphaIndex = -1)
		: textureIndex(textureIndex), alphaIndex(alphaIndex), col(WHITE), metallic(0),
		  startIndex(0), endIndex(0), xDivisions(1), yDivisions(1), animSpeed(0.0f)
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
		numMips = min(4, (int)ceil(log2(sprite->width))); //Number of mips is based on the texture size,
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
	int pathIndex;
	float pathStepsPerSecond;
	bool useTransformAsPathOffset;
	bool pathReverse;
	bool applyPathRotation;

	modifier(vec3d constantRotation = vec3d(), bool isBillboard = false)
		: isBillboard(isBillboard), constantRotation(constantRotation), pathIndex(-1), pathStepsPerSecond(0.01f), useTransformAsPathOffset(false), pathReverse(false), applyPathRotation(true)
	{}
};

struct text
{
	vec3d pos;
	string title;
	float titleSize;
	string description;
	float descSize;
	vi2d borderSize;

	text(float x = 0.0f, float y = 0.0f, float z = 0.0f)
		: pos{ x, y, z }, title(""), description(""), titleSize(1.0f), descSize(1.0f), borderSize{ 40, 40 }
	{}
};

struct infoPoint
{
	int pathPtIndex;
	float speed;
	int lookMeshIndex;
	int fov;
	bool doStop;
	float alpha;
	vector<text> texts;

	infoPoint(bool doStop = false)
		: pathPtIndex(-1), speed(-1.0f), lookMeshIndex(-1), fov(90), doStop(doStop), alpha(0.0f)
	{}
};

struct path
{
	string name;
	int currInfoPt;
	float posT;
	bool isMoving;
	float prevSpeed = 1.0f;
	vec3d position;
	vector<vec3d> pts;
	vector<infoPoint> infoPts;

	//Freezeframe slow-mo effect when at important points

	path(string name, float x = 0.0f, float y = 0.0f, float z = 0.0f)
		: name(name), currInfoPt(0), posT(0.0f), isMoving(false), position(vec3d(x, y, z)), pts{}, infoPts{}
	{}

	void Reset()
	{
		prevSpeed = 1.0f;
		currInfoPt = 0;
		posT = 0.0f;
		isMoving = false;
	}

	void Next()
	{
		if (canMoveForward()) //No info pts, or we are at the end of the path
		{
			if (!isMoving) // Can't move to the next point if you're currently moving already
			{
				currInfoPt++;

				if (infoPts[currInfoPt].speed > 0.0f)
				{
					prevSpeed = infoPts[currInfoPt].speed;
				}

				infoPts[currInfoPt].alpha = 1.0f;

				posT = 0.0f;
				isMoving = true;
			}
		}
		else
		{
			cout << "Nothing to move to." << endl;
		}
	}

	//Returns true if there is a next info point to move to after the current one; false otherwise
	//Also returns false if infoPts is empty
	bool canMoveForward(int infoPtOffset = 0)
	{
		return infoPts.size()-1 > (currInfoPt + infoPtOffset);
	}

	void Prev()
	{
		//TODO
	}

	void Update(float fElapsedTime, vec3d& camTarget, float& camFOV, vector<mesh>& meshes)
	{
		if (isMoving)
		{
			if(infoPts[currInfoPt].speed > 0.0f)
			{
				posT += fElapsedTime * infoPts[currInfoPt].speed;
			}
			else //0 or negative speed means we just use the previous speed from the last infoPt
			{
				posT += fElapsedTime * prevSpeed;
			}
			//cout << "t: " << posT << endl;

			if (posT >= 0.5f) //Display current info point
			{
				infoPts[currInfoPt].alpha = infoPts[currInfoPt].alpha >= 1.0f ? 1.0f : infoPts[currInfoPt].alpha + 0.01f;
			}
			if (posT >= 1.0f) //Reached next point, stop moving
			{
				posT = 0.0f;
				isMoving = false;

				if (!infoPts[currInfoPt].doStop) //Continue to next infoPt
				{
					Next();
				}
			}
		}

		if (infoPts.size() != 0)
		{
			float speed = infoPts[currInfoPt].speed;
			if (infoPts[currInfoPt].lookMeshIndex != -1)
			{
				camTarget = meshes[infoPts[currInfoPt].lookMeshIndex].position;
			}

			if (speed < 0.0f)
			{
				camFOV = lerp(camFOV, infoPts[currInfoPt].fov, 0.8f * prevSpeed);
			}
			else
			{
				camFOV = lerp(camFOV, infoPts[currInfoPt].fov, 0.1f * speed);
			}
		}
	}

	vec3d getCurrPosition(float offset = 0.0f)
	{
		if (infoPts.size() == 0)
		{
			return pts[0];
		}

		//return getLerpPointBetween(infoPts[currInfoPt - 1].pathPtIndex, infoPts[currInfoPt].pathPtIndex, posT, offset);

		if (isMoving) //Get intermediary point, between 2 info points
		{
			return getLerpPointBetween(infoPts[currInfoPt - 1].pathPtIndex, infoPts[currInfoPt].pathPtIndex, posT, offset);
		}
		else //Get exact info point on the path
		{
			return pts[infoPts[currInfoPt].pathPtIndex + trunc(offset)];
		}
	}

	//Lerps along the path; 0.0f < t < 1.0f
	vec3d getLerpPoint(float t, bool reversed = false)
	{
		return getLerpPointBetween(0, pts.size() - 1, t, 0.0f, reversed);
	}

	//Lerps between 2 points along the path; 0.0f < t < 1.0f
	vec3d getLerpPointBetween(int indexA, int indexB, float t, float offset = 0.0f, bool reversed = false)
	{
		if (offset != 0.0f)
		{
			//TODO: Fix - Only works with whole numbers for offset
			//t += offset/(indexB - indexA);

			t += offset;
			int ptOffset = trunc(t);
			t -= ptOffset;
			
			indexA += ptOffset;
			indexB += ptOffset;
		}


		t = max(0.0f, min(1.0f, t));
		indexA = max(0, min((int)pts.size()-1, indexA));
		indexB = max(0, min((int)pts.size()-1, indexB));


		if (reversed)
		{
			t = 1.0f - t;
		}

		if (t == 0.0f)
		{
			//return pts[indexB];
		}
		else if (t == 1.0f)
		{
			//return pts[indexA];
		}

		//2 to 4, 0.7			2
		float x = t * (indexB - indexA); //1.4
		int nearestLower = indexA + floor(x); //3
		int nearestHigher = nearestLower + 1; //4
		float ptT = fmodf(x, 1); //Extract decimal places

		return pts[nearestLower].lerp(pts[nearestHigher], ptT);
	}
};
