#ifndef GUARD_BATTLE_AI_STORY_POLICY_H
#define GUARD_BATTLE_AI_STORY_POLICY_H

#include "battle.h"

struct StoryAiBattleContext
{
    u32 battleTypeFlags;
    u16 opponentA;
    u16 opponentB;
    enum BattlerPosition actingPosition;
    bool8 isDebugBattle;
};

// Stage 1 only: this does not change any AI decision or battle state.
bool32 BattleAI_StoryPublicInfoApplies(const struct StoryAiBattleContext *context);
bool32 BattleAI_UsesStoryPublicInfo(enum BattlerId battler);

#endif // GUARD_BATTLE_AI_STORY_POLICY_H
