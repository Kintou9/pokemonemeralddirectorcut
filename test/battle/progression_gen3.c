#include "global.h"
#include "test/battle.h"
#include "event_data.h"

WILD_BATTLE_TEST("Gen3 progression: original yields with catching and party-wide EXP Share")
{
    bool32 catching = FALSE, share = FALSE;
    PARAMETRIZE { catching = FALSE; share = FALSE; }
    PARAMETRIZE { catching = FALSE; share = TRUE; }
    PARAMETRIZE { catching = TRUE; share = FALSE; }
    PARAMETRIZE { catching = TRUE; share = TRUE; }
    GIVEN {
        FlagClear(I_EXP_SHARE_FLAG);
        if (share) FlagSet(I_EXP_SHARE_FLAG);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); }
        PLAYER(SPECIES_WYNAUT) { Level(50); }
        OPPONENT(SPECIES_CATERPIE) { Level(7); HP(1); }
    } WHEN {
        TURN {
            if (catching) USE_ITEM(player, ITEM_ULTRA_BALL, WITH_RNG(RNG_BALLTHROW_SHAKE, 0));
            else MOVE(player, MOVE_SONIC_BOOM, hit: TRUE);
        }
    } THEN {
        // Emerald Caterpie yield 53 * level 7 / 7; approved Gen 6 sharing
        // gives full EXP to the participant and half to the reserve.
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_EXP), gExperienceTables[gSpeciesInfo[SPECIES_WOBBUFFET].growthRate][50] + 53);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_EXP), gExperienceTables[gSpeciesInfo[SPECIES_WYNAUT].growthRate][50] + (share ? 26 : 0));
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP_EV), 1);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP_EV), share ? 1 : 0);
        FlagClear(I_EXP_SHARE_FLAG);
    }
}

AI_DOUBLE_BATTLE_TEST("Gen3 progression: simultaneous KOs award each original yield once to the party")
{
    GIVEN {
        AI_FLAGS(0);
        FlagSet(I_EXP_SHARE_FLAG);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Speed(100); }
        PLAYER(SPECIES_WYNAUT) { Level(50); Speed(90); }
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Speed(80); }
        OPPONENT(SPECIES_CATERPIE) { Level(7); HP(1); Speed(10); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WEEDLE) { Level(7); HP(1); Speed(9); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SURF); }
    } THEN {
        // floor(53 * 1.5) + floor(52 * 1.5) = 79 + 78.
        // Full EXP for BOTH participants; floor(79/2)+floor(78/2) for reserve.
        for (u32 i = 0; i < 3; i++) {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
            EXPECT_EQ(GetMonData(mon, MON_DATA_EXP), 125000 + (i == 2 ? 78 : 157));
            EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV), 1);
            EXPECT_EQ(GetMonData(mon, MON_DATA_SPEED_EV), 1);
        }
        EXPECT_EQ(gBattleOutcome, B_OUTCOME_WON);
        FlagClear(I_EXP_SHARE_FLAG);
    }
}

SINGLE_BATTLE_TEST("Gen3 progression: Return and Frustration at restored standard friendship", s16 damage)
{
    enum Move move;
    PARAMETRIZE { move = MOVE_RETURN; }
    PARAMETRIZE { move = MOVE_FRUSTRATION; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); Friendship(gSpeciesInfo[SPECIES_WOBBUFFET].friendship); }
        OPPONENT(SPECIES_WOBBUFFET) { Defense(100); }
    } WHEN {
        TURN { MOVE(player, move, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } THEN {
        // Friendship 70: floor(70*2/5)=28 and floor((255-70)*2/5)=74 power.
        EXPECT_EQ(results[i].damage, move == MOVE_RETURN ? 14 : 34);
    }
}
