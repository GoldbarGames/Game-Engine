#include "Material.h"
#include "Material.h"

#include <GL/glew.h>

Material::Material()
{
	specularIntensity = 0.0f;
	shine = 0.0f;
}

Material::Material(float si, float sh)
{
	specularIntensity = si;
	shine = sh;
}

Material::~Material()
{

}

void Material::UseMaterial(int specularIntensityLocation, int shineLocation)
{
	glUniform1f(specularIntensityLocation, specularIntensity);
	glUniform1f(shineLocation, shine);
}