#pragma once
#include "olcPixelGameEngine.h"
#include "constants.h"
#include "types3d.h"
#include <algorithm>
#include <map>
#include <unordered_set>
#include <iomanip>
#include "olcPGEX_Font-master/olcPGEX_Font.h"

using namespace std;
using namespace olc;
using namespace cv;

class Engine3D
{
private:
	int screenW, screenH;

	vector<mesh> meshes;
	vector<material> materials;
	vector<texture> textures;
	vector<modifier> modifiers;
	vector<path> paths;

	mat4x4 matTrans, matCam, matView, matProj;

	vec3d camPos;
	vec3d camDir;
	float camYaw;
	float camTilt;
	float camFOV = 100.0f;
	bool camOverridePathLookAt = false;
	bool debugMode = false;
	float debugDepthValue = 100.0f;
	string debugText = "";
	int cameraMod = -1;

	float timePassed;

	float* depthBuffer = nullptr;
	Pixel* bloomBuffer = nullptr;

	unique_ptr<Font> arial;
	unique_ptr<Font> lato_bold;
	unique_ptr<Font> azeret_mono;
	unique_ptr<Font> martel_light;

	const float mipDist = 1.0f;
	const float mipLogA = log2(mipDist);

	float fogDistance = 0.01f; //Depth map is inversely scaled, with values approaching 0 at infinity

	string ReplaceCharacterInString(string input, char character, string replacement)
	{
		vector<string> pieces = vector<string>();
		stringstream ss(input);
		string piece;
		while (getline(ss, piece, character))
		{
			pieces.push_back(piece);
		}

		string res = "";
		for (int j = 0; j < pieces.size() - 1; j++)
		{
			res += pieces[j] + replacement;
		}
		res += pieces[pieces.size()-1];

		return res;
	}

	string ParseMTLFilePath(string path)
	{
		stringstream res("");

		for (int i = 0; i < path.length(); i++)
		{
			if (path[i] == '\\')
			{
				switch (path[i + 1])
				{
				case '\"':
					res << "\"";
					i++;
					break;
				case '\'':
					res << "'";
					i++;
					break;
				default:
					res << "\\";
					break;
				}
			}
			else
			{
				res << path[i];
			}
		}

		return res.str();
	}

	bool LoadPaths(string fileName, vector<path>& paths, map<string, int>& pathIndices, map<string, vector<intPair>>& pathLookAtIndices)
	{
		ifstream f(fileName + ".pth");
		if (!f.is_open())
		{
			return false;
		}

		string line;
		int prevFOV = 90;

		while (getline(f, line))
		{
			stringstream s(line);

			string prefix;
			s >> prefix;

			if (prefix == "newpath")
			{
				string pathName, ox, oy, oz;
				s >> pathName >> ox >> oy >> oz;

				paths.push_back(path(pathName, stof(ox), stof(oy), stof(oz)));
				pathIndices.insert(pair<string, int>(pathName, paths.size() - 1));
			}
			else if (prefix == "v")
			{
				string x, y, z;
				s >> x >> y >> z;
				paths.back().pts.push_back(vec3d(stod(x), stod(y), stod(z)));
			}
			else if (prefix == "infoPt")
			{
				string name, doStop;
				s >> name >> doStop;

				paths.back().infoPts.push_back(infoPoint(doStop==""?0:stoi(doStop)));
				paths.back().infoPts.back().pathPtIndex = paths.back().pts.size()-1;
				paths.back().infoPts.back().fov = prevFOV;
			}
			else if (prefix == "look")
			{
				string meshName, lookPath;
				s >> meshName >> lookPath;

				map<string, vector<intPair>>::iterator it = pathLookAtIndices.find(meshName);
				if (it != pathLookAtIndices.end()) //Mesh name is contained in pathLookAtIndices
				{
					it->second.push_back(intPair(paths.size() - 1, paths.back().infoPts.size() - 1));
				}
				else
				{
					pathLookAtIndices.insert(pair<string, vector<intPair>>(meshName, vector<intPair>()));
					pathLookAtIndices.at(meshName).push_back(intPair(paths.size() - 1, paths.back().infoPts.size() - 1));
				}
			}
			else if (prefix == "fov")
			{
				string fov;
				s >> fov;
				prevFOV = stoi(fov);
				paths.back().infoPts.back().fov = prevFOV;
			}
			else if (prefix == "speed") //Speed to go to next point
			{
				string speed;
				s >> speed;
				paths.back().infoPts.back().speed = stof(speed);
			}
			else if (prefix == "text")
			{
				string tx, ty, tz;
				s >> tx >> ty >> tz;
				paths.back().infoPts.back().texts.push_back(text(stof(tx), stof(ty), stof(tz)));
			}
			else if (prefix == "title")
			{
				string title = "";
				getline(s, title);
				if (title.size() > 1) //Remove space character at the beginning
				{
					title.erase(0, 1);
				}
				paths.back().infoPts.back().texts.back().title +=
					(paths.back().infoPts.back().texts.back().title.length() > 0 ? "\n" : "") + title;
			}
			else if (prefix == "desc")
			{
				string desc = "";
				getline(s, desc);
				if (desc.size() > 1) //Remove space character at the beginning
				{
					desc.erase(0, 1);
				}
				paths.back().infoPts.back().texts.back().description +=
					(paths.back().infoPts.back().texts.back().description.length() > 0 ? "\n" : "") + desc;
			}
			else if (prefix == "border")
			{
				string xSize, ySize;
				s >> xSize >> ySize;
				paths.back().infoPts.back().texts.back().borderSize = vi2d(stoi(xSize), stoi(ySize));
			}
		}
	}

	bool LoadModifiers(string fileName, vector<modifier>& modifiers, map<string, int>& modIndices, vector<path>& paths, map<string, vector<intPair>>& pathLookAtIndices)
	{
		ifstream f(fileName + ".mdfr");
		if (!f.is_open())
		{
			return false;
		}

		map<string, int> pathIndices;
		bool usePaths = LoadPaths(fileName, paths, pathIndices, pathLookAtIndices);

		string line;

		while (getline(f, line))
		{
			stringstream s(line);

			string prefix;
			s >> prefix;

			if (prefix == "newmod") //Modifier
			{
				string modName;
				s >> modName;
 
				modifiers.push_back(modifier());
				modIndices.insert(pair<string, int>(modName, modifiers.size()-1));

				if (modName.length() >= 2 && modName[1] == '_') //TODO: REMOVE
				{
					switch (modName[0])
					{
						case 'c': //Camera
							cameraMod = modifiers.size() - 1;
							break;
					}
				}
			}

			else if (prefix == "billboard")
			{
				string value;
				s >> value;

				modifiers.back().isBillboard = stoi(value);
			}

			else if (prefix == "rot")
			{
				string rx, ry, rz;
				s >> rx >> ry >> rz;
				modifiers.back().constantRotation = vec3d(stod(rx), stod(ry), stod(rz));
			}

			else if (prefix == "path")
			{
				string pathName, stepsPerSecond, loop = "0", useTransformAsPathOffset = "0", pathReverse = "0", applyRotation = "1";
				s >> pathName >> stepsPerSecond >> loop >> useTransformAsPathOffset >> pathReverse >> applyRotation;

				modifiers.back().pathIndex = pathIndices.at(pathName);
				modifiers.back().pathStepsPerSecond = stof(stepsPerSecond);
				modifiers.back().useTransformAsPathOffset = stoi(useTransformAsPathOffset);
				modifiers.back().pathReverse = stoi(pathReverse);
				modifiers.back().applyPathRotation = stoi(applyRotation);
			}
		}
	}

