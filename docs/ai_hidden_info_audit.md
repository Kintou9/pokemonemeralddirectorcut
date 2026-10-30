# Phase 2A: Gym Leader AI compatibility and hidden-information audit

Audited 2026-10-28 against the installed working tree. Source references below
are repository-relative `file:line` locations at audit time. This is a static,
bounded call-path audit, not proof that every transitive helper is leak-free.
No shared AI implementation or global configuration was changed in Phase 2A.

## Scope and compatibility

Only the eight original Gym Leader `AI:` entries in `src/data/trainers.party`
are changed. Teams, healing inventories, rematches, story, encounters, global
mechanics, and Battle Frontier data are outside this phase.

| Trainer ID | Definition line | AI line | Approved final configuration |
|---|---:|---:|---|
| TRAINER_ROXANNE_1 | 4884 | 4892 | Basic Trainer / HP Aware / Smart Switching |
| TRAINER_BRAWLY_1 | 4921 | 4929 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO |
| TRAINER_WATTSON_1 | 4958 | 4966 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention |
| TRAINER_FLANNERY_1 | 5004 | 5012 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention |
| TRAINER_NORMAN_1 | 5041 | 5049 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention |
| TRAINER_WINONA_1 | 5087 | 5095 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention |
| TRAINER_TATE_AND_LIZA_1 | 5142 | 5150 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention / Double Battle |
| TRAINER_JUAN_1 | 5188 | 5196 | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention |

The corresponding `_2` through `_5` IDs are distinct rematch definitions and
are excluded. Before this phase, the first four leaders used Basic Trainer;
the last four also used Smart Switching and Smart Mon Choices. None of these
entries contained omniscience, assumptions, forced setup, or prediction flags.

All requested symbols exist in `include/constants/battle_ai.h`:
TRY_TO_2HKO:13, DOUBLE_BATTLE:15, HP_AWARE:16, SMART_SWITCHING:24,
SMART_MON_CHOICES:27, PP_STALL_PREVENTION:35, BASIC_TRAINER:49.
`tools/trainerproc/main.c:1666` normalizes names to uppercase with underscores;
`:1894` emits their AI_FLAG constants. The exact requested spellings map to
existing constants, including Try To 2HKO and PP Stall Prevention.

`GetAiFlags` (`src/battle_ai_main.c:260`) automatically adds DOUBLE_BATTLE
for doubles at :289 and SMART_MON_CHOICES for SMART_SWITCHING at :294.
Tate & Liza already have `Double Battle: Yes`; their explicit Double Battle AI
entry is supported and redundant, not a change to battle format.
`SMART_SWITCHING_OMNISCIENT` remains FALSE (`include/config/ai.h:131`).

## Information categories

- **A: Human-knowable**: revealed species, displayed level/gender, visible status,
  weather, screens, observed moves and outcomes, observed switching/fainting.
  Own team information is legitimate and is not a player-information leak.
- **B: Inferable**: a species' possible abilities, estimated Speed from observed
  turn order, or approximate HP from the visible bar. Estimates are not exact
  internal values. Randomly selecting a possible ability is an inference, not
  evidence that the actual ability was revealed.
- **C: Hidden-information peek**: exact private stats/HP/PP, unrevealed held
  items/abilities/moves, or unobserved reserve state. A read is classified even
  if its eventual influence on move choice has not been demonstrated.

## Per-flag implementation and information paths

All flags share per-turn data preparation in `SetBattlerAiData`
(`src/battle_ai_main.c:736`), simulation, and battle history. The C findings
below therefore cannot be isolated merely by disabling OMNISCIENT.

