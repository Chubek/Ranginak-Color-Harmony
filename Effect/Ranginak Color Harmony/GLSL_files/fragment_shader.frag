#version 330
uniform sampler2D videoTexture;
uniform float sliderVal;
uniform float multiplier16bit;
in vec4 out_pos;
in vec2 out_uvs;
out vec4 colourOut;


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
    
    //STROKE
    vec2 stroke = vec2(0.0);
    stroke += step(bottomLeft-halfTickness, st) * (1.0 - step(bottomLeft+halfTickness, st));
    stroke += step(topRight-halfTickness, st) * (1.0 - step(topRight+halfTickness, st));
    vec2 strokeLimit = step(bottomLeft-halfTickness, st) * (1.0 - step(topRight+halfTickness, st));
    stroke *= strokeLimit.x * strokeLimit.y;

    color = mix (color, strokeColor, min(stroke.x + stroke.y, 1.0));
    //
    
    //FILL
    vec2 fill = vec2(0.0);
    fill += step(bottomLeft+halfTickness, st) * (1.0 - step(topRight-halfTickness, st));
    vec2 fillLimit = step(bottomLeft+halfTickness, st) * (1.0 - step(topRight-halfTickness, st));
    fill *=  fillLimit.x * fillLimit.y;
    
    color = mix (color, fillColor, min(fill.x + fill.y, 1.0));
    //

	  return color;
}


void main( void )
{
	 vec2 st = gl_FragCoord.xy/vec2(1920, 1080);
    vec3 color = vec3(0.0);	
	
	vec3 rect1 = drawRect(st, vec2(0.5, 0.5), 0.8, 0.8, 0.0, vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0));
	vec3 rect2 = drawRect(st, vec2(0.5, 0.5), 0.6, 0.6, 0.0, vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0));
	vec3 rect3 = drawRect(st, vec2(0.5, 0.5), 0.3, 0.3, 0.0, vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0));

	

	color = rect1 + rect2 + rect3;

	
	colourOut = vec4(color, 1.0);


}
