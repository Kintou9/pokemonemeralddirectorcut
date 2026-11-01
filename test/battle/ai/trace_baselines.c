#include "global.h"
#include "test/battle.h"
#include "battle_ai_trace.h"
#include "battle_ai_calc_context.h"
#include "battle_ai_util.h"
#include "battle_ai_main.h"
#include "battle_ai_switch.h"
#include "battle_setup.h"
#include "random.h"
#include "debug.h"
#include "constants/opponents.h"

// Existing production entry point is not declared by battle_ai_main.h.
void SetupAIPredictionData(enum BattlerId battler, enum SwitchType switchType);
u32 AI_GetDefaultDamageRollForContext(enum BattlerId battlerAtk, enum BattlerId battlerDef, u32 moveIndex, struct AiLogicData *aiData, u32 aiRoll);

TEST("AI trace: production RNG arithmetic and exact raw operation sequence")
{
    rng_value_t expected;
    AITraceStop();
    SeedRng(12345);
    expected = gRngValue;
    AITraceStart(1u << AI_TRACE_RAW_RNG);
    u32 first = Random32();
    u32 second = RandomUniformDefault(RNG_AI_DMG_ROLL_RANDOM, 85, 100);
    AITraceStop();
    u32 expectedFirst = LocalRandom32(&expected);
    u32 expectedSecond = 85 + ((16 * (LocalRandom32(&expected) >> 16)) >> 16);
    EXPECT_EQ(first, expectedFirst);
    EXPECT_EQ(second, expectedSecond);
    EXPECT_EQ(memcmp(&gRngValue, &expected, sizeof(expected)), 0);
    EXPECT_EQ(memcmp(&gAiTraceLog.finalRng, &expected, sizeof(expected)), 0);
    EXPECT_EQ(gAiTraceLog.count, 2);
    EXPECT(!gAiTraceLog.overflow);
    EXPECT_EQ(gAiTraceLog.events[0].sequence, 1);
    EXPECT_EQ(gAiTraceLog.events[1].sequence, 2);
    EXPECT_EQ(gAiTraceLog.events[0].after[0], first);
    EXPECT_EQ(gAiTraceLog.events[0].before[1], UINT32_MAX);
    EXPECT_EQ(gAiTraceLog.events[0].rngAfter, gAiTraceLog.events[1].rngBefore);
    EXPECT(gAiTraceLog.events[0].site != 0);
}

TEST("AI trace: overflow is explicit and does not overwrite the retained prefix")
{
    AITraceStart(1u << AI_TRACE_ENTER);
    for (u32 i = 0; i <= AI_TRACE_CAPACITY; i++)
        AITraceValue(AI_TRACE_ENTER, i, 0, 0, 0, NULL, NULL, 0);
    AITraceStop();
    EXPECT(gAiTraceLog.overflow);
    EXPECT_EQ(gAiTraceLog.count, AI_TRACE_CAPACITY);
    EXPECT_EQ(gAiTraceLog.events[0].site, 0);
    EXPECT_EQ(gAiTraceLog.events[AI_TRACE_CAPACITY - 1].site, AI_TRACE_CAPACITY - 1);
}

