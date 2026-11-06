#include "global.h"
#include "test/battle.h"

// Original Emerald: HandleAction_TryFinish -> HandleFaintedMonActions runs
// after the whole move, before advancing the action queue. A replacement
// cancels the fainted slot's action; it does not inherit that action.
DOUBLE_BATTLE_TEST("Gen3 faint replacement: entry and abilities precede slower actions")
{
    u32 gen = 0, ability = 0;
    for (u32 g = 0; g < 3; g++)
        for (u32 a = 0; a < 4; a++)
            PARAMETRIZE { gen = g; ability = a; }
    static const enum Ability abilities[] = {ABILITY_INTIMIDATE, ABILITY_DRIZZLE, ABILITY_DROUGHT, ABILITY_SAND_STREAM};
    GIVEN {
        if (gen) WITH_CONFIG(B_FAINT_SWITCH_IN, gen == 1 ? GEN_3 : GEN_4);
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PLAYER(SPECIES_WYNAUT) { Speed(200); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(300); }
        OPPONENT(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT(SPECIES_GYARADOS) { Speed(500); Ability(abilities[ability]); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); SEND_OUT(opponentLeft, 2); }
    } SCENE {
        MESSAGE("The opposing Wobbuffet fainted!");
        if (gen == 2) { MESSAGE("Wynaut used Celebrate!"); MESSAGE("The opposing Wynaut used Celebrate!"); }
        MESSAGE("2 sent out Gyarados!");
        ABILITY_POPUP(opponentLeft, abilities[ability]);
        if (gen != 2) { MESSAGE("Wynaut used Celebrate!"); MESSAGE("The opposing Wynaut used Celebrate!"); }
        NOT MESSAGE("The opposing Gyarados used Celebrate!");
    } THEN {
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT], 2);
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_CELEBRATE) - 1);
        EXPECT_EQ(opponentRight->pp[0], GetMovePP(MOVE_CELEBRATE) - 1);
    }
}

DOUBLE_BATTLE_TEST("Gen3 faint replacement: Tate and Liza format resolves spread recipients before replacements")
{
    u32 gen = GEN_3, reserves = 1;
    for (u32 g = GEN_3; g <= GEN_4; g++)
        for (u32 r = 1; r <= 2; r++)
            PARAMETRIZE { gen = g; reserves = r; }
    GIVEN {
        WITH_CONFIG(B_FAINT_SWITCH_IN, gen);
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PLAYER(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT(SPECIES_LUNATONE) { HP(1); Speed(300); }
        OPPONENT(SPECIES_SOLROCK) { HP(1); Speed(200); }
        OPPONENT(SPECIES_CLAYDOL) { Speed(50); Moves(MOVE_SPLASH); }
        if (reserves == 2) OPPONENT(SPECIES_XATU) { Speed(40); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SURF); SEND_OUT(opponentLeft, 2); if (reserves == 2) SEND_OUT(opponentRight, 3); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight);
        if (gen >= GEN_4) MESSAGE("Wynaut used Celebrate!");
        MESSAGE("2 sent out Claydol!");
        if (reserves == 2) MESSAGE("2 sent out Xatu!");
        if (gen == GEN_3) MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->species, SPECIES_CLAYDOL);
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentLeft->hp, opponentLeft->maxHP); // no spillover from Surf
        if (reserves == 2) {
            EXPECT_EQ(opponentRight->species, SPECIES_XATU);
            EXPECT_EQ(opponentRight->hp, opponentRight->maxHP);
            EXPECT_EQ(opponentRight->pp[0], GetMovePP(MOVE_SPLASH));
            EXPECT_NE(gBattlerPartyIndexes[B_POSITION_OPPONENT_LEFT], gBattlerPartyIndexes[B_POSITION_OPPONENT_RIGHT]);
        } else EXPECT(gAbsentBattlerFlags & (1u << B_POSITION_OPPONENT_RIGHT));
    }
}

DOUBLE_BATTLE_TEST("Gen3 faint replacement: Explosion completes all recipients before replacing its user")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PLAYER(SPECIES_GASTLY) { Speed(100); }
        PLAYER(SPECIES_PSYDUCK) { Speed(50); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); Speed(300); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(200); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(40); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_GOLDEEN) { Speed(30); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_EXPLOSION); SEND_OUT(playerLeft, 2); SEND_OUT(opponentLeft, 2); SEND_OUT(opponentRight, 3); }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentRight);
        SEND_IN_MESSAGE("Psyduck");
        MESSAGE("2 sent out Magikarp!");
        MESSAGE("2 sent out Goldeen!");
        MESSAGE("Gastly used Celebrate!");
    } THEN {
        EXPECT_EQ(playerLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentRight->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(playerLeft->hp, playerLeft->maxHP);
    }
}

