#include "global.h"
#include "test/test.h"
#include "test/secret_base.h"
#include "decoration.h"
#include "decoration_inventory.h"
#include "secret_base.h"
#include "event_data.h"
#include "fieldmap.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "money.h"
#include "main.h"
#include "script.h"
#include "task.h"
#include "window.h"
#include "tilesets.h"
#include "constants/decorations.h"
#include "constants/event_objects.h"
#include "constants/event_bg.h"
#include "event_scripts.h"
#include "constants/maps.h"
#include "constants/metatile_labels.h"
#include "constants/metatile_behaviors.h"

extern const struct Tileset gTileset_SecretBaseRedCave, gTileset_SecretBaseBrownCave;
extern const struct Tileset gTileset_SecretBaseTree, gTileset_SecretBaseShrub;
extern const struct Tileset gTileset_SecretBaseBlueCave, gTileset_SecretBaseYellowCave;
extern const u8 SecretBase_EventScript_HealingBed[], SecretBase_EventScript_StorageUtility[];
extern const u16 FortreeCity_DecorationShop_PokemartDecor_Desks[];
extern void InitSecretBaseAppearance(bool8 hidePC);
extern void InitSecretBaseDecorationSprites(void);
extern void MoveOutOfSecretBaseFromOutside(void);

extern const u8 SecretBase_EventScript_StorageUtilityReturn[];
EWRAM_DATA static u16 sMap[32 * 32] = {0};
EWRAM_DATA static struct MapLayout sLayout = {0};
static const struct MapEvents sEvents = {0};

static void SetupBase(void)
{
    sLayout = (struct MapLayout){.width = 16, .height = 16,
        .primaryTileset = gTilesetPointer_SecretBase,
        .secondaryTileset = gTilesetPointer_SecretBaseRedCave};
    gMapHeader.mapLayout = &sLayout;
    gMapHeader.events = &sEvents;
    gBackupMapLayout = (struct BackupMapLayout){32, 32, sMap};
    for (u32 i = 0; i < ARRAY_COUNT(sMap); i++) sMap[i] = METATILE_SecretBase_Ground;
    memset(gObjectEvents, 0, sizeof(gObjectEvents));
    gObjectEvents[0].active = TRUE;
    gObjectEvents[0].localId = LOCALID_PLAYER;
    gObjectEvents[0].currentCoords.x = 20;
    gObjectEvents[0].currentCoords.y = 20;
    gPlayerAvatar.objectEventId = 0;
    memset(gSaveBlock1Ptr->secretBases, 0, sizeof(gSaveBlock1Ptr->secretBases));
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 1;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_SECRET_BASE_RED_CAVE1);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_SECRET_BASE_RED_CAVE1);
    VarSet(VAR_CURRENT_SECRET_BASE, 0);
    SetDecorationInventoriesPointers();
    ClearDecorationInventories();
    EXPECT(DecorationAdd(DECOR_HEALING_BED));
    EXPECT(DecorationAdd(DECOR_STORAGE_PC));
}

TEST("Secret Base utilities: definitions and original Fortree stock remain stable")
{
    const u16 original[] = {DECOR_SMALL_DESK, DECOR_POKEMON_DESK, DECOR_HEAVY_DESK, DECOR_RAGGED_DESK,
        DECOR_COMFORT_DESK, DECOR_BRICK_DESK, DECOR_CAMP_DESK, DECOR_HARD_DESK};
    EXPECT_EQ(DECOR_HEALING_BED, 121);
    EXPECT_EQ(DECOR_STORAGE_PC, 122);
    EXPECT_EQ(DECOR_MAX_SECRET_BASE, 16);
    for (u32 i = 0; i < ARRAY_COUNT(original); i++) EXPECT_EQ(FortreeCity_DecorationShop_PokemartDecor_Desks[i], original[i]);
    EXPECT_EQ(FortreeCity_DecorationShop_PokemartDecor_Desks[8], DECOR_NONE);
    EXPECT_EQ(gDecorations[121].price, 0);
    EXPECT_EQ(gDecorations[122].price, 0);
    EXPECT_EQ(gDecorations[121].shape, DECORSHAPE_2x2);
    EXPECT_EQ(gDecorations[122].shape, DECORSHAPE_1x2);
    EXPECT_EQ(gDecorations[121].category, DECORCAT_ORNAMENT);
    EXPECT_EQ(gDecorations[122].category, DECORCAT_ORNAMENT);
}