	//Loads the textures from a .mtl file into "textures", and provides a <name, index> map by which these textures can be accessed
	//Returns true if there is at least one texture that was successfully loaded into the textures vector
	bool LoadTextures(string fileName, vector<texture>& textures, map<string, int>& texIndices)
	{
		ifstream f(fileName + ".mtl");
		if (!f.is_open()) //If no file, create default blank texture
		{
			return false;
		}

		unordered_set<string> texFileNames; //Ensures no duplicate textures /////BUG: Unordered set causes RoadTile to appear in ocean animation

		string line;

		//Get unique texture file names
		while (getline(f, line))
		{
			stringstream s(line);

			string prefix;
			s >> prefix;

			//Diffuse texture and alpha texture
			if (prefix == "map_Kd" || prefix == "map_d")
			{
				string texFileName;
				s >> texFileName;

				texFileNames.insert(texFileName);
			}
			if (prefix == "animap_Kd")
			{
				string trash, texFileName;
				s >> trash >> trash >> trash >> trash >> trash >> texFileName;
				texFileNames.insert(texFileName);
			}
		}

		if (texFileNames.size() == 0)
		{
			//If no textures, return false
			return false;
		}
		else
		{
			//Load textures by file name
			for (const auto& fn : texFileNames)
			{
				textures.push_back(texture(new Sprite(fn))); //Textures automatically generates its own mips
				texIndices.insert(pair<string, int>(fn, textures.size()-1));
			}
		}
		f.close();

		return true;
	}

	//Loads the materials from a .mtl file into "materials", and provides a <name, index> map by which these materials can be accessed
	bool LoadMaterials(string fileName, vector<material>& materials, map<string, int>& matIndices, vector<texture>& textures)
	{
		map<string, int> texIndices;
		bool useTexs = LoadTextures(fileName, textures, texIndices);

		ifstream f(fileName + ".mtl");
		if(!f.is_open())
		{
			//If fails, create the default material which all objects will use
			materials = { material() };

			return false;
		}

		string line;
		while (getline(f, line))
		{
			stringstream s(line);

			string prefix;
			s >> prefix;

			if (prefix == "newmtl") //Material
			{
				string matName;
				s >> matName;
 
				materials.push_back(material());
				matIndices.insert(pair<string, int>(matName, materials.size()-1));
			}

			else if (prefix == "Kd") //Diffuse color
			{
				string r, g, b;
				s >> r >> g >> b;
				materials.back().col = PixelF(stof(r), stof(g), stof(b));
			}

			else if (prefix == "map_Kd") //Diffuse texture
			{
				string texFileName, mipScale;
				s >> texFileName >> mipScale;
				materials.back().textureIndex = useTexs ? texIndices[texFileName] : -1;
				if (mipScale != "")
				{
					materials.back().mipScale = stof(mipScale);
				}
			}

			else if (prefix == "animap_Kd") //Animated diffuse texture
			{
				string xDivisions, yDivisions, startIndex, endIndex, animFps, texFileName, mipScale;
				s >> xDivisions >> yDivisions >> startIndex >> endIndex >> animFps >> texFileName >> mipScale;

				materials.back().textureIndex = texIndices.at(texFileName);
				materials.back().xDivisions = stoi(xDivisions);
				materials.back().yDivisions = stoi(yDivisions);
				materials.back().startIndex = stoi(startIndex);
				materials.back().endIndex   = stoi(endIndex);
				materials.back().animSpeed  = stof(animFps);
				if (mipScale != "")
				{
					materials.back().mipScale = stof(mipScale);
				}
			}

			else if (prefix == "map_d")
			{
				string alphaFileName, mipScale;
				s >> alphaFileName >> mipScale;
				materials.back().alphaIndex = useTexs ? texIndices[alphaFileName] : -1;
				if (mipScale != "")
				{
					materials.back().mipScale = stof(mipScale);
				}
			}
		}
		f.close();

		return true;
	}

	//Loads all assets from a .obj file and its corresponding .mtl file, including meshes, materials, and textures
	bool LoadFromObjectFile(string fileName, vector<mesh>& meshes, vector<material>& materials, vector<texture>& textures, vector<modifier>& modifiers, vector<path>& paths)
	{
		ifstream f(fileName + ".obj");
		if(!f.is_open())
		{
			return false;
		}

		meshes = vector<mesh>();
		materials = vector<material>();
		textures = vector<texture>();
		modifiers = vector<modifier>();

		map<string, int> matIndices;
		bool useMats = LoadMaterials(fileName, materials, matIndices, textures);

		map<string, int> modIndices;
		map<string, vector<intPair>> pathLookAtIndices; //<object name, {path index, infoPt index}>
		bool useMods = LoadModifiers(fileName, modifiers, modIndices, paths, pathLookAtIndices);

		vector<vec3d> vts;
		vector<vec2d> uvs;
		vector<vec3d> vns;
		int matIndex = 0; //There will always be at least one material in the materials vector, the default solid white material
		string line;

		while (getline(f, line)) //Loop through lines
		{
			if (line == "")
			{
				continue;
			}
			stringstream s(line);

			string prefix;
			s >> prefix;

			if (prefix == "o") //Object, create a new mesh
			{
				matIndex = 0;
				string meshName, ox, oy, oz;
				s >> meshName >> ox >> oy >> oz;
				meshes.push_back(mesh(meshName));
				if (ox != "") //Origin specified
				{
					meshes.back().position = vec3d(stod(ox), stod(oy), stod(oz));

					map<string, vector<intPair>>::iterator it = pathLookAtIndices.find(meshName);
					if (it != pathLookAtIndices.end()) //Mesh name is contained in pathLookAtIndices
					{
						for (intPair ip : it->second)
						{
							paths[ip.a].infoPts[ip.b].lookMeshIndex = meshes.size() - 1;
						}
					}
				}
				if (meshName.length() >= 2 && meshName[1] == '_') //TODO: REMOVE
				{
					switch (meshName[0])
					{
						case 'b': //Billboard
							meshes.back().modifier = modIndices.at("Billboard");
							break;
					}
				}
			}

			else if (prefix == "usemod") //Modifier
			{
				string modName;
				s >> modName;
				meshes.back().modifier = modIndices.at(modName);
			}

			else if (prefix == "v") //Vertex
			{
				vec3d v;
				s >> v.x >> v.y >> v.z;
				vts.push_back(v);
			}

			else if (prefix == "vt") //UV
			{
				vec2d v;
				s >> v.u >> v.v;
				uvs.push_back(v);
			}

			else if (prefix == "vn") //Vertex normal
			{
				vec3d vn;
				s >> vn.x >> vn.y >> vn.z;
				vns.push_back(vn);
			}

			else if (prefix == "usemtl") //Material
			{
				string matName;
				s >> matName;
				matIndex = matIndices.at(matName);
			}

			else if (prefix == "f") //Face
			{
				int vals[3][3]; //[0][x] = vertices
								//[1][x] = uvs
								//[2][x] = vertex norms

				string p[3];
				s >> p[0] >> p[1] >> p[2];

				int j = 0;
				for (int i = 0; i < 3; i++)
				{
					stringstream pt(p[i]);
					string val;

					j = 0;
					while (getline(pt, val, '/'))
					{
						vals[j][i] = stoi(val);
						j++;
					}
				}

				switch (j)
				{
					case 1: //Just vertices
						meshes.back().tris.push_back(triangle(
																matIndex,
																vts[vals[0][0] - 1], vts[vals[0][1] - 1], vts[vals[0][2] - 1])
															 );
						break;

					case 2: //Vertices and UVs
						meshes.back().tris.push_back(triangle(
																matIndex,
																vts[vals[0][0] - 1], vts[vals[0][1] - 1], vts[vals[0][2] - 1],
																uvs[vals[1][0] - 1], uvs[vals[1][1] - 1], uvs[vals[1][2] - 1])
															 );
						break;

					case 3: //Vertices, UVs, and Vertex normals
						meshes.back().tris.push_back(triangle(
																matIndex,
																vts[vals[0][0] - 1], vts[vals[0][1] - 1], vts[vals[0][2] - 1],
																uvs[vals[1][0] - 1], uvs[vals[1][1] - 1], uvs[vals[1][2] - 1],
																vns[vals[2][0] - 1], vns[vals[2][1] - 1], vns[vals[2][2] - 1])
															 );
						break;
				}
			}
		}
		f.close();

		return true;
	}

