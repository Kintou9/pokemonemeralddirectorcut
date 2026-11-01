#include "global.h"
#include "test/battle.h"
#include "constants/opponents.h"

#define ROXANNE_1_AI_FLAGS (AI_FLAG_BASIC_TRAINER | AI_FLAG_HP_AWARE | AI_FLAG_SMART_SWITCHING)
#define NORMAN_1_AI_FLAGS (AI_FLAG_BASIC_TRAINER | AI_FLAG_HP_AWARE | AI_FLAG_SMART_SWITCHING | AI_FLAG_TRY_TO_2HKO | AI_FLAG_PP_STALL_PREVENTION)
#define WALLY_VR_AI_FLAGS (AI_FLAG_BASIC_TRAINER | AI_FLAG_SMART_SWITCHING | AI_FLAG_TRY_TO_2HKO)
#define FRONTIER_BASIC_AI_FLAGS (AI_FLAG_CHECK_BAD_MOVE | AI_FLAG_CHECK_VIABILITY | AI_FLAG_TRY_TO_FAINT)

// Paired tests use actual enhanced trainer flag profiles and synthetic teams.
// Each branch has the same displayed species, level, HP, moves, and RNG.

AI_SINGLE_BATTLE_TEST("Story AI decision: Roxanne priority choice at hidden Speed boundary")
{
    u16 playerSpeed;

    PARAMETRIZE { playerSpeed = 1; }
    PARAMETRIZE { playerSpeed = 100; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(ROXANNE_1_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 1);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(playerSpeed); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(60); Speed(50); Moves(MOVE_QUICK_ATTACK, MOVE_STRENGTH); }
    } WHEN {
        TURN { EXPECT_MOVES(opponent, MOVE_QUICK_ATTACK, MOVE_STRENGTH); SEND_OUT(player, 1); }
    } THEN {
        EXPECT(gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT] & AI_FLAG_TRY_TO_FAINT);
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], playerSpeed == 1 ? MOVE_STRENGTH : MOVE_QUICK_ATTACK);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman retains Knock Off choice across unrevealed item states")
{
    enum Item item;

    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_ORAN_BERRY; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Item(item); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_KNOCK_OFF, MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_KNOCK_OFF, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_KNOCK_OFF);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's Trick choice can reveal hidden item presence")
{
    enum Item item;

    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_ORAN_BERRY; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 0);
        PLAYER(SPECIES_WOBBUFFET) { Item(item); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH, MOVE_TRICK); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_SPLASH, MOVE_TRICK); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], item == ITEM_NONE ? MOVE_SPLASH : MOVE_TRICK);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's switch choice reacts to unrevealed Magnet Pull")
{
    enum Ability ability;

    PARAMETRIZE { ability = ABILITY_MAGNET_PULL; }
    PARAMETRIZE { ability = ABILITY_STURDY; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_MAGNETON) { Ability(ability); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_ARON) { Moves(MOVE_TOXIC); }
        OPPONENT(SPECIES_MACHOP) { Moves(MOVE_KARATE_CHOP); }
    } WHEN {
        if (ability == ABILITY_MAGNET_PULL)
            TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TOXIC); }
        else
            TURN { MOVE(player, MOVE_SPLASH); EXPECT_SWITCH(opponent, 1); }
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's switch choice reacts to hidden remaining Surf PP")
{
    u8 pp;

    PARAMETRIZE { pp = 1; }
    PARAMETRIZE { pp = 2; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 0);
        PLAYER(SPECIES_BLASTOISE) { SpAttack(200); MovesWithPP({MOVE_SURF, pp}, {MOVE_SPLASH, 40}); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); Moves(MOVE_PROTECT, MOVE_TACKLE); }
        OPPONENT(SPECIES_VAPOREON) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SURF); EXPECT_MOVE(opponent, MOVE_PROTECT); }
        if (pp == 1)
            TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
        else
            TURN { MOVE(player, MOVE_SPLASH); EXPECT_SWITCH(opponent, 1); }
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's switch choice reacts to hidden Special Attack")
{
    u16 spAttack;

    PARAMETRIZE { spAttack = 1; }
    PARAMETRIZE { spAttack = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 0);
        PLAYER(SPECIES_BLASTOISE) { SpAttack(spAttack); Moves(MOVE_SURF, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); Moves(MOVE_PROTECT, MOVE_TACKLE); }
        OPPONENT(SPECIES_VAPOREON) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SURF); EXPECT_MOVE(opponent, MOVE_PROTECT); }
        if (spAttack == 1)
            TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
        else
            TURN { MOVE(player, MOVE_SPLASH); EXPECT_SWITCH(opponent, 1); }
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's action across hidden Attack values")
{
    u16 attack;

    PARAMETRIZE { attack = 1; }
    PARAMETRIZE { attack = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        TIE_BREAK_SCORE(RNG_AI_SCORE_TIE_SINGLES, SCORE_TIE_CHOSEN, 0);
        PLAYER(SPECIES_MACHAMP) { Attack(attack); Moves(MOVE_BRICK_BREAK, MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(30); MaxHP(200); Moves(MOVE_PROTECT, MOVE_TACKLE); }
        OPPONENT(SPECIES_GENGAR) { Moves(MOVE_PSYCHIC); }
    } WHEN {
        TURN { MOVE(player, MOVE_BRICK_BREAK); EXPECT_MOVE(opponent, MOVE_PROTECT); }
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_PROTECT, MOVE_TACKLE); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_TACKLE);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Wally compares attacks across hidden Defense IVs")
{
    u8 defenseIV;

    PARAMETRIZE { defenseIV = 0; }
    PARAMETRIZE { defenseIV = 31; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { DefenseIV(defenseIV); Moves(MOVE_SPLASH); }
        CalculateMonStats(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0]);
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); SpAttack(70); Moves(MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], defenseIV == 0 ? MOVE_STRENGTH : MOVE_THUNDERBOLT);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Wally compares attacks across hidden Defense EVs")
{
    u8 defenseEV;

    PARAMETRIZE { defenseEV = 0; }
    PARAMETRIZE { defenseEV = 252; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        SetMonData(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0], MON_DATA_DEF_EV, &defenseEV);
        CalculateMonStats(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0]);
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); SpAttack(70); Moves(MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], defenseEV == 0 ? MOVE_STRENGTH : MOVE_THUNDERBOLT);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Wally compares attacks across hidden Nature")
{
    u32 nature;

    PARAMETRIZE { nature = NATURE_LONELY; }
    PARAMETRIZE { nature = NATURE_BOLD; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Nature(nature); Moves(MOVE_SPLASH); }
        SetMonData(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0], MON_DATA_HIDDEN_NATURE, &nature);
        CalculateMonStats(&gBattleTestRunnerState->data.recordedBattle.parties[B_TRAINER_PLAYER][0]);
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); SpAttack(75); Moves(MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], nature == NATURE_LONELY ? MOVE_STRENGTH : MOVE_THUNDERBOLT);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's move choice at equal visible HP bars")
{
    u16 hp, maxHP;

    PARAMETRIZE { hp = 40; maxHP = 80; }
    PARAMETRIZE { hp = 41; maxHP = 82; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { HP(hp); MaxHP(maxHP); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_DRAGON_RAGE, MOVE_YAWN); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_DRAGON_RAGE, MOVE_YAWN); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], hp == 40 ? MOVE_DRAGON_RAGE : MOVE_YAWN);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Wally compares attacks across hidden Special Defense")
{
    u16 spDefense;

    PARAMETRIZE { spDefense = 1; }
    PARAMETRIZE { spDefense = 200; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(WALLY_VR_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { Defense(100); SpDefense(spDefense); HP(100); MaxHP(200); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Attack(100); SpAttack(100); Moves(MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } THEN {
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], spDefense == 1 ? MOVE_THUNDERBOLT : MOVE_STRENGTH);
    }
}

AI_SINGLE_BATTLE_TEST("Story AI decision: Norman's move choice across unobserved healthy reserve HP", u16 chosenMove)
{
    u16 reserveHP;

    PARAMETRIZE { reserveHP = 1; }
    PARAMETRIZE { reserveHP = 20; }
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(NORMAN_1_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { HP(200); MaxHP(200); Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_ZIGZAGOON) { HP(reserveHP); MaxHP(20); Moves(MOVE_TACKLE); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVES(opponent, MOVE_STRENGTH, MOVE_THUNDERBOLT); }
    } THEN {
        results[i].chosenMove = gLastMoves[B_POSITION_OPPONENT_LEFT];
    } FINALLY {
        EXPECT_EQ(results[0].chosenMove, results[1].chosenMove);
    }
}

// These two profiles match non-Factory Frontier flags in GetAiFlags. Recorded
// battle tests cannot invoke facility trainer selection or Factory AI routing.
AI_SINGLE_BATTLE_TEST("Frontier AI decision baseline: priority move selection")
{
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(FRONTIER_BASIC_AI_FLAGS);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(100); }
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(1); Moves(MOVE_QUICK_ATTACK, MOVE_STRENGTH); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_QUICK_ATTACK); SEND_OUT(player, 1); }
    }
}

AI_SINGLE_BATTLE_TEST("Frontier AI decision baseline: replacement switch choice")
{
    GIVEN {
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(FRONTIER_BASIC_AI_FLAGS);
        PLAYER(SPECIES_MARSHTOMP) { Level(30); Moves(MOVE_GROWL); Speed(5); }
        OPPONENT(SPECIES_PONYTA) { Level(1); Moves(MOVE_NONE); Speed(6); }
        OPPONENT(SPECIES_TANGELA) { Level(30); Moves(MOVE_SCRATCH); Speed(4); }
        OPPONENT(SPECIES_LOMBRE) { Level(30); Moves(MOVE_SCRATCH); Speed(4); }
        OPPONENT(SPECIES_HARIYAMA) { Level(30); Moves(MOVE_VITAL_THROW); Speed(4); }
    } WHEN {
        TURN { MOVE(player, MOVE_GROWL); EXPECT_SWITCH(opponent, 3); }
    }
}
