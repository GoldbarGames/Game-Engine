#include "SpotLight.h"
#include "render/RenderDevice.h"

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
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformColor), glm::vec3(color.x, color.y, color.z));
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformAmbientIntensity), (float)(ambientIntensity));
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformDiffuseIntensity), (float)(diffuseIntensity));

	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformPosition), glm::vec3(position.x, position.y, position.z));
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformConstant), (float)(constant));
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformLinear), (float)(linear));
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformExponent), (float)(exponent));

	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformDirection), glm::vec3(direction.x, direction.y, direction.z));
	Device().SetUniform((int)(shader.uniformSpotLight[index].uniformEdge), (float)(procEdge));

}
