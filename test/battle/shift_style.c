#include "global.h"
#include "test/battle.h"
#include "main.h"

// Exercise the existing Emerald script and saved preference. Test builds
// normally default to Set; only these fixtures opt into Shift. Recorded
// controller party choices use SEND_OUT, while Yes/No uses real key handling.
#define SHIFT_PROMPT AI_TRAINER_NAME " is about to send out Magikarp. Will you switch your Pokémon?"

AI_SINGLE_BATTLE_TEST("Shift style: saved Shift accepts Yes or No and saved Set suppresses the prompt")
{
    u32 choice;
    PARAMETRIZE { choice = 0; } // Shift / Yes
    PARAMETRIZE { choice = 1; } // Shift / No
    PARAMETRIZE { choice = 2; } // Set (even with a Yes input configured)
    GIVEN {
        AI_FLAGS(0);
        gSaveBlock2Ptr->optionsBattleStyle = choice == 2 ? OPTIONS_BATTLE_STYLE_SET : OPTIONS_BATTLE_STYLE_SHIFT;
        gBattleTestRunnerState->data.yesNoInput = choice == 1 ? B_BUTTON : A_BUTTON;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); Moves(MOVE_SONIC_BOOM, MOVE_SPLASH); }
        PLAYER(SPECIES_WYNAUT) { HP(0); Speed(80); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(2); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN {
            MOVE(player, MOVE_SONIC_BOOM, hit: TRUE);
            EXPECT_MOVE(opponent, MOVE_SPLASH);
            EXPECT_SEND_OUT(opponent, 1);
            if (choice == 0) SEND_OUT(player, 2);
        }
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("The opposing Wynaut fainted!");
        if (choice != 2) MESSAGE(SHIFT_PROMPT);
        else NOT MESSAGE(SHIFT_PROMPT);
        if (choice == 0) SEND_IN_MESSAGE("Psyduck");
        MESSAGE(AI_TRAINER_NAME " sent out Magikarp!");
        if (choice == 0) MESSAGE("Psyduck used Splash!");
        else MESSAGE("Wobbuffet used Splash!");
        MESSAGE("The opposing Magikarp used Splash!");
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, choice == 2 ? 0 : 1);
        EXPECT_EQ((u32)gSaveBlock2Ptr->optionsBattleStyle, choice == 2 ? OPTIONS_BATTLE_STYLE_SET : OPTIONS_BATTLE_STYLE_SHIFT);
        EXPECT_EQ(gBattleScripting.battleStyle, (u32)gSaveBlock2Ptr->optionsBattleStyle);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], choice == 0 ? 2 : 0);
        EXPECT_EQ(player->pp[choice == 0 ? 0 : 1], GetMovePP(MOVE_SPLASH) - 1);
        EXPECT_EQ(opponent->pp[0], GetMovePP(MOVE_SPLASH) - 1);
    }
}

AI_SINGLE_BATTLE_TEST("Shift style: final opposing Pokemon ends battle without a prompt")
{
    GIVEN {
        AI_FLAGS(0);
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SONIC_BOOM, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        MESSAGE("The opposing Wynaut fainted!");
    } THEN {
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
    }
}

AI_SINGLE_BATTLE_TEST("Shift style: no healthy reserve suppresses the prompt")
{
    bool32 faintedReserve;
    PARAMETRIZE { faintedReserve = FALSE; }
    PARAMETRIZE { faintedReserve = TRUE; }
    GIVEN {
        AI_FLAGS(0);
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        if (faintedReserve) PLAYER(SPECIES_PSYDUCK) { HP(0); Speed(90); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(2); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SONIC_BOOM, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SPLASH); EXPECT_SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("The opposing Wynaut fainted!");
        NOT MESSAGE(SHIFT_PROMPT);
        MESSAGE(AI_TRAINER_NAME " sent out Magikarp!");
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
    }
}