	mat4x4 CalculateProjectionMatrix(float zNear, float zFar, float fov, float screenWidth, float screenHeight)
	{
		mat4x4 res;

		float aspectRatio = screenHeight / screenWidth;
		float fovRad = 1.0f / tanf(fov / 360.0f * 3.1415926535898f);

		res.m[0][0] = aspectRatio * fovRad;
		res.m[1][1] = fovRad;
		res.m[2][2] = zFar / (zFar - zNear);
		res.m[3][2] = (-zFar * zNear) / (zFar - zNear);
		res.m[2][3] = 1.0f;
		res.m[3][3] = 0.0f;

		return res;
	}
	mat4x4 CalculateRotationXMatrix(float theta)
	{
		mat4x4 res;

		res.m[0][0] = 1;
		res.m[1][1] = cosf(theta * 0.5f);
		res.m[1][2] = sinf(theta * 0.5f);
		res.m[2][1] = -sinf(theta * 0.5f);
		res.m[2][2] = cosf(theta * 0.5f);
		res.m[3][3] = 1;

		return res;
	}
	mat4x4 CalculateRotationYMatrix(float theta)
	{
		mat4x4 res;

		res.m[0][0] = cosf(theta);
		res.m[0][2] = sinf(theta);
		res.m[2][0] = -sinf(theta);
		res.m[1][1] = 1;
		res.m[2][2] = cosf(theta);
		res.m[3][3] = 1;

		return res;
	}
	mat4x4 CalculateRotationZMatrix(float theta)
	{
		mat4x4 res;

		res.m[0][0] = cosf(theta);
		res.m[0][1] = sinf(theta);
		res.m[1][0] = -sinf(theta);
		res.m[1][1] = cosf(theta);
		res.m[2][2] = 1;
		res.m[3][3] = 1;

		return res;
	}
	mat4x4 IdentityMatrix()
	{
		mat4x4 res;

		res.m[0][0] = 1.0f;
		res.m[1][1] = 1.0f;
		res.m[2][2] = 1.0f;
		res.m[3][3] = 1.0f;

		return res;
	}
	mat4x4 CalculateTranslationMatrix(float x, float y, float z)
	{
		mat4x4 res;

		res.m[0][0] = 1;
		res.m[1][1] = 1;
		res.m[2][2] = 1;
		res.m[3][3] = 1;
		res.m[3][0] = x;
		res.m[3][1] = y;
		res.m[3][2] = z;

		return res;
	}
	mat4x4 MultiplyMatrix(mat4x4 &m1, mat4x4 &m2)
	{
		mat4x4 matrix;
		for (int c = 0; c < 4; c++)
			for (int r = 0; r < 4; r++)
				matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
		return matrix;
	}
	mat4x4 PointAtMatrix(vec3d &pos, vec3d &target, vec3d &up)
	{
		//Calculate new forward direction
		vec3d newForward = target - pos;
		newForward = newForward.normalized();
		
		//Calculate new up direction
		vec3d a = newForward * up.dot(newForward);
		vec3d newUp = up - a;
		newUp = newUp.normalized();

		//Calculate new right direction
		vec3d newRight = newUp.cross(newForward);

		//Construct dimensioning and translation matrix
		mat4x4 res =
		{
			//m:
			{
				{ newRight.x,   newRight.y,   newRight.z,   0.0f },
				{ newUp.x,      newUp.y,      newUp.z,      0.0f },
				{ newForward.x, newForward.y, newForward.z, 0.0f },
				{ pos.x,        pos.y,        pos.z,        1.0f }
			}
		};

		return res;
	}
	mat4x4 QuickInverseMatrix(mat4x4& m)
	{
		mat4x4 res =
		{
			//m:
			{
				{ m.m[0][0], m.m[1][0], m.m[2][0], 0.0f },
				{ m.m[0][1], m.m[1][1], m.m[2][1], 0.0f },
				{ m.m[0][2], m.m[1][2], m.m[2][2], 0.0f },
				{
					-(m.m[3][0] * m.m[0][0] + m.m[3][1] * m.m[0][1] + m.m[3][2] * m.m[0][2]),
					-(m.m[3][0] * m.m[1][0] + m.m[3][1] * m.m[1][1] + m.m[3][2] * m.m[1][2]),
					-(m.m[3][0] * m.m[2][0] + m.m[3][1] * m.m[2][1] + m.m[3][2] * m.m[2][2]),
					1.0f
				}
			}
		};
		return res;
	}
	//Returns the point at which a line segment intersects a plane with normal plane_n, and point plane_p which lies on the plane
	vec3d IntersectPlane(vec3d& plane_p, vec3d& plane_n, vec3d& lineStart, vec3d& lineEnd, float& t)
	{
		plane_n = plane_n.normalized();

		//Calculate coefficients of plane equation: ax + bx + cx + d = 0
		//Coefficients can be determined from the point-normal form of the equation: n.dot(x - p) = 0
		//where n is a normal vector to the plane, and p is a point on the plane
		//[https://math.stackexchange.com/questions/2509095/intersection-between-line-segment-and-a-plane/2509197]

		float plane_d = -(plane_n.dot(plane_p));
		float ad = lineStart.dot(plane_n);
		float bd = lineEnd.dot(plane_n);
		t = (-plane_d - ad) / (bd - ad); //Normalized distance the line at which the point of intersection exists.
										 // E.g. t = 0.5 -> intersection point is halfway between lineStart and lineEnd
		vec3d lineStartToEnd = lineEnd - lineStart;
		vec3d lineToIntersect = lineStartToEnd * t;
		return lineStart + lineToIntersect;
	}
	//Returns number of triangles generated while clipping
	int ClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle& in_tri, triangle& out_tri1, triangle& out_tri2)
	{
		plane_n = plane_n.normalized();

		//Signed shortest distance from point to plane
		//dist > 0: Point lies on 'inside' of plane
		//dist = 0: Point lies within plane
		//dist < 0: Point lies on 'outside' of plane (out of camera view)
		auto dist = [&](vec3d& p)
		{
			vec3d n = p.normalized();
			return (plane_n.dot(p) - plane_n.dot(plane_p));
		};

		vec3d* insidePts[3];
		vec3d* outsidePts[3];
		vec2d* insideTex[3];
		vec2d* outsideTex[3];
		int insidePtCount = 0, insideTexCount = 0;
		int outsidePtCount = 0, outsideTexCount = 0;

		//Get signed distance to plane for each point in the triangle
		float ds[3];
		for (int i = 0; i < 3; i++)
		{
			ds[i] = dist(in_tri.p[i]);

			if (ds[i] >= 0)
			{
				insidePts[insidePtCount++] = &in_tri.p[i];
				insideTex[insideTexCount++] = &in_tri.t[i];
			}
			else
			{
				outsidePts[outsidePtCount++] = &in_tri.p[i];
				outsideTex[outsideTexCount++] = &in_tri.t[i];
			}
		}

		//Classify triangle points; Subdivide the input triangle into smaller output tris if required.
		if (insidePtCount == 0)
		{
			//No points line inside the plane
			return 0;
		}
		else if (insidePtCount == 3)
		{
			//All points lie on the inside of the plane, so do nothing and allow the triangle to simply pass through

			out_tri1 = in_tri;
			return 1;
		}
		else if (insidePtCount == 1 && outsidePtCount == 2)
		{
			//Triangle should be clipped. Two points lie outside the plane, therefore the triangle should just become smaller.

			//Copy appearance info to new triangle
			out_tri1.matIndex = in_tri.matIndex;

			//The inside point remains in place
			out_tri1.p[0] = *insidePts[0];
			out_tri1.t[0] = *insideTex[0];

			//The outside points must be clipped to where they intersect with the plane
			float t;
			out_tri1.p[1] = IntersectPlane(plane_p, plane_n, *insidePts[0], *outsidePts[0], t);
			out_tri1.t[1] = {
								t * (outsideTex[0]->u - insideTex[0]->u) + insideTex[0]->u,
								t * (outsideTex[0]->v - insideTex[0]->v) + insideTex[0]->v,
								t * (outsideTex[0]->w - insideTex[0]->w) + insideTex[0]->w
							};

			out_tri1.p[2] = IntersectPlane(plane_p, plane_n, *insidePts[0], *outsidePts[1], t);
			out_tri1.t[2] = {
								t * (outsideTex[1]->u - insideTex[0]->u) + insideTex[0]->u,
								t * (outsideTex[1]->v - insideTex[0]->v) + insideTex[0]->v,
								t * (outsideTex[1]->w - insideTex[0]->w) + insideTex[0]->w
							};

			return 1; //Only returning this newly created single triangle
		}
		else if (insidePtCount == 2 && outsidePtCount == 1)
		{
			//Triangle should be clipped. Two points lie inside the plane, therefore the clipped triangle should become a quad.
			//This quad is then split into two smaller triangles

			//Copy appearance info to new triangles
			out_tri1.matIndex = in_tri.matIndex;
			out_tri2.matIndex = in_tri.matIndex;

			//First triangle
			out_tri1.p[0] = *insidePts[0];
			out_tri1.p[1] = *insidePts[1];
			out_tri1.t[0] = *insideTex[0];
			out_tri1.t[1] = *insideTex[1];

			float t;
			out_tri1.p[2] = IntersectPlane(plane_p, plane_n, *insidePts[0], *outsidePts[0], t);
			out_tri1.t[2] = {
								t * (outsideTex[0]->u - insideTex[0]->u) + insideTex[0]->u,
								t * (outsideTex[0]->v - insideTex[0]->v) + insideTex[0]->v,
								t * (outsideTex[0]->w - insideTex[0]->w) + insideTex[0]->w
							};

			//Second triangle
			out_tri2.p[0] = *insidePts[1];
			out_tri2.p[1] = out_tri1.p[2];
			out_tri2.t[0] = *insideTex[1];
			out_tri2.t[1] = out_tri1.t[2];

			out_tri2.p[2] = IntersectPlane(plane_p, plane_n, *insidePts[1], *outsidePts[0], t);
			out_tri2.t[2] = {
								t * (outsideTex[0]->u - insideTex[1]->u) + insideTex[1]->u,
								t * (outsideTex[0]->v - insideTex[1]->v) + insideTex[1]->v,
								t * (outsideTex[0]->w - insideTex[1]->w) + insideTex[1]->w
							};

			return 2;
		}
		return 0;
	}

	//Returns false if the point does not appear on screen, true otherwise
	bool WorldToScreenSpace(vec3d worldPt, vi2d& screenPt, mat4x4& matTrans, mat4x4& matView, mat4x4& matProj)
	{
		vec3d ptTrans, ptViewed, ptProj;

		ptTrans = worldPt * matTrans;

		ptViewed = ptTrans * matView;

		//Clip points behind camera
		if (ptViewed.z < 0)
		{
			return false;
		}

		ptProj = ptViewed * matProj;

		//Scale into view
		if (ptProj.w != 0.0f)
		{
			ptProj /= ptProj.w;
		}

		//x/y are inverted, so put them back
		ptProj.x *= -1;
		ptProj.y *= -1;

		//===== PROJECTION SPACE -> SCREEN SPACE =====
			//Scale projection to screen dimensions
		vec3d offsetView = { 1, 1, 0 };
		ptProj += offsetView;

		screenPt = vi2d(ptProj.x * 0.5f * screenW, ptProj.y * 0.5f * screenH);

		return true;
	}

