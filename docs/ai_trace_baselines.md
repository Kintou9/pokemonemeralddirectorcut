# Phase 2E-3D.2F — test-only baseline instrumentation

Stage 1 only. No cache ownership, replay, actor workspace, damage-input,
Speed-estimation, trainer-data, or battle-rule changes are introduced.

## Trace contract

`battle_ai_trace.h` erases hooks in non-TESTING builds. The test build has
a 192-event buffer (9,328 bytes including metadata). Overflow is sticky;
records never wrap. A test must reject overflow before comparing a transcript.
Masks allow bounded windows instead of retaining an entire battle's cache traffic.

Events retain selected-event sequence, site, kind, actor, trainer, policy flag,
simulation ID/depth, three indices, and up to eight bytes before/after a write.
Cache sites are source line numbers; RNG sites are caller addresses. Resolve
them against the exact instrumented sources/ELF, not a different build.
An absent active context is recorded as actor 0xFF/trainer 0xFFFF; tracing does
not infer an owner or change context selection. Simulation IDs describe observed
nesting, not new ownership metadata.

Scalar reads/writes in battle_ai_main/util/switch cover simulated damage,
effectiveness, accuracy, resist-Berry flags, predicted moves, most suitable
candidates, and switch-in IDs. Bulk AI-data reset/restore records each cell of
these seven caches, including equal-value writes. Save/restore markers also
surround the existing BattlePokemon backup path. No new battle-state copies
are made. Existing candidate substitution and restoration remain in place.

Entry/exit hooks cover AI damage evaluation, candidate initialization,
prediction setup, predicted-switch scoring, and acting-battler evaluation.
Decision records capture chosen move index/target; controller responses capture
committed response values and chosen party slots. Player controller responses
are also observable; they are not implicitly classified as opposing AI actions.

## RNG

Test-only wrappers observe Random32, Random2_32, and LOOP_RANDOM, and the test
runner's tagged uniform, uniform-except, weighted-array, and element-array
dispatchers. Original callback caller addresses are forwarded unchanged.
Each draw/dispatch is executed once. No RNG state is restored by tracing.

RNG records contain call site, tag/stream sentinel, domain, result, and before/
after state fingerprints. Full primary and secondary RNG states are captured
at trace start/stop. Uniform domains are lo/hi; weighted domains are n/sum;
element domains are element size/count. Raw domains describe generator output,
not a later modulo operation in the caller. Nested raw draws precede the tagged
completion event; counts must distinguish raw draws from tagged requests.

The battle harness overrides many tagged results. Deterministic transcripts
therefore establish harness parity, not complete production RNG scheduling.
A separate arithmetic test exercises the original assembly generator and
RandomUniformDefault against LocalRandom32 and checks exact raw count/order,
results, and full final state. This does not substitute for hardware timing tests.

## Baselines

Seven new test definitions cover:

- Production generator arithmetic and two exact raw operations.
- Explicit bounded-buffer overflow without prefix replacement.
- Trace enabled/disabled damage, RNG state, and move parity for Tackle,
  Gyro Ball, Electro Ball, Triple Kick, and Psywave at three Speed stages
  (30 parameter cases); includes cache clear/write/read assertions.
- Repeated candidate evaluation, prediction, nested entries, and restoration.
- Doubles final moves, targets, and RNG state with tracing enabled/disabled.
- Observed actor attribution for Maxie/Tabitha in both orders, an unrelated
  trainer, Tower and Factory flags, player-side ally exclusion, and null context.
- Repeated seeded evaluation with byte-identical bounded RNG transcripts,
  explicit counts, full initial/final primary state, and final move/target.

The transcript test starts after fixture setup: GIVEN declares RNGSeed but does
not apply it immediately. Its initial version began too early and correctly
failed the initial-state assertion; the boundary was corrected without removing
that assertion. Existing tests and their expectations are unchanged.

## Limits to carry forward

- Tera damage-array aliases and the debug-screen candidate read are not wrapped.
  These are not claimed as covered Gen 3 decision paths.
- Local scratch caches/arrays and every arbitrary BattlePokemon field are not
  individually traced. Bulk battle restoration has boundary markers, not a full
  state dump. Simulation-state equivalence is checked by focused parity tests.
- Weighted input vectors, element contents, and rejection predicates are not
  serialized. RNG seeding warm-up/local-generator operations are not draw events.
  Event fingerprints are not collision-free full-state proofs.
- Candidate/prediction coverage does not force every predicted-switch branch,
  every nested damage formula, or an active resist-Berry effect. The harness
  can exercise engine moves independently of story learnset availability.
- Space Center and facility tests construct ownership/flag contexts; they do
  not run scripts, facility team generation, or a complete Steven partner battle.
- Trace enabled/disabled comparisons and repeated transcripts are baselines for
  these scenarios, not an exhaustive pre-refactor action/RNG equivalence proof.
- No GBA hardware timing equivalence is claimed. Production instrumentation
  symbols are absent and linked RAM/ROM sizes are unchanged.

## Verification

Use DEVKITARM=/opt/devkitpro/devkitARM and DEVKITPRO=/opt/devkitpro.
Run make check -j4 with each prefix: AI trace:, AI calculation context parity,
AI calculation adapter parity, Story AI, Story public-info, Frontier AI decision
baseline, and Quick Claw. Then run git diff --check and make -j4.

Production linked usage: EWRAM 226,420 B; IWRAM 28,396 B; ROM 26,725,644 B.
The linker reports its existing RWX LOAD-segment warning. Test discovery also
prints unrelated ASSUME failures for excluded configurations; use the selected
suite's final totals to distinguish those from test failures.

Final results: 50 test definitions passed, zero failed (3 context, 3 adapter,
29 story AI, 5 policy, 2 Frontier baselines, 1 Quick Claw, 7 tracing).
`git diff --check` and `make -j4` passed. No compiler warnings appeared in the
final builds. No observed final-action or RNG parity differences remain in
the tested scenarios. The production ROM hash changed despite identical linked
sizes; byte-for-byte ROM identity is not claimed.

Files changed in this phase:

```
include/battle_ai_trace.h
src/battle_ai_trace.c
src/battle_ai_main.c
src/battle_ai_util.c
src/battle_ai_switch.c
src/battle_controllers.c
src/random.c
test/test_runner.c
test/battle/ai/trace_baselines.c
docs/ai_trace_baselines.md
```

Final `git status --short` (includes preserved work from earlier phases):

```
 M include/battle.h
 M include/battle_main.h
 M include/battle_util.h
 M include/config/debug.h
 M src/battle_ai_field_statuses.c
 M src/battle_ai_main.c
 M src/battle_ai_switch.c
 M src/battle_ai_util.c
 M src/battle_controllers.c
 M src/battle_main.c
 M src/battle_util.c
 M src/random.c
 M test/test_runner.c
?? docs/ai_calc_adapter_review.md
?? docs/ai_calc_context_review.md
?? docs/ai_fairness_repro_tests.md
?? docs/ai_trace_baselines.md
?? include/battle_ai_calc_context.h
?? include/battle_ai_story_policy.h
?? include/battle_ai_trace.h
?? src/battle_ai_calc_context.c
?? src/battle_ai_story_policy.c
?? src/battle_ai_trace.c
?? src/data/trainers.party.backup
?? test/battle/ai/calc_adapter_parity.c
?? test/battle/ai/calc_context_parity.c
?? test/battle/ai/story_decision_hidden_info.c
?? test/battle/ai/story_hidden_information.c
?? test/battle/ai/story_public_info_policy.c
?? test/battle/ai/trace_baselines.c
```

No commits or pushes were made. Stage 2 remains unimplemented.
