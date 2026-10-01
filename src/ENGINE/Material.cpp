#include "Material.h"
#include "render/RenderDevice.h"
#include "Material.h"


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
	Device().SetUniform((int)(specularIntensityLocation), (float)(specularIntensity));
	Device().SetUniform((int)(shineLocation), (float)(shine));
}