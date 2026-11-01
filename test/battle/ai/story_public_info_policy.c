#include "global.h"
#include "battle.h"
#include "battle_ai_story_policy.h"
#include "test/battle.h"
#include "constants/opponents.h"

#define STORY_FLAGS (BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER)
#define SPACE_CENTER_FLAGS (STORY_FLAGS | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)

static const u16 sExpectedApprovedIds[] =
{
    TRAINER_ROXANNE_1, TRAINER_ROXANNE_2, TRAINER_ROXANNE_3, TRAINER_ROXANNE_4, TRAINER_ROXANNE_5,
    TRAINER_BRAWLY_1, TRAINER_BRAWLY_2, TRAINER_BRAWLY_3, TRAINER_BRAWLY_4, TRAINER_BRAWLY_5,
    TRAINER_WATTSON_1, TRAINER_WATTSON_2, TRAINER_WATTSON_3, TRAINER_WATTSON_4, TRAINER_WATTSON_5,
    TRAINER_FLANNERY_1, TRAINER_FLANNERY_2, TRAINER_FLANNERY_3, TRAINER_FLANNERY_4, TRAINER_FLANNERY_5,
    TRAINER_NORMAN_1, TRAINER_NORMAN_2, TRAINER_NORMAN_3, TRAINER_NORMAN_4, TRAINER_NORMAN_5,
    TRAINER_WINONA_1, TRAINER_WINONA_2, TRAINER_WINONA_3, TRAINER_WINONA_4, TRAINER_WINONA_5,
    TRAINER_TATE_AND_LIZA_1, TRAINER_TATE_AND_LIZA_2, TRAINER_TATE_AND_LIZA_3, TRAINER_TATE_AND_LIZA_4, TRAINER_TATE_AND_LIZA_5,
    TRAINER_JUAN_1, TRAINER_JUAN_2, TRAINER_JUAN_3, TRAINER_JUAN_4, TRAINER_JUAN_5,
    TRAINER_BRENDAN_ROUTE_103_MUDKIP, TRAINER_BRENDAN_ROUTE_110_MUDKIP, TRAINER_BRENDAN_ROUTE_119_MUDKIP,
    TRAINER_BRENDAN_ROUTE_103_TREECKO, TRAINER_BRENDAN_ROUTE_110_TREECKO, TRAINER_BRENDAN_ROUTE_119_TREECKO,
    TRAINER_BRENDAN_ROUTE_103_TORCHIC, TRAINER_BRENDAN_ROUTE_110_TORCHIC, TRAINER_BRENDAN_ROUTE_119_TORCHIC,
    TRAINER_BRENDAN_RUSTBORO_TREECKO, TRAINER_BRENDAN_RUSTBORO_MUDKIP, TRAINER_BRENDAN_RUSTBORO_TORCHIC,
    TRAINER_BRENDAN_LILYCOVE_MUDKIP, TRAINER_BRENDAN_LILYCOVE_TREECKO, TRAINER_BRENDAN_LILYCOVE_TORCHIC,
    TRAINER_MAY_ROUTE_103_MUDKIP, TRAINER_MAY_ROUTE_110_MUDKIP, TRAINER_MAY_ROUTE_119_MUDKIP,
    TRAINER_MAY_ROUTE_103_TREECKO, TRAINER_MAY_ROUTE_110_TREECKO, TRAINER_MAY_ROUTE_119_TREECKO,
    TRAINER_MAY_ROUTE_103_TORCHIC, TRAINER_MAY_ROUTE_110_TORCHIC, TRAINER_MAY_ROUTE_119_TORCHIC,
    TRAINER_MAY_RUSTBORO_MUDKIP, TRAINER_MAY_RUSTBORO_TREECKO, TRAINER_MAY_RUSTBORO_TORCHIC,
    TRAINER_MAY_LILYCOVE_MUDKIP, TRAINER_MAY_LILYCOVE_TREECKO, TRAINER_MAY_LILYCOVE_TORCHIC,
    TRAINER_WALLY_MAUVILLE, TRAINER_WALLY_VR_1, TRAINER_WALLY_VR_2, TRAINER_WALLY_VR_3, TRAINER_WALLY_VR_4, TRAINER_WALLY_VR_5,
    TRAINER_MAXIE_MT_CHIMNEY, TRAINER_MAXIE_MAGMA_HIDEOUT, TRAINER_MAXIE_MOSSDEEP,
    TRAINER_TABITHA_MT_CHIMNEY, TRAINER_TABITHA_MAGMA_HIDEOUT, TRAINER_TABITHA_MOSSDEEP,
    TRAINER_ARCHIE, TRAINER_MATT, TRAINER_SHELLY_WEATHER_INSTITUTE, TRAINER_SHELLY_SEAFLOOR_CAVERN,
    TRAINER_SIDNEY, TRAINER_PHOEBE, TRAINER_GLACIA, TRAINER_DRAKE, TRAINER_WALLACE, TRAINER_STEVEN,
};

