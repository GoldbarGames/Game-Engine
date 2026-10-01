#include "Light.h"
#include "render/RenderDevice.h"

Light::Light()
{
	color = glm::vec3(1.0f, 1.0f, 1.0f);
	ambientIntensity = 1.0f;
	diffuseIntensity = 0.0f;
}

Light::Light(float red, float green, float blue, float aIntensity, float dIntensity)
{
	color = glm::vec3(red, green, blue);
	ambientIntensity = aIntensity;
	diffuseIntensity = dIntensity;
}

Light::Light(glm::vec3 col, float ai, float di)
{
	color = col;
	ambientIntensity = ai;
	diffuseIntensity = di;
}


void Light::UseLight(const ShaderProgram& shader, int index)
{
	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformColor), glm::vec3(color.x, color.y, color.z));
	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformAmbientIntensity), (float)(ambientIntensity));
	Device().SetUniform((int)(shader.uniformDirectionalLight.uniformDiffuseIntensity), (float)(diffuseIntensity));
}

Light::~Light()
{

}