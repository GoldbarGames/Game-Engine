#include "PointLight.h"
#include "render/RenderDevice.h"

PointLight::PointLight() : Light()
{
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	constant = 1.0f;
	linear = 0.0f;
	exponent = 0.0f;
}

PointLight::PointLight(glm::vec3 col, float ai, float di, glm::vec3 pos, glm::vec3 attenuation) : Light(col, ai, di)
{
	position = pos;
	constant = attenuation.x;
	linear = attenuation.y;
	exponent = attenuation.z;
}

PointLight::PointLight(float red, float green, float blue, float aIntensity, float dIntensity,
	float xpos, float ypos, float zpos, float con, float lin, float exp) : Light (red, green, blue, aIntensity, dIntensity)
{
	position = glm::vec3(xpos, ypos, zpos);
	constant = con;
	linear = lin;
	exponent = exp;
}

PointLight::~PointLight()
{

}

void PointLight::UseLight(const ShaderProgram& shader, int index)
{
	Device().SetUniform((int)(shader.uniformPointLight[index].uniformColor), glm::vec3(color.x, color.y, color.z));
	Device().SetUniform((int)(shader.uniformPointLight[index].uniformAmbientIntensity), (float)(ambientIntensity));
	Device().SetUniform((int)(shader.uniformPointLight[index].uniformDiffuseIntensity), (float)(diffuseIntensity));

	Device().SetUniform((int)(shader.uniformPointLight[index].uniformPosition), glm::vec3(position.x, position.y, position.z));
	Device().SetUniform((int)(shader.uniformPointLight[index].uniformConstant), (float)(constant));
	Device().SetUniform((int)(shader.uniformPointLight[index].uniformLinear), (float)(linear));
	Device().SetUniform((int)(shader.uniformPointLight[index].uniformExponent), (float)(exponent));
}
