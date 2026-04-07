#ifndef BRANCHES_ENGINE_H
#define BRANCHES_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float probability;
    uint32_t seed;
    uint32_t rng_state;
} BranchesEngine;

void branches_engine_init(BranchesEngine *engine, uint32_t seed);
void branches_engine_reset(BranchesEngine *engine);
void branches_engine_set_probability(BranchesEngine *engine, float probability);
uint8_t branches_engine_should_branch(BranchesEngine *engine);

#ifdef __cplusplus
}
#endif

#endif
