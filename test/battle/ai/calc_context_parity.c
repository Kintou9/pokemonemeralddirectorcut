#include "global.h"
#include "test/battle.h"
#include "battle_ai_calc_context.h"
#include "battle_main.h"
#include "pokemon.h"
#include "battle_setup.h"
#include "debug.h"
#include "constants/opponents.h"

AI_SINGLE_BATTLE_TEST("AI calculation context parity: switch candidate with matching species and personality")
{
    bool32 checkEffective;
    PARAMETRIZE { checkEffective = FALSE; }
    PARAMETRIZE { checkEffective = TRUE; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        enum BattlerId battler = B_BATTLER_1;
        struct AiCalcContext context;
        struct BattlePokemon savedMon = gBattleMons[battler];
        u16 savedSpeed = gAiLogicData->speedStats[battler];
        struct Pokemon *party = GetBattlerParty(battler);
        u32 savedIndex = gBattlerPartyIndexes[battler];
        u32 rawSpeed, effectiveSpeed, originalRawSpeed, originalEffectiveSpeed;

        // A personality is not a unique identity. Mirror the existing candidate
        // substitution and party-index restoration in InitializeSwitchinCandidate.
        AI_InitCalcContext(&context, battler);
        PokemonToBattleMon(&party[1], &gBattleMons[battler]);
        gBattleMons[battler].personality = savedMon.personality;
        gBattlerPartyIndexes[battler] = 1;
        gAiLogicData->speedStats[battler] = GetBattlerTotalSpeedStat(battler,
            gAiLogicData->abilities[battler], gAiLogicData->holdEffects[battler]);
        gBattlerPartyIndexes[battler] = savedIndex;

        originalRawSpeed = gBattleMons[battler].speed;
        originalEffectiveSpeed = gAiLogicData->speedStats[battler];
        rawSpeed = AI_GetContextRawSpeed(&context, battler);
        effectiveSpeed = AI_GetContextEffectiveSpeed(&context, battler);

        gBattleMons[battler] = savedMon;
        gAiLogicData->speedStats[battler] = savedSpeed;

        EXPECT_EQ(originalRawSpeed, 100);
        if (checkEffective)
            EXPECT_EQ(effectiveSpeed, originalEffectiveSpeed);
        else
            EXPECT_EQ(rawSpeed, originalRawSpeed);
    }
}

// Exercise the accessor boundary without changing production simulation order.
// Each candidate keeps species/personality unchanged; raw and effective Speed
// change independently, including after the original party index is restored.
static void CheckContextSimulationLifecycle(enum BattlerId actor)
{
    struct AiCalcContext original, rebuilt;
    const struct AiCalcContext *previous = AI_GetActiveCalcContext();
    u16 savedRaw = gBattleMons[actor].speed;
    u16 savedEffective = gAiLogicData->speedStats[actor];
    u8 savedIndex = gBattlerPartyIndexes[actor];

    AI_InitCalcContext(&original, actor);
    AI_SetActiveCalcContext(&original);
    for (u32 candidate = 1; candidate <= 3; candidate++)
    {
        u16 candidateSpeed = 100 + candidate;
        gBattlerPartyIndexes[actor] = (savedIndex + candidate) % PARTY_SIZE;
        gBattleMons[actor].speed = candidateSpeed;
        gAiLogicData->speedStats[actor] = candidateSpeed;
        EXPECT_EQ(AI_GetRawSpeed(actor), candidateSpeed);
        EXPECT_EQ(AI_GetEffectiveSpeed(actor), candidateSpeed);

        gBattlerPartyIndexes[actor] = savedIndex;
        EXPECT_EQ(AI_GetRawSpeed(actor), candidateSpeed);
        EXPECT_EQ(AI_GetEffectiveSpeed(actor), candidateSpeed);
        EXPECT_EQ(AI_GetContextRawSpeed(NULL, actor), candidateSpeed);
        EXPECT_EQ(AI_GetContextEffectiveSpeed(NULL, actor), candidateSpeed);

        AI_InitCalcContext(&rebuilt, actor);
        EXPECT_EQ(rebuilt.rawSpeed[actor], candidateSpeed);
        EXPECT_EQ(rebuilt.effectiveSpeed[actor], candidateSpeed);
        AI_SetActiveCalcContext(&rebuilt);
        // Effective-only change must not be hidden by matching raw Speed.
        gAiLogicData->speedStats[actor] = candidateSpeed * 2;
        EXPECT_EQ(AI_GetRawSpeed(actor), candidateSpeed);
        EXPECT_EQ(AI_GetEffectiveSpeed(actor), candidateSpeed * 2);
        // Raw-only change must not be hidden by matching effective Speed.
        gAiLogicData->speedStats[actor] = candidateSpeed;
        gBattleMons[actor].speed = candidateSpeed + 10;
        EXPECT_EQ(AI_GetRawSpeed(actor), candidateSpeed + 10);
        EXPECT_EQ(AI_GetEffectiveSpeed(actor), candidateSpeed);

        gBattleMons[actor].speed = savedRaw;
        gAiLogicData->speedStats[actor] = savedEffective;
        // A context built for the candidate must also reject restored values.
        EXPECT_EQ(AI_GetRawSpeed(actor), savedRaw);
        EXPECT_EQ(AI_GetEffectiveSpeed(actor), savedEffective);
        AI_SetActiveCalcContext(&original);
        EXPECT_EQ(AI_GetRawSpeed(actor), savedRaw);
        EXPECT_EQ(AI_GetEffectiveSpeed(actor), savedEffective);
        EXPECT_EQ(original.rawSpeed[actor], savedRaw);
        EXPECT_EQ(original.effectiveSpeed[actor], savedEffective);
    }
    AI_SetActiveCalcContext(previous);
}

