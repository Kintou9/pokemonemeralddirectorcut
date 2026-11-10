#include "global.h"
#include "pokemon.h"
#include "daycare.h"
#include "egg_hatch.h"
#include "event_data.h"
#include "config_changes.h"
#include "constants/pokeball.h"
#include "test/overworld_script.h"
#include "test/test.h"

#define RESET_DAYCARE() \
    ZeroPlayerPartyMons(); \
    memset(&gSaveBlock1Ptr->daycare, 0, sizeof(gSaveBlock1Ptr->daycare))
#define COLLECT_EGG() \
    gSpecialVar_0x8004 = 0; \
    StoreSelectedPokemonInDaycare(); \
    StoreSelectedPokemonInDaycare(); \
    GiveEggFromDaycare()

static bool32 HasMove(struct Pokemon *mon, enum Move move)
{
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        if (GetMonData(mon, MON_DATA_MOVE1 + i) == move)
            return TRUE;
    return FALSE;
}

TEST("Gen3 breeding: Classic defaults select legacy inheritance")
{
    EXPECT_EQ(P_EGG_HATCH_LEVEL, GEN_3);
    EXPECT_EQ(GetConfig(MOVE_INHERITANCE), GEN_3);
    EXPECT_EQ(P_INCENSE_BREEDING, GEN_3);
    EXPECT_EQ(GetConfig(NATURE_INHERITANCE), GEN_3);
    EXPECT_EQ(GetConfig(ABILITY_INHERITANCE), GEN_3);
    EXPECT_EQ(GetConfig(BALL_INHERITANCE), GEN_3);
    EXPECT_EQ(GetConfig(EGG_MOVE_TRANSFER), GEN_3);
    EXPECT_EQ(P_UPDATED_EGG_GROUPS, GEN_3);
    EXPECT_EQ(P_NIDORAN_M_DITTO_BREED, GEN_3);
}

TEST("Gen3 breeding: baby species and incense with Ditto in either slot")
{
    enum Species parent = SPECIES_NONE, expected = SPECIES_NONE;
    enum Item item = ITEM_NONE;
    u32 first = 0, gender = MON_MALE;
    for (u32 i = 0; i < 2; i++)
    for (u32 j = 0; j < 2; j++)
    {
        PARAMETRIZE { parent = SPECIES_ELECTABUZZ; expected = SPECIES_ELEKID; item = ITEM_NONE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
        PARAMETRIZE { parent = SPECIES_MAGMAR; expected = SPECIES_MAGBY; item = ITEM_NONE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
        PARAMETRIZE { parent = SPECIES_MARILL; expected = SPECIES_AZURILL; item = ITEM_SEA_INCENSE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
        PARAMETRIZE { parent = SPECIES_MARILL; expected = SPECIES_MARILL; item = ITEM_NONE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
        PARAMETRIZE { parent = SPECIES_WOBBUFFET; expected = SPECIES_WYNAUT; item = ITEM_LAX_INCENSE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
        PARAMETRIZE { parent = SPECIES_WOBBUFFET; expected = SPECIES_WOBBUFFET; item = ITEM_NONE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
        PARAMETRIZE { parent = SPECIES_WOBBUFFET; expected = SPECIES_WOBBUFFET; item = ITEM_SEA_INCENSE; first = i; gender = j ? MON_FEMALE : MON_MALE; }
    }
    RESET_DAYCARE();
    gSpecialVar_0x8000 = parent;
    gSpecialVar_0x8001 = item;
    gSpecialVar_0x8002 = gender;
    RUN_OVERWORLD_SCRIPT(
        givemon VAR_0x8000, 30, gender=VAR_0x8002, item=VAR_0x8001;
        givemon SPECIES_DITTO, 30;
    );
    gSpecialVar_0x8004 = first;
    StoreSelectedPokemonInDaycare();
    gSpecialVar_0x8004 = 0;
    StoreSelectedPokemonInDaycare();
    EXPECT_NE(GetDaycareCompatibilityScore(&gSaveBlock1Ptr->daycare), 0);
    GiveEggFromDaycare();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), expected);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), 5);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG), TRUE);
}

