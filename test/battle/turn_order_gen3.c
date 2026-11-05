#include "global.h"
#include "test/battle.h"
#include "battle_ai_trace.h"

// Oracle: pret/pokeemerald src/battle_main.c, SetActionsAndBattlersTurnOrder
// and GetWhoStrikesFirst; src/battle_util.c, HandleAction_ActionFinished.
// Switches precede moves; priority/Speed are sampled before any action.
// Ordinary Speed changes affect the NEXT turn, never the remaining queue.
// Keep the expansion's tagged RNG/tie policy; this is not binary RNG parity.

DOUBLE_BATTLE_TEST("Gen3 turn order: paralysis and Speed drops keep the queue")
{
    u32 gen = GEN_3;
    enum Move move = MOVE_THUNDER_WAVE;
    for (u32 g = 0; g < 3; g++)
    {
        PARAMETRIZE { gen = g; move = MOVE_THUNDER_WAVE; }
        PARAMETRIZE { gen = g; move = MOVE_SCARY_FACE; }
    }
    GIVEN {
        if (gen) WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, gen == 1 ? GEN_3 : GEN_8);
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        PLAYER(SPECIES_WOBBUFFET) { Speed(300); }
        PLAYER(SPECIES_WYNAUT) { Speed(20); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(200); }
        OPPONENT(SPECIES_WYNAUT) { Speed(150); }
    } WHEN {
        TURN { MOVE(playerLeft, move, target: opponentLeft, hit: TRUE); MOVE(opponentLeft, MOVE_CELEBRATE, WITH_RNG(RNG_PARALYSIS, FALSE)); }
        TURN { MOVE(opponentLeft, MOVE_CELEBRATE, WITH_RNG(RNG_PARALYSIS, FALSE)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, playerLeft);
        if (gen == 2) {
            MESSAGE("The opposing Wynaut used Celebrate!");
            MESSAGE("The opposing Wobbuffet used Celebrate!");
        } else {
            MESSAGE("The opposing Wobbuffet used Celebrate!");
            MESSAGE("The opposing Wynaut used Celebrate!");
        }
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_CELEBRATE) - 2);
        EXPECT_EQ(opponentRight->pp[0], GetMovePP(MOVE_CELEBRATE) - 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 turn order: Salac Speed increase takes effect next turn")
{
    u32 gen = GEN_3;
    PARAMETRIZE { gen = GEN_3; }
    PARAMETRIZE { gen = GEN_8; }
    GIVEN {
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, gen);
        PLAYER(SPECIES_WOBBUFFET) { Speed(300); }
        PLAYER(SPECIES_WYNAUT) { Speed(20); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); HP(30); MaxHP(100); Item(ITEM_SALAC_BERRY); }
        OPPONENT(SPECIES_WYNAUT) { Speed(120); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); }
        TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SONIC_BOOM, playerLeft);
        if (gen == GEN_8) {
            MESSAGE("The opposing Wobbuffet used Celebrate!");
            MESSAGE("The opposing Wynaut used Celebrate!");
        } else {
            MESSAGE("The opposing Wynaut used Celebrate!");
            MESSAGE("The opposing Wobbuffet used Celebrate!");
        }
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_CELEBRATE) - 2);
    }
}

DOUBLE_BATTLE_TEST("Gen3 turn order: weather moves activate Speed for next turn")
{
    u32 gen = GEN_3;
    enum Move weather = MOVE_RAIN_DANCE;
    enum Ability ability = ABILITY_SWIFT_SWIM;
    for (u32 g = GEN_3; g <= GEN_8; g += GEN_8 - GEN_3) {
        PARAMETRIZE { gen = g; weather = MOVE_RAIN_DANCE; ability = ABILITY_SWIFT_SWIM; }
        PARAMETRIZE { gen = g; weather = MOVE_SUNNY_DAY; ability = ABILITY_CHLOROPHYLL; }
    }
    GIVEN {
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, gen);
        PLAYER(SPECIES_WOBBUFFET) { Speed(300); }
        PLAYER(SPECIES_WYNAUT) { Speed(20); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(ability); }
        OPPONENT(SPECIES_WYNAUT) { Speed(150); }
    } WHEN {
        TURN { MOVE(playerLeft, weather); }
        TURN {}
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, weather, playerLeft);
        if (gen == GEN_8) {
            MESSAGE("The opposing Wobbuffet used Celebrate!");
            MESSAGE("The opposing Wynaut used Celebrate!");
        } else {
            MESSAGE("The opposing Wynaut used Celebrate!");
            MESSAGE("The opposing Wobbuffet used Celebrate!");
        }
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
    }
}

