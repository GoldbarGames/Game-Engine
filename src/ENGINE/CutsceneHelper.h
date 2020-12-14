#ifndef CUTSCENE_HELPER_H
#define CUTSCENE_HELPER_H

#pragma once

#include "leak_check.h"
#include "CutsceneCommands.h"

class KINJO_API CutsceneHelper
{
public:
	CutsceneHelper();
	~CutsceneHelper();

	virtual void SetFunctions(CutsceneCommands& commands);
};

#endif