AI_SINGLE_BATTLE_TEST("AI trace: damage cache and RNG enabled versus disabled", struct SimulatedDamage damage; rng_value_t rng; u16 move;)
{
    enum Move move = MOVE_TACKLE;
    bool32 trace = FALSE;
    u32 stage = DEFAULT_STAT_STAGE;
    for (u32 st = DEFAULT_STAT_STAGE - 1; st <= DEFAULT_STAT_STAGE + 1; st++)
    for (u32 i = 0; i < 5; i++)
    {
        static const enum Move moves[] = {MOVE_TACKLE, MOVE_GYRO_BALL, MOVE_ELECTRO_BALL, MOVE_TRIPLE_KICK, MOVE_PSYWAVE};
        PARAMETRIZE { move = moves[i]; trace = FALSE; stage = st; }
        PARAMETRIZE { move = moves[i]; trace = TRUE; stage = st; }
    }
    GIVEN {
        AITraceStop();
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); HP(500); MaxHP(500); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Moves(move); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, move); }
    } THEN {
        struct AiCalcContext context;
        AI_InitCalcContext(&context, B_BATTLER_1);
        context.actingTrainerId = TRAINER_ROXANNE_1;
        context.storyPolicyActive = TRUE;
        AI_SetActiveCalcContext(&context);
        if (trace)
            AITraceStart((1u << AI_TRACE_DAMAGE) | (1u << AI_TRACE_EFFECTIVENESS)
                | (1u << AI_TRACE_ACCURACY) | (1u << AI_TRACE_BERRY)
                | (1u << AI_TRACE_TAGGED_RNG) | (1u << AI_TRACE_RAW_RNG)
                | (1u << AI_TRACE_ENTER) | (1u << AI_TRACE_LEAVE) | (1u << AI_TRACE_READ));
        gBattleMons[B_BATTLER_1].statStages[STAT_SPEED] = stage;
        CalcBattlerAiMovesData(gAiLogicData, B_BATTLER_1, B_BATTLER_0, AI_GetWeather(), gFieldTimers.terrain);
        u32 cached = AI_GetDefaultDamageRollForContext(B_BATTLER_1, B_BATTLER_0, 0, gAiLogicData, AI_ROLL_MEDIAN);
        AITraceStop();
        EXPECT_EQ(cached, gAiLogicData->simulatedDmg[B_BATTLER_1][B_BATTLER_0][0].median);
        results[i].damage = gAiLogicData->simulatedDmg[B_BATTLER_1][B_BATTLER_0][0];
        results[i].rng = gRngValue;
        results[i].move = gLastMoves[B_BATTLER_1];
        if (trace)
        {
            u32 writes = 0, draws = 0, reads = 0;
            EXPECT(!gAiTraceLog.overflow);
            EXPECT_EQ(gAiTraceLog.depth, 0);
            EXPECT_EQ(gAiTraceLog.events[0].kind, AI_TRACE_DAMAGE);
            EXPECT_EQ(gAiTraceLog.events[0].after[0], 0);
            EXPECT_EQ(gAiTraceLog.events[0].after[1], 0);
            for (u32 n = 0; n < gAiTraceLog.count; n++)
            {
                const struct AiTraceEvent *event = &gAiTraceLog.events[n];
                EXPECT_EQ(event->sequence, n + 1);
                EXPECT_EQ(event->actor, B_BATTLER_1);
                EXPECT_EQ(event->trainer, TRAINER_ROXANNE_1);
                if (event->kind == AI_TRACE_READ && event->cacheKind == AI_TRACE_DAMAGE)
                    reads++;
                if (event->kind == AI_TRACE_DAMAGE)
                    writes++;
                if (event->kind == AI_TRACE_RAW_RNG || event->kind == AI_TRACE_TAGGED_RNG)
                    draws++;
            }
            EXPECT_EQ(writes, 5); // Four clears and the one usable move result.
            EXPECT(draws > 0);
            EXPECT(reads > 0);
        }
        AI_SetActiveCalcContext(NULL);
    } FINALLY {
        for (u32 n = 0; n < 30; n += 2)
        {
            EXPECT_EQ(memcmp(&results[n].damage, &results[n + 1].damage, sizeof(results[n].damage)), 0);
            EXPECT_EQ(memcmp(&results[n].rng, &results[n + 1].rng, sizeof(results[n].rng)), 0);
            EXPECT_EQ(results[n].move, results[n + 1].move);
        }
    }
}

AI_SINGLE_BATTLE_TEST("AI trace: repeated candidates, prediction and restoration")
{
    GIVEN {
        AITraceStop();
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER | AI_FLAG_SMART_SWITCHING | AI_FLAG_PREDICT_SWITCH | AI_FLAG_PREDICT_MOVE);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_MACHOP) { Moves(MOVE_KARATE_CHOP); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        struct AiCalcContext context;
        u16 raw = gBattleMons[B_BATTLER_1].speed;
        AI_InitCalcContext(&context, B_BATTLER_1);
        AI_SetActiveCalcContext(&context);
        AITraceStart((1u << AI_TRACE_ENTER) | (1u << AI_TRACE_LEAVE) | (1u << AI_TRACE_RESTORE)
            | (1u << AI_TRACE_PREDICTION) | (1u << AI_TRACE_CANDIDATE));
        GetMostSuitableMonToSwitchInto(B_BATTLER_1, SWITCH_MID_BATTLE_OPTIONAL);
        GetMostSuitableMonToSwitchInto(B_BATTLER_1, SWITCH_MID_BATTLE_OPTIONAL);
        SetupAIPredictionData(B_BATTLER_0, SWITCH_MID_BATTLE_OPTIONAL);
        AITraceStop();
        EXPECT(!gAiTraceLog.overflow);
        EXPECT_EQ(gAiTraceLog.depth, 0);
        EXPECT_EQ(gBattleMons[B_BATTLER_1].speed, raw);
        u32 restores = 0, predictions = 0;
        for (u32 n = 0; n < gAiTraceLog.count; n++)
        {
            restores += gAiTraceLog.events[n].kind == AI_TRACE_RESTORE;
            predictions += gAiTraceLog.events[n].kind == AI_TRACE_PREDICTION;
        }
        EXPECT(restores > 0);
        EXPECT(predictions > 0);
        EXPECT(gAiTraceLog.nextSimulation > 2);
        AI_SetActiveCalcContext(NULL);
    }
}

