#version 330

const int MAX_POINT_LIGHTS = 3;

in vec2 TexCoord;
in vec3 FragPos;

out vec4 color;

// Base light structure (matches engine's Light class)
struct Light {
	vec3 color;
	float ambientIntensity;
	float diffuseIntensity;
};

// Point light with base light and position/attenuation (matches engine's PointLight class)
struct PointLight {
	Light base;
	vec3 position;
	float constant;
	float linear;
	float exponent;
};

uniform sampler2D theTexture;
uniform vec4 spriteColor = vec4(1.0, 1.0, 1.0, 1.0);
uniform float lightRatio = 1.0;

uniform int pointLightCount = 0;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

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
	vec4 finalColor = texColor * spriteColor;

	// If lightRatio is very low, we're in cave mode (point lights provide illumination)
	// Otherwise use normal lightRatio for 2D mode
	vec3 totalLight = lightRatio < 0.1 ? vec3(0.0) : vec3(lightRatio);

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