# Phase 2E-3D.2A: parity regression resolved

The narrow follow-up fix is complete. The earlier audit below is retained as
historical evidence; its failing-test status is superseded by this section.
The broader calculation-context/damage-adapter implementation remains incomplete.

## Demonstrated cause and correction

The original test was rerun unchanged and failed with `EXPECT_EQ(50, 100)`.
`AI_InitCalcContext` snapshots active Speed 50. `InitializeSwitchinCandidate`
substitutes a candidate, updates `speedStats`, and then restores the original
party index while leaving the candidate in `gBattleMons` for scoring. Full mon
and AI-cache restoration happens later in the enclosing switch evaluation.
For matching species/personality, all three accessor identity checks therefore
pass despite candidate Speed 100. The accessor returns its stale snapshot.
This is stale snapshot reuse caused by a simulation-state identity collision;
neither candidate Speed calculation nor the existing restoration order was changed.

Only `src/battle_ai_calc_context.c` changed in production:

```c
// Additional raw snapshot invalidation condition:
|| context->rawSpeed[battler] != gBattleMons[battler].speed
// Additional effective snapshot invalidation condition:
|| context->effectiveSpeed[battler] != gAiLogicData->speedStats[battler]
```

A mismatched snapshot uses the existing original-path fallback. No context is
mutated or replaced, so acting battler/trainer/policy ownership is preserved.
Validation applies regardless of policy activation and catches effective-only
changes as well as raw-only changes. Restored states and rebuilt contexts also
return the original values. This is explicitly an actual-value-phase fix;
estimated values would need a different, explicitly scoped design.

Context creation remains per `ComputeAiBattlerDecisions` invocation, with the
previous active pointer restored on exit. Per-turn preparation and player
prediction still run outside that context scope. This patch neither broadens
that lifecycle nor changes shared caches, switch simulation, live damage,
live Speed calculation, or turn order.

## Follow-up verification

All commands use `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro`.

| Command | Result |
| --- | --- |
| `make check TESTS='AI calculation context parity' -j4` | 3/3 tests passed (8 parameter variants) |
| `make check TESTS='Story AI' -j4` | 29/29 passed |
| `make check TESTS='Frontier AI decision baseline' -j4` | 2/2 passed |
| `make check TESTS='Story public-info' -j4` | 5/5 passed, including the 92-ID guard |
| `git diff --check` | Passed |
| `make -j4` | Passed |

The original test and expected Speed 100 are preserved. New cases exercise
different party slots with identical identity, three repeated candidate states,
restoration, context rebuilding, raw-only/effective-only changes, null-context
fallback, singles over two turns, doubles, both Maxie/Tabitha owner orders,
Steven's excluded ally position, unrelated trainers, Tower and Factory contexts.
Nested context selection restores the caller's pointer and preserves actor IDs.
The 31 existing AI tests and five policy tests have no assertion changes.

The ownership tests inject trainer/battle flags after a deterministic doubles
battle, then exercise context construction and simulated-state accessors. They
do not run Space Center scripts, Factory team generation, or full multi-battle
decision flows. No remaining parity failure was observed in the executed suite;
this is not a claim of full damage-adapter or whole-game equivalence.

Normal build memory compared with the pre-fix workspace:

| Region | Before | After | Delta |
| --- | ---: | ---: | ---: |
| EWRAM | 226,420 B | 226,420 B | 0 B |
| IWRAM | 28,396 B | 28,396 B | 0 B |
| ROM | 26,725,580 B | 26,725,452 B | -128 B |

No compiler warnings. Normal and test linkers retain the RWX LOAD-segment
warning. Unrelated configuration `ASSUME failed` diagnostics appear during
filtered test discovery; reported suite totals above pass. No new allocation,
state copy, RNG call or Speed recomputation was added in production. Disassembly
shows the effective accessor optimized to the original cache read. Cycle-level
overhead was not benchmarked.

Remaining unisolated dependencies are unchanged: `SetBattlerAiData` obtains
effective Speed from the live calculator; snapshot construction and fallback
use live raw Speed / the original effective cache; party-Speed reads use
`GetMonData`; damage without an active context uses live inputs. Shared
per-turn damage/prediction cache ownership and the cached-versus-recomputed
turn-order helper distinction remain broader audit items, not fixes in this
phase. Species base-Speed and Speed-IV consumers also remain unchanged.

Files modified in this follow-up: `src/battle_ai_calc_context.c`,
`test/battle/ai/calc_context_parity.c`, and this report. All other pre-existing
work, Debug Mode configuration, parties and backup files were preserved.
No estimates, public-information filtering, commit or push were introduced.

---

# Phase 2E-3D.2: historical pre-edit parity failure

The workspace already contained an uncommitted partial calculation-context
implementation when this task began. No production source was changed in this
session. Existing work, including Debug Mode configuration, was preserved.
Implementation is **not complete or approved as behavior preserving**.

## Critical discrepancy

`test/battle/ai/calc_context_parity.c` reproduces the candidate-substitution
boundary used by `InitializeSwitchinCandidate`:

1. Capture an actor context with active raw Speed 50.
2. Substitute a reserve Pokémon with Speed 100 and refresh its effective-Speed
   cache using the original calculator.
