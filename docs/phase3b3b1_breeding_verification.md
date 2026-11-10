# Phase 3B-3B1: Core Gen 3 breeding

Baseline: `c264eec1820e506234966c6a5cfd4bdc218b1e31`, branch `emerald-enhanced-classic`. No commit or push.

## Changes and implementation audit

Only production changes: nine defaults in `include/config/pokemon.h`, from `GEN_LATEST` (Gen 9) to `GEN_3`:

- `P_EGG_HATCH_LEVEL`: newly created eggs use level 5 instead of 1.
- `P_MOVE_INHERITANCE`: initial moves, paternal egg moves, paternal compatible TM/HM moves, shared level-up moves, then special item moves. Maternal egg moves no longer independently pass. Light Ball/Volt Tackle remains supported.
- `P_INCENSE_BREEDING`: run the existing incense table; either parent can hold the required incense.
- `P_NATURE_INHERITANCE`: mother/ Ditto Everstone eligibility, Ditto precedence, 50% inheritance check. A failed inheritance check can still randomly generate the same nature.
- `P_ABILITY_INHERITANCE`: skip the parental override; newborn normal ability remains selected from personality parity where the species has two normal abilities.
- `P_BALL_INHERITANCE`: leave the newly created egg in an ordinary Poke Ball.
- `P_EGG_MOVE_TRANSFER`: the daycare deposit transfer call requires Gen 8 or later, so neither same-species transfer nor Mirror Herb transfer runs.
- `P_UPDATED_EGG_GROUPS`: Ralts family becomes Amorphous only; Trapinch family becomes Bug only. The same shared setting selects pre-Gen-8 compatibility for Hawlucha (Human-Like), Bergmite family (Monster), and Noibat family (Flying). It neither enables species nor changes evolution methods or save structure.
- `P_NIDORAN_M_DITTO_BREED`: disable the Gen-5-or-later Nidoran-male/Volbeat reverse offspring branches. Existing Nidoran-female/Illumise logic remains unchanged.

`P_EGG_CYCLE_LENGTH` remains `GEN_LATEST`: the implemented default is 128 steps, including under Gen 9. No engine source, species data, move data, item logic, or save structure was edited.

