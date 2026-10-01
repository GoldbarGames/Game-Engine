#version 330

in vec2 TexCoord;
in vec3 FragPos;

out vec4 color;

uniform sampler2D theTexture;
#include "sprite_draw.glsl"
#include "sprite_lights.glsl"   // pointLightCount, pointLights[] (Light / PointLight structs)

vec3 CalcPointLight(PointLight pLight)
{
	vec3 direction = pLight.position - FragPos;
	float distance = length(direction);

	// Attenuation formula: 1 / (constant + linear*d + exponent*d^2)
	float attenuation = 1.0 / (pLight.constant +
							   pLight.linear * distance +
							   pLight.exponent * distance * distance);

	vec3 ambient = pLight.base.color * pLight.base.ambientIntensity;
	vec3 diffuse = pLight.base.color * pLight.base.diffuseIntensity * attenuation;

	return ambient + diffuse;
}

void main()
{
	vec4 texColor = texture(theTexture, TexCoord.xy);
	vec4 finalColor = texColor * draw.spriteColor;

	// Transparent pixels must not write depth: an invisible quad otherwise
	// occludes everything drawn behind it later
	if (finalColor.a < 0.1)
		discard;

	// If lightRatio is very low, we're in cave mode (point lights provide illumination)
	// Otherwise use normal lightRatio for 2D mode
	vec3 totalLight = draw.lightRatio < 0.1 ? vec3(0.0) : vec3(draw.lightRatio);

	// Add point light contributions
	for (int i = 0; i < pointLightCount; i++)
	{
		totalLight += CalcPointLight(pointLights[i]);
	}

	// Clamp to allow very bright center
	totalLight = clamp(totalLight, 0.0, 5.0);

	finalColor.rgb *= totalLight;
	color = finalColor;
}