AI_DOUBLE_BATTLE_TEST("AI trace: doubles final actions are unchanged", rng_value_t rng; u16 move1; u16 move2; u8 target1; u8 target2;)
{
    bool32 trace;
    PARAMETRIZE { trace = FALSE; }
    PARAMETRIZE { trace = TRUE; }
    GIVEN {
        AITraceStop();
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
        if (trace)
            AITraceStart((1u << AI_TRACE_ACTION) | (1u << AI_TRACE_FINAL));
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SPLASH); MOVE(playerRight, MOVE_SPLASH);
            EXPECT_MOVE(opponentLeft, MOVE_TACKLE); EXPECT_MOVE(opponentRight, MOVE_TACKLE);
        }
    } THEN {
        AITraceStop();
        results[i].rng = gRngValue;
        results[i].move1 = gLastMoves[B_BATTLER_1];
        results[i].move2 = gLastMoves[B_BATTLER_3];
        results[i].target1 = gAiBattleData->chosenTarget[B_BATTLER_1];
        results[i].target2 = gAiBattleData->chosenTarget[B_BATTLER_3];
        if (trace)
        {
            EXPECT(!gAiTraceLog.overflow);
            EXPECT(gAiTraceLog.count > 0);
        }
    } FINALLY {
        EXPECT_EQ(memcmp(&results[0].rng, &results[1].rng, sizeof(results[0].rng)), 0);
        EXPECT_EQ(results[0].move1, results[1].move1);
        EXPECT_EQ(results[0].move2, results[1].move2);
        EXPECT_EQ(results[0].target1, results[1].target1);
        EXPECT_EQ(results[0].target2, results[1].target2);
    }
}

AI_DOUBLE_BATTLE_TEST("AI trace: actor attribution in doubles, story multi and Frontier")
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
        const struct AiCalcContext *contexts[] = {&left, &right, &ally, NULL};
        for (u32 n = 0; n < ARRAY_COUNT(contexts); n++)
        {
            const struct AiCalcContext *context = contexts[n];
            AI_SetActiveCalcContext(context);
            AITraceStart((1u << AI_TRACE_DAMAGE) | (1u << AI_TRACE_READ));
            CalcBattlerAiMovesData(gAiLogicData, B_BATTLER_1, B_BATTLER_0, AI_GetWeather(), gFieldTimers.terrain);
            AITraceStop();
            EXPECT(!gAiTraceLog.overflow);
            EXPECT(gAiTraceLog.count > 0);
            for (u32 k = 0; k < gAiTraceLog.count; k++)
            {
                const struct AiTraceEvent *event = &gAiTraceLog.events[k];
                EXPECT_EQ(event->actor, context == NULL ? 0xFF : context->actingBattler);
                EXPECT_EQ(event->trainer, context == NULL ? 0xFFFF : context->actingTrainerId);
                EXPECT_EQ(event->policy, context != NULL && context->storyPolicyActive);
            }
            EXPECT(AI_GetActiveCalcContext() == context);
        }
        AI_SetActiveCalcContext(NULL);
        gBattleTypeFlags = savedFlags;
        TRAINER_BATTLE_PARAM.opponentA = savedA;
        TRAINER_BATTLE_PARAM.opponentB = savedB;
        gIsDebugBattle = savedDebug;
    }
}

AI_SINGLE_BATTLE_TEST("AI trace: deterministic RNG transcript and final decision", struct AiTraceEvent events[40]; u32 count; rng_value_t initial; rng_value_t final; u16 move; u8 target;)
{
    u32 repeat;
    PARAMETRIZE { repeat = 0; }
    PARAMETRIZE { repeat = 1; }
    GIVEN {
        (void)repeat;
        AITraceStop();
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE, MOVE_POUND); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); }
    } THEN {
        // GIVEN runs before the harness applies RNGSeed. Begin at the seeded
        // evaluation boundary, not while the fixture is still being declared.
        AITraceStart((1u << AI_TRACE_TAGGED_RNG) | (1u << AI_TRACE_RAW_RNG));
        ComputeAiBattlerDecisions(B_BATTLER_1);
        AITraceStop();
        EXPECT(!gAiTraceLog.overflow);
        EXPECT(gAiTraceLog.count > 0);
        EXPECT(gAiTraceLog.count <= ARRAY_COUNT(results[i].events));
        results[i].count = gAiTraceLog.count;
        memcpy(results[i].events, gAiTraceLog.events, gAiTraceLog.count * sizeof(struct AiTraceEvent));
        results[i].initial = gAiTraceLog.initialRng;
        results[i].final = gAiTraceLog.finalRng;
        results[i].move = gAiBattleData->chosenMoveIndex[B_BATTLER_1];
        results[i].target = gAiBattleData->chosenTarget[B_BATTLER_1];
    } FINALLY {
        EXPECT_EQ(results[0].count, results[1].count);
        EXPECT_EQ(memcmp(results[0].events, results[1].events, results[0].count * sizeof(struct AiTraceEvent)), 0);
        EXPECT_EQ(memcmp(&results[0].initial, &results[1].initial, sizeof(rng_value_t)), 0);
        EXPECT_EQ(memcmp(&results[0].final, &results[1].final, sizeof(rng_value_t)), 0);
        EXPECT_EQ(results[0].move, results[1].move);
        EXPECT_EQ(results[0].target, results[1].target);
    }
}
