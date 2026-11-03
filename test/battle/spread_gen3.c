#include "global.h"
#include "test/battle.h"
#include "battle_util.h"
#include "battle_ai_util.h"
#include "battle_ai_calc_context.h"

// Oracle: pret/pokeemerald src/pokemon.c, CalculateBaseDamage and
// CountAliveMonsInBattle. Level 50, offensive/defensive stats 100, neutral
// typing, no STAB/crit/weather/items, highest roll. Gen 3 divides the term
// BEFORE +2; Explosion first halves Defense. Modern controls intentionally
// retain this project's existing post-+2 0.75 multiplier/rounding.
static const enum Move sMoves[] = {MOVE_SURF, MOVE_ROCK_SLIDE, MOVE_EARTHQUAKE, MOVE_EXPLOSION};
static const u16 sSingle[] = {43, 35, 46, 222};
static const u16 sGen3Spread[] = {22, 18, 46, 222};
static const u16 sModernSpread[] = {32, 26, 34, 166};

SINGLE_BATTLE_TEST("Spread restoration: singles remain unreduced", s16 damage)
{
    u32 m = 0, gen = GEN_3;
    for (u32 j = 0; j < 4; j++)
        for (u32 g = GEN_3; g <= GEN_4; g++)
            PARAMETRIZE { m = j; gen = g; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); SpAttack(100); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); SpDefense(100); }
    } WHEN {
        TURN { MOVE(player, sMoves[m], WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, sSingle[m]);
    }
}

DOUBLE_BATTLE_TEST("Spread restoration: real attacks from every position")
{
    u32 m = 0, gen = GEN_3, actor = 0;
    for (u32 j = 0; j < 4; j++)
        for (u32 g = GEN_3; g <= GEN_4; g++)
            for (u32 a = 0; a < 4; a++)
                PARAMETRIZE { m = j; gen = g; actor = a; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); }
        PLAYER(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); }
    } WHEN {
        TURN {
            MOVE(playerLeft, actor == 0 ? sMoves[m] : MOVE_SPLASH, WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
            MOVE(opponentLeft, actor == 1 ? sMoves[m] : MOVE_SPLASH, WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
            MOVE(playerRight, actor == 2 ? sMoves[m] : MOVE_SPLASH, WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
            MOVE(opponentRight, actor == 3 ? sMoves[m] : MOVE_SPLASH, WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
        }
    } THEN {
        for (enum BattlerId b = 0; b < 4; b++)
        {
            if (b == actor)
                EXPECT_EQ(gBattleMons[b].hp, m == 3 ? 0 : 1000);
            else if (m < 2 && IsBattlerAlly(b, (enum BattlerId)actor))
                EXPECT_EQ(gBattleMons[b].hp, 1000);
            else
                EXPECT_EQ(gBattleMons[b].hp, 1000 - (gen == GEN_3 ? sGen3Spread[m] : sModernSpread[m]));
        }
    }
}

DOUBLE_BATTLE_TEST("Spread restoration: one opposing position absent before attack")
{
    u32 m = 0, gen = GEN_3;
    for (u32 j = 0; j < 4; j++)
        for (u32 g = GEN_3; g <= GEN_4; g++)
            PARAMETRIZE { m = j; gen = g; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); SpAttack(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); }
    } WHEN {
        TURN { MOVE(playerRight, MOVE_SONIC_BOOM, target: opponentRight); }
        TURN { MOVE(playerLeft, sMoves[m], WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, 1000 - (gen == GEN_3 || m < 2 ? sSingle[m] : sModernSpread[m]));
        EXPECT_EQ(playerRight->hp, m < 2 ? 1000 : 1000 - (gen == GEN_3 ? sSingle[m] : sModernSpread[m]));
    }
}

DOUBLE_BATTLE_TEST("Spread restoration: protected and immune positions still count")
{
    u32 m = 0, gen = GEN_3;
    bool32 protect = FALSE;
    for (u32 j = 0; j < 4; j++)
        for (u32 g = GEN_3; g <= GEN_4; g++)
            for (u32 p = 0; p < 2; p++)
                PARAMETRIZE { m = j; gen = g; protect = p; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); SpAttack(100); HP(1000); MaxHP(1000); }
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); SpDefense(100); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Ability(protect ? ABILITY_SHADOW_TAG : ABILITY_WONDER_GUARD); }
    } WHEN {
        TURN {
            MOVE(opponentRight, protect ? MOVE_PROTECT : MOVE_SPLASH);
            MOVE(playerLeft, sMoves[m], WITH_RNG(RNG_DAMAGE_MODIFIER, 0));
        }
    } THEN {
        EXPECT_EQ(opponentRight->hp, 1000);
        EXPECT_EQ(opponentLeft->hp, 1000 - (gen == GEN_3 ? sGen3Spread[m] : sModernSpread[m]));
    }
}

