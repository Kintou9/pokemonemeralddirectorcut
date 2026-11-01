#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "battle_stat_change.h"

SINGLE_BATTLE_TEST("Growth increases Attack and Sp. Atk by 1 stage or 2 stages under Sun (Gen 5+)")
{
    enum Move move;
    PARAMETRIZE { move = MOVE_CELEBRATE;}
    PARAMETRIZE { move = MOVE_SUNNY_DAY;}

    GIVEN {
        WITH_CONFIG(B_GROWTH_STAT_RAISE, GEN_5);
        ASSUME(GetMoveEffect(MOVE_GROWTH) == EFFECT_GROWTH);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); }
        TURN { MOVE(player, MOVE_GROWTH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GROWTH, player);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
    } THEN {
        if (move == MOVE_CELEBRATE) {
            EXPECT_EQ(player->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 1);
            EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 1);
        }
        else {
            EXPECT_EQ(player->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + 2);
            EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + 2);
        }
    }
}

AI_SINGLE_BATTLE_TEST("Growth: configured execution matches AI prediction in sun and clear weather")
{
    u32 generation;
    enum Move weatherMove;
    PARAMETRIZE { generation = GEN_3; weatherMove = MOVE_SPLASH; }
    PARAMETRIZE { generation = GEN_3; weatherMove = MOVE_SUNNY_DAY; }
    PARAMETRIZE { generation = GEN_5; weatherMove = MOVE_SPLASH; }
    PARAMETRIZE { generation = GEN_5; weatherMove = MOVE_SUNNY_DAY; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        WITH_CONFIG(B_GROWTH_STAT_RAISE, generation);
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Moves(weatherMove, MOVE_GROWTH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, weatherMove); EXPECT_MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_GROWTH); EXPECT_MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        u32 expected = generation >= GEN_5 && weatherMove == MOVE_SUNNY_DAY ? 2 : 1;
        const struct AdditionalEffect *effect = GetMoveAdditionalEffectById(MOVE_GROWTH, 0);
        // Also check the real compiled move data, independently of WITH_CONFIG.
        EXPECT_EQ(GetStatStage(STAT_ATK, &gMovesInfo[MOVE_GROWTH].additionalEffects[0]), B_GROWTH_STAT_RAISE >= GEN_5 ? 1 : 0);
        EXPECT_EQ(GetStatStage(STAT_SPATK, &gMovesInfo[MOVE_GROWTH].additionalEffects[0]), 1);
        EXPECT_EQ(player->statStages[STAT_SPATK], DEFAULT_STAT_STAGE + expected);
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE + (generation >= GEN_5 ? expected : 0));
        EXPECT_EQ(GetStatStage(STAT_ATK, effect), generation >= GEN_5 ? 1 : 0);
        EXPECT_EQ(GetStatStage(STAT_SPATK, effect), 1);
        EXPECT_EQ(AI_GetAdjustedStatStage(B_BATTLER_0, MOVE_GROWTH, GetStatStage(STAT_SPATK, effect)), expected);
        if (generation >= GEN_5)
            EXPECT_EQ(AI_GetAdjustedStatStage(B_BATTLER_0, MOVE_GROWTH, GetStatStage(STAT_ATK, effect)), expected);
    }
}