DOUBLE_BATTLE_TEST("Gen3 faint replacement: Destiny Bond replaces both sides before the remaining action")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(400); }
        PLAYER(SPECIES_PSYDUCK) { Speed(100); }
        PLAYER(SPECIES_GOLDEEN) { Speed(50); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WYNAUT) { Speed(300); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(200); }
        OPPONENT(SPECIES_GYARADOS) { Speed(40); Ability(ABILITY_NONE); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_DESTINY_BOND); MOVE(opponentLeft, MOVE_SONIC_BOOM, target: playerLeft, hit: TRUE); SEND_OUT(playerLeft, 2); SEND_OUT(opponentLeft, 2); }
    } SCENE {
        MESSAGE("Wobbuffet fainted!");
        MESSAGE("The opposing Wynaut fainted!");
        SEND_IN_MESSAGE("Goldeen");
        MESSAGE("2 sent out Gyarados!");
        MESSAGE("The opposing Magikarp used Celebrate!");
        MESSAGE("Psyduck used Celebrate!");
    } THEN {
        EXPECT_EQ(playerLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
    }
}

DOUBLE_BATTLE_TEST("Gen3 faint replacement: entry hazard KO replaces again before slower actions")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PLAYER(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT(SPECIES_PSYDUCK) { HP(1); Speed(300); }
        OPPONENT(SPECIES_GOLDEEN) { Speed(200); }
        OPPONENT(SPECIES_MAGIKARP) { HP(1); MaxHP(100); Speed(50); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_GYARADOS) { Speed(40); Ability(ABILITY_NONE); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPIKES); }
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); SEND_OUT(opponentLeft, 2); SEND_OUT(opponentLeft, 3); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SONIC_BOOM, playerLeft);
        MESSAGE("The opposing Psyduck fainted!");
        MESSAGE("2 sent out Magikarp!");
        MESSAGE("The opposing Magikarp fainted!");
        MESSAGE("2 sent out Gyarados!");
        MESSAGE("The opposing Goldeen used Celebrate!");
        MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->species, SPECIES_GYARADOS);
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_GT(opponentLeft->hp, 0); // The harness ends an unfinished battle by teleporting.
    }
}

MULTI_BATTLE_TEST("Gen3 faint replacement: Space Center format preserves player and Steven party ownership")
{
    bool32 partner;
    PARAMETRIZE { partner = FALSE; }
    PARAMETRIZE { partner = TRUE; }
    GIVEN {
        PLAYER(SPECIES_SWAMPERT) { HP(1); Speed(100); }
        PLAYER(SPECIES_SCEPTILE) { Speed(90); Moves(MOVE_SPLASH); }
        PARTNER(SPECIES_METANG) { HP(1); Speed(200); }
        PARTNER(SPECIES_SKARMORY) { Speed(80); Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_CAMERUPT) { Speed(400); }
        OPPONENT_B(SPECIES_MIGHTYENA) { Speed(300); Ability(ABILITY_NONE); }
    } WHEN {
        TURN { MOVE(opponentLeft, MOVE_SONIC_BOOM, target: partner ? playerRight : playerLeft, hit: TRUE); SEND_OUT(partner ? playerRight : playerLeft, 1); }
    } SCENE {
        if (partner) MESSAGE("3 sent out Skarmory!");
        else SEND_IN_MESSAGE("Sceptile");
        MESSAGE("The opposing Mightyena used Celebrate!");
    } THEN {
        EXPECT_EQ(playerLeft->species, partner ? SPECIES_SWAMPERT : SPECIES_SCEPTILE);
        EXPECT_EQ(playerRight->species, partner ? SPECIES_SKARMORY : SPECIES_METANG);
        EXPECT_EQ(partner ? playerRight->pp[0] : playerLeft->pp[0], GetMovePP(MOVE_SPLASH));
    }
}

MULTI_BATTLE_TEST("Gen3 faint replacement: opponent trainers replace only from their own parties")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PARTNER(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT_A(SPECIES_MAGIKARP) { HP(1); Speed(300); }
        OPPONENT_A(SPECIES_GYARADOS) { Speed(50); Ability(ABILITY_NONE); Moves(MOVE_SPLASH); }
        OPPONENT_B(SPECIES_GOLDEEN) { HP(1); Speed(200); }
        OPPONENT_B(SPECIES_SEAKING) { Speed(40); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SWIFT); SEND_OUT(opponentLeft, 1); SEND_OUT(opponentRight, 1); }
    } SCENE {
        MESSAGE("2 sent out Gyarados!");
        MESSAGE("4 sent out Seaking!");
        MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->species, SPECIES_GYARADOS);
        EXPECT_EQ(opponentRight->species, SPECIES_SEAKING);
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(opponentRight->pp[0], GetMovePP(MOVE_SPLASH));
    }
}

DOUBLE_BATTLE_TEST("Gen3 faint replacement: final opposing Pokemon ends battle without later moves")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PLAYER(SPECIES_WYNAUT) { Speed(100); }
        OPPONENT(SPECIES_MAGIKARP) { HP(1); Speed(300); }
        OPPONENT(SPECIES_GOLDEEN) { HP(1); Speed(200); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SWIFT); }
    } SCENE {
        MESSAGE("The opposing Magikarp fainted!");
        MESSAGE("The opposing Goldeen fainted!");
        NOT MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_CELEBRATE));
    }
}