References: original [Emerald daycare](https://github.com/pret/pokeemerald/blob/master/src/daycare.c), [egg moves](https://github.com/pret/pokeemerald/blob/master/src/data/pokemon/egg_moves.h). This is a scoped rules restoration, not an attempt to reproduce original RNG consumption or IV-selection bugs.

## Expected offspring

Electabuzz + Ditto produces Elekid; Magmar + Ditto produces Magby. Marill produces Azurill with Sea Incense, otherwise Marill. Wobbuffet produces Wynaut with Lax Incense, otherwise Wobbuffet. The requested families already have suitable egg groups and enabled baby/evolution links. Tests exercise both daycare slot orders and adult sexes.

## Deliberately retained compatibility

The existing egg-move lists remain unchanged. Gen 3 paternal inheritance still accepts a listed modern move if the compatible father knows it. Examples:

| Child | Retained later-generation egg moves |
| --- | --- |
| Bulbasaur | Leaf Storm, Power Whip, Grassy Terrain |
| Charmander | Dragon Rush, Flare Blitz, Dragon Pulse |
| Elekid | Feint, Hammer Arm |
| Magby | Flare Blitz, Power Swap, Belch |
| Azurill | Soak, Copycat |
| Marill | Aqua Jet |

A deterministic Bulbasaur/Leaf Storm test documents this intentional exception. These examples describe inheritance eligibility, not new gameplay acquisition routes for the father. No moves were added to parents, trainers, TMs, tutors, or level-up data by this change. Fixtures can explicitly supply moves to test the inheritance boundary.

All 50 TMs and eight HMs still teach Gen-3-era moves. Paternal machine inheritance nevertheless consults current teachable compatibility, which can permit species/move combinations absent in original Emerald. Current egg lists also contain later additions of old moves; filtering move IDs alone would not recreate original lists.

Destiny Knot's five-IV effect and Power-item forced inheritance remain untouched, as does the corrected distinct-stat IV selection. These modern breeding mechanics are deferred compatibility exceptions. No original Emerald IV-selection bug was reintroduced. The hatching ability list still includes Steam Engine alongside Flame Body/Magma Armor. Later species/forms remain available to shared engine/test infrastructure under existing configuration; this phase changes no encounters or availability policy.

## Save and timer behavior

There is no save migration or layout change. Existing eggs retain stored EXP/level, moves, ability, and ball at hatch. A synthetic pre-change level-one egg regression checks these attributes and completion of a partially elapsed hatch cycle; this is not a disk save/load emulator test. New daycare and gift eggs use level 5. Existing eggs are deliberately not rewritten.

Egg friendship stores remaining cycles. The shared counter still ticks every 128 steps, with Flame Body/Magma Armor acceleration; reaching zero cycles and actually initiating hatch occur on separate checks. Collection/rejection still resets the shared counter through the existing code. Pending daycare eggs are constructed at collection, so collection after updating uses the new defaults. Original pending-egg RNG behavior is not being restored.

Recommended manual checks: load a real pre-update save with party/boxed eggs and a pending daycare egg; collect, hatch, save/reload, and verify party selection and hatch presentation. Existing Nidoran-female/Illumise personality-based branching and regional-form breeding infrastructure are outside this patch.

## Verification

New tests were added before production edits. On the unchanged production defaults: **10 failed, 2 passed**. Failures demonstrated modern level, ball, nature, ability, egg-group, transfer, male/Ditto offspring, and move behavior; the faster cycle and existing-egg tests passed already. Two initial fixture compilation errors (missing event-data declaration and parameter initialization) were corrected before this characterization run.

Commands use `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro` with `make check TESTS='<filter>' -j4`, followed by `make -j4` and `git diff --check`.

| Selection | Passed | Failed | Skipped assumptions | TODO |
| --- | ---: | ---: | ---: | ---: |
| `Gen3 breeding:` | 12 | 0 | 0 | 0 |
| `test/daycare.c` | 38 | 0 | 0 | 0 |
| `Gen3 progression:` | 9 | 0 | 0 | 0 |
| `test/battle/exp.c` | 9 | 0 | 0 | 0 |
| `test/battle/move_effect/return.c` | 3 | 0 | 0 | 0 |
| `test/battle/move_effect/frustration.c` | 3 | 0 | 0 | 0 |
| `test/battle/hold_effect/friendship_up.c` | 0 | 0 | 0 | 1 |
| `Gen3 turn order:` | 10 | 0 | 0 | 0 |
| `Gen3 faint replacement:` | 11 | 0 | 0 | 0 |
| `Shift style:` | 9 | 0 | 0 | 0 |
| `Spread restoration` | 8 | 0 | 0 | 0 |
| `test/battle/spread_moves.c` | 27 | 1 | 2 | 0 |
| `Story AI` | 29 | 0 | 0 | 0 |
| `Frontier AI decision baseline` | 2 | 0 | 0 | 0 |
| `Story public-info` | 5 | 0 | 0 | 0 |
| `AI calculation context parity` | 3 | 0 | 0 | 0 |
| `AI calculation adapter parity` | 3 | 0 | 0 | 0 |
| `AI trace:` | 7 | 0 | 0 | 0 |
| `test/battle/multi_battle_whiteout.c` | 8 | 0 | 0 | 0 |
| `test/battle/evolution_tracker.c` | 8 | 0 | 0 | 0 |
| `Summary screen:` | 3 | 0 | 0 | 0 |
| `test/battle/form_change/mega_evolution.c` | 13 | 1 | 0 | 0 |
| `test/battle/status1/paralysis.c` | 7 | 1 | 0 | 0 |

Selected tests: **227 passed, 3 failed, 2 skipped by assumptions, 1 TODO (233 total)**. Counts are test titles; parameterized cases execute within those titles. This is not a full-suite run.

- The new 12 breeding tests and all 38 existing daycare tests pass. Explicit modern-generation compatibility tests remain intact.
- All selected progression, AI, core Gen 3 battle restoration, multi-battle, evolution tracking, and Summary Screen checks pass.
- Previously documented failures remain: Mega Sableye ability replacement (`test/battle/form_change/mega_evolution.c:106`, unmatched Celebrate message at line 118), and Thunder Wave's miss message (`test/battle/status1/paralysis.c:110`, unmatched avoided-attack message at line 119). Mega support remains a test-only compatibility path under the existing Classic policy.
- The broader spread suite fails `test/battle/spread_moves.c:308`, Eiscue/Mimikyu/Eject Button, at the expected Ice Face popup (line 324). Final triage reproduced this on the previous checkpoint and isolated a Gen 3 damage-category fixture mismatch; see below. No battle assertion or implementation was changed to mask it.
- Two spread-suite assumptions are incompatible with existing Gen 3 targeting: Cotton Spore is not TARGET_BOTH (line 580), and Surf is not TARGET_FOES_AND_ALLY (line 643). These are selected-test skips, not passes.
- Existing Soothe Bell test-title TODO remains. Name-filter registration also prints existing unrelated Harvest, Pickup, and special_attack_up_3 ASSUME diagnostics; these are separate from selected test results.

`make -j4` succeeded (exit 0). No compiler warnings were emitted; the existing linker warning remains: `pokeemerald.elf has a LOAD segment with RWX permissions`.

| Region | Used | Usage |
| --- | ---: | ---: |
| EWRAM | 226,416 B | 86.37% |
| IWRAM | 28,396 B | 86.66% |
| ROM | 25,033,588 B | 74.61% |

EWRAM/IWRAM match the previous checkpoint; ROM is 448 bytes smaller. `git diff --check` passed. Only the nine approved defaults differ in tracked production code. `P_EGG_CYCLE_LENGTH`, EV/friendship settings, and the normal-build no-Mega default remain unchanged.

Files changed: `include/config/pokemon.h`, new `test/breeding_gen3.c`, and this document. Nothing is staged, committed, or pushed. Build artifacts are ignored and not included.

Final `git status --short`:

```text
 M include/config/pokemon.h
?? docs/phase3b3b1_breeding_verification.md
?? src/data/trainers.party.backup
?? test/breeding_gen3.c
```

The trainer backup was already untracked and is untouched.

## Scope

Protected progression settings, battle mechanics, AI, trainers, Frontier data, trade-free evolutions, Summary Screen, following Pokemon, shiny odds, Eevee RTC, Debug Mode, and trainer backup are unchanged. No Phase 4 work was performed.


## Final regression triage — November 10, 2026

**Conclusion: the Eiscue/Mimikyu/Eject Button failure predates the breeding changes. It is a test/configuration interaction with the existing Gen 3 physical/special split. The breeding patch is safe to checkpoint with this separately documented test issue; this is not a claim that the whole battle suite is green.**

Exact test: `Spread Moves: Spread move vs Eiscue and Mimikyu with 1 Eject Button`, `test/battle/spread_moves.c:308`. Exact error: `test/battle/spread_moves.c:324: Unmatched ABILITY_POPUP`, for `ABILITY_POPUP(opponentRight, ABILITY_ICE_FACE)`.

### Controlled comparison

Built committed checkpoint `c264eec1820e506234966c6a5cfd4bdc218b1e31` from `git archive HEAD` in an isolated `/tmp` directory. The archive has the original nine breeding defaults. No working-tree production files were reverted. Host tool binaries and generated asset cache were reused; baseline C objects were built from the archived sources using the same devkitARM toolchain and normal test flags. An archive-only `.histignore` bypassed the build's requirement for Git metadata; it does not alter game configuration. The fresh baseline asset rebuild also emitted existing libpng bKGD warnings.

For the direct comparison, copied each revision's test ELF, patched only its filter/headless settings using the repository `patchelf`, and ran `mgba-rom-test-hydra` with `MAKEFLAGS` unset (one runner). Both revisions ran these selections in the same order: isolated failing test, full spread file, Ice Face, Disguise, Eject Button, Gen 3 spread restoration, then isolated failing test again. The single-runner test name/result sequences and all seven selection summaries matched exactly. The initial ordinary `make check -j4` isolated run also failed on both revisions.

The runner resets global state and seeds RNG/RNG2 to zero for each test (`test/test_runner.c:340`). Battle setup uses the same default seed; the accuracy override returns a hit and the default critical-hit override returns false (`test/test_runner_battle.c`). No seed or RNG override was added to the original fixture. This excludes random Razor Leaf misses as the explanation and provides no evidence of execution-order dependence in the tested runs.

| Selection | Previous checkpoint | Current breeding tree |
| --- | --- | --- |
| Exact test, before suites | Same Ice Face assertion failure | Same failure |
| Full `test/battle/spread_moves.c` | 27 pass, 1 fail, 2 assumption skips | Identical |
| `test/battle/ability/ice_face.c` | 8 pass | 8 pass |
| `test/battle/ability/disguise.c` | 11 pass | 11 pass |
| `test/battle/hold_effect/eject_button.c` | 13 pass | 13 pass |
| `Spread restoration` | 8 pass | 8 pass |
| Exact test, after suites | Same Ice Face assertion failure | Same failure |

Current-tree preservation checks also passed again: **12 breeding, 38 daycare, and 9 Gen 3 progression tests**. No existing test was removed or weakened.

### Mechanism and diagnostic control

The fixture uses Razor Leaf and assumes `GetMoveCategory(MOVE_RAZOR_LEAF) == DAMAGE_CATEGORY_PHYSICAL`. That accessor reads the modern per-move category. Both revisions already set `B_PHYSICAL_SPECIAL_SPLIT = GEN_3`; `GetBattleMoveCategory` (`src/battle_util.c:9117`) instead selects the move type's category. Grass is special under that rule. `DoesIceFaceBlockMove` (`src/battle_script_commands.c:7715`) requires `IsBattleMovePhysical(move)`, so Razor Leaf does not activate Ice Face in this configuration. Disguise can still activate, leaving the following Ice Face expectation unmatched.

To test this mechanism, only in the temporary baseline copy, added a duplicate diagnostic fixture using Rock Slide instead of Razor Leaf. It retains the same Pokemon, speeds, Eject Button, switch, category/target assumptions, and all three scene assertions (both ability popups plus the switch message). Rock Slide is physical under Gen 3, and the diagnostic **passed 1/1**. The original failing test remained present and unchanged. This control was not added to the working tree and is not part of the proposed checkpoint.

No production fix is indicated by this failure. A separately reviewed fixture correction could explicitly exercise the intended physical spread interaction while adding a Gen 3 special-move control. This investigation does not alter that test, modern abilities, damage resolution, AI, or breeding.

### Investigation scope and final checks

Only this verification document changed in the working tree during triage. The nine configuration edits and `test/breeding_gen3.c` are unchanged from the implemented breeding patch. Temporary baseline/diagnostic files and logs reside outside the repository. Debug Mode and trainer backup remain untouched. No commit or push.

Final `git diff --check` passed and `make -j4` succeeded (exit 0). The only build warning is the existing RWX LOAD-segment linker warning. Memory remains EWRAM 226,416 B, IWRAM 28,396 B, ROM 25,033,588 B. Final Git status remains exactly the four entries recorded above; nothing is staged.