TEST("Gen3 breeding: original egg groups reject modern-only partners")
{
    enum Species mother, father;
    bool32 compatible;
    PARAMETRIZE { mother = SPECIES_RALTS; father = SPECIES_ABRA; compatible = FALSE; }
    PARAMETRIZE { mother = SPECIES_RALTS; father = SPECIES_GASTLY; compatible = TRUE; }
    PARAMETRIZE { mother = SPECIES_TRAPINCH; father = SPECIES_BAGON; compatible = FALSE; }
    PARAMETRIZE { mother = SPECIES_TRAPINCH; father = SPECIES_CATERPIE; compatible = TRUE; }
    RESET_DAYCARE();
    gSpecialVar_0x8000 = mother;
    gSpecialVar_0x8001 = father;
    RUN_OVERWORLD_SCRIPT(
        givemon VAR_0x8000, 30, gender=MON_FEMALE;
        givemon VAR_0x8001, 30, gender=MON_MALE;
    );
    gSpecialVar_0x8004 = 0;
    StoreSelectedPokemonInDaycare();
    StoreSelectedPokemonInDaycare();
    EXPECT_EQ(GetDaycareCompatibilityScore(&gSaveBlock1Ptr->daycare) != 0, compatible);
}

TEST("Gen3 breeding: male Nidoran and Volbeat retain their species with Ditto")
{
    enum Species species;
    PARAMETRIZE { species = SPECIES_NIDORAN_M; }
    PARAMETRIZE { species = SPECIES_VOLBEAT; }
    RESET_DAYCARE();
    gSpecialVar_0x8000 = species;
    RUN_OVERWORLD_SCRIPT(givemon VAR_0x8000, 30; givemon SPECIES_DITTO, 30;);
    COLLECT_EGG();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), species);
}

TEST("Gen3 breeding: paternal egg TM HM and shared level-up moves")
{
    enum Species species;
    enum Move dad, mom, checked;
    bool32 inherited;
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_CURSE; mom = MOVE_TACKLE; checked = MOVE_CURSE; inherited = TRUE; }
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_TOXIC; mom = MOVE_TACKLE; checked = MOVE_TOXIC; inherited = TRUE; }
    PARAMETRIZE { species = SPECIES_SQUIRTLE; dad = MOVE_SURF; mom = MOVE_TACKLE; checked = MOVE_SURF; inherited = TRUE; }
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_SURF; mom = MOVE_TACKLE; checked = MOVE_SURF; inherited = FALSE; }
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_RAZOR_LEAF; mom = MOVE_RAZOR_LEAF; checked = MOVE_RAZOR_LEAF; inherited = TRUE; }
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_RAZOR_LEAF; mom = MOVE_TACKLE; checked = MOVE_RAZOR_LEAF; inherited = FALSE; }
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_TACKLE; mom = MOVE_CURSE; checked = MOVE_CURSE; inherited = FALSE; }
    // Deliberately preserved modern egg data: this does not grant parents a new learning route.
    PARAMETRIZE { species = SPECIES_BULBASAUR; dad = MOVE_LEAF_STORM; mom = MOVE_TACKLE; checked = MOVE_LEAF_STORM; inherited = TRUE; }
    RESET_DAYCARE();
    gSpecialVar_0x8000 = species;
    gSpecialVar_0x8001 = dad;
    gSpecialVar_0x8002 = mom;
    RUN_OVERWORLD_SCRIPT(
        givemon VAR_0x8000, 30, gender=MON_MALE, move1=VAR_0x8001, move2=MOVE_NONE, move3=MOVE_NONE, move4=MOVE_NONE;
        givemon VAR_0x8000, 30, gender=MON_FEMALE, move1=VAR_0x8002, move2=MOVE_NONE, move3=MOVE_NONE, move4=MOVE_NONE;
    );
    COLLECT_EGG();
    EXPECT_EQ(HasMove(&gParties[B_TRAINER_PLAYER][0], checked), inherited);
}

TEST("Gen3 breeding: Mirror Herb does not transfer a partner's moves")
{
    RESET_DAYCARE();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_BULBASAUR, 30, item=ITEM_MIRROR_HERB, move1=MOVE_TACKLE, move2=MOVE_NONE, move3=MOVE_NONE, move4=MOVE_NONE;
        givemon SPECIES_BULBASAUR, 30, move1=MOVE_CURSE;
    );
    gSpecialVar_0x8004 = 0;
    StoreSelectedPokemonInDaycare();
    StoreSelectedPokemonInDaycare();
    EXPECT_EQ(GetBoxMonData(&gSaveBlock1Ptr->daycare.mons[0].mon, MON_DATA_MOVE2), MOVE_NONE);
}

TEST("Gen3 breeding: Everstone uses Ditto precedence and a controlled 50 percent check")
{
    bool32 success;
    PARAMETRIZE { success = TRUE; }
    PARAMETRIZE { success = FALSE; }
    RESET_DAYCARE();
    SET_RNG(RNG_DAYCARE_NATURE_INHERITANCE, success);
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_BULBASAUR, 30, gender=MON_FEMALE, item=ITEM_EVERSTONE, nature=NATURE_ADAMANT;
        givemon SPECIES_DITTO, 30, item=ITEM_EVERSTONE, nature=NATURE_MODEST;
    );
    gSpecialVar_0x8004 = 0;
    StoreSelectedPokemonInDaycare();
    StoreSelectedPokemonInDaycare();
    EXPECT_EQ(GetChildNature(&gSaveBlock1Ptr->daycare), success ? NATURE_MODEST : NATURE_RANDOM);
}

