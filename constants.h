#pragma once
#include "../olcPixelGameEngine/olcPixelGameEngine.h"

using namespace olc;

namespace cv
{
	static const double PI = 3.14159265358979323846;
	static const double TWO_PI = 2 * PI;
	static const Pixel
		//c_DarkBackground(8, 9, 15),
		c_DarkBackground(0),
		c_MagentaPink(217, 20, 204),
		c_NeonGreen(204, 255, 0), c_HalfWhite(255, 255, 255, 127);

	static float lerp(float val, float fromMin, float fromMax, float toMin, float toMax)
	{
		return (toMax - toMin) * (val - fromMin) / (fromMax - fromMin) + toMin;
	}

	static float lerp(float from, float to, float t)
	{
		return (to - from) * t + from;
	}

	// Lerps through shortest modulo distance
	// Assumes all inputs are positive
	static float modLerp(float from, float to, float t, float mod)
	{
		const float min = std::fmin(from, to);
		const float max = std::fmax(from, to);

		if (std::abs(to - from) < min + std::abs(mod - max)) {
			// Perform normal lerp (no wrap-around)
			return lerp(from, to, t);
		}
		else {
			// Wrap-around lerp
			float res = 0;
			if (from < to)	{ res = lerp(from + mod, to, t); }
			else			{ res = lerp(from, to + mod, t); }
			if (res >= mod) res -= mod;
			return res;
		}
	}

	static float PixelDotProduct(const Pixel &a, const Pixel &b)
	{
		return a.r*b.r + a.g*b.g + a.b*b.b;
	}

	static Pixel PixelCrossProduct(const Pixel &a, const Pixel &b)
	{
		return Pixel(
					a.g*b.b - b.g*a.b,
					a.b*b.r - b.b*a.r,
					a.r*b.g - b.r*a.g
		);
	}

	static Pixel HueShift(const Pixel &in, float H)
	{
		const Pixel k(0.57735, 0.57735, 0.57735);
		float cosAngle = cosf(H);
		return Pixel(in * cosAngle + PixelCrossProduct(k, in) * sin(H) + k * PixelDotProduct(k, in) * (1.0 - cosAngle));
	}

}