DOUBLE_BATTLE_TEST("Gen3 turn order: weather switch-in must not trigger the Mega sort")
{
    u32 gen = GEN_3;
    enum Ability weather = ABILITY_DRIZZLE, speed = ABILITY_SWIFT_SWIM;
    for (u32 g = GEN_3; g <= GEN_8; g += GEN_8 - GEN_3) {
        PARAMETRIZE { gen = g; weather = ABILITY_DRIZZLE; speed = ABILITY_SWIFT_SWIM; }
        PARAMETRIZE { gen = g; weather = ABILITY_DROUGHT; speed = ABILITY_CHLOROPHYLL; }
    }
    GIVEN {
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, gen);
        WITH_CONFIG(B_MEGA_EVO_TURN_ORDER, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Speed(300); }
        PLAYER(SPECIES_WYNAUT) { Speed(20); }
        PLAYER(SPECIES_PSYDUCK) { Speed(10); Ability(weather); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(speed); }
        OPPONENT(SPECIES_WYNAUT) { Speed(150); }
    } WHEN {
        TURN { SWITCH(playerLeft, 2); }
        TURN {}
    } SCENE {
        MESSAGE("Go! Psyduck!");
        ABILITY_POPUP(playerLeft, weather);
        if (gen == GEN_8) {
            MESSAGE("The opposing Wobbuffet used Celebrate!");
            MESSAGE("The opposing Wynaut used Celebrate!");
        } else {
            MESSAGE("The opposing Wynaut used Celebrate!");
            MESSAGE("The opposing Wobbuffet used Celebrate!");
        }
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("Psyduck used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_CELEBRATE) - 2);
        EXPECT_EQ(playerLeft->pp[0], GetMovePP(MOVE_SPLASH)); // no extra action on switch-in
    }
}

MULTI_BATTLE_TEST("Gen3 turn order: partner battle preserves weather queue")
{
    GIVEN {
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, GEN_3);
        PLAYER(SPECIES_WOBBUFFET) { Speed(300); }
        PARTNER(SPECIES_WYNAUT) { Speed(100); Ability(ABILITY_SWIFT_SWIM); }
        OPPONENT_A(SPECIES_WOBBUFFET) { Speed(150); }
        OPPONENT_B(SPECIES_WYNAUT) { Speed(20); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_RAIN_DANCE); }
        TURN {}
    } SCENE {
        MESSAGE("Wobbuffet used Rain Dance!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
    }
}

SINGLE_BATTLE_TEST("Gen3 turn order: priority precedes Speed and Quick Claw")
{
    bool32 claw;
    PARAMETRIZE { claw = FALSE; }
    PARAMETRIZE { claw = TRUE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(200); Item(ITEM_QUICK_CLAW); }
    } WHEN {
        TURN { MOVE(player, MOVE_QUICK_ATTACK, WITH_RNG(RNG_QUICK_CLAW, claw)); MOVE(opponent, MOVE_SCRATCH, WITH_RNG(RNG_QUICK_CLAW, claw)); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_QUICK_ATTACK, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, opponent);
    }
}

SINGLE_BATTLE_TEST("Gen3 turn order: Quick Claw activation and nonactivation are stable")
{
    bool32 claw;
    PARAMETRIZE { claw = FALSE; }
    PARAMETRIZE { claw = TRUE; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); Item(ITEM_QUICK_CLAW); }
        OPPONENT(SPECIES_WYNAUT) { Speed(200); }
        AITraceStart(1u << AI_TRACE_TAGGED_RNG);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH, WITH_RNG(RNG_QUICK_CLAW, claw)); MOVE(opponent, MOVE_SPLASH, WITH_RNG(RNG_QUICK_CLAW, claw)); }
    } SCENE {
        if (claw) {
            MESSAGE("Wobbuffet used Scratch!");
            MESSAGE("The opposing Wynaut used Splash!");
        } else {
            MESSAGE("The opposing Wynaut used Splash!");
            MESSAGE("Wobbuffet used Scratch!");
        }
    } THEN {
        AITraceStop();
        u32 count = 0;
        for (u32 n = 0; n < gAiTraceLog.count; n++)
            if (gAiTraceLog.events[n].after[1] == RNG_QUICK_CLAW)
                count++;
        EXPECT(!gAiTraceLog.overflow);
        EXPECT_EQ(count, 1); // only the holder rolls, once per turn
    }
}

