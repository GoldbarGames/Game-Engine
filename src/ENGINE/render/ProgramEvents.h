#ifndef PROGRAM_EVENTS_H
#define PROGRAM_EVENTS_H
#pragma once

// Engine-internal. Modules that cache per-program data (uniform locations,
// keyed by program id) hear when a program is deleted, because the id can be
// reused by the next program created. ShaderProgram::ClearShader notifies.

using ProgramDeletedFn = void (*)(unsigned int program);

void AddProgramDeletedListener(ProgramDeletedFn fn);   // idempotent
void NotifyProgramDeleted(unsigned int program);

#endif
