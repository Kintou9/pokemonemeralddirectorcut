#include "global.h"
#include "test/battle.h"
#include "battle_ai_util.h"
#include "constants/opponents.h"

// The battle-test build does not populate gTrainers for these story IDs. Keep
// these profiles aligned with their AI: entries in src/data/trainers.party.
#define BRENDAN_103_AI_FLAGS AI_FLAG_BASIC_TRAINER
#define WALLY_VR_AI_FLAGS (AI_FLAG_BASIC_TRAINER | AI_FLAG_SMART_SWITCHING | AI_FLAG_TRY_TO_2HKO)
#define NORMAN_1_AI_FLAGS (AI_FLAG_BASIC_TRAINER | AI_FLAG_HP_AWARE | AI_FLAG_SMART_SWITCHING | AI_FLAG_TRY_TO_2HKO | AI_FLAG_PP_STALL_PREVENTION)

// These use real story-trainer flag profiles with synthetic parties. The player
// species, level, moves and RNG stay fixed within each parameterized pair.
// A changed decision proves influence in this setup; an unchanged one does not
// establish that the AI lacks access to the varied value.

AI_SINGLE_BATTLE_TEST("Story AI fairness: Brendan's move choice changes with unrevealed player Speed")
{
    u16 playerSpeed;
    PARAMETRIZE { playerSpeed = 1; }
    PARAMETRIZE { playerSpeed = 100; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(BRENDAN_103_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(playerSpeed); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_QUICK_ATTACK, MOVE_STRENGTH); }
    } WHEN {
        TURN { EXPECT_MOVES(opponent, MOVE_QUICK_ATTACK, MOVE_STRENGTH); }
    } THEN {
        EXPECT(gAiLogicData->speedStats[B_POSITION_PLAYER_LEFT] == playerSpeed);
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], playerSpeed == 1 ? MOVE_STRENGTH : MOVE_QUICK_ATTACK);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's move choice changes with unrevealed Speed")
{
    u16 playerSpeed;

    PARAMETRIZE { playerSpeed = 1; }
    PARAMETRIZE { playerSpeed = 100; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(playerSpeed); Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(50); Moves(MOVE_QUICK_ATTACK, MOVE_STRENGTH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_QUICK_ATTACK, MOVE_STRENGTH); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], playerSpeed == 1 ? MOVE_STRENGTH : MOVE_QUICK_ATTACK);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Norman's flags evaluate a fixed-damage KO at matching HP bars")
{
    u16 hp, maxHP;

    PARAMETRIZE { hp = 40; maxHP = 80; }
    PARAMETRIZE { hp = 41; maxHP = 82; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { HP(hp); MaxHP(maxHP); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_DRAGON_RAGE, MOVE_SWORDS_DANCE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); SCORE_EQ_VAL(opponent, MOVE_DRAGON_RAGE, hp == 40 ? 111 : 103); EXPECT_MOVES(opponent, MOVE_DRAGON_RAGE, MOVE_SWORDS_DANCE); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_DRAGON_RAGE);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's flags compare hidden player Defense")
{
    u16 playerDefense;

    PARAMETRIZE { playerDefense = 1; }
    PARAMETRIZE { playerDefense = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Defense(playerDefense); HP(100); MaxHP(200); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); Moves(MOVE_STRENGTH, MOVE_DRAGON_RAGE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_DRAGON_RAGE); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        EXPECT(playerDefense == 1 ? damage > 100 : damage < 100);
        EXPECT(gLastMoves[B_POSITION_OPPONENT_LEFT] == MOVE_STRENGTH);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's move choice changes with hidden Defense")
{
    u16 playerDefense;

    PARAMETRIZE { playerDefense = 1; }
    PARAMETRIZE { playerDefense = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Defense(playerDefense); SpDefense(100); HP(100); MaxHP(200); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); SpAttack(200); Moves(MOVE_STRENGTH, MOVE_SURF); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_SURF); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], playerDefense == 1 ? MOVE_STRENGTH : MOVE_SURF);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's flags receive hidden player PP limitations")
{
    u8 pp;

    PARAMETRIZE { pp = 0; }
    PARAMETRIZE { pp = 1; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { MovesWithPP({MOVE_TACKLE, pp}, {MOVE_SPLASH, 40}); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT(((gAiLogicData->moveLimitations[B_POSITION_PLAYER_LEFT] & 1) != 0) == (pp == 0));
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Norman's flags receive actual trapping ability")
{
    enum Ability ability;

    PARAMETRIZE { ability = ABILITY_MAGNET_PULL; }
    PARAMETRIZE { ability = ABILITY_STURDY; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_MAGNETON) { Ability(ability); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_MAGNETON) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        if (ability == ABILITY_MAGNET_PULL)
            EXPECT(gAiLogicData->abilities[B_POSITION_PLAYER_LEFT] == ABILITY_MAGNET_PULL);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's flags simulate hidden player Special Defense")
{
    u16 playerSpDefense;

    PARAMETRIZE { playerSpDefense = 1; }
    PARAMETRIZE { playerSpDefense = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { SpDefense(playerSpDefense); HP(100); MaxHP(200); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { SpAttack(100); Moves(MOVE_THUNDERBOLT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_THUNDERBOLT); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        EXPECT(playerSpDefense == 1 ? damage > 100 : damage < 100);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's damage simulation incorporates player Defense IV")
{
    u8 defenseIV;

    PARAMETRIZE { defenseIV = 0; }
    PARAMETRIZE { defenseIV = 31; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { DefenseIV(defenseIV); HP(200); MaxHP(200); Moves(MOVE_SPLASH); }
        CalculateMonStats(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0]);
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); Moves(MOVE_STRENGTH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_STRENGTH); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        EXPECT(defenseIV == 0 ? damage > 50 : damage < 50);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's damage simulation incorporates player Defense EV")
{
    u8 defenseEV;

    PARAMETRIZE { defenseEV = 0; }
    PARAMETRIZE { defenseEV = 252; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); Moves(MOVE_SPLASH); }
        SetMonData(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0], MON_DATA_DEF_EV, &defenseEV);
        CalculateMonStats(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0]);
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); Moves(MOVE_STRENGTH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_STRENGTH); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        EXPECT(defenseEV == 0 ? damage > 45 : damage < 45);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's incoming damage simulation incorporates hidden player Attack")
{
    u16 playerAttack;

    PARAMETRIZE { playerAttack = 1; }
    PARAMETRIZE { playerAttack = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Attack(playerAttack); Moves(MOVE_STRENGTH, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Moves(MOVE_PROTECT); }
    } WHEN {
        TURN { MOVE(player, MOVE_STRENGTH); EXPECT_MOVE(opponent, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_PROTECT); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_PLAYER_LEFT][B_POSITION_OPPONENT_LEFT][0].median;
        EXPECT(playerAttack == 1 ? damage < 20 : damage > 20);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's incoming damage simulation incorporates hidden player Sp Attack")
{
    u16 playerSpAttack;

    PARAMETRIZE { playerSpAttack = 1; }
    PARAMETRIZE { playerSpAttack = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { SpAttack(playerSpAttack); Moves(MOVE_THUNDERBOLT, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Moves(MOVE_PROTECT); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDERBOLT); EXPECT_MOVE(opponent, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_PROTECT); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_PLAYER_LEFT][B_POSITION_OPPONENT_LEFT][0].median;
        EXPECT(playerSpAttack == 1 ? damage < 20 : damage > 20);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's damage simulation incorporates player Nature")
{
    u32 nature;

    PARAMETRIZE { nature = NATURE_LONELY; }
    PARAMETRIZE { nature = NATURE_BOLD; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Nature(nature); HP(200); MaxHP(200); Moves(MOVE_SPLASH); }
        SetMonData(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0], MON_DATA_HIDDEN_NATURE, &nature);
        CalculateMonStats(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0]);
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); Moves(MOVE_STRENGTH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_STRENGTH); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        EXPECT(nature == NATURE_LONELY ? damage > 53 : damage < 53);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Norman's flags receive an unrevealed player item ID")
{
    enum Item item;

    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_ORAN_BERRY; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Item(item); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT(gAiLogicData->items[B_POSITION_PLAYER_LEFT] == item);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI fairness: Wally's flags receive unrevealed player reserve state")
{
    bool32 reserveFainted;

    PARAMETRIZE { reserveFainted = FALSE; }
    PARAMETRIZE { reserveFainted = TRUE; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_ZIGZAGOON) { HP(reserveFainted ? 0 : 20); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        EXPECT(gAiPartyData->mons[B_TRAINER_PLAYER][1].isFainted == reserveFainted);
    }
}

// This is the non-Factory Frontier flag profile from GetAiFlags. It locks down
// the shared damage path for a later story-only correction; it does not exercise
// Frontier trainer-data selection or its battle rules.
AI_SINGLE_BATTLE_TEST("Story AI fairness: Frontier flag profile retains shared damage baseline")
{
    u16 playerDefense;

    PARAMETRIZE { playerDefense = 1; }
    PARAMETRIZE { playerDefense = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT);
        PLAYER(SPECIES_WOBBUFFET) { Defense(playerDefense); HP(100); MaxHP(200); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); Moves(MOVE_STRENGTH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_STRENGTH); }
    } THEN {
        u16 damage = gAiLogicData->simulatedDmg[B_POSITION_OPPONENT_LEFT][B_POSITION_PLAYER_LEFT][0].median;
        EXPECT(playerDefense == 1 ? damage > 100 : damage < 100);
    }
}
