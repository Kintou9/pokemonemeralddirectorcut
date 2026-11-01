#include "global.h"
#include "battle.h"
#include "battle_ai_calc_context.h"
#include "battle_ai_story_policy.h"
#include "battle_setup.h"
#include "battle_main.h"
#include "pokemon.h"

static const struct AiCalcContext *sActiveAiCalcContext;

void AI_InitCalcContext(struct AiCalcContext *context, enum BattlerId actor)
{
    enum BattlerPosition position = GetBattlerPosition(actor);

    memset(context, 0, sizeof(*context));
    context->actingBattler = actor;
    context->actingTrainerId = 0xFFFF;
    context->storyPolicyActive = BattleAI_UsesStoryPublicInfo(actor);

    if (position == B_POSITION_OPPONENT_LEFT)
        context->actingTrainerId = TRAINER_BATTLE_PARAM.opponentA;
    else if (position == B_POSITION_OPPONENT_RIGHT)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
            context->actingTrainerId = TRAINER_BATTLE_PARAM.opponentB;
        else
            context->actingTrainerId = TRAINER_BATTLE_PARAM.opponentA;
    }

    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
    {
        context->partyIndex[battler] = gBattlerPartyIndexes[battler];
        context->species[battler] = gBattleMons[battler].species;
        context->personality[battler] = gBattleMons[battler].personality;
        context->rawSpeed[battler] = gBattleMons[battler].speed;
        context->effectiveSpeed[battler] = gAiLogicData->speedStats[battler];
    }
}

const struct AiCalcContext *AI_GetActiveCalcContext(void)
{
    return sActiveAiCalcContext;
}

void AI_SetActiveCalcContext(const struct AiCalcContext *context)
{
    sActiveAiCalcContext = context;
}

u32 AI_GetContextRawSpeed(const struct AiCalcContext *context, enum BattlerId battler)
{
    // Switch simulation restores the party index before restoring the mon.
    // Species/personality can also match a different candidate. In this
    // actual-value phase, validate the snapshot against its original source.
    if (context == NULL || context->partyIndex[battler] != gBattlerPartyIndexes[battler]
        || context->species[battler] != gBattleMons[battler].species
        || context->personality[battler] != gBattleMons[battler].personality
        || context->rawSpeed[battler] != gBattleMons[battler].speed)
        return gBattleMons[battler].speed; // Existing switch-in simulation substituted this battler.
    return context->rawSpeed[battler];
}

u32 AI_GetContextEffectiveSpeed(const struct AiCalcContext *context, enum BattlerId battler)
{
    if (context == NULL || context->partyIndex[battler] != gBattlerPartyIndexes[battler]
        || context->species[battler] != gBattleMons[battler].species
        || context->personality[battler] != gBattleMons[battler].personality
        // Effective Speed can change even when raw Speed and identity do not.
        || context->effectiveSpeed[battler] != gAiLogicData->speedStats[battler])
        return gAiLogicData->speedStats[battler];
    return context->effectiveSpeed[battler];
}

u32 AI_GetRawSpeed(enum BattlerId battler)
{
    return AI_GetContextRawSpeed(sActiveAiCalcContext, battler);
}

u32 AI_GetEffectiveSpeed(enum BattlerId battler)
{
    return AI_GetContextEffectiveSpeed(sActiveAiCalcContext, battler);
}

u32 AI_GetPartyMonRawSpeed(struct Pokemon *party, u32 partyIndex)
{
    return GetMonData(&party[partyIndex], MON_DATA_SPEED);
}

s32 AI_CompareTurnOrder(const struct AiCalcContext *context, struct BattleCalcValues *values, bool32 ignoreChosenMoves)
{
    enum BattlerId attacker = values->battlerAtk;
    enum BattlerId defender = values->battlerDef;
    s32 priorityAtk = 0, priorityDef = 0;
    u32 speedAtk, speedDef;

    if (context == NULL || !context->storyPolicyActive)
        return GetWhichBattlerFasterOrTies(values, ignoreChosenMoves);

    // This caller originally recalculated Speed, unlike AI_WhoStrikesFirst.
    // Apply current stages/status/field effects to validated raw inputs using
    // the same calculator and supplied abilities/items as the original path.
    speedAtk = GetBattlerTotalSpeedStatWithBase(attacker, values->abilities[attacker],
        values->holdEffects[attacker], AI_GetContextRawSpeed(context, attacker));
    speedDef = GetBattlerTotalSpeedStatWithBase(defender, values->abilities[defender],
        values->holdEffects[defender], AI_GetContextRawSpeed(context, defender));

    if (!ignoreChosenMoves)
    {
        if (gChosenActionByBattler[attacker] == B_ACTION_USE_MOVE)
            priorityAtk = GetChosenMovePriority(attacker, values->abilities[attacker]);
        if (gChosenActionByBattler[defender] == B_ACTION_USE_MOVE)
            priorityDef = GetChosenMovePriority(defender, values->abilities[defender]);
    }
    return GetWhichBattlerFasterArgs(values, ignoreChosenMoves, speedAtk, speedDef, priorityAtk, priorityDef);
}
