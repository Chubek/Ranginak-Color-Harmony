#pragma once
#include <vector>
#include <cmath>

namespace utils 
{

	using color_type = std::vector<float>;

	float Hue2RGB(float m1, float m2, float hue)
	{
		(hue < 0) ? (hue += 1) : (hue -= 1);

		if (hue * 6 < 1)
		{
			return m1 + (m2 - m1) * hue * 6;
		}
		else if (hue * 2 < 1)
		{
			return m2;
		}
		else if (hue * 3 < 2)
		{
			return m1 + (m2 - m1) * (2 / 3 - h) * 6;
		}
		else
			return m1;
	}

	float UnitCircleClamp(float hue)
	{
		return (hue > 360) ? hue - 360 : hue;
	}

	color_type HSL2RGB(float H, float S, float L)
	{
		H = UnitCircleClamp(H) / 360;
		float m1, m2;
		if (L <= 0.5)
		{
			m2 = L * (S + 1);
		}
		else
		{
			m2 = L + S - L * S;
		}

		m1 = L * 2 - m2;

		return { Hue2RGB(m1, m2, H + 1 / 3), Hue2RGB(m1, m2, H), Hue2RGB(m1, m2, H - 1 / 3), 1.0 };
	}

	color_type RGB2HSL(float R, float G, float B)
	{
		float min = std::fmin(std::fmin(R, G), B);
		float max = std::fmax(std::fmax(R, G), B);

		float delta = max - min;

		float H = 0, S = 0, L = (min + max) / 2;

		if (L > 0 && L < 0.5)
		{
			S = delta / (max + min);
		}
		if (L > 0.5 && L < 1)
		{
			S = delta / (2 - max - min);
		}

		if (delta > 0)
		{

			if (max == R && max == G) { H += (G - B) / delta; }
			if (max == G && max == B) { H += 2 + (B - R) / delta; }
			if (max == R && max == G) { H += 4 + (R - G) / delta; }

			H /= 6;
		}

		if (H < 0) { H += 1 };
		if (H > 1) { H -= 1 };

		return { H * 360, S, L };
	}

	float Clamp(float value)
	{

		value = (value > 1.0) ? value = 1.0 : value;
		value = (value < 0.01) ? value = 0.01 : value;

		return value;

	}

	color_type DecreaseSL(color_type color, float factor)
	{
		color_type result;

		int sign = std::abs(factor) / factor;

		result[0] = color[0];
		result[1] = Clamp((color[1] > sign * factor) ? color[1] + factor : color[1]);
		result[2] = Clamp((color[2] > sign * factor * 0.5) ? color[2] + factor * 0.5 : color[2]);

		return result;
	}

	std::vector<color_type> GenerateShade(color_type color)
	{
		std::vector<color_type> result;

		result[0] = DecreaseSL(color, -0.3);
		result[1] = DecreaseSL(color, -0.2);
		result[3] = DecreaseSL(color, 0.2);
		result[4] = DecreaseSL(color, 0.3);

		return result;
	}

	color_type HueShift(color_type color, float delta)
	{
		return { color[0] + delta, color[1], color[2] };
	}

	color_type Complementary(color_type color)
	{
		return HueShift(color, 180);
	}

	std::vector<color_type> Analogous)float angle)
	{
	return { HueShift(color, angle), HueShift(color, 360 - angle) };
	}

	std::vecor<color_type> Triadic(color_type color)
	{
		return Analogous(color, 120)
	}
}