| Flag | Implementation | Player information consulted and classification |
|---|---|---|
| BASIC_TRAINER / CHECK_BAD_MOVE | `AI_CheckBadMove`, `src/battle_ai_main.c:1285` | Types, statuses, weather, last moves (A); inferred abilities (B); shared exact speed, HP, item IDs and move-limit data (C). Item-related effect branches consume raw item IDs at :2136, :2537. |
| BASIC_TRAINER / TRY_TO_FAINT | `AI_TryToFaint`, `src/battle_ai_main.c:3075` | Visible matchup and observed moves (A), ability inference (B); simulated damage and exact target HP for KO decisions (C). Uses `AI_GetDamage`, `src/battle_ai_util.c:111`, and KO helpers. |
| BASIC_TRAINER / CHECK_VIABILITY | `AI_CheckViability`, `src/battle_ai_main.c:5996`, effect scoring earlier in that file | Known last moves/status/screens/weather (A), possible abilities (B); shared speed/damage/HP/item inputs (C). Encore scoring :4722 uses lastUsedMove; weather scoring :4872 calls CalcWeatherScore. Raw-item consumers include :5095 and :5134. |
| HP_AWARE | `AI_HPAware`, `src/battle_ai_main.c:6259` | HP-bar condition is knowable approximately (A/B), but hpPercents is calculated from exact internal hp/maxHP (C). Its KO check also inherits exact-stat damage inputs (C). |
| SMART_SWITCHING | `ShouldSwitch`, `src/battle_ai_switch.c:1414`; `GetShouldSwitchMoveData`:1325 | Types, status, revealed moves and battle history (A/B); exact HP/stats through simulation, and direct player PP at :1338 (C). Unrevealed full moves are conditional on SMART_SWITCHING_OMNISCIENT at :1337, currently FALSE. |
| SMART_MON_CHOICES (implicit) | `GetBestMonIntegrated`, `src/battle_ai_switch.c:2211`; dispatch :2629 | Legitimate own reserve data; observed opponent moves/types (A), ability inference (B), shared exact-stat/HP simulation (C). Integrated selection is explicitly singles-only. |
| TRY_TO_2HKO | `AI_TryTo2HKO`, `src/battle_ai_main.c:6168` | Adds bonuses for calculated one-/two-hit KOs. Inherits exact HP and damage-stat reads (C), alongside known matchup information (A/B). Does not itself grant full move/item/ability knowledge. |
| PP_STALL_PREVENTION | `AI_CheckPpStall`, `src/battle_ai_main.c:6675` -> `PpStallReduction`:904 | Observed immunity-switch counters (A); reads current reserve HP and copies real reserve data for recorded stall candidates (C; details below). No omniscience guard on this path. |
| DOUBLE_BATTLE | `AI_DoubleBattle`, `src/battle_ai_main.c:3131` | Legitimate own partner moves, immunity and intended action; two visible opponents and field effects (A), inferred opponent abilities (B), shared HP/speed/damage inputs (C). No complete joint multi-turn search. |

## Confirmed Category C reads

1. **Exact Speed, without omniscience.** `SetBattlerAiData`,
   `src/battle_ai_main.c:747`, calls `GetBattlerTotalSpeedStat`.
   `src/battle_main.c:4375` starts from `gBattleMons[battler].speed`.
   This is actual private Speed, not an estimate from revealed turn order.

2. **Exact HP and maximum HP, without omniscience.** `GetHealthPercentage`,
   `src/battle_ai_util.c:426`, calculates from hp/maxHP; stored at
   `src/battle_ai_main.c:745`. KO checking additionally compares damage to
   actual HP (`src/battle_ai_util.c:2554`). A visible HP bar does not expose
   both exact values. Relevant to HP_AWARE, damage flags, switches and doubles.

3. **Exact offensive and defensive stats in damage simulation.**
   `AI_CalcDamage` (`src/battle_ai_util.c:868`) calls shared damage calculation.
   `CalcAttackStat` (`src/battle_util.c:6840`, reads at :6888/:6893) and
   `CalcDefenseStat` (:7127, reads :7136/:7137) use actual battle stats.
   `SetBattlerData` (`src/battle_ai_util.c:367`) masks unknown moves/items/
   abilities, but does not replace these stats with estimates. No omniscience
   requirement protects the stat reads. Applies to both damage dealt and
   damage received when a known player move is simulated.

4. **Unrevealed item IDs copied to AI state.** `SetBattlerAiData`,
   `src/battle_ai_main.c:742`, directly assigns `gBattleMons[battler].item` to
   `aiData->items[battler]`. This is separate from the filtered holdEffects.
   Confirmed consumers include `AI_CheckBadMove` at :2136 and item-effect
   viability branches at :5095/:5134. Whether a particular current Gym moveset
   reaches those consumers is move-dependent; copying the raw ID is confirmed.

5. **Actual ability exceptions and actual item absence.**
   `AI_DecideKnownAbilityForTurn`, `src/battle_ai_util.c:1761`, obtains the real
   ability at :1765; it returns trapping abilities directly at :1785 even
   without a recorded reveal. Null/suppressed and overwritten ability paths
   also use actual state. Some cases are observable, but a previously
   unrevealed trapping ability can be exposed before a player tries to flee.
   `AI_DecideHoldEffectForTurn` at :1812 checks actual ITEM_NONE before using
   recorded hold effects. Confirmed raw presence/absence read; this alone does
   not reveal every nonempty item's identity.

6. **Player move limitations and remaining PP.** `SetBattlerAiData`,
   `src/battle_ai_main.c:746`, calls CheckMoveLimitations on player battlers.
   `GetShouldSwitchMoveData`, `src/battle_ai_switch.c:1338`, directly checks
   `gBattleMons[opposingBattler].pp[moveIndex] > 0` even when moves came from
   history. This is actual PP/slot state, not a reconstructed observed count.
   Further auditing is needed to enumerate all CheckMoveLimitations consumers.