TEST("Secret Base utilities: placement collision bounds and sixteen-slot limit")
{
    SetupBase();
    EXPECT(!TestDecor_Place(DECOR_HEALING_BED, 0, 0));
    EXPECT(!TestDecor_Place(DECOR_STORAGE_PC, 16, 1));
    EXPECT(!TestDecor_Place(DECOR_STORAGE_PC, 3, 0));
    EXPECT(!TestDecor_Place(DECOR_HEALING_BED, 15, 2));
    EXPECT(!TestDecor_Place(DECOR_STORAGE_PC, -1, 1));
    EXPECT(!TestDecor_Place(DECOR_HEALING_BED, 1, 16));
    MapGridSetMetatileIdAt(8, 8, METATILE_SecretBase_PC);
    EXPECT(!TestDecor_Place(DECOR_STORAGE_PC, 1, 1));
    EXPECT(!TestDecor_Place(DECOR_HEALING_BED, 0, 1)); // Wider bed now overlaps the fixed PC.
    MapGridSetMetatileIdAt(8, 8, METATILE_SecretBase_Ground);
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 0, 1));
    EXPECT(TestDecor_Place(DECOR_STORAGE_PC, 15, 15));
    EXPECT(!TestDecor_Place(DECOR_STORAGE_PC, 0, 1));
    EXPECT_EQ(MapGridGetMetatileBehaviorAt(7, 8), MB_SECRET_BASE_IMPASSABLE);
    for (u32 i = 2; i < DECOR_MAX_SECRET_BASE; i++) gSaveBlock1Ptr->secretBases[0].decorations[i] = DECOR_SMALL_DESK;
    EXPECT(!TestDecor_Place(DECOR_STORAGE_PC, 4, 4));
}

TEST("Secret Base utilities: footprint ownership elevation removal and relocation")
{
    SetupBase();
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 2, 3));
    EXPECT(TestDecor_Place(DECOR_STORAGE_PC, 5, 5));
    // Targets reached by facing either bed tile from an adjacent walkable tile.
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 9, 0), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 0), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(GetSecretBaseUtilityScript(12, 12, 0), SecretBase_EventScript_StorageUtility);
    EXPECT_EQ(GetSecretBaseUtilityScript(12, 11, 0), SecretBase_EventScript_StorageUtility);
    EXPECT_EQ(GetSecretBaseUtilityScript(10, 10, 0), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(GetSecretBaseUtilityScript(10, 9, 0), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(GetSecretBaseUtilityScript(11, 10, 0), NULL);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 3), NULL);
    VarSet(VAR_CURRENT_SECRET_BASE, 1);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 0), NULL);
    VarSet(VAR_CURRENT_SECRET_BASE, 0);
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_LITTLEROOT_TOWN);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_LITTLEROOT_TOWN);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 0), NULL);
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_SECRET_BASE_RED_CAVE1);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_SECRET_BASE_RED_CAVE1);
    TestDecor_Remove(0);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 0), NULL);
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 6, 7));
    EXPECT_EQ(GetSecretBaseUtilityScript(13, 14, 0), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 0), NULL);
}

