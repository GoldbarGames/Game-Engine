#ifndef DRAW_STATS_H
#define DRAW_STATS_H
#pragma once

#include <cstdint>
#include <string>

// Where the World pass's draw calls go (since 2026-10-10), for
// KINJO_GPU_TIMINGS: Game::Render's entity loop gives each entity's draws to
// its type, and the pass report lists them after the passes ("World draws by
// entity type"), averaged per frame. Implemented in RenderPass.cpp.

// Whether KINJO_GPU_TIMINGS is gathering (so callers skip the counting).
bool DrawStatsOn();
void AttributeDraws(const std::string& who, uint64_t draws);

#endif
