#include "global.h"
#include "test/battle.h"
#include "battle_ai_calc_context.h"
#include "battle_ai_main.h"
#include "battle_main.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "debug.h"
#include "pokemon.h"
#include "random.h"
#include "constants/opponents.h"

AI_SINGLE_BATTLE_TEST("AI calculation adapter parity: turn order after simulated Speed stage change")
{
    bool32 lowerSpeed;
    PARAMETRIZE { lowerSpeed = FALSE; }
    PARAMETRIZE { lowerSpeed = TRUE; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(75); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        struct AiCalcContext context;
        struct BattleCalcValues values = {0};
        const struct AiCalcContext *previous = AI_GetActiveCalcContext();
        u8 savedStage = gBattleMons[B_BATTLER_1].statStages[STAT_SPEED];
        s32 originalOrder, contextOrder;
        u32 recalculatedSpeed, cachedSpeed;

        values.battlerAtk = B_BATTLER_1;
        values.battlerDef = B_BATTLER_0;
        for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
        {
            values.abilities[battler] = gAiLogicData->abilities[battler];
            values.holdEffects[battler] = gAiLogicData->holdEffects[battler];
        }
        AI_InitCalcContext(&context, B_BATTLER_1);
        context.storyPolicyActive = TRUE; // Recorded fixtures otherwise exclude the policy.
        AI_SetActiveCalcContext(&context);
        // InitializeSwitchinCandidate calls SetBattlerAiData before applying
        // switch-in stat stages (e.g. Sticky Web). Do not refresh that cache:
        // this fixture preserves the existing simulation's ordering.
        if (lowerSpeed)
            gBattleMons[B_BATTLER_1].statStages[STAT_SPEED]--;

        originalOrder = GetWhichBattlerFasterOrTies(&values, TRUE);
        // Exercise the same adapter now called by the private AI helper.
        contextOrder = AI_CompareTurnOrder(&context, &values, TRUE);
        recalculatedSpeed = GetBattlerTotalSpeedStat(B_BATTLER_1,
            values.abilities[B_BATTLER_1], values.holdEffects[B_BATTLER_1]);
        cachedSpeed = AI_GetEffectiveSpeed(B_BATTLER_1);

        gBattleMons[B_BATTLER_1].statStages[STAT_SPEED] = savedStage;
        AI_SetActiveCalcContext(previous);

        EXPECT_EQ(cachedSpeed, 100);
        EXPECT_EQ(recalculatedSpeed, lowerSpeed ? 66 : 100);
        EXPECT_EQ(originalOrder, lowerSpeed ? AI_IS_SLOWER : AI_IS_FASTER);
        EXPECT_EQ(contextOrder, originalOrder);
    }
}

static void CheckTurnOrderModifiers(const struct AiCalcContext *context, enum BattlerId actor, enum BattlerId foe)
{
    static const struct {
        u8 stage;
        u32 status;
        enum Ability ability;
        enum HoldEffect item;
        u16 weather;
    } cases[] = {
        {6, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_NONE, 0},
        {5, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_NONE, 0},
        {8, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_NONE, 0},
        {4, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_NONE, 0},
        {6, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_NONE, 0},
        {6, STATUS1_PARALYSIS, ABILITY_SHADOW_TAG, HOLD_EFFECT_NONE, 0},
        {6, 0, ABILITY_SWIFT_SWIM, HOLD_EFFECT_NONE, B_WEATHER_RAIN},
        {5, 0, ABILITY_CHLOROPHYLL, HOLD_EFFECT_NONE, B_WEATHER_SUN},
        {6, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_MACHO_BRACE, 0},
        {5, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_QUICK_CLAW, 0},
        {6, 0, ABILITY_SHADOW_TAG, HOLD_EFFECT_QUICK_CLAW, 0},
    };
    struct BattleCalcValues values = {0};
    const struct AiCalcContext *previous = AI_GetActiveCalcContext();
    u8 stage = gBattleMons[actor].statStages[STAT_SPEED];
    u32 status = gBattleMons[actor].status1;
    u16 weather = gBattleWeather;
    u16 cache = gAiLogicData->speedStats[actor];
    bool32 actorQuick = gProtectStructs[actor].usedCustapBerry;
    bool32 foeQuick = gProtectStructs[foe].usedCustapBerry;
    u16 savedIme = REG_IME;
    REG_IME = 0; // Exclude VBlank's AdvanceRandom from this synchronous probe.
    rng_value_t rng = gRngValue;

    EXPECT(!(gFieldStatuses & STATUS_FIELD_TRICK_ROOM));
    values.battlerAtk = actor;
    values.battlerDef = foe;
    values.abilities[foe] = gAiLogicData->abilities[foe];
    values.holdEffects[foe] = gAiLogicData->holdEffects[foe];
    AI_SetActiveCalcContext(context);
    for (u32 i = 0; i < ARRAY_COUNT(cases); i++)
    {
        gBattleMons[actor].statStages[STAT_SPEED] = cases[i].stage;
        gBattleMons[actor].status1 = cases[i].status;
        gBattleWeather = cases[i].weather;
        values.abilities[actor] = cases[i].ability;
        values.holdEffects[actor] = cases[i].item;
        for (u32 quick = 0; quick < (cases[i].item == HOLD_EFFECT_QUICK_CLAW ? 4 : 1); quick++)
        {
            // TryChangingTurnOrderEffects stores Quick Claw activation in
            // usedCustapBerry too. Cover neither, either and both activating.
            gProtectStructs[actor].usedCustapBerry = (quick & 1) != 0;
            gProtectStructs[foe].usedCustapBerry = (quick & 2) != 0;
            for (u32 ignore = 0; ignore <= 1; ignore++)
            {
                s32 original = GetWhichBattlerFasterOrTies(&values, ignore);
                EXPECT_EQ(AI_CompareTurnOrder(context, &values, ignore), original);
                EXPECT_EQ(AI_CompareTurnOrder(NULL, &values, ignore), original);
            }
        }
        EXPECT_EQ(gAiLogicData->speedStats[actor], cache);
        EXPECT(AI_GetActiveCalcContext() == context);
    }
    // This prediction helper must not roll Quick Claw or resolve Speed ties
    // randomly. It preserves the original comparator's ambiguous result.
    EXPECT_EQ(memcmp(&rng, &gRngValue, sizeof(rng)), 0);
    gBattleMons[actor].statStages[STAT_SPEED] = stage;
    gBattleMons[actor].status1 = status;
    gBattleWeather = weather;
    gProtectStructs[actor].usedCustapBerry = actorQuick;
    gProtectStructs[foe].usedCustapBerry = foeQuick;
    AI_SetActiveCalcContext(previous);
    REG_IME = savedIme;
}

