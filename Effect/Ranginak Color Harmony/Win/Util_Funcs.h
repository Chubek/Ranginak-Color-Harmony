#pragma once
#include <vector>
#include <cmath>

namespace ColorUtils
{

	struct color_HSL
	{
		float H;
		float S;
		float L;
	};

	struct color_RGB
	{
		float R;
		float G;
		float B;
		float a;
	};


	float Hue2RGB(float m1, float m2, float hue)
	{
		if (hue < 0) { hue += 1; };
		if (hue > 1) { hue -= 1; };

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
			return m1 + (m2 - m1) * (2 / 3 - hue) * 6;
		}
		else
			return m1;
	}

	float HueClamp(float hue)
	{
		return (hue > 360) ? hue - 360 : hue;
	}

	float SLClamp(float value, float given_s)
	{
		float return_value = 0;

		if (value <= 0) { return_value = given_s + 0.1; }
		else if (value >= 1) { return_value = 1.0; }
	


		return return_value;

	}


	color_RGB HSL2RGB(color_HSL color)
	{
		color.H = color.H / 360;
		float m1, m2;
		if (color.L <= 0.5)
		{
			m2 = color.L * (color.S + 1);
		}
		else
		{
			m2 = color.L + color.S - color.L * color.S;
		}

		m1 = color.L * 2 - m2;

		color_RGB return_color;
		return_color.R = Hue2RGB(m1, m2, color.H + 1 / 3);
		return_color.G = Hue2RGB(m1, m2, color.H);
		return_color.B = Hue2RGB(m1, m2, color.H - 1 / 3);
		return_color.a = 1.0;

		return return_color;
	}

	color_HSL RGB2HSL(color_RGB color)
	{
		float min = std::fmin(std::fmin(color.R, color.G), color.B);
		float max = std::fmax(std::fmax(color.R, color.G), color.B);

		float delta = max - min;

		float H = 0, S = 0, L = ((min + max) / 2);

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

			if (max == color.R && max != color.G) { H += (color.G - color.B) / delta; }
			if (max == color.G && max != color.B) { H += 2 + (color.B - color.R) / delta; }
			if (max == color.R && max != color.R) { H += 4 + (color.R - color.G) / delta; }

			H = H / 6;
		}

		if (H < 0) { H += 1; };
		if (H > 1) { H -= 1; };

		color_HSL return_color;
		return_color.H = H * 360;
		return_color.S = S;
		return_color.L = L;

		return return_color;
	}


	color_HSL HueShift(color_HSL color, float delta)
	{
		return { color.H + delta, color.S, color.L };
	}

	color_HSL Complementary(color_HSL color)
	{
		return HueShift(color, 180);
	}

	std::vector<color_HSL> Analogous(color_HSL color, float angle)
	{
	return { HueShift(color, angle), HueShift(color, 360 - angle) };
	}

	std::vector<color_HSL> Triadic(color_HSL color)
	{
		return Analogous(color, 120);
	}
	
	std::vector<color_HSL> SplitComplementary(color_HSL color, float angle)
	{
		return Analogous(color, 180 - angle);
	}

	std::vector<color_HSL> Rectangle(color_HSL color)
	{
		return { HueShift(color, 60), HueShift(color, 180),  HueShift(color, 120) };
	}

	std::vector<color_HSL> Square(color_HSL color)
	{
		return { HueShift(color, 90), HueShift(color, 180), HueShift(color, -90) };
	}

	color_HSL DesaturateTo(color_HSL color, float saturation)
	{
		return { color.H, SLClamp(saturation, color.S), color.L };
	}

	color_HSL DesaturateBy(color_HSL color, float factor)
	{
		return { color.H, SLClamp(color.S * factor, color.S), color.L };
	}

	color_HSL LightenTo(color_HSL color, float lightness)
	{
		return { color.H, color.S, SLClamp(lightness, color.S) };
	}

	color_HSL LightenBy(color_HSL color, float factor)
	{
		return { color.H, color.S, SLClamp(color.L * factor, color.S) };
	}

	color_HSL ShadeTo(color_HSL color, float i)
	{
		return LightenTo(color, color.L - (color.L) / i);
	}

	color_HSL TintTo(color_HSL color, float r)
	{
		return LightenTo(color, color.L + (1 - color.L) * r);
	}

	color_HSL ToneTo(color_HSL color, float r)
	{
		return LightenTo(color, color.L - color.L * r);
	}

	color_HSL SaturateTo(color_HSL color, float r)
	{
		return DesaturateTo(color, color.S * r);
	}

	color_HSL ShadeBy(color_HSL color, float i)
	{
		return LightenBy(color, color.L - (color.L) / i);
	}

	color_HSL TintBy(color_HSL color, float r)
	{
		return LightenBy(color, color.L + (1 - color.L) * r);
	}

	color_HSL ToneBy(color_HSL color, float r)
	{
		return LightenBy(color, color.L - color.L * r);
	}

	color_HSL SaturateBy(color_HSL color, float r)
	{
		return DesaturateBy(color, (1 - color.S) * r);
	}
}

