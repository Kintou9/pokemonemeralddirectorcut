#include "global.h"
#include "battle.h"
#include "battle_ai_story_policy.h"
#include "battle_setup.h"
#include "debug.h"
#include "constants/opponents.h"

static const u16 sApprovedStoryTrainerIds[] =
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

STATIC_ASSERT(ARRAY_COUNT(sApprovedStoryTrainerIds) == 92, StoryAiTrainerAllowlistMustHave92Ids);

static bool32 IsApprovedStoryTrainer(u16 trainerId)
{
    for (u32 i = 0; i < ARRAY_COUNT(sApprovedStoryTrainerIds); i++)
    {
        if (sApprovedStoryTrainerIds[i] == trainerId)
            return TRUE;
    }
    return FALSE;
}

bool32 BattleAI_StoryPublicInfoApplies(const struct StoryAiBattleContext *context)
{
    const u32 supported = BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE
                        | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    const u32 excluded = BATTLE_TYPE_FRONTIER | BATTLE_TYPE_LINK | BATTLE_TYPE_LINK_IN_BATTLE
                       | BATTLE_TYPE_TOWER_LINK_MULTI | BATTLE_TYPE_RECORDED | BATTLE_TYPE_RECORDED_LINK
                       | BATTLE_TYPE_RECORDED_IS_MASTER | BATTLE_TYPE_EREADER_TRAINER
                       | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SECRET_BASE | BATTLE_TYPE_SAFARI
                       | BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_ROAMER
                       | BATTLE_TYPE_RAID | BATTLE_TYPE_LEGENDARY | BATTLE_TYPE_GHOST | BATTLE_TYPE_POKEDUDE;
    u16 actingTrainer;

    if (context == NULL || context->isDebugBattle || !(context->battleTypeFlags & BATTLE_TYPE_TRAINER)
        || (context->battleTypeFlags & excluded) || (context->battleTypeFlags & ~supported))
        return FALSE;

    if (context->actingPosition != B_POSITION_OPPONENT_LEFT && context->actingPosition != B_POSITION_OPPONENT_RIGHT)
        return FALSE;

    if (context->battleTypeFlags & BATTLE_TYPE_MULTI)
    {
        // The only supported story multi battle has Steven beside the player.
        if ((context->battleTypeFlags & (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER))
            != (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER))
            return FALSE;
        if (!((context->opponentA == TRAINER_MAXIE_MOSSDEEP && context->opponentB == TRAINER_TABITHA_MOSSDEEP)
           || (context->opponentA == TRAINER_TABITHA_MOSSDEEP && context->opponentB == TRAINER_MAXIE_MOSSDEEP)))
            return FALSE;
    }
    else if (context->battleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        // Other partner battles are not part of this policy.
        return FALSE;
    }

    if (context->battleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
    {
        if (!(context->battleTypeFlags & BATTLE_TYPE_DOUBLE) || context->opponentB == 0 || context->opponentB == 0xFFFF)
            return FALSE;
        actingTrainer = context->actingPosition == B_POSITION_OPPONENT_LEFT ? context->opponentA : context->opponentB;
    }
    else
    {
        if (context->opponentB != 0 && context->opponentB != 0xFFFF)
            return FALSE;
        if (context->actingPosition == B_POSITION_OPPONENT_RIGHT && !(context->battleTypeFlags & BATTLE_TYPE_DOUBLE))
            return FALSE;
        actingTrainer = context->opponentA;
    }

    return IsApprovedStoryTrainer(actingTrainer);
}

bool32 BattleAI_UsesStoryPublicInfo(enum BattlerId battler)
{
    struct StoryAiBattleContext context;

    if (battler >= gBattlersCount)
        return FALSE;

    context.battleTypeFlags = gBattleTypeFlags;
    context.opponentA = TRAINER_BATTLE_PARAM.opponentA;
    context.opponentB = TRAINER_BATTLE_PARAM.opponentB;
    context.actingPosition = GetBattlerPosition(battler);
    context.isDebugBattle = gIsDebugBattle;
    return BattleAI_StoryPublicInfoApplies(&context);
}
