#ifndef GUARD_BATTLE_AI_TRACE_H
#define GUARD_BATTLE_AI_TRACE_H

// Observation only. No ownership or evaluation policy lives here.
enum AiTraceKind { AI_TRACE_DAMAGE, AI_TRACE_EFFECTIVENESS, AI_TRACE_ACCURACY,
    AI_TRACE_BERRY, AI_TRACE_PREDICTION, AI_TRACE_CANDIDATE, AI_TRACE_SWITCH,
    AI_TRACE_ENTER, AI_TRACE_LEAVE, AI_TRACE_RESTORE, AI_TRACE_ACTION,
    AI_TRACE_RAW_RNG, AI_TRACE_TAGGED_RNG, AI_TRACE_READ, AI_TRACE_FINAL, AI_TRACE_SAVE, AI_TRACE_RESET };

#if TESTING
#include "random.h"
#define AI_TRACE_CAPACITY 192
struct AiTraceEvent
{
    u32 sequence, site;
    u32 before[2], after[2];
    u32 rngBefore, rngAfter;
    u16 trainer, simulation, c;
    u8 kind, actor, a, b, depth, cacheKind, policy;
};
struct AiTraceLog
{
    struct AiTraceEvent events[AI_TRACE_CAPACITY];
    rng_value_t initialRng, finalRng, initialRng2, finalRng2;
    u32 count, mask;
    u16 simulation, nextSimulation, parents[16];
    u8 depth;
    bool8 enabled, overflow;
};
extern struct AiTraceLog gAiTraceLog;
void AITraceStart(u32 mask);
void AITraceStop(void);
void AITraceValue(u32 kind, u32 site, u32 a, u32 b, u32 c, const void *before, const void *after, u32 size);
void AITracePoint(u32 kind, u32 site, u32 a, u32 b, u32 c);
void AITraceRng(u32 kind, void *site, u32 tag, u32 lo, u32 hi, u32 result, const rng_value_t *before);
void AITraceRead(u32 kind, u32 site, u32 a, u32 b, u32 c, const void *value, u32 size);
struct AiLogicData;
void AITraceRestore(const struct AiLogicData *saved);
void AITraceReset(const struct AiLogicData *data);
#define AI_TRACE_RESET_DATA(data) AITraceReset(data)
#define AI_TRACE_RESTORE_DATA(saved) AITraceRestore(saved)
#define AI_TRACE_POINT(kind, a, b, c) AITracePoint(kind, (u32)__builtin_return_address(0), a, b, c)
#define AI_TRACE_WRITE(expr, value, kind, a, b, c) do { \
    typeof(expr) _traceNew = (value); \
    typeof(expr) *_tracePtr = &(expr); \
    typeof(expr) _traceOld = *_tracePtr; \
    *_tracePtr = _traceNew; \
    AITraceValue(kind, __LINE__, a, b, c, &_traceOld, _tracePtr, sizeof(*_tracePtr)); \
} while (0)
#define AI_TRACE_READ_VALUE(expr, kind, a, b, c) ({ \
    typeof(expr) _traceValue = (expr); \
    AITraceRead(kind, __LINE__, a, b, c, &_traceValue, sizeof(_traceValue)); \
    _traceValue; })
#else
#define AI_TRACE_RESET_DATA(data) ((void)0)
#define AI_TRACE_RESTORE_DATA(saved) ((void)0)
#define AI_TRACE_POINT(kind, a, b, c) ((void)0)
#define AI_TRACE_WRITE(expr, value, kind, a, b, c) ((expr) = (value))
#define AI_TRACE_READ_VALUE(expr, kind, a, b, c) (expr)
#endif
#endif
