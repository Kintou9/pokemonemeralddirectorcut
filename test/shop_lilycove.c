#include "global.h"
#include "event_data.h"
#include "item.h"
#include "money.h"
#include "script.h"
#include "tv.h"
#include "test/shop.h"
#include "test/test.h"
#include "constants/maps.h"
#include "constants/tv.h"
#include "constants/map_event_ids.h"

extern const u8 LilycoveCity_DepartmentStore_3F_EventScript_SelectStock[];
extern const u16 LilycoveCity_DepartmentStore_3F_Pokemart_StatBoosters[];
extern const u16 LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems[];
static const u16 sOriginal[] = { ITEM_X_SPEED, ITEM_X_SP_ATK, ITEM_X_ATTACK, ITEM_X_DEFENSE, ITEM_DIRE_HIT, ITEM_GUARD_SPEC, ITEM_X_ACCURACY };
static const u16 sEvolution[] = { ITEM_METAL_COAT, ITEM_KINGS_ROCK, ITEM_DRAGON_SCALE, ITEM_UPGRADE, ITEM_DEEP_SEA_TOOTH, ITEM_DEEP_SEA_SCALE };
static const u16 sStones[] = { ITEM_FIRE_STONE, ITEM_WATER_STONE, ITEM_THUNDER_STONE, ITEM_LEAF_STONE, ITEM_MOON_STONE, ITEM_SUN_STONE };
static const u16 sPrices[] = { 8000, 8000, 10000, 10000, 7500, 7500 };
static const u16 sGlobalPrices[] = { 3000, 10000, 4000, 4000, 2000, 2000 };
static const u16 sOtherShop[] = { ITEM_METAL_COAT, ITEM_NONE };

static const u16 *SelectedStock(void)
{
    struct ScriptContext ctx;
    RunScriptImmediatelyUntilEffect(SCREFF_V1 | SCREFF_HARDWARE, LilycoveCity_DepartmentStore_3F_EventScript_SelectStock, &ctx);
    // The effect boundary is the pokemart opcode, followed by its stock pointer.
    ctx.scriptPtr++;
    return (const u16 *)ScriptReadWord(&ctx);
}

static void AdvanceMessages(void)
{
    for (u32 i = 0; i < 600; i++) TestShop_Input(0);
}

TEST("Lilycove shop: Feather Badge selects stock without counting other badges")
{
    bool32 feather;
    PARAMETRIZE { feather = FALSE; }
    PARAMETRIZE { feather = TRUE; }
    for (u32 flag = FLAG_BADGE01_GET; flag <= FLAG_BADGE08_GET; flag++) FlagSet(flag);
    if (!feather) FlagClear(FLAG_BADGE06_GET);
    const u16 *stock = SelectedStock();
    EXPECT_EQ(stock, feather ? LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems : LilycoveCity_DepartmentStore_3F_Pokemart_StatBoosters);
    for (u32 i = 0; i < ARRAY_COUNT(sOriginal); i++) EXPECT_EQ(stock[i], sOriginal[i]);
    if (feather) {
        for (u32 i = 0; i < ARRAY_COUNT(sEvolution); i++) EXPECT_EQ(stock[7+i], sEvolution[i]);
        for (u32 i = 0; i < ARRAY_COUNT(sStones); i++) EXPECT_EQ(stock[13+i], sStones[i]);
    }
    EXPECT_EQ(stock[feather ? 19 : 7], ITEM_NONE);
}

TEST("Lilycove shop: local prices survive filtering and do not alter resale or another shop")
{
    bool32 sale;
    PARAMETRIZE { sale = FALSE; }
    PARAMETRIZE { sale = TRUE; }
    // Force the normal shop discount to be active, even though Lilycove 3F
    // normally cannot qualify for the Energy Guru sale.
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_SLATEPORT_CITY);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_SLATEPORT_CITY);
    gSpecialVar_LastTalked = LOCALID_SLATEPORT_ENERGY_GURU;
    gSaveBlock1Ptr->pokeNews[0].kind = POKENEWS_SLATEPORT;
    gSaveBlock1Ptr->pokeNews[0].state = sale ? POKENEWS_STATE_ACTIVE : 0;
    EXPECT_EQ(IsPokeNewsActive(POKENEWS_SLATEPORT), sale);
    TestShop_Open(LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems, 0);
    EXPECT_EQ(TestShop_Count(), 19);
    for (u32 i = 0; i < ARRAY_COUNT(sEvolution); i++) {
        EXPECT_EQ(TestShop_Price(sEvolution[i]), sPrices[i]);
        EXPECT_EQ(GetItemPrice(sEvolution[i]), sGlobalPrices[i]);
        EXPECT_EQ(GetItemSellPrice(sEvolution[i]), sGlobalPrices[i] / 4);
    }
    for (u32 i = 0; i < ARRAY_COUNT(sOriginal); i++) EXPECT_EQ(TestShop_Price(sOriginal[i]), GetItemPrice(sOriginal[i]) >> sale);
    for (u32 i = 0; i < ARRAY_COUNT(sStones); i++) EXPECT_EQ(TestShop_Price(sStones[i]), GetItemPrice(sStones[i]) >> sale);
    TestShop_Close();
    TestShop_Open(sOtherShop, 0);
    EXPECT_EQ(TestShop_Price(ITEM_METAL_COAT), GetItemPrice(ITEM_METAL_COAT) >> sale);
    TestShop_Close();
}