AI_SINGLE_BATTLE_TEST("AI calculation context parity: repeated singles simulations and restoration")
{
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT(AI_GetActiveCalcContext() == NULL);
        CheckContextSimulationLifecycle(B_BATTLER_1);
        EXPECT(AI_GetActiveCalcContext() == NULL);
    }
}

AI_DOUBLE_BATTLE_TEST("AI calculation context parity: scoped actors in doubles, story multi and Frontier")
{
    u32 flags;
    u16 trainerA, trainerB;
    bool32 policyActive;
    PARAMETRIZE {
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER;
        trainerA = TRAINER_MAXIE_MOSSDEEP; trainerB = TRAINER_TABITHA_MOSSDEEP; policyActive = TRUE;
    }
    PARAMETRIZE {
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER;
        trainerA = TRAINER_TABITHA_MOSSDEEP; trainerB = TRAINER_MAXIE_MOSSDEEP; policyActive = TRUE;
    }
    PARAMETRIZE {
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE;
        trainerA = TRAINER_MATTHEW; trainerB = 0; policyActive = FALSE;
    }
    PARAMETRIZE {
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_BATTLE_TOWER;
        trainerA = TRAINER_ROXANNE_1; trainerB = 0; policyActive = FALSE;
    }
    PARAMETRIZE {
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_FACTORY;
        trainerA = TRAINER_ROXANNE_1; trainerB = 0; policyActive = FALSE;
    }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(76); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(60); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SPLASH); MOVE(playerRight, MOVE_SPLASH);
            EXPECT_MOVE(opponentLeft, MOVE_TACKLE); EXPECT_MOVE(opponentRight, MOVE_TACKLE);
        }
    } THEN {
        struct AiCalcContext left, right, ally;
        u32 savedFlags = gBattleTypeFlags;
        u16 savedA = TRAINER_BATTLE_PARAM.opponentA;
        u16 savedB = TRAINER_BATTLE_PARAM.opponentB;
        bool8 savedDebug = gIsDebugBattle;
        EXPECT(AI_GetActiveCalcContext() == NULL);

        // Inject ownership only for context construction: this does not claim
        // to run Space Center scripts or Factory team selection in the harness.
        gBattleTypeFlags = flags;
        TRAINER_BATTLE_PARAM.opponentA = trainerA;
        TRAINER_BATTLE_PARAM.opponentB = trainerB;
        gIsDebugBattle = FALSE;
        AI_InitCalcContext(&left, B_BATTLER_1);
        AI_InitCalcContext(&right, B_BATTLER_3);
        AI_InitCalcContext(&ally, B_BATTLER_2);
        EXPECT_EQ(left.actingTrainerId, trainerA);
        EXPECT_EQ(right.actingTrainerId, (flags & BATTLE_TYPE_TWO_OPPONENTS) ? trainerB : trainerA);
        EXPECT_EQ(left.storyPolicyActive, policyActive);
        EXPECT_EQ(right.storyPolicyActive, policyActive);
        EXPECT(!ally.storyPolicyActive);
        AI_SetActiveCalcContext(&left);
        CheckContextSimulationLifecycle(B_BATTLER_1);
        EXPECT(AI_GetActiveCalcContext() == &left);
        AI_SetActiveCalcContext(&right);
        CheckContextSimulationLifecycle(B_BATTLER_3);
        EXPECT(AI_GetActiveCalcContext() == &right);
        EXPECT_EQ(left.actingBattler, B_BATTLER_1);
        EXPECT_EQ(right.actingBattler, B_BATTLER_3);
        EXPECT_EQ(left.rawSpeed[B_BATTLER_1], 50);
        EXPECT_EQ(right.rawSpeed[B_BATTLER_3], 60);
        AI_SetActiveCalcContext(&left);
        EXPECT_EQ(AI_GetRawSpeed(B_BATTLER_3), 60);
        AI_SetActiveCalcContext(NULL);
        gBattleTypeFlags = savedFlags;
        TRAINER_BATTLE_PARAM.opponentA = savedA;
        TRAINER_BATTLE_PARAM.opponentB = savedB;
        gIsDebugBattle = savedDebug;
    }
}
