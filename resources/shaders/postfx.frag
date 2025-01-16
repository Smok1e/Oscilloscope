#version 430 core

//=================================================

uniform float     distortion_power;
uniform vec2      texture_size;
uniform vec2      texture_offset;
uniform sampler2D texture_data;
uniform vec4      background_color;
uniform int       glow_radius;

out vec4 out_Color;

//=================================================

vec2 barrel_distortion(vec2 uv)
{
	vec2 pos = 2.0f * uv - 1.0f;

	float len = length(pos);
	len = pow(len, distortion_power);

	pos = normalize(pos);
	pos *= len;

	pos = 0.5f * (pos + 1.0f);

	return pos;
}

vec4 interpolate(vec4 a, vec4 b, float t)
{
	return a + (b - a) * t;
}

vec4 calc_color(vec2 coords)
{
	vec2 uv = (coords - texture_offset) / texture_size;
	vec4 color = texture(texture_data, barrel_distortion(uv));

	if ((int(coords.y) % 4) * (int(coords.x) % 4) == 0)
		return interpolate(background_color, color, .6f);

	else
		return color;
}

void main()
{
	vec3 color = calc_color(gl_FragCoord.xy).rgb;

	if ((length(color - background_color.rgb)) < 0.01)
	{
		color = vec3(0, 0, 0);
		int pixels = 0;

		for (int x = -glow_radius; x <= glow_radius; x++)
		{
			for (int y = -glow_radius; y <= glow_radius; y++)
			{
				vec2 pos = vec2(x, y);
				if (length(pos) <= glow_radius)
				{
					color += calc_color(gl_FragCoord.xy + pos).rgb;
					pixels++;
				}
			}
		}

		color /= pixels;
	}

	out_Color = vec4(color, 1);
}

//=================================================
