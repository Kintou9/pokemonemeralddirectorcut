# Classic Edition: no Mega Evolution

Audit and change: November 4, 2026, before the approved Git checkpoint.

## Findings reported before the change

- `P_MEGA_EVOLUTIONS` was enabled. The normal preprocessed Pokémon data contained 97 enabled Mega species and 97 Mega transformation entries, including item-based transformations and Rayquaza's Dragon Ascent route.
- `CanMegaEvolve` requires the player to own the Mega Ring, plus a matching held stone or the move-based transformation requirement. Opposing trainers/partners do not require a player-owned ring. The battle move-selection controller uses Start to toggle an available gimmick.
- The item table contains 92 Mega Stones and the Mega Ring. These are bag/held/key-item definitions, not direct out-of-battle transformation actions.
- Searches across map scripts, rewards, shops, encounter/held-item data, trainer parties, and Frontier data found no ordinary progression source for these items, no Mega-form trainer loadouts, and no trainer-held Mega Stones. The generic random-item generator is only called by Debug tooling in the current source.
- The enabled Debug menu is a reachable exception: its item picker/fill commands can grant the ring and stones; its Pokémon tools can supply matching Pokémon/moves. Therefore the old playable build did allow a route to Mega Evolution through Debug. Existing externally supplied ring/stone inventory could also satisfy the old activation checks.

## Change

Set the existing `P_MEGA_EVOLUTIONS` switch to `FALSE` in `include/config/species_enabled.h`. The existing conditional form/species tables then omit all Mega forms and Mega transformation entries, including the newer Mega forms controlled by `P_GEN_9_MEGA_EVOLUTIONS` and Rayquaza's move-based entry. With no Mega transformation target, `CanMegaEvolve` returns false, so no player, opposing trainer, or partner can select Mega Evolution, even with debug-granted items.

No shared engine functions, item IDs, species IDs, battle scripts, trainer parties, or save data were removed or rewritten. Item definitions remain for compatibility and Debug; they cannot enable Mega Evolution in Classic. Debug Mode itself is unchanged. No other gimmick configuration was changed.

The existing `include/config/test.h` override deliberately retains Mega forms in `TESTING=1` builds. It is compatibility support, not a Classic gameplay setting. The earlier Mega Sableye failure is an existing compatibility/test-configuration issue and is not a reason to enable Mega Evolution in the game.

## Verification

`python3 dev_scripts/verify_classic_no_mega.py` preprocesses the actual Pokémon data in three configurations:

| Configuration | Mega transformation entries | Enabled Mega species |
| --- | ---: | ---: |
| Classic normal (`TESTING=0`) | 0 | 0 |
| Classic release (`TESTING=0`, `RELEASE`) | 0 | 0 |
| Engine compatibility tests (`TESTING=1`) | 97 | 97 |

Before/after comparisons of the preprocessed `enum Species`, `struct SaveBlock1`, and `struct SaveBlock2` declarations were identical. This change does not renumber species or change the save-block layout. No save file was edited or migrated.

`DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make -j4` succeeded. This builds the normal playable ROM; the release configuration was checked by preprocessing, not a separate LTO release build.

| Region | Used | Usage |
| --- | ---: | ---: |
| EWRAM | 226,416 B | 86.37% |
| IWRAM | 28,396 B | 86.66% |
| ROM | 25,034,228 B | 74.61% |

The existing linker warning remains: `has a LOAD segment with RWX permissions`.

## Battle regression results

Commands used `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS="<filter>" -j4`.

| Filter | Result |
| --- | --- |
| `Gen3 turn order:` | PASSED:          10; TOTAL:           10 |
| `Shift style:` | PASSED:          9; TOTAL:           9 |
| `Spread restoration` | PASSED:          8; TOTAL:           8 |
| `AI calculation context parity` | PASSED:          3; TOTAL:           3 |
| `AI calculation adapter parity` | PASSED:          3; TOTAL:           3 |
| `test/battle/form_change/mega_evolution.c` | 13 passed; 1 existing Mega Sableye compatibility failure |

The remaining compatibility failure is `Abilities replaced by Mega Evolution do not affect turn order` (`test/battle/form_change/mega_evolution.c:106`, unmatched opposing Wobbuffet Celebrate message at line 118). It is the same failure already reproduced on the earlier baseline. No Mega Sableye gameplay behavior was enabled or repaired to accommodate it. The normal playable build has no Mega Sableye form. Test registration also emits the previously recorded unrelated ASSUME diagnostics for Harvest, Pickup, and `special_attack_up_3`.

`git diff --check` passed. Checksums confirmed that Debug Mode, the trainer backup, trainer parties, AI source files, and Summary Screen settings are unchanged from the start of this request. The Phase 3B-2C and Shift/Set work remains intact.

Files added or changed specifically for this requirement:

- `include/config/species_enabled.h`: disable Mega forms/routes using the existing configuration switch.
- `dev_scripts/verify_classic_no_mega.py`: reusable check of actual preprocessed normal, release, and compatibility-test data.
- `docs/classic_no_mega_audit.md`: audit findings and verification results.
- `docs/phase3b2c_shift_verification.md`: clarify that earlier Mega tests now describe test compatibility, not Classic gameplay, and point to current ROM memory usage.

Implementation verification preceded the approved checkpoint commits. The existing trainer backup remains untracked and untouched.

The November 4 checkpoint rerun reconfirmed all three preprocessing configurations, the successful normal ROM build and memory figures above, and the same 13 passing Mega compatibility tests plus the existing Mega Sableye failure. No new failure was found.
