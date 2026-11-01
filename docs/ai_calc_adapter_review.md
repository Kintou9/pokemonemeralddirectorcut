# Phase 2E-3D.2C: simulated turn-order parity resolved

The narrow turn-order correction is complete. The Phase 2E-3D.2B report below
is retained as history; its failing-test status is superseded here. Broader
adapter and cache-ownership work has not continued.

## Cause and correction

The original two-variant test reproduced `EXPECT_EQ(1, -1)` before editing.
Raw Speed remains 100. A one-stage reduction makes the original calculator
apply the stage ratio and integer division to produce 66. The effective cache
still contains 100 because candidate setup fills it before simulated stages
are applied. Snapshot validation correctly matches that cache; it cannot make
the cache equivalent to a freshly calculated value.

`AI_GetWhichBattlerFasterOrTies` now delegates to `AI_CompareTurnOrder` in
`battle_ai_calc_context.c`. This small AI-only adapter:

1. Calls the original `GetWhichBattlerFasterOrTies` directly if context is null
   or story policy is inactive.
2. Otherwise reads validated **raw** Speed through `AI_GetContextRawSpeed`.
3. Runs the existing `GetBattlerTotalSpeedStatWithBase` for each battler with
   the same supplied abilities/items and current simulated stages, status,
   weather, field and other modifiers as the original calculator.
4. Applies the original chosen-priority handling and shared order comparator.

It does not change context selection, write snapshots, refresh `speedStats`,
copy battle state, roll RNG, or modify any live battle calculation. The existing
raw/effective snapshot guards and `AI_WhoStrikesFirst` cached-Speed semantics
remain unchanged. No trainer data, configuration or other adapter paths changed.

## Tests

The original test now invokes the production adapter directly, rather than
mirroring its former implementation. All original assertions remain: cached
Speed 100, fresh Speed 100/66, expected original order, and equal adapter order.
The fixture explicitly activates the context policy so recorded-battle policy
exclusion cannot bypass the corrected path.

Additional deterministic fixtures compare the production adapter to the
original calculator with equivalent state. They cover:

- Unmodified Speed and ties; repeated stages 6, 5, 8, 4, 6.
- Gen 3 paralysis; Swift Swim in rain; Chlorophyll in sun; Macho Brace.
- Quick Claw held but inactive, either battler activating, both activating,
  and the original unresolved-tie result. The engine stores Quick Claw
  activation in `usedCustapBerry`; the fixture uses that same flag.
- Chosen priorities considered and ignored; null and policy-inactive fallback.
- No Trick Room, asserted explicitly without enabling unsupported mechanics.
- Same-species/personality candidate substitution and restoration, preserving
  the original context snapshot and cache.
- Singles, doubles and injected Space Center ownership for Maxie and Tabitha;
  Steven's ally context remains policy-inactive.

The modifier probe disables interrupts only inside the test helper to exclude
VBlank's `AdvanceRandom`, then asserts no RNG change and restores interrupts.
An initial non-atomic RNG assertion was affected by VBlank in doubles; all
order comparisons already matched. This fixture correction did not change
production code or expected outcomes.

Commands use `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro`:

| Command | Result |
| --- | --- |
| `make check TESTS='AI calculation adapter parity' -j4` | 3/3 passed (6 parameter variants, modifier matrices within variants) |
| `make check TESTS='AI calculation context parity' -j4` | 3/3 passed |
| `make check TESTS='Story AI' -j4` | 29/29 passed |
| `make check TESTS='Frontier AI decision baseline' -j4` | 2/2 passed |
| `make check TESTS='Story public-info' -j4` | 5/5 passed, including all 92 IDs |
| `make check TESTS='Quick Claw' -j4` | 1/1 actual-battle activation test passed |
| `git diff --check` | Passed |
| `make -j4` | Passed |

No remaining failure in the executed suites. Unrelated configuration
`ASSUME failed` messages occur during filtered discovery; the runner totals
above pass. No compiler warnings; test/normal linkers retain the existing
RWX LOAD-segment warning.

| Memory | Before | After | Delta |
| --- | ---: | ---: | ---: |
| EWRAM | 226,420 B | 226,420 B | 0 B |
| IWRAM | 28,396 B | 28,396 B | 0 B |
| ROM | 26,725,452 B | 26,725,644 B | +192 B |

