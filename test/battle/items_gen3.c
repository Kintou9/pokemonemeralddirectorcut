#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Gen3 items: medicines restore original amounts and clamp at max HP")
{
    enum Item item = ITEM_NONE;
    u32 amount = 0, hp = 0;
    static const u16 cases[][2] = {
        {ITEM_SUPER_POTION, 50}, {ITEM_HYPER_POTION, 200},
        {ITEM_FRESH_WATER, 50}, {ITEM_SODA_POP, 60}, {ITEM_LEMONADE, 80},
        {ITEM_ENERGY_POWDER, 50}, {ITEM_ENERGY_ROOT, 200}, {ITEM_SITRUS_BERRY, 30},
    };
    for (u32 j = 0; j < ARRAY_COUNT(cases); j++) {
        PARAMETRIZE { item = cases[j][0]; amount = cases[j][1]; hp = 1; }
        PARAMETRIZE { item = cases[j][0]; amount = 5; hp = 395; }
    }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { HP(hp); MaxHP(400); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN { USE_ITEM(player, item, partyIndex: 0); }
    } SCENE {
        HP_BAR(player, damage: -amount);
    }
}

SINGLE_BATTLE_TEST("Gen3 items: type boosters use ten percent and Sea Incense five percent", s16 damage)
{
    enum Item item;
    PARAMETRIZE { item = ITEM_NONE; }
    PARAMETRIZE { item = ITEM_MYSTIC_WATER; }
    PARAMETRIZE { item = ITEM_SEA_INCENSE; }
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET) { Item(item); Level(50); SpAttack(100); }
        OPPONENT(SPECIES_WOBBUFFET) { SpDefense(100); }
    } WHEN {
        TURN { MOVE(player, MOVE_SURF, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.1), results[1].damage);
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(1.05), results[2].damage);
    }
}

SINGLE_BATTLE_TEST("Gen3 items: Lax Incense leaves 95 percent accuracy")
{
    PASSES_RANDOMLY(95, 100, RNG_ACCURACY);
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { Item(ITEM_LAX_INCENSE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
    }
}
