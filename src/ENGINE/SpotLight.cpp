#include "SpotLight.h"

SpotLight::SpotLight() : PointLight()
{
	direction = glm::vec3(0.0f, 0.0f, 0.0f);
	edge = 0.0f;
	procEdge = cosf(glm::radians(edge));
}

SpotLight::SpotLight(glm::vec3 col, float ai, float di, glm::vec3 pos, glm::vec3 att, glm::vec3 dir, float edg) : 
	PointLight(col, ai, di, pos, att)
{
	position = pos;
	direction = dir;
	edge = edg;
	procEdge = cosf(glm::radians(edge));
}

SpotLight::SpotLight(float red, float green, float blue, float aIntensity, float dIntensity,
	float xpos, float ypos, float zpos, float xdir, float ydir, float zdir,
	float con, float lin, float exp, float edg) : PointLight(red, green, blue, 
		aIntensity, dIntensity, xpos, ypos, zpos, con, lin, exp)
{
	direction = glm::vec3(xdir, ydir, zdir);
	edge = edg;
	procEdge = cosf(glm::radians(edge));
}

SpotLight::~SpotLight()
{

}

void SpotLight::UseLight(const ShaderProgram& shader, int index)
{
	glUniform3f(shader.uniformSpotLight[index].uniformColor, color.x, color.y, color.z);
	glUniform1f(shader.uniformSpotLight[index].uniformAmbientIntensity, ambientIntensity);
	glUniform1f(shader.uniformSpotLight[index].uniformDiffuseIntensity, diffuseIntensity);

	glUniform3f(shader.uniformSpotLight[index].uniformPosition, position.x, position.y, position.z);
	glUniform1f(shader.uniformSpotLight[index].uniformConstant, constant);
	glUniform1f(shader.uniformSpotLight[index].uniformLinear, linear);
	glUniform1f(shader.uniformSpotLight[index].uniformExponent, exponent);

	glUniform3f(shader.uniformSpotLight[index].uniformDirection, direction.x, direction.y, direction.z);
	glUniform1f(shader.uniformSpotLight[index].uniformEdge, procEdge);

}
