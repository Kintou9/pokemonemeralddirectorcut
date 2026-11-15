#include "global.h"
#include "test/test.h"
#include "secret_base.h"
#include "region_map.h"
#include "overworld.h"
#include "event_data.h"
#include "constants/maps.h"
#include "constants/event_bg.h"
#include "constants/heal_locations.h"

static void SetupBaseFly(void)
{
    memset(gSaveBlock1Ptr->secretBases, 0, sizeof(gSaveBlock1Ptr->secretBases));
    gMapHeader = *Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_ROUTE110), MAP_NUM(MAP_ROUTE110));
    FlagSet(FLAG_BADGE06_GET);
}

TEST("Secret Base Fly: all 75 entrances resolve to safe outdoor destinations and selectable markers")
{
    const u8 routes[][2] = {
        {MAP_GROUP(MAP_ROUTE110), MAP_NUM(MAP_ROUTE110)},
        {MAP_GROUP(MAP_ROUTE111), MAP_NUM(MAP_ROUTE111)},
        {MAP_GROUP(MAP_ROUTE113), MAP_NUM(MAP_ROUTE113)},
        {MAP_GROUP(MAP_ROUTE114), MAP_NUM(MAP_ROUTE114)},
        {MAP_GROUP(MAP_ROUTE115), MAP_NUM(MAP_ROUTE115)},
        {MAP_GROUP(MAP_ROUTE116), MAP_NUM(MAP_ROUTE116)},
        {MAP_GROUP(MAP_ROUTE118), MAP_NUM(MAP_ROUTE118)},
        {MAP_GROUP(MAP_ROUTE119), MAP_NUM(MAP_ROUTE119)},
        {MAP_GROUP(MAP_ROUTE120), MAP_NUM(MAP_ROUTE120)},
        {MAP_GROUP(MAP_ROUTE121), MAP_NUM(MAP_ROUTE121)},
        {MAP_GROUP(MAP_ROUTE123), MAP_NUM(MAP_ROUTE123)},
        {MAP_GROUP(MAP_ROUTE125), MAP_NUM(MAP_ROUTE125)},
        {MAP_GROUP(MAP_ROUTE127), MAP_NUM(MAP_ROUTE127)},
    };
    u32 count = 0;
    SetupBaseFly();
    for (u32 i = 0; i < ARRAY_COUNT(routes); i++)
    {
        const struct MapHeader *header = Overworld_GetMapHeaderByGroupAndId(routes[i][0], routes[i][1]);
        for (u32 j = 0; j < header->events->bgEventCount; j++)
        {
            const struct BgEvent *event = &header->events->bgEvents[j];
            struct WarpData warp;
            struct RegionMap regionMap = {0};
            u16 section, x, y;
            if (event->kind != BG_EVENT_SECRET_BASE)
                continue;
            gSaveBlock1Ptr->secretBases[0].secretBaseId = event->bgUnion.secretBaseId;
            EXPECT(GetSecretBaseFlyDestination(&warp, &section));
            EXPECT_EQ(warp.mapGroup, routes[i][0]);
            EXPECT_EQ(warp.mapNum, routes[i][1]);
            EXPECT_EQ(warp.x, event->x);
            EXPECT_EQ(warp.y, event->y + 1);
            EXPECT_EQ(warp.warpId, WARP_ID_NONE);
            EXPECT_EQ(header->mapLayout->map[warp.y * header->mapLayout->width + warp.x] & MAPGRID_COLLISION_MASK, 0);
            EXPECT(GetSecretBaseFlyMapPosition(&x, &y));
            EXPECT_EQ(GetRegionMapSecIdAt(x, y), section);
            regionMap.mapSecId = section;
            regionMap.cursorPosX = x;
            regionMap.cursorPosY = y;
            EXPECT(IsSecretBaseFlySelection(&regionMap));
            SetFlyDestination(&regionMap);
            EXPECT_EQ(GetDestinationWarpMapHeader(), header);
            ApplyCurrentWarp();
            EXPECT_EQ(gSaveBlock1Ptr->location.x, event->x);
            EXPECT_EQ(gSaveBlock1Ptr->location.y, event->y + 1);
            regionMap.cursorPosX++;
            EXPECT(!IsSecretBaseFlySelection(&regionMap));
            count++;
        }
    }
    EXPECT_EQ(count, 75);
}

TEST("Secret Base Fly: absent foreign corrupt and packed-up bases are unavailable")
{
    struct WarpData warp;
    u16 section, x, y;
    SetupBaseFly();
    gSaveBlock1Ptr->secretBases[1].secretBaseId = 1;
    EXPECT(!GetSecretBaseFlyDestination(&warp, &section));
    EXPECT(!GetSecretBaseFlyMapPosition(&x, &y));
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 255;
    EXPECT(!GetSecretBaseFlyDestination(&warp, &section));
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 1;
    EXPECT(GetSecretBaseFlyDestination(&warp, &section));
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 0;
    EXPECT(!GetSecretBaseFlyMapPosition(&x, &y));
}

TEST("Secret Base Fly: moving the base changes destination without persistent flags or warp changes")
{
    struct WarpData before, after, dynamic, respawn;
    u16 section;
    SetupBaseFly();
    dynamic = gSaveBlock1Ptr->dynamicWarp;
    respawn = gSaveBlock1Ptr->lastHealLocation;
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 1;
    EXPECT(GetSecretBaseFlyDestination(&before, &section));
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 2;
    EXPECT(GetSecretBaseFlyDestination(&after, &section));
    EXPECT(before.mapNum != after.mapNum || before.x != after.x || before.y != after.y);
    EXPECT_EQ(memcmp(&dynamic, &gSaveBlock1Ptr->dynamicWarp, sizeof(dynamic)), 0);
    EXPECT_EQ(memcmp(&respawn, &gSaveBlock1Ptr->lastHealLocation, sizeof(respawn)), 0);
}

TEST("Secret Base Fly: normal Fly badge region and existing town destinations remain respected")
{
    struct RegionMap regionMap = {0};
    u16 x, y;
    SetupBaseFly();
    gSaveBlock1Ptr->secretBases[0].secretBaseId = 1;
    FlagClear(FLAG_BADGE06_GET);
    EXPECT(!GetSecretBaseFlyMapPosition(&x, &y));
    FlagSet(FLAG_BADGE06_GET);
    EXPECT(GetSecretBaseFlyMapPosition(&x, &y));
    gMapHeader.regionMapSectionId = MAPSEC_PALLET_TOWN;
    EXPECT(!GetSecretBaseFlyMapPosition(&x, &y));
    gMapHeader.regionMapSectionId = MAPSEC_ROUTE_110;
    regionMap.mapSecId = MAPSEC_LITTLEROOT_TOWN;
    EXPECT(!IsSecretBaseFlySelection(&regionMap));
    EXPECT_EQ(FilterFlyDestination(&regionMap), gSaveBlock2Ptr->playerGender == MALE
        ? HEAL_LOCATION_LITTLEROOT_TOWN_BRENDANS_HOUSE : HEAL_LOCATION_LITTLEROOT_TOWN_MAYS_HOUSE);
    SetFlyDestination(&regionMap);
    EXPECT_EQ(GetDestinationWarpMapHeader()->regionMapSectionId, MAPSEC_LITTLEROOT_TOWN);
}
