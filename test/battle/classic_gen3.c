#include "global.h"
#include "test/battle.h"
#include "battle_util.h"

SINGLE_BATTLE_TEST("Classic Gen 3: Explosion and Self-Destruct halve Defense", s16 damage)
{
    enum Move move;
    u32 generation;
    PARAMETRIZE { move = MOVE_SELF_DESTRUCT; generation = GEN_3; }
    PARAMETRIZE { move = MOVE_SELF_DESTRUCT; generation = GEN_5; }
    PARAMETRIZE { move = MOVE_EXPLOSION; generation = GEN_3; }
    PARAMETRIZE { move = MOVE_EXPLOSION; generation = GEN_5; }
    GIVEN {
        WITH_CONFIG(B_EXPLOSION_DEFENSE, generation);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(200); }
    } WHEN {
        TURN { MOVE(player, move, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); SEND_OUT(player, 1); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, 90);
        EXPECT_EQ(results[1].damage, 46);
        EXPECT_EQ(results[2].damage, 112);
        EXPECT_EQ(results[3].damage, 57);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Rough Skin removes one sixteenth")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(160); MaxHP(160); }
        OPPONENT(SPECIES_CARVANHA) { Ability(ABILITY_ROUGH_SKIN); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(player->hp, 150);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Blizzard can miss in hail")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_HAIL); MOVE(opponent, MOVE_SPLASH); }
        TURN { MOVE(player, MOVE_BLIZZARD, hit: FALSE); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_HAIL, player);
        NOT ANIMATION(ANIM_TYPE_MOVE, MOVE_BLIZZARD, player);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Poison-type Toxic can miss")
{
    GIVEN {
        PLAYER(SPECIES_EKANS);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, MOVE_TOXIC, hit: FALSE); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(opponent->status1, STATUS1_NONE);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Knock Off has no held-item damage bonus", s16 damage)
{
    enum Item item;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_POTION; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(item); }
    } WHEN {
        TURN { MOVE(player, MOVE_KNOCK_OFF); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Electric types can be paralyzed")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_PIKACHU);
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDER_WAVE); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT(opponent->status1 & STATUS1_PARALYSIS);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Lightning Rod does not absorb Electric damage")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_ELECTRIKE) { Ability(ABILITY_LIGHTNING_ROD); }
    } WHEN {
        TURN { MOVE(player, MOVE_THUNDER_SHOCK); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        HP_BAR(opponent);
    } THEN {
        EXPECT_EQ(opponent->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: sleep and Yawn select counters from two through five")
{
    bool32 yawn;
    u32 counter, turn;
    PARAMETRIZE { yawn = FALSE; counter = 2; }
    PARAMETRIZE { yawn = FALSE; counter = 3; }
    PARAMETRIZE { yawn = FALSE; counter = 4; }
    PARAMETRIZE { yawn = FALSE; counter = 5; }
    PARAMETRIZE { yawn = TRUE; counter = 2; }
    PARAMETRIZE { yawn = TRUE; counter = 3; }
    PARAMETRIZE { yawn = TRUE; counter = 4; }
    PARAMETRIZE { yawn = TRUE; counter = 5; }
    PASSES_RANDOMLY(1, 4, RNG_SLEEP_TURNS);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Speed(1); }
        OPPONENT(SPECIES_WOBBUFFET) { Speed(100); }
    } WHEN {
        if (yawn) {
            TURN { MOVE(player, MOVE_YAWN); MOVE(opponent, MOVE_SPLASH); }
            TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
        } else {
            TURN { MOVE(player, MOVE_SPORE); MOVE(opponent, MOVE_SPLASH); }
        }
        for (turn = 0; turn < counter; turn++)
            TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        // The target acted before direct sleep; Yawn applies at end of turn.
        MESSAGE("The opposing Wobbuffet fell asleep!");
        for (turn = 0; turn < counter - 1; turn++)
            MESSAGE("The opposing Wobbuffet is fast asleep.");
        MESSAGE("The opposing Wobbuffet woke up!");
    }
}

DOUBLE_BATTLE_TEST("Classic Gen 3: Lightning Rod does not redirect its ally's move")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_ELECTRIKE) { Ability(ABILITY_LIGHTNING_ROD); }
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_THUNDER_SHOCK, target: opponentLeft);
            MOVE(playerRight, MOVE_SPLASH);
            MOVE(opponentLeft, MOVE_SPLASH);
            MOVE(opponentRight, MOVE_SPLASH);
        }
    } SCENE {
        HP_BAR(opponentLeft);
        NOT HP_BAR(playerRight);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Ghost types cannot escape Shadow Tag")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_SHADOW_TAG); }
        OPPONENT(SPECIES_GASTLY);
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); MOVE(opponent, MOVE_SPLASH); }
    } THEN {
        EXPECT_EQ(IsAbilityPreventingEscape(B_BATTLER_1), B_BATTLER_0 + 1);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Guts and burned Facade stack", s16 damage)
{
    u32 status;
    PARAMETRIZE { status = STATUS1_NONE; }
    PARAMETRIZE { status = STATUS1_BURN; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Ability(ABILITY_GUTS); Status1(status); Attack(100); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(500); MaxHP(500); Defense(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_FACADE); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(3.0), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: Wrap residual damage and two-to-five-turn duration", s16 residual)
{
    u32 turns;
    PARAMETRIZE { turns = 2; }
    PARAMETRIZE { turns = 3; }
    PARAMETRIZE { turns = 4; }
    PARAMETRIZE { turns = 5; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(160); MaxHP(160); }
    } WHEN {
        TURN { MOVE(player, MOVE_WRAP, WITH_RNG(RNG_WRAP, turns)); MOVE(opponent, MOVE_SPLASH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_WRAP, player);
        HP_BAR(opponent);
        HP_BAR(opponent, captureDamage: &results[i].residual);
    } THEN {
        EXPECT_EQ((u32)opponent->volatiles.wrapTurns, turns - 1);
    } FINALLY {
        for (u32 j = 0; j < 4; j++)
            EXPECT_EQ(results[j].residual, 10);
    }
}

DOUBLE_BATTLE_TEST("Classic Gen 3: Lightning Rod redirects opposing Electric moves without absorption")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_RAICHU) { Ability(ABILITY_LIGHTNING_ROD); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {
            MOVE(playerLeft, MOVE_THUNDERBOLT, target: opponentRight);
            MOVE(playerRight, MOVE_THUNDERBOLT, target: opponentRight);
            MOVE(opponentLeft, MOVE_CELEBRATE);
            MOVE(opponentRight, MOVE_CELEBRATE);
        }
    } SCENE {
        HP_BAR(opponentLeft);
        HP_BAR(opponentLeft);
        NOT HP_BAR(opponentRight);
    } THEN {
        EXPECT_EQ(opponentLeft->status1, STATUS1_PARALYSIS);
        EXPECT(opponentLeft->hp > 0);
        EXPECT_EQ(opponentLeft->statStages[STAT_SPATK], DEFAULT_STAT_STAGE);
    }
}

SINGLE_BATTLE_TEST("Classic Gen 3: consecutive Protect and Detect succeed half the time")
{
    enum Move move;
    PARAMETRIZE { move = MOVE_PROTECT; }
    PARAMETRIZE { move = MOVE_DETECT; }
    PASSES_RANDOMLY(1, 2, RNG_PROTECT_FAIL);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { MOVE(player, move); }
        TURN { MOVE(player, move); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, move, player);
        ANIMATION(ANIM_TYPE_MOVE, move, player);
    }
}
