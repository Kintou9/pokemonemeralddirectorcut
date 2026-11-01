#ifndef GUARD_BATTLE_AI_CALC_CONTEXT_H
#define GUARD_BATTLE_AI_CALC_CONTEXT_H

#include "battle.h"

// Actual-value AI inputs for this infrastructure phase. Later stages may
// replace opponent inputs only after actor-specific cache invalidation exists.
struct AiCalcContext
{
    enum BattlerId actingBattler;
    u16 actingTrainerId;
    bool8 storyPolicyActive;
    u8 partyIndex[MAX_BATTLERS_COUNT];
    enum Species species[MAX_BATTLERS_COUNT];
    u32 personality[MAX_BATTLERS_COUNT];
    u16 rawSpeed[MAX_BATTLERS_COUNT];
    u32 effectiveSpeed[MAX_BATTLERS_COUNT];
};

void AI_InitCalcContext(struct AiCalcContext *context, enum BattlerId actor);
const struct AiCalcContext *AI_GetActiveCalcContext(void);
void AI_SetActiveCalcContext(const struct AiCalcContext *context);
u32 AI_GetRawSpeed(enum BattlerId battler);
u32 AI_GetEffectiveSpeed(enum BattlerId battler);
u32 AI_GetContextRawSpeed(const struct AiCalcContext *context, enum BattlerId battler);
u32 AI_GetContextEffectiveSpeed(const struct AiCalcContext *context, enum BattlerId battler);
u32 AI_GetPartyMonRawSpeed(struct Pokemon *party, u32 partyIndex);

struct BattleCalcValues;
// Recalculates for the current simulated state; does not refresh speedStats.
s32 AI_CompareTurnOrder(const struct AiCalcContext *context, struct BattleCalcValues *values, bool32 ignoreChosenMoves);

#endif // GUARD_BATTLE_AI_CALC_CONTEXT_H