DOUBLE_BATTLE_TEST("Gen3 faint replacement: timing Speed and spread damage configuration matrix", s16 damage)
{
    u32 faintGen = GEN_3, orderGen = GEN_3, spreadGen = GEN_3;
    for (u32 f = GEN_3; f <= GEN_4; f++)
        for (u32 o = GEN_3; o <= GEN_8; o += GEN_8 - GEN_3)
            for (u32 d = GEN_3; d <= GEN_4; d++)
                PARAMETRIZE { faintGen = f; orderGen = o; spreadGen = d; }
    GIVEN {
        WITH_CONFIG(B_FAINT_SWITCH_IN, faintGen);
        WITH_CONFIG(B_RECALC_TURN_AFTER_ACTIONS, orderGen);
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, spreadGen);
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); }
        PLAYER(SPECIES_WYNAUT) { Speed(100); Level(50); SpAttack(100); Ability(ABILITY_SWIFT_SWIM); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(300); HP(1); }
        OPPONENT(SPECIES_WYNAUT) { Speed(150); HP(1000); MaxHP(1000); SpDefense(100); }
        OPPONENT(SPECIES_PSYDUCK) { Speed(50); HP(1000); MaxHP(1000); Ability(ABILITY_DRIZZLE); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE);
            MOVE(playerRight, MOVE_SURF, WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
            SEND_OUT(opponentLeft, 2);
        }
    } SCENE {
        MESSAGE("The opposing Wobbuffet fainted!");
        if (faintGen == GEN_3) { MESSAGE("2 sent out Psyduck!"); ABILITY_POPUP(opponentLeft, ABILITY_DRIZZLE); }
        if (faintGen != GEN_3 || orderGen == GEN_3) MESSAGE("The opposing Wynaut used Celebrate!");
        MESSAGE("Wynaut used Surf!");
        HP_BAR(opponentRight, captureDamage: &results[i].damage);
        if (faintGen == GEN_3 && orderGen == GEN_8) MESSAGE("The opposing Wynaut used Celebrate!");
        if (faintGen != GEN_3) { MESSAGE("2 sent out Psyduck!"); ABILITY_POPUP(opponentLeft, ABILITY_DRIZZLE); }
    } THEN {
        // Rain and two targets only exist during Surf with immediate replacement.
        EXPECT_EQ(results[i].damage, faintGen == GEN_3 ? (spreadGen == GEN_3 ? 33 : 48) : 43);
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_SURF) - 1);
    }
}

AI_MULTI_BATTLE_TEST("Gen3 faint replacement: Space Center AI partner continues after enemy replacement")
{
    GIVEN {
        AI_FLAGS(0);
        PLAYER(SPECIES_SWAMPERT) { Speed(400); }
        PARTNER(SPECIES_METANG) { Speed(100); Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_CAMERUPT) { HP(1); Speed(300); Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_GYARADOS) { Speed(50); Ability(ABILITY_NONE); Moves(MOVE_SPLASH); }
        OPPONENT_B(SPECIES_MIGHTYENA) { Speed(200); Ability(ABILITY_NONE); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); EXPECT_SEND_OUT(opponentLeft, 1); EXPECT_MOVE(playerRight, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("The opposing Camerupt fainted!");
        MESSAGE(AI_TRAINER_NAME " sent out Gyarados!");
        MESSAGE("The opposing Mightyena used Splash!");
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SPLASH, playerRight);
    } THEN {
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(playerRight->pp[0], GetMovePP(MOVE_SPLASH) - 1);
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
    }
}

AI_DOUBLE_BATTLE_TEST("Gen3 faint replacement: trainer doubles and Battle Tower retain format and EXP rules")
{
    bool32 frontier;
    PARAMETRIZE { frontier = FALSE; }
    PARAMETRIZE { frontier = TRUE; }
    GIVEN {
        AI_FLAGS(0);
        if (frontier) gBattleTestRunnerState->data.recordedBattle.battleFlags |= BATTLE_TYPE_BATTLE_TOWER;
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(400); Level(50); }
        PLAYER(SPECIES_WYNAUT) { Speed(100); Level(50); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); }
        OPPONENT(SPECIES_LUNATONE) { HP(1); Speed(300); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_SOLROCK) { Speed(200); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_CLAYDOL) { Speed(50); Ability(ABILITY_INTIMIDATE); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); EXPECT_SEND_OUT(opponentLeft, 2); }
    } SCENE {
        MESSAGE("The opposing Lunatone fainted!");
        ABILITY_POPUP(opponentLeft, ABILITY_INTIMIDATE); // Entry marker independent of Frontier trainer names.
        MESSAGE("The opposing Solrock used Splash!");
        MESSAGE("Wynaut used Celebrate!");
    } THEN {
        EXPECT_EQ(opponentLeft->pp[0], GetMovePP(MOVE_SPLASH));
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ((u32)gSaveBlock2Ptr->optionsBattleStyle, OPTIONS_BATTLE_STYLE_SHIFT);
        u32 exp = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_EXP);
        u32 initialExp = gExperienceTables[gSpeciesInfo[SPECIES_WOBBUFFET].growthRate][50];
        if (frontier) EXPECT_EQ(exp, initialExp);
        else EXPECT_GT(exp, initialExp);
    }
}
