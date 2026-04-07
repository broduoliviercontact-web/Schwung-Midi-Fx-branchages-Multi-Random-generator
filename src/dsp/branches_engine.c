#include "branches_engine.h"

static float clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static uint32_t branches_rand(BranchesEngine *engine)
{
    engine->rng_state = engine->rng_state * 1664525u + 1013904223u;
    return engine->rng_state;
}

static float branches_rand_float(BranchesEngine *engine)
{
    uint32_t r = branches_rand(engine);
    return (float)(r & 0x00FFFFFFu) / (float)0x01000000u;
}

void branches_engine_init(BranchesEngine *engine, uint32_t seed)
{
    engine->probability = 0.0f;
    engine->seed = seed ? seed : 1u;
    branches_engine_reset(engine);
}

void branches_engine_reset(BranchesEngine *engine)
{
    engine->rng_state = engine->seed ? engine->seed : 1u;
}

void branches_engine_set_probability(BranchesEngine *engine, float probability)
{
    engine->probability = clamp01(probability);
}

uint8_t branches_engine_should_branch(BranchesEngine *engine)
{
    return (uint8_t)(branches_rand_float(engine) < engine->probability);
}