Changed files this phase: `include/battle_ai_calc_context.h`,
`src/battle_ai_calc_context.c`, `src/battle_ai_main.c`,
`test/battle/ai/calc_adapter_parity.c`, and this report.

Remaining scope limits: ownership fixtures do not execute full Space Center
scripts or Factory generation. Per-turn damage/prediction caches remain shared
and outside the per-decision context. `SetBattlerAiData` and damage fallback
still use original live calculations; party-Speed and species/IV readers remain
unchanged. This fix deliberately preserves existing cached readers instead of
globally refreshing them. No complete adapter, whole-game equivalence or cycle
benchmark claim is made. No estimates, filtering, commit or push.

The git-status inventory at the end of the historical report remains current:
all five changed files were already uncommitted, and no other file was changed.

---

# Phase 2E-3D.2B: historical turn-order calculation parity failure

The adapter is not complete. No production source changed in this phase.
The Phase 2E-3D.2A snapshot validation remains intact. Work stopped under the
requested condition to report any unexpected turn-order calculation difference
before further architectural changes.

## Failure reproduced

`test/battle/ai/calc_adapter_parity.c` uses a fixed RNG seed and two parameter
variants: unchanged Speed stage, then a simulated one-stage reduction.

| Input/result | Unchanged stage | Stage reduced by one |
| --- | ---: | ---: |
| Actor raw Speed | 100 | 100 |
| Opponent Speed | 75 | 75 |
| Original effective-Speed cache | 100 | 100 |
| Recalculated actor Speed | 100 | 66 |
| Original comparator result | 1 (faster) | -1 (slower) |
| Active-context calculation result | 1 (faster) | 1 (faster) |

The second variant fails at `EXPECT_EQ(contextOrder, originalOrder)` with
`EXPECT_EQ(1, -1)`. All fixture Speed and original-order assertions before that
comparison pass. State is restored before assertions. No live raw Speed is
overwritten by this test; only the simulated Speed stage changes.

The test mirrors the calculation in the private
`AI_GetWhichBattlerFasterOrTies` active-context branch, invoking the same public
comparator and context accessors with ignored move priorities. It does not
invoke that private helper directly or demonstrate a changed final AI action.
It establishes a calculation-boundary mismatch, not an end-to-end claim.

## Exact dependency

- `src/battle_ai_main.c:3089`: with no active context, the helper calls
  `GetWhichBattlerFasterOrTies`, which recalculates effective Speed using
  current stat stages and the supplied AI abilities/hold effects.
- `src/battle_ai_main.c:3100`: with an active context, it instead supplies
  `AI_GetEffectiveSpeed` values to `GetWhichBattlerFasterArgs`.
- `src/battle_ai_calc_context.c:62`: effective snapshot validation compares
  against `gAiLogicData->speedStats`. It correctly preserves parity with that
  cache, but does not promise parity with a fresh effective-Speed calculation.
- `src/battle_ai_switch.c:82`: candidate simulation populates the cache via
  `SetBattlerAiData` before applying status, volatiles and switch-in stat stages.
- `src/battle_ai_switch.c:95` and `:2867`: later stages include a Sticky Web
  Speed reduction, without refreshing the effective-Speed cache at that point.

The previous raw-Speed identity-collision fix is not the cause and was not
reverted. Cached effective Speed and freshly calculated effective Speed have
different existing semantics. Globally refreshing the cache could alter
`AI_WhoStrikesFirst` and other consumers that originally used cached values.
A future correction should preserve those cache readers while giving the
originally recalculating helper an explicit recalculation path using context
raw Speed and its supplied abilities/hold effects. That correction was not
implemented after the stop condition triggered.

## Reviewed integration and unresolved boundaries

