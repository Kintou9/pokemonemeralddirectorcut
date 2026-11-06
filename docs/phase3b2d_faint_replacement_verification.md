# Phase 3B-2D: Generation 3 faint replacement

Base: `7680cb5973` on `emerald-enhanced-classic`. No commit or push.

## Implementation audit

Original Emerald references: [HandleFaintedMonActions and HandleAction_TryFinish](https://github.com/pret/pokeemerald/blob/master/src/battle_util.c), [faint/replacement scripts](https://github.com/pret/pokeemerald/blob/master/data/battle_scripts_1.s).

Original Emerald processes fainted battlers after an action, before advancing the queue. It handles experience first, then replacements. Its scripts preserve the Shift/Set choice and format exclusions, execute switch-in effects, and cancel actions belonging to replaced battlers. A spread move resolves its recipients before this action boundary. Hazard-fainted replacements require another replacement rather than resuming the remaining move queue with an empty slot.

The expansion retains this path in `HandleFaintedMonActions`. Two Gen 4+ gates defer replacements to turn end. `Cmd_end` enters `B_ACTION_TRY_FINISH`; `HandleAction_TryFinish` invokes the faint state machine before `HandleAction_ActionFinished` advances the queue. `BattleScriptExecute`/`RunBattleScriptCommands_PopCallbacksStack` suspend and restore execution around EXP and replacement scripts. `RunBattleScriptCommands` waits for controller completion. `UpdateSentMonFlags` cancels the replaced slot's queued action; singles additionally use `cancelallactions`. `switchinevents` handles hazards and abilities. EXP remains governed by `BattleTypeAllowsExp`, including Frontier exclusions and trainer/party ownership.

The two timing gates now use the existing `GetConfig(B_FAINT_SWITCH_IN)` accessor, enabling paired tests. Non-test builds still resolve the accessor to the configured constant. The already-existing config definition no longer carries its obsolete “use in tests” TODO.

Settings:

- `B_FAINT_SWITCH_IN = GEN_3` (changed).
- `B_RECALC_TURN_AFTER_ACTIONS = GEN_3` (unchanged).
- `B_MULTIPLE_TARGETS_DMG = GEN_3` (unchanged).
- `P_MEGA_EVOLUTIONS = FALSE` (unchanged; existing test-only compatibility override retained).

## Characterization and expected differences

Before production edits, eight characterization tests compiled and ran: one passed (final opponent/battle end) and seven failed at the expected immediate-replacement ordering assertions. This also exposed that the existing faint-timing test override was bypassed by the direct macro checks.

New tests cover immediate entry before slower actions; Intimidate, Drizzle, Drought, and Sand Stream; no inherited move or extra PP use; spread recipient completion; simultaneous opposing KOs with one/two reserves; Explosion; Destiny Bond; entry-hazard KO chains; final opponent/battle end; player/partner/opponent party ownership; real AI partner flow; normal trainer doubles and Battle Tower EXP/Shift exclusions.

Tate & Liza coverage uses Lunatone/Solrock and shared-party Claydol/Xatu reserves. Space Center coverage uses player/Steven-owned parties, two opposing trainers, and an AI partner battle. These are controlled battle-format fixtures, not overworld launches or edits to enhanced trainer teams.

The configuration matrix varies faint timing (Gen 3/4), turn ordering (Gen 3/8), and spread damage (Gen 3/4). An immediate Drizzle replacement activates rain before the ally's Surf; Gen 3 retains the old action queue while Gen 8 reorders for Swift Swim. Under delayed replacement, the opposing slot remains absent during Surf and rain begins afterward. Expected damage is 33/48 with immediate rain and two targets, versus 43 with delayed entry and one target. These are deliberate generation differences, not relaxed assertions.

## Verification

New faint-replacement tests: **11 passed, 34 parameter variants**, no failures or TODOs. No further gameplay correction was required: changing the default is sufficient for the verified scenarios. The accessor changes only make the existing setting testable; no battle script, controller, EXP formula, ability implementation, AI policy, or action-cancellation logic changed.

Commands used `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS="<filter>" -j4`.

| Filter | Passed | Failed | TODO |
| --- | ---: | ---: | ---: |
| `Gen3 turn order:` | 10 | 0 | 0 |
| `Shift style:` | 9 | 0 | 0 |
| `Spread restoration` | 8 | 0 | 0 |
| `Story AI` | 29 | 0 | 0 |
| `Frontier AI decision baseline` | 2 | 0 | 0 |
| `Story public-info` | 5 | 0 | 0 |
| `AI calculation context parity` | 3 | 0 | 0 |
| `AI calculation adapter parity` | 3 | 0 | 0 |
| `AI trace:` | 7 | 0 | 0 |
| `test/battle/multi_battle_whiteout.c` | 8 | 0 | 0 |
| `test/battle/switch_in_abilities.c` | 8 | 0 | 0 |
| `test/battle/battle_message.c` | 11 | 0 | 1 |
| `test/battle/exp.c` | 8 | 1 | 0 |
| `test/battle/move_effect/spikes.c` | 5 | 0 | 0 |
| `test/battle/move_effect/destiny_bond.c` | 6 | 0 | 14 |
| `test/battle/form_change/mega_evolution.c` | 13 | 1 | 0 |
| `test/battle/status1/paralysis.c` | 7 | 1 | 0 |

Combined selection including the new file: **153 passed, 3 existing failures, 15 existing TODOs**. No new failures or skips. Test registration emits the existing unrelated ASSUME diagnostics for Harvest, Pickup, and `special_attack_up_3`; this is not a full-suite pass claim.

Existing failures retained without weakening assertions:

- `test/battle/exp.c:217`: two-opponent EXP assertion expects 2,771 but receives 4,156. Reproduced identically in the unchanged `fc1d03dfcb` baseline with later-generation faint timing. The test omits the configured Gen 3 trainer EXP multiplier from its expectation; its file is unchanged between that baseline and this checkpoint.
- `test/battle/form_change/mega_evolution.c:106`: Mega Sableye ability-replacement test has the already documented unmatched opposing Wobbuffet Celebrate message. Test compatibility only; Classic Mega Evolution stays disabled.
- `test/battle/status1/paralysis.c:110`: the previously documented Thunder Wave avoided-attack message mismatch.

The latter two were already reproduced on `fc1d03dfcb` during Phase 3B-2C and fail identically in this rerun. No new replacement-timing or Shift/Set discrepancy remains. The nine Shift/Set tests retain saved Shift Yes/No and Set behavior and their battle-format exceptions.

`python3 dev_scripts/verify_classic_no_mega.py` passes: normal and release preprocessing each contain zero Mega routes/forms; test compatibility data retain 97 of each. The release/LTO ROM was not separately built.

`DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make -j4` succeeded. The only build warning was the existing linker warning: `has a LOAD segment with RWX permissions`. No compiler warnings were reported.

| Region | Used | Usage |
| --- | ---: | ---: |
| EWRAM | 226,416 B | 86.37% |
| IWRAM | 28,396 B | 86.66% |
| ROM | 25,034,036 B | 74.61% |

`git diff --check` passed. Checksums match for Debug Mode, the trainer backup, trainer parties, all AI source files, and Summary Screen settings. No Frontier data/scripts, story encounters, approved QoL settings, or Phase 4 plans were edited.

## Changed files and final Git status

- `include/config/battle.h`: select Gen 3 faint replacement.
- `include/constants/config_changes.h`: remove the now-obsolete test-coverage TODO for the existing config entry.
- `src/battle_util.c`: use the existing config accessor at the two replacement-delay gates.
- `test/battle/faint_replacement_gen3.c`: eleven focused deterministic tests with 34 parameter variants.
- `docs/phase3b2d_faint_replacement_verification.md`: audit, expected generation differences, and verification record.

Final `git status --short`:

```
 M include/config/battle.h
 M include/constants/config_changes.h
 M src/battle_util.c
?? docs/phase3b2d_faint_replacement_verification.md
?? src/data/trainers.party.backup
?? test/battle/faint_replacement_gen3.c
```

The trainer backup was already untracked and is untouched. Generated ROM/build outputs remain ignored. Nothing is staged, committed, or pushed; HEAD remains `7680cb5973`.


## In-game follow-up

Automated party selections use recorded controller inputs rather than navigating the graphical party menu. Manually verify Shift Yes/No/cancel and saved Set, simultaneous replacement menus, entry-hazard KO chains, EXP/level-up presentation, Tate & Liza, the Space Center partner encounter, and Frontier battle entry/exit. The fixtures do not cover every Frontier facility, scripted encounter, or link transport.