TEST("Secret Base utilities: reload appearance across all six tilesets and preview graphics")
{
    const struct Tileset *tileset = NULL;
    PARAMETRIZE { tileset = &gTileset_SecretBaseRedCave; }
    PARAMETRIZE { tileset = &gTileset_SecretBaseBrownCave; }
    PARAMETRIZE { tileset = &gTileset_SecretBaseTree; }
    PARAMETRIZE { tileset = &gTileset_SecretBaseShrub; }
    PARAMETRIZE { tileset = &gTileset_SecretBaseBlueCave; }
    PARAMETRIZE { tileset = &gTileset_SecretBaseYellowCave; }
    SetupBase();
    sLayout.secondaryTileset = tileset;
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 2, 3));
    EXPECT(TestDecor_Place(DECOR_STORAGE_PC, 5, 5));
    struct SecretBase saved = gSaveBlock1Ptr->secretBases[0];
    for (u32 i = 0; i < ARRAY_COUNT(sMap); i++) sMap[i] = METATILE_SecretBase_Ground;
    gSaveBlock1Ptr->secretBases[0] = saved;
    MapGridSetMetatileIdAt(8, 8, METATILE_SecretBase_PC | MAPGRID_IMPASSABLE);
    InitSecretBaseAppearance(FALSE);
    EXPECT_EQ(MapGridGetMetatileIdAt(8, 8), METATILE_SecretBase_PC);
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 9, 0), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(GetSecretBaseUtilityScript(12, 12, 0), SecretBase_EventScript_StorageUtility);
    EXPECT_EQ(GetSecretBaseUtilityScript(12, 11, 0), SecretBase_EventScript_StorageUtility);
    EXPECT_EQ(memcmp(&saved, &gSaveBlock1Ptr->secretBases[0], sizeof(saved)), 0);
    u8 ALIGNED(4) preview[32];
    TestDecor_CopyTile(512 + 83, preview);
    EXPECT_EQ(memcmp(preview, &tileset->tiles[83 * 8], sizeof(preview)), 0);
    EXPECT_EQ(memcmp(&tileset->tiles[83 * 8], &gTileset_SecretBaseRedCave.tiles[83 * 8], 24 * 32), 0);
}

TEST("Secret Base utilities: party-only healing preserves eggs boxes PP investments and respawn")
{
    SetupBase();
    ZeroPlayerPartyMons();
    for (u32 i = 0; i < 3; i++) {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        CreateMon(mon, SPECIES_PIKACHU, 30, 0, OTID_STRUCT_PLAYER_ID);
        CalculateMonStats(mon);
        EXPECT_NE(GetMonData(mon, MON_DATA_MAX_HP), 0);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++) SetMonMoveSlot(mon, MOVE_THUNDERBOLT, slot);
        u32 v = i == 0 ? 1 : 0;
        SetMonData(mon, MON_DATA_HP, &v);
        v = STATUS1_POISON;
        SetMonData(mon, MON_DATA_STATUS, &v);
        v = 3;
        SetMonData(mon, MON_DATA_PP_BONUSES, &v);
        v = 0;
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++) SetMonData(mon, MON_DATA_PP1 + slot, &v);
    }
    gPartiesCount[B_TRAINER_PLAYER] = 3;
    u32 eggFlag = TRUE;
    SetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_IS_EGG, &eggFlag);
    struct Pokemon egg = gParties[B_TRAINER_PLAYER][2];
    struct BoxPokemon boxed = gParties[B_TRAINER_PLAYER][0].box;
    gPokemonStoragePtr->boxes[0][0] = boxed;
    struct WarpData respawn = gSaveBlock1Ptr->lastHealLocation;
    u8 flags[sizeof(gSaveBlock1Ptr->flags)];
    memcpy(flags, gSaveBlock1Ptr->flags, sizeof(flags));
    HealSecretBaseParty();
    HealSecretBaseParty();
    EXPECT_EQ(memcmp(flags, gSaveBlock1Ptr->flags, sizeof(flags)), 0);
    for (u32 i = 0; i < 2; i++) {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        EXPECT_EQ(GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP));
        EXPECT_EQ(GetMonData(mon, MON_DATA_STATUS), 0);
        EXPECT_EQ(GetMonData(mon, MON_DATA_PP_BONUSES), 3);
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
            EXPECT_EQ(GetMonData(mon, MON_DATA_PP1 + slot), CalculatePPWithBonus(MOVE_THUNDERBOLT, 3, slot));
    }
    EXPECT_EQ(memcmp(&egg, &gParties[B_TRAINER_PLAYER][2], sizeof(egg)), 0);
    EXPECT_EQ(memcmp(&boxed, &gPokemonStoragePtr->boxes[0][0], sizeof(boxed)), 0);
    EXPECT_EQ(memcmp(&respawn, &gSaveBlock1Ptr->lastHealLocation, sizeof(respawn)), 0);
    u32 hp = 1;
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP, &hp);
    VarSet(VAR_CURRENT_SECRET_BASE, 1);
    HealSecretBaseParty();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP), 1);
}

