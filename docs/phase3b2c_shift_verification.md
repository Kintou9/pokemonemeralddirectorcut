# Phase 3B-2C and Shift/Set verification

Base: `f06dac6c2e`. Implementation and verification performed November 3–4, 2026, before the approved Git checkpoint.

**Classic Edition update:** Mega Evolution is now disabled in playable builds. Mega-related checks below concern shared-engine test compatibility only. See [the no-Mega audit](classic_no_mega_audit.md) for the current configuration and ROM memory usage.

## Turn ordering

Characterization tests were written and run before production edits: six passed and three failed. Failures isolated the default dynamic-Speed setting, the pre-move sort after a weather switch-in, and the same unintended sort on a later turn after Mega Evolution. After correction all nine initial tests passed (24 parameter variants). A later paired Gen 3/Gen 8 multiple-switch test brings the final result to 10/10 (26 variants).

The oracle is original [Emerald battle_main.c](https://github.com/pret/pokeemerald/blob/master/src/battle_main.c) (`SetActionsAndBattlersTurnOrder`, `GetWhoStrikesFirst`) and [battle_util.c](https://github.com/pret/pokeemerald/blob/master/src/battle_util.c) (`HandleAction_ActionFinished`). Switches precede moves; move priority and effective Speed determine the initial queue; ordinary Speed changes do not re-sort that queue. Tests also check next-turn Speed updates. The expansion's existing RNG generator and tie-permutation policy are retained; this is behavioral ordering restoration, not bit-for-bit reproduction of Emerald's RNG stream.

`B_RECALC_TURN_AFTER_ACTIONS` is now `GEN_3`. The pre-move gimmick sort requires a newly activated gimmick or explicitly configured Gen 8 dynamic Speed, still subject to the existing Gen 7 Mega-order option. Its pending flag is cleared after sorting and at turn start. This preserves Gen 6/7 Mega differences, Ultra Burst and Z/Max priority handling without letting an earlier Mega Evolution enable later re-sorts.

AI effective-Speed calculations, calculation contexts, policy, and tracing are unchanged.

## Phase 3B-2C test results

All commands use `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro`. Each filter below was run as `make check TESTS="<filter>" -j4`.

| Filter | Result |
| --- | --- |
| `Gen3 turn order:` | PASSED:          9; TOTAL:           9 |
| `Story AI` | PASSED:          29; TOTAL:           29 |
| `Frontier AI decision baseline` | PASSED:          2; TOTAL:           2 |
| `Story public-info` | PASSED:          5; TOTAL:           5 |
| `AI calculation context parity` | PASSED:          3; TOTAL:           3 |
| `AI calculation adapter parity` | PASSED:          3; TOTAL:           3 |
| `AI trace:` | PASSED:          7; TOTAL:           7 |
| `Spread restoration` | PASSED:          8; TOTAL:           8 |
| `Turn order is determined` | PASSED:          4; TOTAL:           4 |
| `Quick Claw` | PASSED:          1; TOTAL:           1 |
| `test/battle/form_change/mega_evolution.c` | FAILED:          1; PASSED:          13; TOTAL:           14 |
| `test/battle/form_change/ultra_burst.c` | PASSED:          8; TOTAL:           8 |
| `test/battle/move_effect/tailwind.c` | PASSED:          3; TOTAL:           3 |
| `test/battle/move_effect/quash.c` | PASSED:          5; TOTAL:           5 |
| `test/battle/move_effect/after_you.c` | PASSED:          6; TOTAL:           6 |
| `(Z-MOVE) Z-Moves do not retain priority` | PASSED:          1; TOTAL:           1 |
| `test/battle/move_effect/copycat.c` | TO_DO:           15; PASSED:          3; TOTAL:           18 |
| `test/battle/status1/paralysis.c` | FAILED:          1; PASSED:          7; TOTAL:           8 |

The Mega Sableye ability-replacement test and Thunder Wave miss-message test also fail identically in the isolated earlier `fc1d03dfcb` baseline. The first expects the opposing Wobbuffet's Celebrate message; the second expects the avoided-attack message. They were not changed. Copycat has 15 existing TODO tests. The previously documented damage-formula failures remain outside this change; that whole file was not rerun here.

## Shift/Set

The feature already exists and is enabled in release builds. `src/option_menu.c` reads/writes `optionsBattleStyle`; `src/new_game.c` defaults new games to Shift; battle initialization reads the saved option. The [original Emerald script](https://github.com/pret/pokeemerald/blob/master/data/battle_scripts_1.s) and current `BattleScript_HandleFaintedMon` retain the announcement, optional party selection, Set exclusion, battle-format exclusions, and action cancellation. No release feature toggle, save rewrite, or script correction was needed.

The automated-test-only forced Set assignment was moved from battle initialization into test setup, allowing opt-in tests to exercise the real saved preference. A test-only Yes/No input hook drives the existing command; party choices use the existing recorded controller. These tests do not navigate the graphical party menu interactively.

Final Shift/Set result: **9/9 passed (12 parameter variants)**. Coverage includes Shift Yes/No, saved Set, selecting a healthy reserve after a fainted party slot, PP checks proving no extra action, final opposing Pokémon, no reserve/all reserves fainted, doubles, partner multi, recorded link, Battle Tower, player fainting, and wild victory. Real party-menu eligibility checks in `src/party_menu.c` still reject fainted Pokémon, eggs, active Pokémon, and partner-owned slots; these checks and the menu were not edited. Scripted encounters and other Frontier facilities retain the original script guards; they were not each launched through their overworld entry points.

## Final regression pass

After the shared test-harness changes, the following filters were rerun with the same `make check` command.

| Filter | Result |
| --- | --- |
| `Gen3 turn order:` | 10 passed (latest rerun includes the multiple-switch test) |
| `Shift style:` | PASSED:          9; TOTAL:           9 |
| `Story AI` | PASSED:          29; TOTAL:           29 |
| `Frontier AI decision baseline` | PASSED:          2; TOTAL:           2 |
| `Story public-info` | PASSED:          5; TOTAL:           5 |
| `AI calculation context parity` | PASSED:          3; TOTAL:           3 |
| `AI calculation adapter parity` | PASSED:          3; TOTAL:           3 |
| `AI trace:` | PASSED:          7; TOTAL:           7 |
| `Spread restoration` | PASSED:          8; TOTAL:           8 |
| `test/battle/test_runner_features.c` | PASSED:          5; TOTAL:           5 |
| `test/battle/battle_message.c` | 11 passed, 1 existing TODO (latest rerun) |
| `test/battle/switch_in_abilities.c` | PASSED:          8; TOTAL:           8 |
| `test/battle/multi_battle_whiteout.c` | PASSED:          8; TOTAL:           8 |
| `Summary screen:` | PASSED:          3; TOTAL:           3 |

Two existing 2v1 battle-message tests depended on dynamic re-sorting between switches. They passed on the earlier baseline, then failed under the new Gen 3 default. They now explicitly select `GEN_8`, retaining their original expected messages. The new paired multiple-switch regression checks the original fixed Gen 3 switch sequence, the modern dynamic sequence, switches preceding Quick Attack, and unchanged PP on all incoming Pokémon.

## Build and warnings

`make -j4` succeeded after each phase. Final ROM: `pokeemerald.gba`.

| Region | Used | Capacity | Usage |
| --- | ---: | ---: | ---: |
| EWRAM | 226,416 B | 256 KB | 86.37% |
| IWRAM | 28,396 B | 32 KB | 86.66% |
| ROM | 26,726,092 B | 32 MB | 79.65% |

The ROM and test links emit the existing warning: `has a LOAD segment with RWX permissions`. No compiler warnings remain in the final builds. Headless test registration also emits the existing unrelated ASSUME diagnostics for Harvest, Pickup, and `special_attack_up_3`. These are separate from the selected-test results above. No full-suite pass is claimed.

## Files changed

| File | Purpose |
| --- | --- |
| `include/config/battle.h` | Select Gen 3 turn ordering only |
| `include/battle.h` | Per-turn pending gimmick-sort flag |
| `src/battle_main.c` | Guard/reset the gimmick sort; let tests exercise the saved battle style |
| `test/battle/turn_order_gen3.c` | Deterministic Gen 3/modern ordering characterization and regressions |
| `test/battle/battle_message.c` | Explicit Gen 8 configuration for two existing modern switch-message sequences |
| `include/test/battle.h` | Test-only Yes/No input and per-trial prompt count |
| `include/test_runner.h` | Test-only Yes/No input hook declaration |
| `test/test_runner_battle.c` | Default Set in test setup; supply/count prompt inputs |
| `src/battle_script_commands.c` | Inject test-only input into the existing Yes/No command; release code unchanged |
| `test/battle/shift_style.c` | Saved Shift/Set, replacements, and exception tests |
| `docs/phase3b2c_shift_verification.md` | Review record |

## Preservation and final state

`git diff --check` passed. Checksums match for Debug Mode, the trainer backup, trainer parties, AI source files, and Summary Screen settings. `B_FAINT_SWITCH_IN`, `B_MULTIPLE_TARGETS_DMG`, and `B_CHARGE` are unchanged. No Frontier data/scripts, story events/encounters, or approved QoL settings were edited. No save-file modification was performed. Implementation verification finished at HEAD `f06dac6c2e`, before checkpoint commits.

Pre-checkpoint `git status --short` (the trainer backup was already untracked and remains untouched):

```
 M include/battle.h
 M include/config/battle.h
 M include/test/battle.h
 M include/test_runner.h
 M src/battle_main.c
 M src/battle_script_commands.c
 M test/battle/battle_message.c
 M test/test_runner_battle.c
?? docs/phase3b2c_shift_verification.md
?? src/data/trainers.party.backup
?? test/battle/shift_style.c
?? test/battle/turn_order_gen3.c
```

## Approved Git checkpoint verification

The November 4 checkpoint rerun passed 92 tests across turn order, Shift/Set, spread restoration, all six AI baseline filters, test-runner features, and battle messages. Battle messages retain one existing TODO. Protected-file checksums matched. The changes are staged separately from the Classic no-Mega configuration and audit.