TEST("Gen3 breeding: parents do not override the newborn normal ability")
{
    u32 slot;
    PARAMETRIZE { slot = 0; }
    PARAMETRIZE { slot = 1; }
    RESET_DAYCARE();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_ABRA, 30, gender=MON_FEMALE, abilityNum=2;
        givemon SPECIES_DITTO, 30;
    );
    gSpecialVar_0x8004 = 0;
    StoreSelectedPokemonInDaycare();
    StoreSelectedPokemonInDaycare();
    struct Pokemon egg;
    CreateMon(&egg, SPECIES_ABRA, 5, slot, OTID_STRUCT_PLAYER_ID);
    SET_RNG(RNG_DAYCARE_ABILITY_INHERITANCE, TRUE);
    InheritAbility(&egg, &gSaveBlock1Ptr->daycare);
    EXPECT_EQ(GetMonData(&egg, MON_DATA_ABILITY_NUM), slot);
}

TEST("Gen3 breeding: newborns use ordinary Poke Balls")
{
    RESET_DAYCARE();
    RUN_OVERWORLD_SCRIPT(
        givemon SPECIES_BULBASAUR, 30, gender=MON_FEMALE, ball=BALL_ULTRA;
        givemon SPECIES_DITTO, 30, ball=BALL_GREAT;
    );
    COLLECT_EGG();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_POKEBALL), BALL_POKE);
}

TEST("Gen3 breeding: gift eggs hatch at level five")
{
    RESET_DAYCARE();
    CreateEgg(&gParties[B_TRAINER_PLAYER][0], SPECIES_WYNAUT, TRUE);
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    gSpecialVar_0x8004 = 0;
    ScriptHatchMon();
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), 5);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG), FALSE);
}

TEST("Gen3 breeding: egg cycles remain 128 steps with optional Flame Body")
{
    bool32 incubator;
    PARAMETRIZE { incubator = FALSE; }
    PARAMETRIZE { incubator = TRUE; }
    RESET_DAYCARE();
    if (incubator)
        RUN_OVERWORLD_SCRIPT(givemon SPECIES_MAGMAR, 30;);
    u32 slot = incubator ? 1 : 0;
    CreateEgg(&gParties[B_TRAINER_PLAYER][slot], SPECIES_BULBASAUR, FALSE);
    gPartiesCount[B_TRAINER_PLAYER] = slot + 1;
    u32 cycles = 2;
    SetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_FRIENDSHIP, &cycles);
    for (u32 i = 0; i < 127; i++)
        EXPECT_EQ(ShouldEggHatch(), FALSE);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_FRIENDSHIP), 2);
    EXPECT_EQ(ShouldEggHatch(), FALSE);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_FRIENDSHIP), incubator ? 0 : 1);
    if (!incubator)
        for (u32 i = 0; i < 128; i++)
            EXPECT_EQ(ShouldEggHatch(), FALSE);
    for (u32 i = 0; i < 127; i++)
        EXPECT_EQ(ShouldEggHatch(), FALSE);
    EXPECT_EQ(ShouldEggHatch(), TRUE);
    EXPECT_EQ(gSpecialVar_0x8004, slot);
}

TEST("Gen3 breeding: existing level-one eggs keep their stored attributes")
{
    RESET_DAYCARE();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_ABRA, 1, ball=BALL_ULTRA, abilityNum=1, move1=MOVE_LEAF_STORM;);
    struct Pokemon *egg = &gParties[B_TRAINER_PLAYER][0];
    u32 value = TRUE;
    SetMonData(egg, MON_DATA_IS_EGG, &value);
    value = 0;
    SetMonData(egg, MON_DATA_FRIENDSHIP, &value);
    gSaveBlock1Ptr->daycare.stepCounter = 127;
    EXPECT_EQ(ShouldEggHatch(), TRUE);
    ScriptHatchMon();
    EXPECT_EQ(GetMonData(egg, MON_DATA_LEVEL), 1);
    EXPECT_EQ(GetMonData(egg, MON_DATA_MOVE1), MOVE_LEAF_STORM);
    EXPECT_EQ(GetMonData(egg, MON_DATA_ABILITY_NUM), 1);
    EXPECT_EQ(GetMonData(egg, MON_DATA_POKEBALL), BALL_ULTRA);
    EXPECT_EQ(GetMonData(egg, MON_DATA_IS_EGG), FALSE);
}