public:
	void Create(PixelGameEngine* ge, string objectFile)
	{
		screenW = ge->ScreenWidth();
		screenH = ge->ScreenHeight();

		depthBuffer = new float[screenW * screenH];
		bloomBuffer = new Pixel[screenW * screenH];

		mesh m("Test1");
		m.position = vec3d();
		m.rotation = vec3d();
		m.tris.push_back(triangle());

		//meshes.push_back(m);

		arial = make_unique<Font>("./olcPGEX_Font-master/arial.png");
		lato_bold = make_unique<Font>("./olcPGEX_Font-master/Lato-Bold.png");
		azeret_mono = make_unique<Font>("./olcPGEX_Font-master/AzeretMono-Regular.png");
		martel_light = make_unique<Font>("./olcPGEX_Font-master/Martel-Light.png");

		LoadFromObjectFile(objectFile, meshes, materials, textures, modifiers, paths);

		matProj = CalculateProjectionMatrix(0.1f, 1000.0f, 100.0f, screenW, screenH);

	}

	void MoveRight(float amt)
	{
		vec3d right = camDir.cross({ 0, 1, 0 }) * amt;
		camPos += right;
	}
	void MoveUp(float amt)
	{
		camPos.y += amt;
	}
	void MoveForward(float amt)
	{
		vec3d forward = camDir * amt;
		camPos += forward;
	}
	void LookRight(float amt)
	{
		camYaw += amt;
	}
	void LookUp(float amt)
	{
		camTilt -= amt;
		camTilt = fmax(-9*PI/10, fmin(9*PI/10, camTilt));
	}
	void ToggleDebugMode()
	{
		debugMode = !debugMode;
		debugText = "Debug mode enabled.";
	}
	void ZoomFOV(float amt)
	{
		camFOV += amt;
		debugText = "FOV: " + to_string(camFOV);
	}
	void ZoomDepth(float amt)
	{
		debugDepthValue += amt;
		debugDepthValue = max(debugDepthValue, 0.0f);
		debugText = "Debug depth: " + to_string(debugDepthValue);
	}
	void NextPathPoint()
	{
		//Prevent one path from moving in front of the other, all of them must have stopped moving to continue
		bool canMove = true;
		for (path& p : paths)
		{
			canMove &= !p.isMoving;
		}

		if (canMove)
		{
			for (path& p : paths)
			{
				p.Next();
			}
		}
	}
	void SetOverrideLookAt(bool value)
	{
		camOverridePathLookAt = value;
	}
	void ResetPaths()
	{
		for (path& p : paths)
		{
			p.Reset();
		}
	}

	void Update(PixelGameEngine* ge, float fElapsedTime)
	{
		timePassed += fElapsedTime;

		ge->Clear(GREY);

		//Clear depth buffer
		for (int i = 0; i < screenW * screenH; i++)
		{
			depthBuffer[i] = 0.0f;
		}

		vec3d pathLookAtTarget;
		//Update paths
		for (path& p : paths)
		{
			p.Update(fElapsedTime, pathLookAtTarget, camFOV, meshes);
		}

		//===== CAMERA TRANSFORMATIONS =====
		matProj = CalculateProjectionMatrix(0.1f, 1000.0f, camFOV, screenW, screenH);
		if (!debugMode)
		{
			//Camera modifiers
			if (cameraMod != -1)
			{
				modifier& camMod = modifiers[cameraMod];
				if (camMod.pathIndex != -1)
				{
					if (camMod.useTransformAsPathOffset)
					{
						camPos += paths[camMod.pathIndex].getCurrPosition();
					}
					else
					{
						camPos = paths[camMod.pathIndex].getCurrPosition();
					}
				}
			}
		}

		vec3d upVec = { 0, 1, 0 };
		vec3d target = { 0, 0, 1 };

		if (!camOverridePathLookAt && !debugMode) //Use path target for lookat
		{
			camDir = (pathLookAtTarget - camPos).normalized();

			float toCamTilt = (asin(-camDir.y));
			float toCamYaw = (atan2(camDir.z, camDir.x) - PI/2);

			//Modulus to [0, TWO_PI)
			toCamYaw = toCamYaw - TWO_PI*floor(toCamYaw / TWO_PI);
			camYaw = camYaw - TWO_PI*floor(camYaw / TWO_PI);

			camYaw = modLerp(camYaw, toCamYaw, min(1.0f, 3*fElapsedTime), TWO_PI);
			camTilt = lerp(camTilt, toCamTilt, min(1.0f, 3*fElapsedTime));
		}

		mat4x4 camRot = CalculateRotationXMatrix(camTilt);
		camRot *= CalculateRotationYMatrix(camYaw);
		camDir = target * camRot;

		target = camPos + camDir;

		matCam = PointAtMatrix(camPos, target, upVec);
		matView = QuickInverseMatrix(matCam);


	
		//Calculate triangles for drawing
		vector<triangle> trisToRaster;

		#pragma region Draw Meshes
		//Cycle through each mesh
		for (mesh &m : meshes)
		{
			if (m.modifier != -1) //Apply modifier to mesh
			{
				modifier& mod = modifiers[m.modifier];

				vec3d dpos = m.position;

				vec3d& constRot = mod.constantRotation;
				//CONSTANT ROTATION
				matTrans  = CalculateRotationXMatrix(m.rotation.x + timePassed*constRot.x);
				matTrans *= CalculateRotationYMatrix(m.rotation.y + timePassed*constRot.y);
				matTrans *= CalculateRotationZMatrix(m.rotation.z + timePassed*constRot.z);

				//FOLLOW PATH
				if (mod.pathIndex != -1)
				{
					if (mod.useTransformAsPathOffset)
					{
						dpos += paths[mod.pathIndex].getCurrPosition();
					}
					else
					{
						m.setPos(paths[mod.pathIndex].getCurrPosition());
						vec3d lookPos = paths[mod.pathIndex].getCurrPosition(1.0f);

						if (mod.applyPathRotation)
						{
							matTrans *= CalculateRotationYMatrix(PI);
							matTrans *= PointAtMatrix(vec3d(), (m.position - lookPos), upVec);
						}
					}
				}


				//BILLBOARD
				if (mod.isBillboard)
				{
					matTrans *= PointAtMatrix(vec3d(), (m.position - camPos + camDir), upVec);
				}

				//Vertices are origin-based, centered at <0, 0, 0>, so only translate them at the end, after all the rotations
				matTrans *= CalculateTranslationMatrix(dpos.x, dpos.y, dpos.z);

			}
			else //Apply basic mesh transformations
			{
				matTrans  = CalculateRotationXMatrix(m.rotation.x);
				matTrans *= CalculateRotationYMatrix(m.rotation.y);
				matTrans *= CalculateRotationZMatrix(m.rotation.z);
				matTrans *= CalculateTranslationMatrix(m.position.x, m.position.y, m.position.z);
			}


			for (const triangle &tri : m.tris)
			{
				// World Transform > View Space > Projection Space
				triangle triTrans, triViewed, triProj;

				//===== TRANSFORM =====
				//Apply transformation matrix to triangle
				for (int v = 0; v < 3; v++)
				{
					triTrans.p[v] = tri.p[v] * matTrans;
					triTrans.t[v] = tri.t[v];
				}
				triTrans.matIndex = tri.matIndex;

				//Establish vectors for 2 sides of the triangle
				vec3d normal, line1, line2;
				line1 = triTrans.p[1] - triTrans.p[0];
				line2 = triTrans.p[2] - triTrans.p[0];

				//Compute surface normal
				normal = line1.cross(line2);
				normal = normal.normalized();

				vec3d diff = triTrans.p[0] - camPos;
				float dot = normal.dot(diff);

				//Triangle is facing camera (normal pointing towards where camera is)
				if (dot < 0.0f)
				{
					//===== CALCULATE LIGHTING ===== //TODO: Move to DrawTriangle function
					vec3d dirLight = camDir * -1;// { 0, 1, -1 };
						dirLight = dirLight.normalized();

					//Calculate dot product between light and surface normal; This gives the light intensity on the surface
					float lightDot = normal.dot(dirLight);
					//materials.at(0).col = WHITE * fmax(0.1f, lightDot); //ERROR: Calculate this when drawing triangle
					//triTrans.mat.col = WHITE * fmax(0.1f, lightDot);


					//===== WORLD SPACE -> VIEW SPACE =====
						//View is calculated based on camera position
					for (int v = 0; v < 3; v++)
					{
						triViewed.p[v] = triTrans.p[v] * matView;
						triViewed.t[v] = triTrans.t[v];
					}
					triViewed.matIndex = triTrans.matIndex;

					//===== DISTANCE CLIPPING =====

					//Clip viewed triangle against near plane
					triangle clipped[2];

					//Populate clipped[]
						//Coords are in View Space; so are relative to camera position and direction
					int numClippedTris = ClipAgainstPlane({ 0.0f, 0.0f, 0.1f }, { 0.0f, 0.0f, 1.0f }, triViewed, clipped[0], clipped[1]);

					//The clip may produce up to 2 resultant triangles
					for (int n = 0; n < numClippedTris; n++)
					{
						//===== VIEW SPACE -> PROJECTION SPACE =====
							//Converts clipped 3D triangles to 2D projection
						for (int v = 0; v < 3; v++)
						{
							triProj.p[v] = clipped[n].p[v] * matProj;
							triProj.t[v] = clipped[n].t[v];
						}
						triProj.matIndex = clipped[n].matIndex;

						//UV Perspective Correction
						for (int v = 0; v < 3; v++)
						{
							triProj.t[v] /= triProj.p[v].w;
							triProj.t[v].w = 1.0f / triProj.p[v].w;
						}

						//Scale into view
						for (int v = 0; v < 3; v++)
						{
							if (triProj.p[v].w != 0.0f)
							{
								triProj.p[v] /= triProj.p[v].w;
							}
						}

						//x/y are inverted, so put them back
						for (int v = 0; v < 3; v++)
						{
							//triProj.p[v] = triProj.p[v].normalized();
							triProj.p[v].x *= -1;
							triProj.p[v].y *= -1;
						}

						//===== PROJECTION SPACE -> SCREEN SPACE =====
							//Scale projection to screen dimensions
						vec3d offsetView = { 1, 1, 0 };
						for (int v = 0; v < 3; v++)
						{
							triProj.p[v] += offsetView;
						}

						for (int i = 0; i < 3; i++)
						{
							triProj.p[i].x *= 0.5f * screenW;
							triProj.p[i].y *= 0.5f * screenH;
						}

						//Load screen space coords into list for rasterization
						trisToRaster.push_back(triProj);
					}
				}
			}
		}
		#pragma endregion

		for (triangle& triToRaster : trisToRaster)
		{
			#pragma region SCREEN CLIPPING
			//Clip triangles against all four screen edges
			triangle clipped[2];
			list<triangle> tris;
			tris.push_back(triToRaster);
			int newTris = 1;

			for (int p = 0; p < 4; p++)
			{
				int trisToAdd = 0;
				while (newTris > 0)
				{
					triangle test = tris.front();
					tris.pop_front();
					newTris--;

					//Clip in Screen Space along each of the screen border planes
					switch (p)
					{
					case 0:
						trisToAdd = ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					case 1:
						trisToAdd = ClipAgainstPlane({ 0.0f, (float)screenH-1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					case 2:
						trisToAdd = ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					case 3:
						trisToAdd = ClipAgainstPlane({ (float)screenW-1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]);
						break;
					}

					//Clipping may yield many triangles, which must be added to the back of the queue
					//to be tested for clipping against the other sides of the screen border
					for (int t = 0; t < trisToAdd; t++)
					{
						tris.push_back(clipped[t]);
					}
				}
				newTris = tris.size();
			}
			#pragma endregion

			#pragma region RASTERIZE TRIANGLES
			for (triangle& t : tris)
			{

				if (materials[t.matIndex].textureIndex == -1) //Use solid material color
				{
					ColouredTriangle(t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w,
									 t.p[1].x, t.p[1].y, t.t[1].u, t.t[1].v, t.t[1].w,
									 t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w,
									 materials[t.matIndex].col, ge);
				//	TexturedTriangle(t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w,
				//					 t.p[1].x, t.p[1].y, t.t[1].u, t.t[1].v, t.t[1].w,
				//					 t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w,
				//					 0, ge);
				}
				else										  //Use material texture
				{
						TexturedTriangle(t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w,
										 t.p[1].x, t.p[1].y, t.t[1].u, t.t[1].v, t.t[1].w,
										 t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w,
										 t, ge);
				}


				//Center points
				vec2d center(t.p[0].x + t.p[1].x + t.p[2].x, t.p[0].y + t.p[1].y + t.p[2].y);
				center /= 3;
				//ge->DrawString(center.x, center.y, to_string(t.matIndex), BLACK);
				//ge->DrawString(center.x, center.y, to_string(depthBuffer[(int)center.x*screenW + (int)center.y]), WHITE);
				//Wireframe
				//ge->DrawTriangle(t.p[0].x, t.p[0].y, t.p[1].x, t.p[1].y, t.p[2].x, t.p[2].y, GREEN);
			}

			//TODO: Particles
			//for(particle& p : particles) ...
			#pragma endregion
		}

		//Draw Info Points Text
		matTrans = IdentityMatrix();
		for (path& p : paths)
		{
			if (p.canMoveForward(-1) || p.currInfoPt == 0)
			{
				infoPoint& ip = p.infoPts[p.currInfoPt];

				vi2d screenPos;
				for (text& tx : ip.texts)
				{
					if (WorldToScreenSpace(tx.pos, screenPos, matTrans, matView, matProj))
					{
						float allScale = max(0.0f, 1.0f-(screenPos - vi2d(screenW/2, screenH/2)).mag2()/(float)vi2d(screenW/2, screenH/2).mag2());
						allScale *= allScale * 0.9f;

						vi2d titleSize = lato_bold->GetTextSizeProp(tx.title);
						titleSize = vi2d(titleSize.x * allScale, titleSize.y * allScale);

						vi2d descSize = azeret_mono->GetTextSizeProp(tx.description);
						descSize = vi2d(descSize.x * 0.25f * allScale, descSize.y * 0.25f * allScale);

						vi2d boxSize = vi2d(max(titleSize.x, descSize.x) + tx.borderSize.x, titleSize.y + descSize.y + tx.borderSize.y);
						vi2d screenPosShift = screenPos - boxSize / 2; //Center text around point

						ge->SetPixelMode(Pixel::ALPHA);
						ge->FillRect(screenPosShift.x, screenPosShift.y, boxSize.x, boxSize.y, Pixel(255, 255, 255, 230));
						ge->SetPixelMode(Pixel::NORMAL);

						//Title
						lato_bold->DrawStringPropDecal(screenPosShift + tx.borderSize/2, tx.title, BLACK, vf2d(allScale, allScale));

						//Description
						azeret_mono->DrawStringPropDecal(screenPosShift + vi2d(2, 0 + titleSize.y) + tx.borderSize/2, tx.description, BLACK, vf2d(0.25f * allScale, 0.25f * allScale));

						if (debugMode)
						{
							ge->Draw(screenPos, RED);
							ge->Draw(screenPosShift, BLUE);
						}

					}
				}
			}
		}

		//Info point continue text
		bool canMove = true;
		for (path& p : paths)
		{
			canMove &= !p.isMoving;
		}
		if (canMove)
		{
			vf2d txSize = azeret_mono->GetTextSizeProp("Press [Space] to continue, [RMB] to look around.");
			txSize *= 0.3f;
			azeret_mono->DrawStringPropDecal(vi2d(screenW/2 - txSize.x/2, screenH - 20 - txSize.y/2), "Press [Space] to continue, [RMB] to look around.", WHITE, {0.3f, 0.3f});
		}

		//Debug overlay
		if (debugMode)
		{
			//Draw path points
			int j = 1;
			for (path& p : paths)
			{
				Pixel debugCol = Pixel(j*127, 0, 255-j*127);
				//Path points
				int i = 0;
				for (vec3d& pt : p.pts)
				{
					matTrans = CalculateTranslationMatrix(p.position.x, p.position.y, p.position.z);

					vi2d ptToRaster;
					if (WorldToScreenSpace(pt, ptToRaster, matTrans, matView, matProj))
					{
						matTrans *= CalculateTranslationMatrix(pt.x, pt.y, pt.z);
						matTrans *= CalculateTranslationMatrix(p.position.x, p.position.y, p.position.z);

						ge->DrawRect(ptToRaster, { 2, 2 }, debugCol);
						ge->DrawString(ptToRaster + vi2d(3, 3), to_string(i), RED, 1);
					}
					i++;
				}

				matTrans = CalculateTranslationMatrix(p.position.x, p.position.y, p.position.z);
				//Info points
				for (infoPoint& ip : p.infoPts)
				{
					vi2d ptToRaster;
					if (WorldToScreenSpace(p.pts[ip.pathPtIndex], ptToRaster, matTrans, matView, matProj))
					{
						ge->DrawRect(ptToRaster, { 2, 2 }, GREEN);
						ge->DrawString(ptToRaster + vi2d(10, 10), to_string(ip.fov), GREEN, 1);
					}
				}

				matTrans = CalculateTranslationMatrix(p.position.x, p.position.y, p.position.z);

				//Current pos & Ahead pos on path
				vi2d currPosToRaster, aheadPosToRaster;
				vec3d currPos = p.getCurrPosition();
				vec3d aheadPos = p.getCurrPosition(1.0f);
				//Ahead pos
				if (WorldToScreenSpace(aheadPos, aheadPosToRaster, matTrans, matView, matProj))
				{
					ge->DrawLine(aheadPosToRaster + vi2d(3, -3), aheadPosToRaster + vi2d(-3, 3), CYAN);
					ge->DrawLine(aheadPosToRaster + vi2d(-3, -3), aheadPosToRaster + vi2d(3, 3), CYAN);
				}
				//Curr pos
				if (WorldToScreenSpace(currPos, currPosToRaster, matTrans, matView, matProj))
				{
					ge->DrawLine(currPosToRaster + vi2d(3, -3), currPosToRaster + vi2d(-3, 3), MAGENTA);
					ge->DrawLine(currPosToRaster + vi2d(-3, -3), currPosToRaster + vi2d(3, 3), MAGENTA);
					ge->DrawLine(currPosToRaster, currPosToRaster + 0.4f*(aheadPosToRaster-currPosToRaster), MAGENTA);
				}

				j++;
			}

			matTrans = IdentityMatrix();
			matCam = PointAtMatrix(camPos, target, upVec);
			matView = QuickInverseMatrix(matCam);

			//Axis indicator
			vec3d  axes[] = { vec3d(100, 0, 0), vec3d(0, 100, 0), vec3d(0, 0, 100) };
			Pixel  cols[] = { RED,			  GREEN,		  BLUE			 };
			string syms[] = { "X",			  "Y",			  "Z"			 };
			vi2d originToRaster;
			WorldToScreenSpace(vec3d(), originToRaster, matTrans, matView, matProj);
			for (int a = 0; a < 3; a++)
			{
				vi2d axisToRaster;
				WorldToScreenSpace(axes[a], axisToRaster, matTrans, matView, matProj);
				ge->DrawLine(originToRaster, axisToRaster, cols[a]);
				ge->DrawString(axisToRaster, syms[a], cols[a]);
			}

			//Depth Buffer
			for (int i = 0; i < screenH/4; i++)
			{
				for (int j = 0; j < screenW/4; j++)
				{
					ge->Draw(3*screenW/4 + j, 3*screenH/4 + i, WHITE*(1.0f/(debugDepthValue*depthBuffer[(4*i*screenW) + 4*j])));
				}
			}

			//Debug text
			string debugOutput = debugText + "\n FOV: " + to_string(camFOV) + "\n Curr Info Pts: [";
			for (path& p : paths)
			{
				debugOutput += to_string(p.currInfoPt) + ", ";
			}
			debugOutput += "]\n";

			arial->DrawStringPropDecal({ 5, 5 }, debugOutput, GREEN, {0.2, 0.2});
		}
	}

	inline void DrawTexturePixel(PixelGameEngine* ge, float uTex, float vTex, float wTex, int i, int j, texture& tex, triangle& tri)
	{
		//TOP
		float mipScale = materials[tri.matIndex].mipScale;
		//int m = floor(max(0.0f, min(tex.numMips - 1.0f, 0.5f*(mipLogA - log2(tex.numMips * wTex)))));
		//m = tex.numMips - 1;
		int m = max(0, min(tex.numMips - 1, tex.numMips - (int)(1000 * wTex))); //Needs adjustment
		Pixel p;

		float tx = (uTex / wTex);
		float ty = (1.0f - vTex / wTex);

		tx -= floor(tx);
		ty -= floor(ty);

		if (materials[tri.matIndex].startIndex < materials[tri.matIndex].endIndex) //Use animated texture
		{
			int numFrames = materials[tri.matIndex].endIndex - materials[tri.matIndex].startIndex + 1;
			int frameIndex = (int)floor(timePassed * materials[tri.matIndex].animSpeed) % (numFrames);

			float frameW = 1.0f / materials[tri.matIndex].xDivisions;
			float frameH = 1.0f / materials[tri.matIndex].yDivisions;

			tx = tx / materials[tri.matIndex].xDivisions + frameW * (frameIndex % materials[tri.matIndex].xDivisions);
			ty = ty / materials[tri.matIndex].yDivisions + frameH * (frameIndex / materials[tri.matIndex].yDivisions);

			p = tex.mips[m]->Sample(tx, ty);
			ge->Draw(j, i, p);
		}
		else //Use normal texture
		{
			p = tex.mips[m]->Sample(tx, ty);
			//p = m % 2 == 0 ? olc::BLACK : olc::WHITE; //View mips as stripes
			ge->Draw(j, i, p);
		}

		//Write depth
		if(round(p.a/255.0f))
		depthBuffer[i * screenW + j] = wTex;
	}

	void TexturedTriangle(int x1, int y1, float u1, float v1, float w1,
						  int x2, int y2, float u2, float v2, float w2,
						  int x3, int y3, float u3, float v3, float w3,
						  triangle& tri, PixelGameEngine* ge)
	{
		bool useAlpha = (materials[tri.matIndex].alphaIndex != -1);
		ge->SetPixelMode(useAlpha ? Pixel::ALPHA : Pixel::NORMAL);

		texture& tex = textures[materials[tri.matIndex].textureIndex];



		//Sort arguments by y-position
		if (y2 < y1)
		{
			swap(y1, y2);
			swap(x1, x2);
			swap(u1, u2);
			swap(v1, v2);
			swap(w1, w2);
		}
		if (y3 < y1)
		{
			swap(y1, y3);
			swap(x1, x3);
			swap(u1, u3);
			swap(v1, v3);
			swap(w1, w3);
		}
		if (y3 < y2)
		{
			swap(y2, y3);
			swap(x2, x3);
			swap(u2, u3);
			swap(v2, v3);
			swap(w2, w3);
		}

		#pragma region DRAW TOP OF TRIANGLE

		int dy1 = y2 - y1;
		int dx1 = x2 - x1;
		float dv1 = v2 - v1;
		float du1 = u2 - u1;
		float dw1 = w2 - w1;

		int dy2 = y3 - y1;
		int dx2 = x3 - x1;
		float dv2 = v3 - v1;
		float du2 = u3 - u1;
		float dw2 = w3 - w1;

		float uTex, vTex, wTex;

		float daxStep = 0, dbxStep = 0,
			  du1Step = 0, dv1Step = 0,
			  du2Step = 0, dv2Step = 0,
			  dw1Step = 0, dw2Step = 0;

		if (dy1) { daxStep = dx1 / (float)abs(dy1); }
		if (dy2) { dbxStep = dx2 / (float)abs(dy2); }

		if (dy1) { du1Step = du1 / (float)abs(dy1); }
		if (dy1) { dv1Step = dv1 / (float)abs(dy1); }
		if (dy1) { dw1Step = dw1 / (float)abs(dy1); }

		if (dy2) { du2Step = du2 / (float)abs(dy2); }
		if (dy2) { dv2Step = dv2 / (float)abs(dy2); }
		if (dy2) { dw2Step = dw2 / (float)abs(dy2); }

		if (dy1)
		{
			for (int i = y1; i <= y2; i++)
			{
				float step = (float)i - y1;

				//Calculate vertex position along the edge
				int ax = x1 + step * daxStep;
				int bx = x1 + step * dbxStep;

				//Start UVs
				float suTex = u1 + step * du1Step;
				float svTex = v1 + step * dv1Step;
				float swTex = w1 + step * dw1Step;

				//End UVs
				float euTex = u1 + step * du2Step;
				float evTex = v1 + step * dv2Step;
				float ewTex = w1 + step * dw2Step;

				//Sort along x-axis
				if (ax > bx)
				{
					swap(ax, bx);
					swap(suTex, euTex);
					swap(svTex, evTex);
					swap(swTex, ewTex);
				}

				uTex = suTex;
				vTex = svTex;
				wTex = swTex;

				float tStep = 1.0f / ((float)(bx - ax));
				float tLerp = 0.0f;

				for (int j = ax; j < bx; j++)
				{
					uTex = (1.0f - tLerp) * suTex + tLerp * euTex;
					vTex = (1.0f - tLerp) * svTex + tLerp * evTex;
					wTex = (1.0f - tLerp) * swTex + tLerp * ewTex;

					//For some reason the image gets flipped vertically,
					//so just do (1.0f - (v-coord)) to counteract this.
					if (wTex > depthBuffer[i * screenW + j])
					{
						DrawTexturePixel(ge, uTex, vTex, wTex, i, j, tex, tri);
					}

					tLerp += tStep;
				}
			}
		}
#pragma endregion

		#pragma region DRAW BOTTOM OF TRIANGLE

		//Update gradients
		dx1 = x3 - x2;
		dy1 = y3 - y2;
		du1 = u3 - u2;
		dv1 = v3 - v2;
		dw1 = w3 - w2;

		if (dy1) { daxStep = dx1 / (float)abs(dy1); }
		if (dy2) { dbxStep = dx2 / (float)abs(dy2); }

		du1Step = dv1Step = dw1Step = 0;
		if (dy1) { du1Step = du1 / (float)abs(dy1); }
		if (dy1) { dv1Step = dv1 / (float)abs(dy1); }
		if (dy1) { dw1Step = dw1 / (float)abs(dy1); }

		if (dy1)
		{
			for (int i = y2; i <= y3; i++)
			{
				float step1 = (float)i - y1;
				float step2 = (float)i - y2;

				//Calculate vertex position along the edge
				int ax = x2 + step2 * daxStep;
				int bx = x1 + step1 * dbxStep;

				//Start UVs
				float suTex = u2 + step2 * du1Step;
				float svTex = v2 + step2 * dv1Step;
				float swTex = w2 + step2 * dw1Step;

				//End UVs
				float euTex = u1 + step1 * du2Step;
				float evTex = v1 + step1 * dv2Step;
				float ewTex = w1 + step1 * dw2Step;

				//Sort along x-axis
				if (ax > bx)
				{
					swap(ax, bx);
					swap(suTex, euTex);
					swap(svTex, evTex);
					swap(swTex, ewTex);
				}

				uTex = suTex;
				vTex = svTex;
				wTex = swTex;

				float tStep = 1.0f / (float)(bx - ax);
				float tLerp = 0.0f;

				for (int j = ax; j < bx; j++)
				{
					uTex = (1.0f - tLerp) * suTex + tLerp * euTex;
					vTex = (1.0f - tLerp) * svTex + tLerp * evTex;
					wTex = (1.0f - tLerp) * swTex + tLerp * ewTex;

					//BOTTOM
					if (wTex > depthBuffer[i * screenW + j])
					{
						DrawTexturePixel(ge, uTex, vTex, wTex, i, j, tex, tri);

						//int m = floor(max(0.0f, min(tex.numMips - 1.0f, 0.5f*(mipLogA - log2(tex.numMips * wTex)))));

						//Pixel p = tex.mips[m]->Sample(uTex / wTex, 1.0f - vTex / wTex);
						//ge->Draw(j, i, p);
						////ge->Draw(j, i, tex.mips[0]->Sample(exp2(m) * uTex / wTex, 1.0f - exp2(m)*vTex / wTex));

						////ge->Draw(j, i, WHITE * (0.01f / wTex));

						//if(round(p.a/255.0f))
						//depthBuffer[i * screenW + j] = wTex;
					}

					tLerp += tStep;
				}
			}
		}
#pragma endregion

	}

	void ColouredTriangle(int x1, int y1, float u1, float v1, float w1,
		int x2, int y2, float u2, float v2, float w2,
		int x3, int y3, float u3, float v3, float w3,
		Pixel col, PixelGameEngine* ge)
	{
		//Sort arguments by y-position
		if (y2 < y1)
		{
			swap(y1, y2);
			swap(x1, x2);
			swap(u1, u2);
			swap(v1, v2);
			swap(w1, w2);
		}
		if (y3 < y1)
		{
			swap(y1, y3);
			swap(x1, x3);
			swap(u1, u3);
			swap(v1, v3);
			swap(w1, w3);
		}
		if (y3 < y2)
		{
			swap(y2, y3);
			swap(x2, x3);
			swap(u2, u3);
			swap(v2, v3);
			swap(w2, w3);
		}

		#pragma region DRAW TOP OF TRIANGLE

		int dy1 = y2 - y1;
		int dx1 = x2 - x1;
		float dv1 = v2 - v1;
		float du1 = u2 - u1;
		float dw1 = w2 - w1;

		int dy2 = y3 - y1;
		int dx2 = x3 - x1;
		float dv2 = v3 - v1;
		float du2 = u3 - u1;
		float dw2 = w3 - w1;

		float uTex, vTex, wTex;

		float daxStep = 0, dbxStep = 0,
			du1Step = 0, dv1Step = 0,
			du2Step = 0, dv2Step = 0,
			dw1Step = 0, dw2Step = 0;

		if (dy1) { daxStep = dx1 / (float)abs(dy1); }
		if (dy2) { dbxStep = dx2 / (float)abs(dy2); }

		if (dy1) { du1Step = du1 / (float)abs(dy1); }
		if (dy1) { dv1Step = dv1 / (float)abs(dy1); }
		if (dy1) { dw1Step = dw1 / (float)abs(dy1); }

		if (dy2) { du2Step = du2 / (float)abs(dy2); }
		if (dy2) { dv2Step = dv2 / (float)abs(dy2); }
		if (dy2) { dw2Step = dw2 / (float)abs(dy2); }

		if (dy1)
		{
			for (int i = y1; i <= y2; i++)
			{
				float step = (float)i - y1;

				//Calculate vertex position along the edge
				int ax = x1 + step * daxStep;
				int bx = x1 + step * dbxStep;

				//Start UVs
				float suTex = u1 + step * du1Step;
				float svTex = v1 + step * dv1Step;
				float swTex = w1 + step * dw1Step;

				//End UVs
				float euTex = u1 + step * du2Step;
				float evTex = v1 + step * dv2Step;
				float ewTex = w1 + step * dw2Step;

				//Sort along x-axis
				if (ax > bx)
				{
					swap(ax, bx);
					swap(suTex, euTex);
					swap(svTex, evTex);
					swap(swTex, ewTex);
				}

				uTex = suTex;
				vTex = svTex;
				wTex = swTex;

				float tStep = 1.0f / ((float)(bx - ax));
				float tLerp = 0.0f;

				for (int j = ax; j < bx; j++)
				{
					uTex = (1.0f - tLerp) * suTex + tLerp * euTex;
					vTex = (1.0f - tLerp) * svTex + tLerp * evTex;
					wTex = (1.0f - tLerp) * swTex + tLerp * ewTex;

					//For some reason the image gets flipped vertically,
					//so just do (1.0f - (v-coord)) to counteract this.
					if (wTex > depthBuffer[i * screenW + j])
					{
						ge->Draw(j, i, col);
						depthBuffer[i * screenW + j] = wTex;
						//LitPixel(i, j, uTex, vTex, wTex, tex, t, ge);
						//depthBuffer[i * screenW + j] = wTex;
					}

					tLerp += tStep;
				}
			}
		}
#pragma endregion

#pragma region DRAW BOTTOM OF TRIANGLE

		//Update gradients
		dx1 = x3 - x2;
		dy1 = y3 - y2;
		du1 = u3 - u2;
		dv1 = v3 - v2;
		dw1 = w3 - w2;

		if (dy1) { daxStep = dx1 / (float)abs(dy1); }
		if (dy2) { dbxStep = dx2 / (float)abs(dy2); }

		du1Step = dv1Step = dw1Step = 0;
		if (dy1) { du1Step = du1 / (float)abs(dy1); }
		if (dy1) { dv1Step = dv1 / (float)abs(dy1); }
		if (dy1) { dw1Step = dw1 / (float)abs(dy1); }

		if (dy1)
		{
			for (int i = y2; i <= y3; i++)
			{
				float step1 = (float)i - y1;
				float step2 = (float)i - y2;

				//Calculate vertex position along the edge
				int ax = x2 + step2 * daxStep;
				int bx = x1 + step1 * dbxStep;

				//Start UVs
				float suTex = u2 + step2 * du1Step;
				float svTex = v2 + step2 * dv1Step;
				float swTex = w2 + step2 * dw1Step;

				//End UVs
				float euTex = u1 + step1 * du2Step;
				float evTex = v1 + step1 * dv2Step;
				float ewTex = w1 + step1 * dw2Step;

				//Sort along x-axis
				if (ax > bx)
				{
					swap(ax, bx);
					swap(suTex, euTex);
					swap(svTex, evTex);
					swap(swTex, ewTex);
				}

				uTex = suTex;
				vTex = svTex;
				wTex = swTex;

				float tStep = 1.0f / (float)(bx - ax);
				float tLerp = 0.0f;

				for (int j = ax; j < bx; j++)
				{
					uTex = (1.0f - tLerp) * suTex + tLerp * euTex;
					vTex = (1.0f - tLerp) * svTex + tLerp * evTex;
					wTex = (1.0f - tLerp) * swTex + tLerp * ewTex;

					if (wTex > depthBuffer[i * screenW + j])
					{
						ge->Draw(j, i, col);
						depthBuffer[i * screenW + j] = wTex;
					}

					tLerp += tStep;
				}
			}
		}
#pragma endregion
	}
};