TEST("Story public-info policy includes exactly 92 unique approved IDs")
{
    struct StoryAiBattleContext context = {STORY_FLAGS, TRAINER_NONE, 0, B_POSITION_OPPONENT_LEFT, FALSE};

    EXPECT_EQ(ARRAY_COUNT(sExpectedApprovedIds), 92);
    for (u32 i = 0; i < ARRAY_COUNT(sExpectedApprovedIds); i++)
    {
        context.opponentA = sExpectedApprovedIds[i];
        EXPECT(BattleAI_StoryPublicInfoApplies(&context));
        for (u32 j = i + 1; j < ARRAY_COUNT(sExpectedApprovedIds); j++)
            EXPECT(sExpectedApprovedIds[i] != sExpectedApprovedIds[j]);
    }

    context.opponentA = TRAINER_MATTHEW;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.opponentA = TRAINER_BRENDAN_PLACEHOLDER;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.opponentA = TRAINER_MAY_PLACEHOLDER;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.opponentA = TRAINER_NONE;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));

    // Also prove no other ordinary trainer ID can activate the policy.
    for (u16 trainerId = 0; trainerId < TRAINERS_COUNT; trainerId++)
    {
        bool32 expected = FALSE;
        for (u32 i = 0; i < ARRAY_COUNT(sExpectedApprovedIds); i++)
        {
            if (trainerId == sExpectedApprovedIds[i])
                expected = TRUE;
        }
        context.opponentA = trainerId;
        EXPECT_EQ(BattleAI_StoryPublicInfoApplies(&context), expected);
    }
}