DOUBLE_BATTLE_TEST("Spread restoration: KO during the same spread attack preserves other damage")
{
    u32 m = 0, gen = GEN_3;
    for (u32 j = 0; j < 4; j++)
        for (u32 g = GEN_3; g <= GEN_4; g++)
            PARAMETRIZE { m = j; gen = g; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); SpAttack(100); }
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); SpDefense(100); }
    } WHEN {
        TURN { MOVE(playerLeft, sMoves[m], WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, 0);
        EXPECT_EQ(opponentRight->hp, 1000 - (gen == GEN_3 ? sGen3Spread[m] : sModernSpread[m]));
    }
}

AI_DOUBLE_BATTLE_TEST("Spread restoration: AI and live calculator agree across occupancy states")
{
    u32 gen;
    PARAMETRIZE { gen = GEN_3; }
    PARAMETRIZE { gen = GEN_4; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        RNGSeed(((rng_value_t){ .ctr = 1 }));
        AI_FLAGS(AI_FLAG_BASIC_TRAINER);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); Moves(MOVE_SPLASH); }
        PLAYER(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); Moves(MOVE_SPLASH); }
        OPPONENT(SPECIES_WOBBUFFET) { Level(50); HP(1000); MaxHP(1000); Attack(100); SpAttack(100); Defense(100); SpDefense(100); Moves(MOVE_SPLASH); }
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SPLASH); MOVE(playerRight, MOVE_SPLASH); }
    } THEN {
        // Restore only the fields changed by this fixture; no live Speed injection.
        u8 savedAbsent = gAbsentBattlerFlags;
        for (enum BattlerId actor = 0; actor < 4; actor++)
        {
            enum BattlerId foe = GetBattlerLeftFoe(actor);
            enum BattlerId otherFoe = GetPartnerBattler(foe);
            enum BattlerId ally = GetPartnerBattler(actor);
            struct AiCalcContext context;
            const struct AiCalcContext *previous = AI_GetActiveCalcContext();
            AI_InitCalcContext(&context, actor);
            AI_SetActiveCalcContext(&context);
            for (u32 state = 0; state < 5; state++)
            {
                // All present; ally absent; one foe absent; both absent;
                // zero HP but not yet marked absent (same-action faint handling).
                gAbsentBattlerFlags = state == 1 ? 1u << ally : state == 2 ? 1u << otherFoe : state == 3 ? (1u << ally | 1u << otherFoe) : 0;
                gBattleMons[otherFoe].hp = state == 4 ? 0 : 1000;
                for (u32 m = 0; m < 4; m++)
                {
                    struct DamageContext ctx = {0};
                    struct AiCalcValues ai = {0};
                    ctx.battlerAtk = actor;
                    ctx.battlerDef = foe;
                    ctx.move = ctx.chosenMove = ctx.baseMove = sMoves[m];
                    ctx.moveType = GetMoveType(sMoves[m]);
                    ctx.typeEffectivenessModifier = UQ_4_12(1.0);
                    ai.move = sMoves[m];
                    for (enum BattlerId b = 0; b < 4; b++)
                    {
                        SetBattlerAiData(b, gAiLogicData);
                        ctx.abilities[b] = ABILITY_SHADOW_TAG;
                    }
                    u32 expected;
                    if (gen == GEN_3)
                        expected = m < 2 && state != 2 && state != 3 ? sGen3Spread[m] : sSingle[m];
                    else
                        expected = state == 3 || (m < 2 && (state == 2 || state == 4)) ? sSingle[m] : sModernSpread[m];
                    EXPECT_EQ(CalculateMoveDamageVars(&ctx), expected);
                    EXPECT_EQ(AI_CalcDamage(&ai, actor, foe).maximum, expected);
                    if (m >= 2 && state != 1 && state != 3)
                    {
                        ctx.battlerDef = ally;
                        expected = gen == GEN_3 ? sSingle[m] : sModernSpread[m];
                        EXPECT_EQ(CalculateMoveDamageVars(&ctx), expected);
                        EXPECT_EQ(AI_CalcDamage(&ai, actor, ally).maximum, expected);
                    }
                }
            }
            gBattleMons[otherFoe].hp = 1000;
            gAbsentBattlerFlags = savedAbsent;
            AI_SetActiveCalcContext(previous);
        }
    }
}

