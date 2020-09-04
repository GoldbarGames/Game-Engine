#include "GUI.h"
#include "HealthComponent.h"

void GUI::Render(const Renderer& renderer)
{
	for (int i = 0; i < healthComponents.size(); i++)
	{
		healthComponents[i]->Render(renderer);
	}

	healthComponents.clear();
}