SINGLE_BATTLE_TEST("Gen3 turn order: controlled Speed ties execute each action once")
{
    u32 tie;
    PARAMETRIZE { tie = 0; }
    PARAMETRIZE { tie = 6; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WYNAUT) { Speed(100); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH, WITH_RNG(RNG_SPEED_TIE, tie)); MOVE(opponent, MOVE_SPLASH, WITH_RNG(RNG_SPEED_TIE, tie)); }
    } SCENE {
        if (tie == 0) {
            MESSAGE("Wobbuffet used Splash!");
            MESSAGE("The opposing Wynaut used Splash!");
        } else {
            MESSAGE("The opposing Wynaut used Splash!");
            MESSAGE("Wobbuffet used Splash!");
        }
    } THEN {
        EXPECT_EQ(player->pp[0], GetMovePP(MOVE_SPLASH) - 1);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_SPLASH) - 1);
    }
}

DOUBLE_BATTLE_TEST("Gen3 turn order: a previous Mega Evolution does not enable later re-sorts")
{
    GIVEN {
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, GEN_3);
        WITH_CONFIG(B_MEGA_EVO_TURN_ORDER, GEN_7);
        PLAYER(SPECIES_WOBBUFFET) { Speed(300); }
        PLAYER(SPECIES_GARDEVOIR) { Speed(20); Item(ITEM_GARDEVOIRITE); }
        PLAYER(SPECIES_PSYDUCK) { Speed(10); Ability(ABILITY_DRIZZLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_SWIFT_SWIM); }
        OPPONENT(SPECIES_WYNAUT) { Speed(150); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_CELEBRATE, gimmick: GIMMICK_MEGA); }
        TURN { SWITCH(playerLeft, 2); }
    } SCENE {
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_MEGA_EVOLUTION, playerRight);
        MESSAGE("Wobbuffet used Celebrate!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("Gardevoir used Celebrate!");
        MESSAGE("Go! Psyduck!");
        MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("The opposing Wobbuffet used Celebrate!");
        MESSAGE("Gardevoir used Celebrate!");
    }
}

TWO_VS_ONE_BATTLE_TEST("Gen3 turn order: multiple switches keep their queue and precede priority moves")
{
    u32 gen;
    PARAMETRIZE { gen = GEN_3; }
    PARAMETRIZE { gen = GEN_8; }
    GIVEN {
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, gen);
        PLAYER(SPECIES_GASTLY) { Speed(50); Moves(MOVE_QUICK_ATTACK); }
        PARTNER(SPECIES_HAUNTER) { Speed(10); }
        PARTNER(SPECIES_GENGAR) { Speed(90); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_WYNAUT) { Speed(200); Ability(ABILITY_NONE); }
        OPPONENT(SPECIES_GIRAFARIG) { Speed(20); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_FARIGIRAF) { Speed(30); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN {
            SWITCH(opponentLeft, 3);
            SWITCH(playerRight, 1);
            SWITCH(opponentRight, 2);
            MOVE(playerLeft, MOVE_QUICK_ATTACK, target: opponentLeft);
        }
    } SCENE {
        MESSAGE("2 withdrew Wobbuffet!");
        MESSAGE("2 sent out Farigiraf!");
        if (gen == GEN_3) {
            MESSAGE("3 withdrew Haunter!");
            MESSAGE("3 sent out Gengar!");
            MESSAGE("2 withdrew Wynaut!");
            MESSAGE("2 sent out Girafarig!");
        } else {
            MESSAGE("2 withdrew Wynaut!");
            MESSAGE("2 sent out Girafarig!");
            MESSAGE("3 withdrew Haunter!");
            MESSAGE("3 sent out Gengar!");
        }
        MESSAGE("Gastly used Quick Attack!");
    } THEN {
        EXPECT_EQ(playerLeft->pp[0], GetMovePP(MOVE_QUICK_ATTACK) - 1);
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentRight->pp[0], GetMovePP(MOVE_SPLASH));
    }
}
