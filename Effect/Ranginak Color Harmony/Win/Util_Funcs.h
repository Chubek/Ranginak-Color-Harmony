#pragma once
#include <vector>
#include <cmath>

namespace cColorUtils
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

	float HueClamp(float hue)
	{
		return (hue > 360) ? hue - 360 : hue;
	}

	float SLClamp(float value)
	{

		value = (value > 1.0) ? value = 1.0 : value;
		value = (value < 0.01) ? value = 0.01 : value;

		return value;

	}

	color_type HSL2RGB(float H, float S, float L)
	{
		H = HueClamp(H) / 360;
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



	color_type HueShift(color_type color, float delta)
	{
		return { color[0] + delta, color[1], color[2] };
	}

	color_type Complementary(color_type color)
	{
		return HueShift(color, 180);
	}

	std::vector<color_type> Analogous(color_type color, float angle)
	{
	return { HueShift(color, angle), HueShift(color, 360 - angle) };
	}

	std::vector<color_type> Triadic(color_type color)
	{
		return Analogous(color, 120)
	}
	
	std::vector<color_type> SplitComplementary(color_type color, float angle)
	{
		return Analogous(color, 180 - angle);
	}

	std::vector<color_type> Rectangle(color_type color)
	{
		return { HueShift(color, 60), HueShift(color, 180),  HueShift(color, 120)};
	}

	std::vector<color_type> Square(color_type color)
	{
		return { HueShift(color, 90), HueShift(color, 180), HueShift(color, -90) };
	}

	color_type DesaturateTo(color_type color, float saturation)
	{
		return { color[0], saturation, color[2] };
	}

	color_type DesaturateBy(color_type color, float factor)
	{
		return { color[0], SLClamp(color[1] * factor), color[2] };
	}

	color_type LightenTo(color_type color, float lightness)
	{
		return { color[0], color[1], lightness };
	}

	color_type LightenBy(color_type color, float factor)
	{
		return { color[0], color[1], SLClamp(color[2] * factor) };
	}


}

