#ifndef STARTUPTRACE_H
#define STARTUPTRACE_H
#pragma once

#include "leak_check.h"

// KINJO_STARTUP_TRACE=1 prints how long each step of start-up takes, from the
// moment the engine is loaded to the first frame on screen (docs/STARTUP.md).
// The engine marks its own steps; a game can mark steps of its own set-up with
// this too, so its share of the wait shows up by name. Silent otherwise.
KINJO_API void StartupStep(const char* what);

#endif
