#include "global.h"
#include "pokemon.h"
#include "pokemon_summary_screen.h"
#include "string_util.h"
#include "text.h"
#include "test/test.h"

static void SetSummaryTestStats(struct Pokemon *mon)
{
    const u8 ivs[] = {0, 31, 17, 1, 30, 16}; // HP, Atk, Def, Speed, SpAtk, SpDef
    const u8 evs[] = {252, 0, 4, 1, 1, 252};
    u8 nature = NATURE_ADAMANT;
    CreateMon(mon, SPECIES_WOBBUFFET, 50, 0, OTID_STRUCT_PRESET(0));
    SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
    for (u32 i = 0; i < NUM_STATS; i++)
    {
        SetMonData(mon, MON_DATA_HP_IV + i, &ivs[i]);
        SetMonData(mon, MON_DATA_HP_EV + i, &evs[i]);
    }
    CalculateMonStats(mon);
}

TEST("Summary screen: six numeric IV/EV values, nature colors, and column widths")
{
    u8 mode;
    PARAMETRIZE { mode = SUMMARY_SKILLS_MODE_IVS; }
    PARAMETRIZE { mode = SUMMARY_SKILLS_MODE_EVS; }
    struct Pokemon mon;
    u8 left[128], right[128];
    SetSummaryTestStats(&mon);
    Test_SummaryScreenFormatStats(&mon, mode, left, right);
    if (mode == SUMMARY_SKILLS_MODE_IVS)
    {
        EXPECT_EQ(StringCompare(left, COMPOUND_STRING("{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}0\n{COLOR}{05}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}31\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}17")), 0);
        EXPECT_EQ(StringCompare(right, COMPOUND_STRING("{COLOR}{08}{UNK_SPACER}30\n{COLOR}{01}{UNK_SPACER}16\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}1")), 0);
    }
    else
    {
        EXPECT_EQ(StringCompare(left, COMPOUND_STRING("{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}252\n{COLOR}{05}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}0\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}4")), 0);
        EXPECT_EQ(StringCompare(right, COMPOUND_STRING("{COLOR}{08}{UNK_SPACER}{UNK_SPACER}1\n{COLOR}{01}252\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}1")), 0);
    }
    EXPECT_LE(GetStringWidth(FONT_NORMAL, left, 0) + 4, 6 * 8);
    EXPECT_LE(GetStringWidth(FONT_NORMAL, right, 0) + 2, 3 * 8);
}

TEST("Summary screen: all five neutral natures leave all six stats unhighlighted")
{
    u8 nature;
    PARAMETRIZE { nature = NATURE_HARDY; }
    PARAMETRIZE { nature = NATURE_DOCILE; }
    PARAMETRIZE { nature = NATURE_SERIOUS; }
    PARAMETRIZE { nature = NATURE_BASHFUL; }
    PARAMETRIZE { nature = NATURE_QUIRKY; }
    struct Pokemon mon;
    u8 left[128], right[128];
    SetSummaryTestStats(&mon);
    SetMonData(&mon, MON_DATA_HIDDEN_NATURE, &nature);
    Test_SummaryScreenFormatStats(&mon, SUMMARY_SKILLS_MODE_IVS, left, right);
    EXPECT_EQ(StringCompare(left, COMPOUND_STRING("{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}0\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}31\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}{UNK_SPACER}17")), 0);
    EXPECT_EQ(StringCompare(right, COMPOUND_STRING("{COLOR}{01}{UNK_SPACER}30\n{COLOR}{01}{UNK_SPACER}16\n{COLOR}{01}{UNK_SPACER}{UNK_SPACER}1")), 0);
}

TEST("Summary screen: Stats IVs EVs cycle restores stats without modifying Pokemon")
{
    struct Pokemon mon, before;
    u8 left[128], right[128], originalLeft[128], originalRight[128];
    SetSummaryTestStats(&mon);
    memcpy(&before, &mon, sizeof(mon));
    EXPECT_EQ(Test_SummaryScreenFormatStats(&mon, SUMMARY_SKILLS_MODE_STATS, originalLeft, originalRight), SUMMARY_SKILLS_MODE_IVS);
    EXPECT_EQ(Test_SummaryScreenFormatStats(&mon, SUMMARY_SKILLS_MODE_IVS, left, right), SUMMARY_SKILLS_MODE_EVS);
    EXPECT_EQ(Test_SummaryScreenFormatStats(&mon, SUMMARY_SKILLS_MODE_EVS, left, right), SUMMARY_SKILLS_MODE_STATS);
    Test_SummaryScreenFormatStats(&mon, SUMMARY_SKILLS_MODE_STATS, left, right);
    EXPECT_EQ(StringCompare(left, originalLeft), 0);
    EXPECT_EQ(StringCompare(right, originalRight), 0);
    EXPECT_EQ(memcmp(&mon, &before, sizeof(mon)), 0);
}
