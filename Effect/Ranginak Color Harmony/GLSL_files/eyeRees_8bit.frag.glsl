#version 330
uniform sampler2D videoTexture;
uniform float sliderVal;
uniform float multiplier16bit;
in vec4 out_pos;
in vec2 out_uvs;
out vec4 colourOut;
uniform vec4 main_color;
uniform vec4 complement_color_1;
uniform vec4 complement_color_2;
uniform vec4 complement_color_3;
uniform vec4 complement_color_4;
uniform vec4 factor_1_1_color;
uniform vec4 factor_1_2_color;
uniform vec4 factor_1_3_color;
uniform vec4 factor_1_4_color;
uniform vec4 sub_1_1_color;
uniform vec4 sub_1_2_color;
uniform vec4 sub_1_3_color;
uniform vec4 sub_1_4_color;
uniform vec4 factor_2_1_color;
uniform vec4 factor_2_2_color;
uniform vec4 factor_2_3_color;
uniform vec4 factor_2_4_color;
uniform vec4 sub_2_1_color;
uniform vec4 sub_2_2_color;
uniform vec4 sub_2_3_color;
uniform vec4 sub_2_4_color;
uniform vec2 resolution;
uniform float factor;
uniform float angle;


vec3 drawRect(in vec2 st,
              in vec2 center,
              in float width,
              in float height,
              in float thickness,
              in vec3 fillColor, 
              in vec3 strokeColor)
{
    vec3 color = vec3(0);
    
    float halfWidth = width * .5;
    float halfHeight = height * .5;
    float halfTickness = thickness * .5;
    
    vec2 bottomLeft = vec2(center.x - halfWidth, center.y - halfHeight);
    vec2 topRight = vec2(center.x + halfWidth, center.y + halfHeight);
    
    
    vec2 stroke = vec2(0.0);
    stroke += step(bottomLeft-halfTickness, st) * (1.0 - step(bottomLeft+halfTickness, st));
    stroke += step(topRight-halfTickness, st) * (1.0 - step(topRight+halfTickness, st));
    vec2 strokeLimit = step(bottomLeft-halfTickness, st) * (1.0 - step(topRight+halfTickness, st));
    stroke *= strokeLimit.x * strokeLimit.y;

    color = mix (color, strokeColor, min(stroke.x + stroke.y, 1.0));
    
    
    
    vec2 fill = vec2(0.0);
    fill += step(bottomLeft+halfTickness, st) * (1.0 - step(topRight-halfTickness, st));
    vec2 fillLimit = step(bottomLeft+halfTickness, st) * (1.0 - step(topRight-halfTickness, st));
    fill *=  fillLimit.x * fillLimit.y;
    
    color = mix (color, fillColor, min(fill.x + fill.y, 1.0));
    

	  return color;
}

vec4 drawGradient(vec4 color1, vec4 color2, vec4 color3, vec4 color4, float stop1, float stop2, float stop3, float stop4, float angle)
{
	float t = dot(out_uvs,vec2(sin(angle), cos(angle)));

    vec4 color;
    if (t < stop1)
        color = color1;
    else if (t < stop2)
    {
        float tLocal = (t - stop1)/(stop2 - stop1);
        color = mix(color1,color2,tLocal);
    }
    else if (t < stop3)
    {
        float tLocal = (t - stop2)/(stop3 - stop2);
        color = mix(color2,color3,tLocal);
    }
    else if (t < stop4)
    {
        float tLocal = (t - stop3)/(stop4 - stop3);
        color = mix(color3,color4,tLocal);
    }
    else
	{
		color = color4;
	}

	return color;
}

vec3 drawShade(vec2 st, vec2 center, vec2 size, vec4 color_main, vec4 color_1, vec4 color_2, vec4 color_3, vec4 color_4)
{
	vec3 rect_main = drawRect(st, center, size.x, size.y, 0.0, color_main.rgb, color_main.rgb);
	vec3 rect_1 =  drawRect(st, center + vec2(size.x, 0 ), size.x, size.y, 0.0, color_1.rgb, color_1.rgb);
	vec3 rect_2 =  drawRect(st, center + vec2(size.x * 2, 0 ), size.x, size.y, 0.0, color_2.rgb, color_2.rgb);
	vec3 rect_3 =  drawRect(st, center + vec2(size.x * 3, 0 ), size.x, size.y, 0.0, color_3.rgb, color_3.rgb);
	vec3 rect_4 =  drawRect(st, center + vec2(size.x * 4, 0 ), size.x, size.y, 0.0, color_4.rgb, color_4.rgb);
	vec4 gradient = drawGradient(color_1, color_2, color_3, color_4, 0.25, 0.25, 0.25, 0.25, angle);
	vec3 rect_5 =  drawRect(st, center +vec2(size.x * 5, 0 ), size.x, size.y, 0.0, gradient.rgb, gradient.rgb);

	vec3 add_1 = rect_main * factor + rect_1;
	vec3 add_2 = add_1 * factor + rect_2;
	vec3 add_3 = add_2 * factor + rect_3;
	vec3 add_4 = add_3 * factor + rect_4;
	vec3 add_5 = add_4 * factor + rect_5;

	return add_5;
}

void main(){
    vec2 st = gl_FragCoord.xy/resolution.xy;
    vec3 color = vec3(0.0);	
	
	vec3 color1 = drawShade(st, vec2(0.08, 1), vec2(1.0 / 6.0, 0.5), main_color, complement_color_1, complement_color_2, complement_color_4, complement_color_4);
	vec3 color2 =  drawShade(st, vec2(0.08, 0.5), vec2(1.0 / 6.0, 0.5), main_color, complement_color_2, complement_color_1, complement_color_2, complement_color_3);
	vec3 color3 = drawShade(st, vec2(0.08, 0.75), vec2(1.0 / 6.0, 0.5), main_color, complement_color_3, complement_color_3, complement_color_1, complement_color_2);
	vec3 color4 = drawShade(st, vec2(0.08, 0), vec2(1.0 / 6.0, 0.5), main_color, complement_color_4, complement_color_4, complement_color_3, complement_color_1);

	vec3 a1 = color1 * factor + color2;
	vec3 a2 = a1 * factor + color3;
	vec3 a3 = a2 * factor + color4;

    colourOut = vec4(sub_1_1_color.g);
	
}