| Consumer | Current state |
| --- | --- |
| Speed Swap scoring | Context raw-Speed accessor |
| Tailwind, paralysis and field-status scoring | Context effective-cache accessor |
| `AI_WhoStrikesFirst`, move comparisons, switch ranking using it | Context accessor preserving the original cached Speed semantics |
| `AI_GetWhichBattlerFasterOrTies` | Failing cached-versus-recalculated boundary above |
| Encore reserve prediction | Party Speed via `GetMonData` wrapper, active raw Speed via context |
| Electro Ball / Gyro Ball | Damage helper recalculates effective Speed from context raw Speed with damage-specific ability/hold-effect inputs when context exists |
| Damage modifier ordering (`GetOtherModifiers`) | Context raw Speed on the AI path; original live reads otherwise |
| `SetBattlerAiData` | Original live effective-Speed calculation into shared `speedStats` |
| Per-turn damage preparation | Shared cache filled outside per-decision context |
| Player move/switch prediction | Runs during per-turn setup outside per-acting-trainer context |
| Candidate simulation | Existing battle-mon substitution and AI-cache save/restore; not newly isolated |
| Species base-Speed scoring and Speed-IV inputs | Unchanged species/mechanics inputs, distinct from current battler Speed |

`AI_CalcDamage` attaches the active context to `DamageContext`. The shared
damage helpers use it only for AI calculations; live battle calculation stays
on its original path. No new damage tests or full live-battle equivalence proof
were completed after finding the turn-order discrepancy.

The context lifecycle is unchanged: `ComputeAiBattlerDecisions` builds a stack
context, selects it for switching and move scoring, and restores the prior
pointer on exit. The corrected accessors reject snapshots differing from their
original sources. There is no per-actor copy or ownership association for the
shared per-turn `simulatedDmg`/prediction results yet. Cache ownership still
needs work for damage, resist-berry flags, Dragon Darts targeting flags,
switch candidates, predicted moves, scores and selected actions/targets.
Recomputation must preserve existing RNG consumption and order.

Existing context tests cover both Maxie/Tabitha owner orders and Steven's
excluded ally position using injected ownership fixtures. Policy tests reject
unknown trainer IDs/unsupported ownership. These passing tests do not prove
whole Space Center decision/cache isolation; that remains unresolved.

## Verification performed

All make commands used `DEVKITARM=/opt/devkitpro/devkitARM` and
`DEVKITPRO=/opt/devkitpro`.

| Command | Result |
| --- | --- |
| `make check TESTS='AI calculation context parity' -j4` | 3/3 passed |
| `make check TESTS='Story AI' -j4` | 29/29 passed |
| `make check TESTS='Frontier AI decision baseline' -j4` | 2/2 passed |
| `make check TESTS='Story public-info' -j4` | 5/5 passed, including the 92-ID guard |
| `make check TESTS='AI calculation adapter parity' -j4` | 1 test failed, second parameter variant |
| `git diff --check` | Passed |
| `make -j4` | Passed |

No existing assertion or outcome was changed. Filtered test discovery emits
unrelated configuration `ASSUME failed` diagnostics; suite totals above are
the runner's reported results.

Normal ROM: EWRAM **226,420 B**, IWRAM **28,396 B**, ROM **26,725,452 B**,
unchanged from Phase 2E-3D.2A. No new compiler warnings; the linker retains its
RWX LOAD-segment warning. No production runtime or memory cost was added.

Only added files in this phase:

- `test/battle/ai/calc_adapter_parity.c`
- `docs/ai_calc_adapter_review.md`

All previous uncommitted files, trainer data, flags, mechanics, Debug Mode and
backup files were preserved. No estimates, filtering, commit or push.

Final `git status --short`:

```text
 M include/battle.h
 M include/battle_main.h
 M include/battle_util.h
 M include/config/debug.h
 M src/battle_ai_field_statuses.c
 M src/battle_ai_main.c
 M src/battle_ai_util.c
 M src/battle_main.c
 M src/battle_util.c
?? docs/ai_calc_adapter_review.md
?? docs/ai_calc_context_review.md
?? docs/ai_fairness_repro_tests.md
?? include/battle_ai_calc_context.h
?? include/battle_ai_story_policy.h
?? src/battle_ai_calc_context.c
?? src/battle_ai_story_policy.c
?? src/data/trainers.party.backup
?? test/battle/ai/calc_adapter_parity.c
?? test/battle/ai/calc_context_parity.c
?? test/battle/ai/story_decision_hidden_info.c
?? test/battle/ai/story_hidden_information.c
?? test/battle/ai/story_public_info_policy.c
```