TEST("Secret Base utilities: unknown IDs and full ornament inventory are safe")
{
    SetupBase();
    EXPECT(!DecorationAdd(255));
    EXPECT(!CheckHasDecoration(255));
    EXPECT(!DecorationCheckSpace(255));
    EXPECT_EQ(DecorationRemove(255), 0);
    gSaveBlock1Ptr->decorationOrnaments[2] = 255;
    gSaveBlock1Ptr->secretBases[0].decorations[0] = 255;
    gSaveBlock1Ptr->secretBases[0].decorationPositions[0] = 0x23;
    SetDecorationInventoriesPointers();
    EXPECT_EQ(gSaveBlock1Ptr->decorationOrnaments[2], DECOR_NONE);
    EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].decorations[0], DECOR_NONE);
    EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].decorationPositions[0], 0);
    VarSet(VAR_CURRENT_SECRET_BASE, 1);
    gSaveBlock1Ptr->secretBases[1].decorations[0] = 255;
    InitSecretBaseDecorationSprites();
    for (u32 i = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->decorationOrnaments); i++) gSaveBlock1Ptr->decorationOrnaments[i] = DECOR_HEALING_BED;
    EXPECT(!DecorationAdd(DECOR_STORAGE_PC));
    EXPECT(!DecorationCheckSpace(DECOR_STORAGE_PC));
}

TEST("Secret Base utilities: free menu grants support existing saves without duplicates or charges")
{
    SetupBase();
    ClearDecorationInventories();
    SetMoney(&gSaveBlock1Ptr->money, 1234);
    EXPECT(DecorationAdd(DECOR_RED_BRICK));
    EnsureSecretBaseUtilities();
    EXPECT(CheckHasDecoration(DECOR_HEALING_BED));
    EXPECT(CheckHasDecoration(DECOR_STORAGE_PC));
    EXPECT_EQ(GetNumOwnedDecorationsInCategory(DECORCAT_ORNAMENT), 3);
    EnsureSecretBaseUtilities();
    EXPECT_EQ(GetNumOwnedDecorationsInCategory(DECORCAT_ORNAMENT), 3);
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 2, 3));
    EnsureSecretBaseUtilities();
    EXPECT_EQ(GetNumOwnedDecorationsInCategory(DECORCAT_ORNAMENT), 3);
    EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].decorations[0], DECOR_HEALING_BED);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 1234);
    ClearDecorationInventories();
    VarSet(VAR_CURRENT_SECRET_BASE, 1);
    EnsureSecretBaseUtilities();
    EXPECT_EQ(GetNumOwnedDecorationsInCategory(DECORCAT_ORNAMENT), 0);
}

TEST("Secret Base utilities: storage menu exits and enforces full-party and last-mon restrictions")
{
    u32 count;
    PARAMETRIZE { count = 1; }
    PARAMETRIZE { count = PARTY_SIZE; }
    SetupBase();
    ZeroPlayerPartyMons();
    for (u32 i = 0; i < count; i++) CreateMon(&gParties[B_TRAINER_PLAYER][i], SPECIES_PIKACHU, 30, 0, OTID_STRUCT_PLAYER_ID);
    gPartiesCount[B_TRAINER_PLAYER] = count;
    TestStorage_OpenMenu();
    for (u32 i = 0; i < TestStorage_MenuOption(count == 1 ? 1 : 0); i++)
        TestStorage_Input(DPAD_DOWN);
    EXPECT_EQ(TestStorage_Input(A_BUTTON), 3); // STATE_ERROR_MSG
    TestStorage_Input(B_BUTTON); // Dismiss error
    ScriptContext_SetupScript(SecretBase_EventScript_StorageUtilityReturn);
    ScriptContext_Stop();
    EXPECT_EQ(TestStorage_Input(B_BUTTON), 255); // Task destroyed on exit
    EXPECT(ScriptContext_IsEnabled());
    EXPECT(!ScriptContext_RunScript());
    EXPECT(!ArePlayerFieldControlsLocked());
    FreeAllWindowBuffers();
}

