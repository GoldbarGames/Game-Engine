#ifndef SCENE3D_INTERNAL_H
#define SCENE3D_INTERNAL_H
#pragma once

// Engine-internal helpers shared by the Scene3D*.cpp files (Scene3D,
// Scene3DLighting, Scene3DShadows, Scene3DWeather). Not exported.

#include "render/RenderDevice.h"

namespace Scene3DInternal
{
	// Engine shaders declare the uniform blocks; old copies kept in a game's
	// data/shaders predate them and still need the loose uniforms.
	inline bool ProgramHasBlock(unsigned int program, const char* name)
	{
		return Device().HasUniformBlock(ProgramHandle(program), name);
	}
}

#endif
