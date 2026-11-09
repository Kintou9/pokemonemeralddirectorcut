#include "global.h"
#include "pokemon.h"
#include "test/test.h"
#include "data.h"
#include "trainer_util.h"

struct Gen3Progression { enum Species species; u16 exp; u8 evs[NUM_STATS]; u8 friendship; };
#include "data/progression_gen3.h"

TEST("Gen3 progression: all original species EXP yields")
{
    EXPECT_EQ(ARRAY_COUNT(sGen3Progression), 386);
    for (u32 i = 0; i < ARRAY_COUNT(sGen3Progression); i++)
        EXPECT_EQ(gSpeciesInfo[sGen3Progression[i].species].expYield, sGen3Progression[i].exp);
}

TEST("Gen3 progression: all original species EV yields")
{
    for (u32 i = 0; i < ARRAY_COUNT(sGen3Progression); i++) {
        const struct SpeciesInfo *info = &gSpeciesInfo[sGen3Progression[i].species];
        const u8 evs[] = {info->evYield_HP, info->evYield_Attack, info->evYield_Defense, info->evYield_Speed, info->evYield_SpAttack, info->evYield_SpDefense};
        for (u32 stat = 0; stat < NUM_STATS; stat++) EXPECT_EQ(evs[stat], sGen3Progression[i].evs[stat]);
    }
}

TEST("Gen3 progression: newly created Pokemon retain original starting friendship")
{
    struct Pokemon mon;
    for (u32 i = 0; i < ARRAY_COUNT(sGen3Progression); i++) {
        CreateMon(&mon, sGen3Progression[i].species, 50, 0, OTID_STRUCT_PRESET(0));
        EXPECT_EQ(GetMonData(&mon, MON_DATA_FRIENDSHIP), sGen3Progression[i].friendship);
    }
}

TEST("Gen3 progression: friendship evolution requires 220")
{
    enum Species species, evolved;
    u8 friendship;
    PARAMETRIZE { species = SPECIES_PICHU; evolved = SPECIES_PIKACHU; friendship = 219; }
    PARAMETRIZE { species = SPECIES_PICHU; evolved = SPECIES_PIKACHU; friendship = 220; }
    PARAMETRIZE { species = SPECIES_GOLBAT; evolved = SPECIES_CROBAT; friendship = 219; }
    PARAMETRIZE { species = SPECIES_GOLBAT; evolved = SPECIES_CROBAT; friendship = 220; }
    PARAMETRIZE { species = SPECIES_CHANSEY; evolved = SPECIES_BLISSEY; friendship = 219; }
    PARAMETRIZE { species = SPECIES_CHANSEY; evolved = SPECIES_BLISSEY; friendship = 220; }
    struct Pokemon mon;
    bool32 canStop = FALSE;
    CreateMon(&mon, species, 50, 0, OTID_STRUCT_PRESET(0));
    SetMonData(&mon, MON_DATA_FRIENDSHIP, &friendship);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, &canStop, CHECK_EVO), friendship < 220 ? SPECIES_NONE : evolved);
}

TEST("Gen3 progression: EV awards retain the approved 252 per-stat cap")
{
    struct Pokemon mon;
    u8 ev = 251;
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0, OTID_STRUCT_PRESET(0));
    SetMonData(&mon, MON_DATA_HP_EV, &ev);
    MonGainEVs(&mon, SPECIES_WOBBUFFET);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HP_EV), 252);
    MonGainEVs(&mon, SPECIES_WOBBUFFET);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HP_EV), 252);
}

TEST("Gen3 progression: explicit trainer EVs override species yield defaults")
{
    // Test builds replace the story trainer table. Exercise the real trainer
    // generation function with an explicit EV template; separately checksum
    // trainers.party to verify all enhanced teams remain byte-identical.
    static const u8 evs[] = {12, 24, 36, 48, 60, 72}; // Showdown order
    static const struct TrainerMon template = {
        .species = SPECIES_PIKACHU, .lvl = 50, .gender = TRAINER_MON_MALE,
        .nature = NATURE_HARDY, .ability = ABILITY_STATIC,
        .moves = {MOVE_SPLASH}, .ev = evs,
    };
    static const u8 fields[] = {MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV};
    struct TrainerGenerator generator = {0};
    struct Pokemon mon;
    generator.localRngState.ctr = 1;
    GenerateMonFromTrainerMon(&mon, &template, &generator);
    for (u32 stat = 0; stat < NUM_STATS; stat++)
        EXPECT_EQ(GetMonData(&mon, fields[stat]), evs[stat]);
}