TEST("Secret Base utilities: standard storage launch and return callbacks")
{
    u32 option;
    PARAMETRIZE { option = 0; }
    PARAMETRIZE { option = 1; }
    PARAMETRIZE { option = 2; }
    MainCallback previous = gMain.callback2;
    EXPECT(TestStorage_Launch(option));
    EXPECT(TestStorage_Return());
    SetMainCallback2(previous);
    gFieldCallback = NULL;
}

TEST("Secret Base utilities: standard deposit withdrawal box moves and capacity helpers")
{
    ZeroPlayerPartyMons();
    ResetPokemonStorageSystem();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_PIKACHU, 30, 0, OTID_STRUCT_PLAYER_ID);
    CreateMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_MARILL, 25, 0, OTID_STRUCT_PLAYER_ID);
    gPartiesCount[B_TRAINER_PLAYER] = 2;
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][0]);
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][1]);
    EXPECT_EQ(CountPartyAliveNonEggMonsExcept(1), 1);
    TestStorage_Transfer(TOTAL_BOXES_COUNT, 1, 0, 0);
    EXPECT_EQ(GetBoxMonDataAt(0, 0, MON_DATA_SPECIES), SPECIES_MARILL);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_NONE);
    EXPECT_EQ(CountPartyAliveNonEggMonsExcept(0), 0);
    EXPECT(TestStorage_IsLastPartyMon(0));
    TestStorage_Transfer(0, 0, 1, 3);
    EXPECT_EQ(GetBoxMonDataAt(0, 0, MON_DATA_SPECIES), SPECIES_NONE);
    EXPECT_EQ(GetBoxMonDataAt(1, 3, MON_DATA_SPECIES), SPECIES_MARILL);
    TestStorage_Transfer(1, 3, TOTAL_BOXES_COUNT, 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES), SPECIES_MARILL);
    EXPECT_EQ(GetBoxMonDataAt(1, 3, MON_DATA_SPECIES), SPECIES_NONE);
    for (u32 i = 0; i < IN_BOX_COUNT; i++) SetBoxMonAt(0, i, &gParties[B_TRAINER_PLAYER][0].box);
    EXPECT_EQ(GetFirstFreeBoxSpot(0), -1);
    EXPECT(TestStorage_RejectFullBox(0));
}

TEST("Secret Base utilities: object and background interactions retain priority")
{
    static const u8 objectScript[] = {2};
    static const u8 backgroundScript[] = {2};
    static const struct BgEvent bg = {.x = 2, .y = 3, .kind = BG_EVENT_PLAYER_FACING_ANY, .bgUnion.script = backgroundScript};
    struct MapEvents events = {.objectEventCount = 1, .bgEventCount = 1, .bgEvents = &bg};
    SetupBase();
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 2, 3));
    gMapHeader.events = &events;
    gSaveBlock1Ptr->objectEventTemplates[0] = (struct ObjectEventTemplate){.localId = 1, .script = objectScript};
    gObjectEvents[1].active = TRUE;
    gObjectEvents[1].localId = 1;
    gObjectEvents[1].mapNum = gSaveBlock1Ptr->location.mapNum;
    gObjectEvents[1].mapGroup = gSaveBlock1Ptr->location.mapGroup;
    gObjectEvents[1].currentCoords.x = 9;
    gObjectEvents[1].currentCoords.y = 10;
    EXPECT_EQ(TestUtility_Interaction(9, 10, 0, DIR_NORTH), objectScript);
    gObjectEvents[1].active = FALSE;
    EXPECT_EQ(TestUtility_Interaction(9, 10, 0, DIR_NORTH), backgroundScript);
    gMapHeader.events = &sEvents;
    EXPECT_EQ(TestUtility_Interaction(9, 10, 0, DIR_NORTH), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(TestUtility_Interaction(9, 10, 0, DIR_EAST), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(TestUtility_Interaction(9, 10, 0, DIR_WEST), SecretBase_EventScript_HealingBed);
    EXPECT_EQ(TestUtility_Interaction(9, 9, 0, DIR_SOUTH), SecretBase_EventScript_HealingBed);
    MapGridSetMetatileIdAt(8, 8, METATILE_SecretBase_PC);
    EXPECT_EQ(TestUtility_Interaction(8, 8, 0, DIR_NORTH), SecretBase_EventScript_PC);
}

TEST("Secret Base utilities: changing bases clears placements but retains inventory")
{
    SetupBase();
    EXPECT(TestDecor_Place(DECOR_HEALING_BED, 2, 3));
    EXPECT(TestDecor_Place(DECOR_STORAGE_PC, 5, 5));
    MoveOutOfSecretBaseFromOutside();
    EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].secretBaseId, 0);
    for (u32 i = 0; i < DECOR_MAX_SECRET_BASE; i++) {
        EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].decorations[i], DECOR_NONE);
        EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].decorationPositions[i], 0);
    }
    EXPECT(CheckHasDecoration(DECOR_HEALING_BED));
    EXPECT(CheckHasDecoration(DECOR_STORAGE_PC));
    EXPECT_EQ(GetSecretBaseUtilityScript(9, 10, 0), NULL);
}

