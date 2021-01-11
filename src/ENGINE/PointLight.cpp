#include "PointLight.h"

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

void PointLight::UseLight(const ShaderProgram& shader)
{
	glUniform3f(shader.uniformPointLight[0].uniformColor, color.x, color.y, color.z);
	glUniform1f(shader.uniformPointLight[0].uniformAmbientIntensity, ambientIntensity);
	glUniform1f(shader.uniformPointLight[0].uniformDiffuseIntensity, diffuseIntensity);

	glUniform3f(shader.uniformPointLight[0].uniformPosition, position.x, position.y, position.z);
	glUniform1f(shader.uniformPointLight[0].uniformConstant, constant);
	glUniform1f(shader.uniformPointLight[0].uniformLinear, linear);
	glUniform1f(shader.uniformPointLight[0].uniformExponent, exponent);
}