AI_DOUBLE_BATTLE_TEST("Shift style: double battles suppress the prompt")
{
    GIVEN {
        AI_FLAGS(0);
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); }
        PLAYER(SPECIES_GOLDEEN) { Speed(80); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(3); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(2); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); EXPECT_SEND_OUT(opponentLeft, 2); }
    } SCENE {
        MESSAGE("The opposing Wynaut fainted!");
        NOT MESSAGE(SHIFT_PROMPT);
        MESSAGE(AI_TRAINER_NAME " sent out Magikarp!");
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_RIGHT], 1);
    }
}

AI_MULTI_BATTLE_TEST("Shift style: partner multi battles suppress the prompt")
{
    GIVEN {
        AI_FLAGS(0);
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_GOLDEEN) { Speed(80); }
        PARTNER(SPECIES_PSYDUCK) { Speed(90); Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_WYNAUT) { HP(1); Speed(3); Moves(MOVE_SPLASH); }
        OPPONENT_A(SPECIES_MAGIKARP) { Speed(1); Moves(MOVE_SPLASH); }
        OPPONENT_B(SPECIES_WOBBUFFET) { Speed(2); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SONIC_BOOM, target: opponentLeft, hit: TRUE); EXPECT_SEND_OUT(opponentLeft, 1); }
    } SCENE {
        MESSAGE("The opposing Wynaut fainted!");
        NOT MESSAGE(SHIFT_PROMPT);
        MESSAGE(AI_TRAINER_NAME " sent out Magikarp!");
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 0);
    }
}

SINGLE_BATTLE_TEST("Shift style: recorded link battles suppress the prompt")
{
    GIVEN {
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(2); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SONIC_BOOM, hit: TRUE); SEND_OUT(opponent, 1); }
    } SCENE {
        MESSAGE("The opposing Wynaut fainted!");
        MESSAGE("2 sent out Magikarp!");
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
    }
}

AI_SINGLE_BATTLE_TEST("Shift style: Battle Tower suppresses the prompt without changing the saved option")
{
    GIVEN {
        AI_FLAGS(0);
        gBattleTestRunnerState->data.recordedBattle.battleFlags |= BATTLE_TYPE_BATTLE_TOWER;
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(2); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(1); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(player, MOVE_SONIC_BOOM, hit: TRUE); EXPECT_MOVE(opponent, MOVE_SPLASH); EXPECT_SEND_OUT(opponent, 1); }
    } THEN {
        EXPECT(gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER);
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ((u32)gSaveBlock2Ptr->optionsBattleStyle, OPTIONS_BATTLE_STYLE_SHIFT);
        EXPECT_EQ(opponent->species, SPECIES_MAGIKARP);
    }
}

AI_SINGLE_BATTLE_TEST("Shift style: player fainting requires a replacement without the optional prompt")
{
    GIVEN {
        AI_FLAGS(0);
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { HP(1); Speed(1); }
        PLAYER(SPECIES_PSYDUCK) { Speed(2); }
        OPPONENT(SPECIES_WYNAUT) { Speed(100); Moves(MOVE_SONIC_BOOM); }
        OPPONENT(SPECIES_MAGIKARP) { Speed(3); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { EXPECT_MOVE(opponent, MOVE_SONIC_BOOM); SEND_OUT(player, 1); }
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ(gBattlerPartyIndexes[B_POSITION_PLAYER_LEFT], 1);
        EXPECT_EQ(player->species, SPECIES_PSYDUCK);
    }
}

WILD_BATTLE_TEST("Shift style: wild battle victory never offers a trainer switch")
{
    GIVEN {
        gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SHIFT;
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        PLAYER(SPECIES_PSYDUCK) { Speed(90); }
        OPPONENT(SPECIES_WYNAUT) { HP(1); Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SONIC_BOOM, hit: TRUE); }
    } THEN {
        EXPECT_EQ(gBattleTestRunnerState->data.trial.yesNoCount, 0);
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
    }
}
