#include "global.h"
#include "pokemon.h"
#include "item.h"
#include "item_use.h"
#include "item_menu.h"
#include "test/test.h"
#include "party_menu.h"
#include "event_data.h"
#include "main.h"
#include "task.h"
#include "window.h"
#include "bg.h"
#include "text.h"

TEST("Gen3 items: field healing and HP clamping use original quantities")
{
    enum Item item = ITEM_NONE;
    u32 amount = 0, missing = 0;
    static const u16 cases[][2] = {
        {ITEM_SUPER_POTION, 50}, {ITEM_HYPER_POTION, 200},
        {ITEM_FRESH_WATER, 50}, {ITEM_SODA_POP, 60}, {ITEM_LEMONADE, 80},
        {ITEM_ENERGY_POWDER, 50}, {ITEM_ENERGY_ROOT, 200}, {ITEM_SITRUS_BERRY, 30},
    };
    for (u32 j = 0; j < ARRAY_COUNT(cases); j++) {
        PARAMETRIZE { item = cases[j][0]; amount = cases[j][1]; missing = 300; }
        PARAMETRIZE { item = cases[j][0]; amount = 5; missing = 5; }
    }
    struct Pokemon mon;
    CreateMon(&mon, SPECIES_BLISSEY, 100, 0, OTID_STRUCT_PLAYER_ID);
    CalculateMonStats(&mon);
    u32 maxHP = GetMonData(&mon, MON_DATA_MAX_HP), hp = maxHP - missing;
    SetMonData(&mon, MON_DATA_HP, &hp);
    EXPECT_EQ(ExecuteTableBasedItemEffect(&mon, item, 0, 0), FALSE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HP), hp + amount);
}

TEST("Gen3 items: vitamins retain the 252 stat cap and 510 total cap")
{
    u32 start, expected;
    PARAMETRIZE { start = 100; expected = 110; }
    PARAMETRIZE { start = 251; expected = 252; }
    PARAMETRIZE { start = 252; expected = 252; }
    struct Pokemon mon;
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&mon, MON_DATA_HP_EV, &start);
    ExecuteTableBasedItemEffect(&mon, ITEM_HP_UP, 0, 0);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HP_EV), expected);
    u32 value = 252;
    SetMonData(&mon, MON_DATA_HP_EV, &value);
    SetMonData(&mon, MON_DATA_ATK_EV, &value);
    ExecuteTableBasedItemEffect(&mon, ITEM_IRON, 0, 0);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_DEF_EV), 6);
}

TEST("Gen3 items: all eight approved Bag evolution routes remain usable")
{
    enum Species species = SPECIES_NONE, expected = SPECIES_NONE;
    enum Item item = ITEM_NONE;
    static const u16 cases[][3] = {
        {SPECIES_ONIX, ITEM_METAL_COAT, SPECIES_STEELIX},
        {SPECIES_SCYTHER, ITEM_METAL_COAT, SPECIES_SCIZOR},
        {SPECIES_POLIWHIRL, ITEM_KINGS_ROCK, SPECIES_POLITOED},
        {SPECIES_SLOWPOKE, ITEM_KINGS_ROCK, SPECIES_SLOWKING},
        {SPECIES_SEADRA, ITEM_DRAGON_SCALE, SPECIES_KINGDRA},
        {SPECIES_PORYGON, ITEM_UPGRADE, SPECIES_PORYGON2},
        {SPECIES_CLAMPERL, ITEM_DEEP_SEA_TOOTH, SPECIES_HUNTAIL},
        {SPECIES_CLAMPERL, ITEM_DEEP_SEA_SCALE, SPECIES_GOREBYSS},
    };
    for (u32 j = 0; j < ARRAY_COUNT(cases); j++)
        PARAMETRIZE { species = cases[j][0]; item = cases[j][1]; expected = cases[j][2]; }
    struct Pokemon mon;
    bool32 canStop;
    CreateMon(&mon, species, 30, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetItemFieldFunc(item), ItemUseOutOfBattle_EvolutionStone);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, item, NULL, &canStop, CHECK_EVO), expected);
    CreateMon(&mon, SPECIES_WOBBUFFET, 30, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, item, NULL, &canStop, CHECK_EVO), SPECIES_NONE);
    EXPECT_EQ(ExecuteTableBasedItemEffect(&mon, item, 0, 0), TRUE);
}


static void EvolutionTestIdle(u8 taskId) { }

TEST("Gen3 items: Bag evolution callback consumes one item only on valid use")
{
    enum Species species = SPECIES_NONE, expected = SPECIES_NONE;
    enum Item item = ITEM_NONE;
    bool32 valid = FALSE;
    static const u16 cases[][3] = {
        {SPECIES_ONIX, ITEM_METAL_COAT, SPECIES_STEELIX},
        {SPECIES_SCYTHER, ITEM_METAL_COAT, SPECIES_SCIZOR},
        {SPECIES_POLIWHIRL, ITEM_KINGS_ROCK, SPECIES_POLITOED},
        {SPECIES_SLOWPOKE, ITEM_KINGS_ROCK, SPECIES_SLOWKING},
        {SPECIES_SEADRA, ITEM_DRAGON_SCALE, SPECIES_KINGDRA},
        {SPECIES_PORYGON, ITEM_UPGRADE, SPECIES_PORYGON2},
        {SPECIES_CLAMPERL, ITEM_DEEP_SEA_TOOTH, SPECIES_HUNTAIL},
        {SPECIES_CLAMPERL, ITEM_DEEP_SEA_SCALE, SPECIES_GOREBYSS},
    };
    for (u32 j = 0; j < ARRAY_COUNT(cases); j++) {
        PARAMETRIZE { species = cases[j][0]; item = cases[j][1]; expected = cases[j][2]; valid = TRUE; }
        PARAMETRIZE { species = SPECIES_WOBBUFFET; item = cases[j][1]; expected = SPECIES_NONE; valid = FALSE; }
    }
    // The callback schedules evolution; rendering the evolution animation is
    // deliberately left to an in-game check. Verify its actual scheduled target.
    static const struct WindowTemplate windows[] = {
        {0, 1, 1, 27, 4, 0, 1}, {0, 1, 1, 27, 4, 0, 1},
        {0, 1, 1, 27, 4, 0, 1}, {0, 1, 1, 27, 4, 0, 1},
        {0, 1, 1, 27, 4, 0, 1}, {0, 1, 1, 27, 4, 0, 1},
        {0, 1, 1, 27, 4, 0, 1}, DUMMY_WIN_TEMPLATE,
    };
    ClearBag();
    EXPECT(AddBagItem(item, 2));
    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], species, 30, 0, OTID_STRUCT_PLAYER_ID);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    gPartyMenu.slotId = 0;
    gSpecialVar_ItemId = item;
    ResetTasks();
    ResetBgsAndClearDma3BusyFlags(0);
    InitWindows(windows);
    MainCallback savedCallback = gMain.callback2;
    u8 task = CreateTask(EvolutionTestIdle, 0);
    ItemUseCB_EvolutionStone(task, EvolutionTestIdle);
    EXPECT_EQ(CountTotalItemQuantityInBag(item), valid ? 1 : 2);
    if (valid) {
        EXPECT(gMain.callback2 != savedCallback);
        // BeginEvolutionScene creates the next task with target in data[2].
        EXPECT_EQ(gTasks[task + 1].data[2], expected);
    } else {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), species);
        FreeAllWindowBuffers();
    }
    DeactivateAllTextPrinters();
    SetMainCallback2(savedCallback);
    ResetTasks();
}