3. Give the candidate the same species/personality as the active Pokémon.
   Personality is not guaranteed to be unique.
4. Restore the original party index, as the existing switch simulation does
   before scoring the candidate.
5. Compare the context accessor with the original direct read.

Result: `EXPECT_EQ(50, 100)` fails for raw Speed. The context's identity check
mistakes the candidate for the original occupant. The effective-Speed branch
is present but the runner stops this test at the first failed parameter;
it is not reported as separately verified. This is an input-parity failure,
not proof of a particular changed final move or switch in a full battle.

Per the requested stop condition, production implementation stopped here.
Candidate ownership needs an explicit simulation scope or equivalent reliable
invalidation; species/personality/temporarily restored party index is insufficient.

## Existing architecture inspected

- `ComputeAiBattlerDecisions` creates a stack context and restores the previous
  active pointer on exit. It records actor, opposing trainer ID, policy status,
  raw Speed and cached effective Speed. Opponent-right selects opponent B in
  two-opponent battles. Steven's player-side position cannot activate policy.
- Shared per-turn damage preparation and player prediction occur in
  `SetAiLogicDataForTurn`, outside that decision scope. Actor-specific cache
  isolation is not yet established by the existing partial implementation.
- `AI_WhoStrikesFirst`, Speed Swap, Tailwind, paralysis scoring, field-status
  scoring and the Encore reserve check use the existing context accessors.
- `AI_GetWhichBattlerFasterOrTies` now supplies cached effective Speed when a
  context exists; the original path recalculated it. Equivalence across
  simulation changes still needs proof.
- `AI_CalcDamage` attaches the active context to `DamageContext`.
  `GetOtherModifiers` consumes raw Speed to choose modifier order, so stale
  raw Speed can affect rounding. Electro Ball and Gyro Ball derive effective
  Speed from raw inputs with damage-context abilities and hold effects.
  Reusing the general cached effective Speed there would lose those inputs.
- The damage helpers select context inputs only for AI calculations with a
  non-null context. Live battle callers use the original Speed calculator.
  This design separation was inspected; full runtime battle parity was not
  established in this session.
- Existing switch simulation copies/substitutes battle Pokémon and saves and
  restores AI logic. Existing known-information simulation also substitutes
  abilities, items, moves and types. Those dependencies were not changed.

Caches requiring explicit ownership review include `speedStats`, `simulatedDmg`,
`resistBerryAffected`, `dragonDartsHitsBothTarget`, `mostSuitableMonId`,
`monToSwitchInId`, `shouldSwitch`, `predictedMove`, chosen move/target state and
move scores. Damage effectiveness/accuracy accompany the damage-cache lifecycle.
Recomputing randomized damage or predictions must preserve RNG consumption.

Remaining original-Speed inputs include `SetBattlerAiData` through
`GetBattlerTotalSpeedStat`, context initialization and fallback reads of
`gBattleMons[].speed`/`speedStats`, party Speed through `GetMonData`, and
damage fallback calculations without an active context. Species base-Speed
heuristics and Hidden Power Speed-IV inputs also remain; they are distinct
from current battler Speed. No estimates or masking were introduced.

## Verification

Commands used `DEVKITARM=/opt/devkitpro/devkitARM` and
`DEVKITPRO=/opt/devkitpro`:

| Command | Result |
| --- | --- |
| `make check TESTS='Story AI' -j4` | 29/29 passed |
| `make check TESTS='Frontier AI decision baseline' -j4` | 2/2 passed |
| `make check TESTS='Story public-info' -j4` | 5/5 passed, including all 92 IDs, other-ID exclusions and Space Center ownership |
| `make check TESTS='AI calculation context parity' -j4` | Failed: raw Speed 50 versus original 100 |
| `make -j4` | Passed |
| `git diff --check` | Passed |

The 29 story tests and two Frontier tests account for the existing 31 AI
regression tests. None of their assertions or expectations was edited.
The initial test link failed because four pre-existing object files were empty;
they were rebuilt with `make -W` for battle_end_turn.c, battle_frontier.c,
battle_factory_screen.c and battle_factory.c. No facility source was modified.
Test output also contains unrelated configuration `ASSUME failed` diagnostics;
the successful filtered runs report the totals above.

The normal ROM build reports EWRAM 226,420 B / 256 KiB, IWRAM 28,396 B / 32 KiB,
ROM 26,725,580 B / 32 MiB. Linker warning: a LOAD segment has RWX permissions.
The map assigns 4 IWRAM bytes to the existing context module's static pointer.
No production code was added in this session, hence this session adds no
production memory or calculation overhead. The pre-existing partial refactor's
total memory delta and cycle cost were **not measured against a clean baseline**.

Coverage gaps: full move/switch/target parity against the original implementation;
damage parity and live turn-order parity; context lifetime through turns,
fainting and replacement switches; actual multi-battle actor isolation;
Factory behavior; and runtime overhead. Policy unit tests do not establish
runtime actor isolation. No complete-equivalence claim is made.

## Files added in this session

- `test/battle/ai/calc_context_parity.c`
- `docs/ai_calc_context_review.md`

No commit or push was performed.
