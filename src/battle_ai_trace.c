#include "global.h"
#include "battle_ai_trace.h"
#if TESTING
#include "battle_ai_calc_context.h"

EWRAM_DATA struct AiTraceLog gAiTraceLog = {0};

static u32 StateFingerprint(const rng_value_t *state)
{
    return state->a ^ (state->b * 33) ^ (state->c * 65599) ^ state->ctr;
}

void AITraceStart(u32 mask)
{
    memset(&gAiTraceLog, 0, sizeof(gAiTraceLog));
    gAiTraceLog.mask = mask;
    gAiTraceLog.initialRng = gRngValue;
    gAiTraceLog.initialRng2 = gRng2Value;
    gAiTraceLog.enabled = TRUE;
}

void AITraceStop(void)
{
    gAiTraceLog.enabled = FALSE;
    gAiTraceLog.finalRng = gRngValue;
    gAiTraceLog.finalRng2 = gRng2Value;
}

void AITraceValue(u32 kind, u32 site, u32 a, u32 b, u32 c, const void *before, const void *after, u32 size)
{
    struct AiTraceEvent *event;
    const struct AiCalcContext *context;
    if (!gAiTraceLog.enabled || !(gAiTraceLog.mask & (1u << kind)))
        return;
    if (gAiTraceLog.count == AI_TRACE_CAPACITY)
    {
        gAiTraceLog.overflow = TRUE;
        return;
    }
    event = &gAiTraceLog.events[gAiTraceLog.count++];
    event->sequence = gAiTraceLog.count;
    event->site = site;
    event->kind = kind;
    event->a = a;
    event->b = b;
    event->c = c;
    event->simulation = gAiTraceLog.simulation;
    event->depth = gAiTraceLog.depth;
    context = AI_GetActiveCalcContext();
    event->actor = context == NULL ? 0xFF : context->actingBattler;
    event->trainer = context == NULL ? 0xFFFF : context->actingTrainerId;
    event->policy = context != NULL && context->storyPolicyActive;
    if (size > sizeof(event->before))
    {
        gAiTraceLog.overflow = TRUE;
        size = sizeof(event->before);
    }
    if (before != NULL)
        memcpy(event->before, before, size);
    if (after != NULL)
        memcpy(event->after, after, size);
    event->rngBefore = event->rngAfter = StateFingerprint(&gRngValue);
}

void AITracePoint(u32 kind, u32 site, u32 a, u32 b, u32 c)
{
    if (!gAiTraceLog.enabled)
        return;
    if (kind == AI_TRACE_ENTER)
    {
        if (gAiTraceLog.depth == ARRAY_COUNT(gAiTraceLog.parents))
        {
            gAiTraceLog.overflow = TRUE;
            return;
        }
        gAiTraceLog.parents[gAiTraceLog.depth] = gAiTraceLog.simulation;
        gAiTraceLog.simulation = ++gAiTraceLog.nextSimulation;
        gAiTraceLog.depth++;
    }
    AITraceValue(kind, site, a, b, c, NULL, NULL, 0);
    if (kind == AI_TRACE_LEAVE && gAiTraceLog.depth)
        gAiTraceLog.simulation = gAiTraceLog.parents[--gAiTraceLog.depth];
}

void AITraceRead(u32 kind, u32 site, u32 a, u32 b, u32 c, const void *value, u32 size)
{
    u32 count = gAiTraceLog.count;
    AITraceValue(AI_TRACE_READ, site, a, b, c, value, value, size);
    if (gAiTraceLog.count != count)
        gAiTraceLog.events[count].cacheKind = kind;
}

void AITraceRng(u32 kind, void *site, u32 tag, u32 lo, u32 hi, u32 result, const rng_value_t *before)
{
    u32 domain[2] = {lo, hi};
    u32 sampled[2] = {result, tag};
    u32 count = gAiTraceLog.count;
    AITraceValue(kind, (u32)site, 0, 0, 0, domain, sampled, sizeof(domain));
    if (gAiTraceLog.count != count)
    {
        gAiTraceLog.events[count].rngBefore = StateFingerprint(before);
        if (tag == 0xFFFFFFFE)
            gAiTraceLog.events[count].rngAfter = StateFingerprint(&gRng2Value);
    }
}

static void TraceCacheCopy(const struct AiLogicData *current, const struct AiLogicData *saved)
{
    const u32 zero[2] = {0};
    if (!gAiTraceLog.enabled)
        return;
    for (u32 a = 0; a < MAX_BATTLERS_COUNT; a++)
    {
#define RESTORE_VALUE(field, kind, b, c) do { \
        AITraceValue(kind, __LINE__, a, b, c, &current->field, saved == NULL ? (const void *)zero : &saved->field, sizeof(current->field)); \
    } while (0)
        RESTORE_VALUE(predictedMove[a], AI_TRACE_PREDICTION, 0, 0);
        RESTORE_VALUE(mostSuitableMonId[a], AI_TRACE_CANDIDATE, 0, 0);
        RESTORE_VALUE(monToSwitchInId[a], AI_TRACE_SWITCH, 0, 0);
        for (u32 b = 0; b < MAX_BATTLERS_COUNT; b++)
            for (u32 c = 0; c < MAX_MON_MOVES; c++)
            {
                RESTORE_VALUE(simulatedDmg[a][b][c], AI_TRACE_DAMAGE, b, c);
                RESTORE_VALUE(effectiveness[a][b][c], AI_TRACE_EFFECTIVENESS, b, c);
                RESTORE_VALUE(moveAccuracy[a][b][c], AI_TRACE_ACCURACY, b, c);
                RESTORE_VALUE(resistBerryAffected[a][b][c], AI_TRACE_BERRY, b, c);
            }
#undef RESTORE_VALUE
    }
}
void AITraceRestore(const struct AiLogicData *saved)
{
    TraceCacheCopy(gAiLogicData, saved);
}

void AITraceReset(const struct AiLogicData *data)
{
    TraceCacheCopy(data, NULL);
}
#endif