TEST("Secret Base utilities: free grants preserve full inventories and retry after space is freed")
{
    SetupBase();
    ClearDecorationInventories();
    for (u32 i = 0; i < ARRAY_COUNT(gSaveBlock1Ptr->decorationOrnaments); i++) EXPECT(DecorationAdd(DECOR_RED_BRICK));
    EnsureSecretBaseUtilities();
    EXPECT_EQ(GetNumOwnedDecorationsInCategory(DECORCAT_ORNAMENT), 30);
    for (u32 i = 0; i < 30; i++) EXPECT_EQ(gSaveBlock1Ptr->decorationOrnaments[i], DECOR_RED_BRICK);
    EXPECT_EQ(DecorationRemove(DECOR_RED_BRICK), 1);
    EnsureSecretBaseUtilities();
    EXPECT(CheckHasDecoration(DECOR_HEALING_BED));
    EXPECT(!CheckHasDecoration(DECOR_STORAGE_PC));
    EXPECT_EQ(DecorationRemove(DECOR_RED_BRICK), 1);
    EnsureSecretBaseUtilities();
    EXPECT(CheckHasDecoration(DECOR_STORAGE_PC));
    EXPECT_EQ(GetNumOwnedDecorationsInCategory(DECORCAT_ORNAMENT), 30);
    EXPECT_EQ(DecorationRemove(DECOR_HEALING_BED), 1);
    EnsureSecretBaseUtilities();
    EXPECT(CheckHasDecoration(DECOR_HEALING_BED));
}

TEST("Secret Base utilities: enlarged bed never overwrites fixed PC or walls on reload")
{
    SetupBase();
    gSaveBlock1Ptr->secretBases[0].decorations[0] = DECOR_HEALING_BED;
    gSaveBlock1Ptr->secretBases[0].decorationPositions[0] = 0x01;
    MapGridSetMetatileIdAt(8, 8, METATILE_SecretBase_PC | MAPGRID_IMPASSABLE);
    InitSecretBaseAppearance(FALSE);
    EXPECT_EQ(MapGridGetMetatileIdAt(8, 8), METATILE_SecretBase_PC);
    EXPECT_EQ(GetSecretBaseUtilityScript(7, 8, 0), NULL);
    EXPECT_EQ(gSaveBlock1Ptr->secretBases[0].decorations[0], DECOR_HEALING_BED);
    EXPECT(CheckHasDecoration(DECOR_HEALING_BED)); // Still available to put away/reposition.
    ShowDecorationOnMap(22, 8, DECOR_HEALING_BED); // Outside the 16-wide layout.
    EXPECT_EQ(MapGridGetMetatileIdAt(22, 8), METATILE_SecretBase_Ground);
}
