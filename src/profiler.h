#ifndef PROFILER_H
#define PROFILER_H

#include "types.h"

void Profiler_Init(ProfilerMetrics* prof);
void Profiler_StartFrame(ProfilerMetrics* prof);
void Profiler_EndFrame(ProfilerMetrics* prof);

uint64_t Profiler_GetTimestamp(void);
double Profiler_ElapsedMs(uint64_t start_counter);

void Profiler_DrawOverlay(GameState* game);

#endif // PROFILER_H