DOUBLE_BATTLE_TEST("Spread restoration: rounding at zero base term", s16 damage)
{
    enum Move move;
    u32 gen;
    PARAMETRIZE { move = MOVE_ROCK_SLIDE; gen = GEN_3; }
    PARAMETRIZE { move = MOVE_SURF; gen = GEN_3; }
    PARAMETRIZE { move = MOVE_ROCK_SLIDE; gen = GEN_4; }
    PARAMETRIZE { move = MOVE_SURF; gen = GEN_4; }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(1); SpAttack(1); }
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(65535); SpDefense(65535); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(65535); SpDefense(65535); }
    } WHEN {
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } SCENE {
        HP_BAR(opponentLeft, captureDamage: &results[i].damage);
    } THEN {
        EXPECT_EQ(results[i].damage, gen == GEN_4 ? 1 : move == MOVE_SURF ? 2 : 3);
    }
}

DOUBLE_BATTLE_TEST("Spread restoration: Water Absorb Levitate and Ghost immunity")
{
    enum Move move;
    enum Ability ability;
    enum Species species;
    u32 expected, gen;
    for (u32 g = GEN_3; g <= GEN_4; g++)
    {
        PARAMETRIZE { move = MOVE_SURF; ability = ABILITY_WATER_ABSORB; species = SPECIES_WOBBUFFET; expected = g == GEN_3 ? 22 : 32; gen = g; }
        PARAMETRIZE { move = MOVE_EARTHQUAKE; ability = ABILITY_LEVITATE; species = SPECIES_WOBBUFFET; expected = g == GEN_3 ? 46 : 34; gen = g; }
        PARAMETRIZE { move = MOVE_EXPLOSION; ability = ABILITY_LEVITATE; species = SPECIES_GASTLY; expected = g == GEN_3 ? 222 : 166; gen = g; }
    }
    GIVEN {
        WITH_CONFIG(B_MULTIPLE_TARGETS_DMG, gen);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); Attack(100); SpAttack(100); }
        PLAYER(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); }
        OPPONENT(SPECIES_WOBBUFFET) { HP(1000); MaxHP(1000); Defense(100); SpDefense(100); }
        OPPONENT(species) { HP(1000); MaxHP(1000); Ability(ability); }
    } WHEN {
        TURN { MOVE(playerLeft, move, WITH_RNG(RNG_DAMAGE_MODIFIER, 0)); }
    } THEN {
        EXPECT_EQ(opponentLeft->hp, 1000 - expected);
        EXPECT_EQ(opponentRight->hp, 1000);
    }
}