7. **Reserve presence/fainted state during initialization.**
   `Ai_InitPartyStruct`, `src/battle_ai_main.c:619`, counts actual parties at
   :628 and scans species/HP at :650-654 to set isFainted. These reads are not
   behind the full-party-knowledge flag. Opponent party size may be displayed
   in a normal trainer battle, but preexisting fainted reserve state is not
   necessarily revealed. Species storage, full moves, abilities and item
   storage in the subsequent branches are separately flag-gated.

8. **PP-stall reserve access.** `PpStallReduction`,
   `src/battle_ai_main.c:924`, checks actual HP for party slots with a nonzero
   observed stall counter, then :926 copies that real Pokemon into a temporary
   battle slot with PokemonToBattleMon. It recalculates blocking/immunity with
   ability/item helpers at :928-931. Confirmed hidden-state reads/copy; not an
   unconditional scan of every reserve's moves for optimal prediction.
   Whether every copied private field influences scoring, and whether helper
   history is correctly indexed for the simulated reserve, need further tracing.

## Conditional reads not enabled by these configurations

- `GetMovesArray`, `src/battle_ai_util.c:2559`, returns actual moves for aware
  battlers/partners or MOVE_OMNISCIENCE; otherwise observed usedMoves. Own-side
  awareness is legitimate. The proposed leaders have no move-omniscience flag.
- `Ai_InitPartyStruct`, `src/battle_ai_main.c:656-673`, gates reserve species,
  actual abilities/items/moves on the corresponding knowledge flags.
- `RecordMovesBasedOnStab` (:713) and `RecordStatusMoves` (:723) inspect actual
  moves when assumption flags activate. Those flags are excluded here.
- `src/battle_ai_switch.c:1337`, :2083 and :2113 can use actual player moves if
  SMART_SWITCHING_OMNISCIENT is TRUE; it is FALSE in this branch.
- Unknown abilities can be randomly selected from species possibilities
  (`src/battle_ai_util.c:1788-1804`): B, distinct from confirmed actual-ability
  exceptions. Weighted ability prediction is not enabled by this phase.

## Behavioral limitations and further tracing

- Weather scoring (`src/battle_ai_field_statuses.c:65`, :529) examines active
  allies and weather-relevant abilities/moves, not a full reserve plan.
- Encore uses observed lastUsedMove (`src/battle_ai_main.c:4722`); its score
  bonus does not guarantee selection. HP and 2HKO bonuses may compete with setup.
- Smart replacement selection falls back in doubles
  (`src/battle_ai_switch.c:2629-2632`). DOUBLE_BATTLE supports partner-aware
  scoring, not a guarantee of optimal coordinated play.
- Truant's existing switching conditions remain intact
  (`src/battle_ai_switch.c:422`). No special restriction is added for Norman.
- Additional dependencies requiring tracing include all status-duration and
  move-limit helpers, switch-in simulations, history recording/reveal timing,
  exact accuracy/damage modifiers, and all raw item/ability consumers. The
  shared engine supports later-generation mechanics; some branches are not
  reachable with current teams but must not be called universally safe.

Conclusion: the approved flags are supported and introduce no explicit
omniscience/prediction configuration. Nevertheless, existing Category C reads
remain. This phase does not certify human-information-only AI and does not fix
those reads. Any future fixes require separately approved, scoped work.

## Verification scope

Trainer verification compares against a pre-edit byte snapshot, requiring exactly
eight AI-line replacements and no other trainer-data changes. Existing modified
configuration files and the untracked trainer backup are hash-checked unchanged.
Compilation verifies flag-name resolution. Battle decisions, fairness, and
difficulty require subsequent in-game or targeted automated testing; a successful
build does not establish those properties. No Phase 2B/2C work is included.

Verification completed: the byte comparison passed for all trainer content except
the eight approved AI lines. Hashes of the three pre-existing modified config
files and `src/data/trainers.party.backup` remained unchanged. `git diff --check`
passed. `make -j4 DEVKITARM=/opt/devkitpro/devkitARM` completed successfully and
produced the 32 MiB ROM; the explicit DEVKITARM path locates the installed compiler.
The generated `src/data/trainers.h` contains the exact intended flag expressions.

The build log (`/tmp/phase2a-build.log`) contains one linker warning: the ELF has
a LOAD segment with RWX permissions. The same warning is present in the saved
prior-build log (`/tmp/phase2a-prior-build.log`). No new warning text or compiler
warnings were observed in this incremental build. The prior log is comparison
evidence, not a separately rebuilt clean pre-edit baseline. No commit or push was
performed. In-game AI behavior and information-use regression tests remain pending.