TEST("Lilycove shop: purchase confirmation charges exact quantity and supports repeat purchases")
{
    u32 index = 0;
    bool32 sale = FALSE;
    for (u32 i = 0; i < ARRAY_COUNT(sEvolution); i++) {
        PARAMETRIZE { index = i; sale = FALSE; }
        PARAMETRIZE { index = i; sale = TRUE; }
    }
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_SLATEPORT_CITY);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_SLATEPORT_CITY);
    gSpecialVar_LastTalked = LOCALID_SLATEPORT_ENERGY_GURU;
    gSaveBlock1Ptr->pokeNews[0].kind = POKENEWS_SLATEPORT;
    gSaveBlock1Ptr->pokeNews[0].state = sale ? POKENEWS_STATE_ACTIVE : 0;
    ClearBag();
    SetMoney(&gSaveBlock1Ptr->money, 100000);
    TestShop_Open(LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems, 7 + index);
    for (u32 repeat = 0; repeat < 2; repeat++) {
        TestShop_Input(A_BUTTON);
        AdvanceMessages();
        EXPECT_EQ(TestShop_Stage(), TEST_SHOP_QUANTITY);
        TestShop_Input(DPAD_UP);
        EXPECT_EQ(TestShop_Total(), sPrices[index] * 2);
        TestShop_Input(A_BUTTON);
        AdvanceMessages();
        TestShop_Input(A_BUTTON);
        AdvanceMessages();
        EXPECT_EQ(TestShop_Stage(), TEST_SHOP_RECEIPT);
        EXPECT_EQ(CountTotalItemQuantityInBag(sEvolution[index]), (repeat + 1) * 2);
        EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 100000 - sPrices[index] * 2 * (repeat + 1));
        TestShop_Input(A_BUTTON);
    }
    TestShop_Close();
}

TEST("Lilycove shop: insufficient funds and cancelled confirmation leave money and bag unchanged")
{
    bool32 affordable;
    PARAMETRIZE { affordable = FALSE; }
    PARAMETRIZE { affordable = TRUE; }
    ClearBag();
    u32 money = affordable ? 8000 : 7999;
    SetMoney(&gSaveBlock1Ptr->money, money);
    TestShop_Open(LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems, 7);
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    if (affordable) {
        EXPECT_EQ(TestShop_Stage(), TEST_SHOP_QUANTITY);
        TestShop_Input(A_BUTTON);
        AdvanceMessages();
        TestShop_Input(B_BUTTON);
    } else {
        TestShop_Input(A_BUTTON); // Dismiss the insufficient-money message.
        AdvanceMessages();
    }
    EXPECT_EQ(TestShop_Stage(), TEST_SHOP_LIST);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), money);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_METAL_COAT), 0);
    TestShop_Close();
}

TEST("Lilycove shop: full Bag does not deduct payment")
{
    ClearBag();
    SetMoney(&gSaveBlock1Ptr->money, 8000);
    // Fill all available slots; AddBagItem itself determines pocket capacity.
    while (AddBagItem(ITEM_METAL_COAT, MAX_BAG_ITEM_CAPACITY)) { }
    u32 count = CountTotalItemQuantityInBag(ITEM_METAL_COAT);
    EXPECT_NE(count, 0);
    TestShop_Open(LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems, 7);
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    TestShop_Input(A_BUTTON); // Dismiss the full-Bag message.
    AdvanceMessages();
    EXPECT_EQ(TestShop_Stage(), TEST_SHOP_LIST);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 8000);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_METAL_COAT), count);
    TestShop_Close();
}

TEST("Lilycove shop: quantity is limited by funds and exact funds buy one item")
{
    ClearBag();
    SetMoney(&gSaveBlock1Ptr->money, 8000);
    TestShop_Open(LilycoveCity_DepartmentStore_3F_Pokemart_EvolutionItems, 7);
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    EXPECT_EQ(TestShop_Stage(), TEST_SHOP_QUANTITY);
    TestShop_Input(DPAD_UP);
    EXPECT_EQ(TestShop_Total(), 8000);
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    TestShop_Input(A_BUTTON);
    AdvanceMessages();
    EXPECT_EQ(TestShop_Stage(), TEST_SHOP_RECEIPT);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 0);
    EXPECT_EQ(CountTotalItemQuantityInBag(ITEM_METAL_COAT), 1);
    TestShop_Close();
}