AI_SINGLE_BATTLE_TEST("AI calculation adapter parity: Gen 3 modifiers, ties, candidates and restoration")
{
    bool32 policyActive;
    PARAMETRIZE { policyActive = FALSE; }
    PARAMETRIZE { policyActive = TRUE; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(150); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        struct AiCalcContext context;
        struct BattlePokemon saved = gBattleMons[B_BATTLER_1];
        AI_InitCalcContext(&context, B_BATTLER_1);
        context.storyPolicyActive = policyActive;
        CheckTurnOrderModifiers(&context, B_BATTLER_1, B_BATTLER_0);
        // Same identity and restored party index, different candidate Speed.
        PokemonToBattleMon(&GetBattlerParty(B_BATTLER_1)[1], &gBattleMons[B_BATTLER_1]);
        gBattleMons[B_BATTLER_1].personality = saved.personality;
        EXPECT_EQ(gBattleMons[B_BATTLER_1].speed, 150);
        CheckTurnOrderModifiers(&context, B_BATTLER_1, B_BATTLER_0);
        gBattleMons[B_BATTLER_1] = saved;
        CheckTurnOrderModifiers(&context, B_BATTLER_1, B_BATTLER_0);
        EXPECT_EQ(context.rawSpeed[B_BATTLER_1], 100);
        EXPECT_EQ(context.storyPolicyActive, policyActive);
    }
}

AI_DOUBLE_BATTLE_TEST("AI calculation adapter parity: doubles and independent Space Center actors")
{
    bool32 multi;
    PARAMETRIZE { multi = FALSE; }
    PARAMETRIZE { multi = TRUE; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_TACKLE); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SPLASH); MOVE(playerRight, MOVE_SPLASH);
            EXPECT_MOVE(opponentLeft, MOVE_TACKLE); EXPECT_MOVE(opponentRight, MOVE_TACKLE);
        }
    } THEN {
        struct AiCalcContext context;
        u32 flags = gBattleTypeFlags;
        u16 trainerA = TRAINER_BATTLE_PARAM.opponentA;
        u16 trainerB = TRAINER_BATTLE_PARAM.opponentB;
        bool8 debug = gIsDebugBattle;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE;
        if (multi)
            gBattleTypeFlags |= BATTLE_TYPE_MULTI | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER;
        TRAINER_BATTLE_PARAM.opponentA = TRAINER_MAXIE_MOSSDEEP;
        TRAINER_BATTLE_PARAM.opponentB = multi ? TRAINER_TABITHA_MOSSDEEP : 0;
        gIsDebugBattle = FALSE;
        for (enum BattlerId actor = B_BATTLER_1; actor <= B_BATTLER_3; actor += 2)
        {
            AI_InitCalcContext(&context, actor);
            EXPECT(context.storyPolicyActive);
            u16 trainer = actor == B_BATTLER_3 && multi ? TRAINER_TABITHA_MOSSDEEP : TRAINER_MAXIE_MOSSDEEP;
            EXPECT_EQ(context.actingTrainerId, trainer);
            CheckTurnOrderModifiers(&context, actor, B_BATTLER_0);
            EXPECT_EQ(context.actingTrainerId, trainer);
            EXPECT_EQ(context.actingBattler, actor);
        }
        AI_InitCalcContext(&context, B_BATTLER_2);
        EXPECT(!context.storyPolicyActive); // Steven remains outside the policy.
        CheckTurnOrderModifiers(&context, B_BATTLER_2, B_BATTLER_1);
        gBattleTypeFlags = flags;
        TRAINER_BATTLE_PARAM.opponentA = trainerA;
        TRAINER_BATTLE_PARAM.opponentB = trainerB;
        gIsDebugBattle = debug;
        EXPECT(AI_GetActiveCalcContext() == NULL);
    }
}