TEST("Story public-info policy accepts ordinary singles and doubles by opposing owner")
{
    struct StoryAiBattleContext context = {STORY_FLAGS, TRAINER_ROXANNE_1, 0, B_POSITION_OPPONENT_LEFT, FALSE};

    EXPECT(BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_OPPONENT_RIGHT;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.battleTypeFlags |= BATTLE_TYPE_DOUBLE;
    EXPECT(BattleAI_StoryPublicInfoApplies(&context));
    context.battleTypeFlags |= BATTLE_TYPE_TWO_OPPONENTS;
    context.opponentB = TRAINER_MATTHEW;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_OPPONENT_LEFT;
    EXPECT(BattleAI_StoryPublicInfoApplies(&context));
    context.opponentB = TRAINER_BRAWLY_1;
    context.actingPosition = B_POSITION_OPPONENT_RIGHT;
    EXPECT(BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_PLAYER_LEFT;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_PLAYER_RIGHT;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
}

TEST("Story public-info policy allows only identified Space Center opponents in multi battles")
{
    struct StoryAiBattleContext context = {SPACE_CENTER_FLAGS, TRAINER_MAXIE_MOSSDEEP, TRAINER_TABITHA_MOSSDEEP, B_POSITION_OPPONENT_LEFT, FALSE};

    EXPECT(BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_OPPONENT_RIGHT;
    EXPECT(BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_PLAYER_RIGHT; // Steven is the player's partner.
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.actingPosition = B_POSITION_OPPONENT_RIGHT;
    context.opponentB = 0xFFFF;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.opponentB = 0;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.opponentB = TRAINER_MATT;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.opponentB = TRAINER_TABITHA_MOSSDEEP;
    context.battleTypeFlags &= ~BATTLE_TYPE_TWO_OPPONENTS;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.battleTypeFlags = STORY_FLAGS | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
}

TEST("Story public-info policy excludes every Frontier and special battle context")
{
    static const u32 excludedFlags[] = {
        BATTLE_TYPE_BATTLE_TOWER, BATTLE_TYPE_DOME, BATTLE_TYPE_PALACE, BATTLE_TYPE_ARENA,
        BATTLE_TYPE_FACTORY, BATTLE_TYPE_PIKE, BATTLE_TYPE_PYRAMID, BATTLE_TYPE_LINK,
        BATTLE_TYPE_LINK_IN_BATTLE, BATTLE_TYPE_TOWER_LINK_MULTI, BATTLE_TYPE_RECORDED,
        BATTLE_TYPE_RECORDED_LINK, BATTLE_TYPE_RECORDED_IS_MASTER, BATTLE_TYPE_EREADER_TRAINER,
        BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_SECRET_BASE, BATTLE_TYPE_SAFARI,
        BATTLE_TYPE_FIRST_BATTLE, BATTLE_TYPE_CATCH_TUTORIAL, BATTLE_TYPE_ROAMER,
        BATTLE_TYPE_RAID, BATTLE_TYPE_LEGENDARY, BATTLE_TYPE_GHOST, BATTLE_TYPE_POKEDUDE,
        BATTLE_TYPE_14, BATTLE_TYPE_30,
    };
    struct StoryAiBattleContext context = {STORY_FLAGS, TRAINER_ROXANNE_1, 0, B_POSITION_OPPONENT_LEFT, FALSE};

    for (u32 i = 0; i < ARRAY_COUNT(excludedFlags); i++)
    {
        context.battleTypeFlags = STORY_FLAGS | excludedFlags[i];
        EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    }
    context.battleTypeFlags = STORY_FLAGS;
    context.isDebugBattle = TRUE;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    context.isDebugBattle = FALSE;
    context.battleTypeFlags = BATTLE_TYPE_IS_MASTER;
    EXPECT(!BattleAI_StoryPublicInfoApplies(&context));
    EXPECT(!BattleAI_StoryPublicInfoApplies(NULL));
}

AI_SINGLE_BATTLE_TEST("Story public-info guard leaves existing AI flags and move choice unchanged")
{
    GIVEN {
        AI_FLAGS(AI_FLAG_BASIC_TRAINER | AI_FLAG_SMART_SWITCHING);
        PLAYER(SPECIES_WOBBUFFET) { Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Moves(MOVE_TACKLE); }
    } WHEN {
        TURN { MOVE(player, MOVE_SPLASH); EXPECT_MOVE(opponent, MOVE_TACKLE); }
    } THEN {
        struct StoryAiBattleContext context = {STORY_FLAGS, TRAINER_ROXANNE_1, 0, B_POSITION_OPPONENT_LEFT, FALSE};
        u64 flags = gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT];

        EXPECT(BattleAI_StoryPublicInfoApplies(&context));
        EXPECT(!BattleAI_UsesStoryPublicInfo(B_BATTLER_1)); // Recorded test battles are excluded.
        EXPECT_EQ(gAiLogicData->storyPublicInfoActiveMask, 0);
        EXPECT_EQ(gAiThinkingStruct->aiFlags[B_POSITION_OPPONENT_LEFT], flags);
        EXPECT_EQ(gLastMoves[B_POSITION_OPPONENT_LEFT], MOVE_TACKLE);
    }
}
