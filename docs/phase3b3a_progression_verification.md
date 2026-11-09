# Phase 3B-3A: Gen 3 EXP, EV, and friendship

Verified November 8, 2026 against checkpoint `2ed1a7300fff1524c4d897b02ec860db3a5b86ef` on `emerald-enhanced-classic`. No commit or push.

## Source audit and implementation

Original references: [Emerald species data](https://github.com/pret/pokeemerald/blob/master/src/data/pokemon/species_info.h), [Pokémon creation and evolution](https://github.com/pret/pokeemerald/blob/master/src/pokemon.c), and [EXP awards](https://github.com/pret/pokeemerald/blob/master/src/battle_script_commands.c).

All 386 original base-species EXP yields, six EV-yield fields, and starting-friendship values were compared with the actual preprocessed expansion data. The independent numeric oracle is checked into `test/data/progression_gen3.h`, and runtime tests validate the compiled species table and newly created Pokémon against it. Castform and Deoxys use their normal-form aliases. Later-generation species/forms are not claimed to have original Emerald counterparts.

`P_UPDATED_EXP_YIELDS`, `P_UPDATED_EVS`, `P_UPDATED_FRIENDSHIP`, and `P_FRIENDSHIP_EVO_THRESHOLD` changed from `GEN_LATEST` to `GEN_3` in `include/config/pokemon.h`.

The configuration alone matched 385 of 386 species. Skarmory had an unconditional EXP yield of 163; original Emerald has 168. `src/data/pokemon/species_info/gen_2_families.h` now selects 168 specifically for `GEN_3`, preserving the existing 163 value for other configurations. With this correction, all 386 records match the original oracle.

Actual paths checked:

- EXP reads `gSpeciesInfo[faintedSpecies].expYield` in `Cmd_getexp`.
- `MonGainEVs` reads the species EV fields, separately applying the existing caps and bonuses.
- `STANDARD_FRIENDSHIP` selects 70 instead of 50; `CreateBoxMon` reads the species friendship value. Species-specific exceptions remain intact.
- `FRIENDSHIP_EVO_THRESHOLD` becomes 220 instead of 160 in evolution conditions; `IF_MIN_FRIENDSHIP` compares using `>=`. Tests check Pichu, Golbat, and Chansey at 219 and 220.
- Return and Frustration derive power from actual friendship. At standard friendship 70 their powers are 28 and 74; controlled level-50 neutral damage is 14 and 34. Stored friendship is not rewritten.

## Values changed from the checkpoint

Among original species: **376 EXP yields, seven EV-yield records, and 326 starting-friendship values** change. The runtime fixture covers all records, including unchanged exceptions.

| Species | Previous EXP yield | Original/restored EXP yield |
| --- | ---: | ---: |
| Bulbasaur | 64 | 64 |
| Caterpie | 39 | 53 |
| Treecko | 62 | 65 |
| Skarmory | 163 | 168 |
| Wynaut | 52 | 44 |
| Wobbuffet | 142 | 177 |
| Chansey | 395 | 255 |

| Species | Previous EV yield | Original/restored EV yield |
| --- | --- | --- |
| Yanma | 1 Speed | 2 Speed |
| Slowking | 2 Sp. Def | 3 Sp. Def |
| Misdreavus | 1 Sp. Def | 1 Sp. Atk + 1 Sp. Def |
| Blissey | 3 HP | 2 HP |
| Duskull | 1 Sp. Def | 1 Defense + 1 Sp. Def |
| Dusclops | 1 Defense + 1 Sp. Def | 1 Defense + 2 Sp. Def |
| Roselia | 2 Sp. Atk | 1 Sp. Atk |

Standard starting friendship changes 50 → 70. Special values, such as Chansey's 140, remain unchanged. The evolution threshold changes 160 → 220.

## EXP discrepancy diagnosis

The original two-opponent test fails on unmodified checkpoint `2ed1a7300f`: actual total EXP 4,156, expected 2,771. It also failed on the earlier baseline recorded in Phase 3B-2D.

The expectation already reads the active species yields, so it does not hardcode newer yields. It omitted the enabled trainer multiplier. For each defeated Pokémon, the tested full-participant configuration awards `floor(yield * level / 7)`, then `floor(EXP * 3 / 2)` for trainer battles. The initial level-one EXP is 1. The test now explicitly controls trainer multiplier Gen 3 versus Gen 8, preserving full-participant distribution, and derives each opponent's contribution separately before summing.

| Data | Without trainer multiplier (Gen 8 control) | With trainer multiplier (Gen 3) |
| --- | ---: | ---: |
| Previous Wynaut 52 / Wobbuffet 142 | 2,771 | 4,156 |
| Restored Wynaut 44 / Wobbuffet 177 | 3,157 | 4,735 |

Both multiplier variants passed **before** any production restoration, and pass afterward. Restoring yields changes the amounts but does not repair an expectation that omits the multiplier. No engine EXP-calculation fix was required; no distribution policy was changed. These formulas describe the approved full-participant configuration, not Emerald's original split-between-participants policy.

## Preserved features and trainer data

Unchanged settings: `B_SCALED_EXP = GEN_3`, `B_TRAINER_EXP_MULTIPLIER = GEN_3`, `B_SPLIT_EXP = GEN_6`, `B_EXP_CATCH = GEN_6`, `I_EXP_SHARE_FLAG = FLAG_EXP_SHARE`, and `P_EV_CAP = GEN_LATEST` (252 per stat). The new-game party-wide EXP Share flag remains enabled by the existing code.

A wild level-seven Caterpie awards 53 EXP to the participant and, with party-wide sharing enabled, 26 to the reserve, for both defeat and capture. Simultaneous trainer Caterpie/Weedle KOs award 157 to each participant and 78 to the reserve, with one HP and one Speed EV each. These exact assertions guard the approved QoL distribution.

Trainer creation copies explicit EV templates independently of species EV yields; a test exercises all six fields through `GenerateMonFromTrainerMon`. The story trainer table is replaced in test builds, so preservation of the actual enhanced parties is additionally checked by byte-identical source checksums. The current story `trainers.party` has no explicit EV entries; its existing teams and IV assignments were not altered. No source editing, regeneration changes, or species-yield-derived EV assignment was applied to trainers.

No evolution methods, trade-free replacements, item usability, breeding/egg settings, RTC behavior, Frontier content, following Pokémon, shiny odds, Debug Mode, trainer backup, or Phase 4 plans were edited. Gen 3 battle settings and the no-Mega policy remain unchanged.

## Tests and build

Before production edits, the six initial characterization tests produced five expected failures (EXP data, EV data, starting friendship, threshold, and wild EXP) and one pass (252 EV cap). Final new coverage: **9 passed**, including the 386-record oracle, six threshold variants, four catch/share variants, simultaneous KOs, two Return/Frustration variants, and trainer EV overrides.

Commands: `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS="<filter>" -j4`.

| Filter | Passed | Failed | TODO |
| --- | ---: | ---: | ---: |
| `test/battle/exp.c` | 9 | 0 | 0 |
| `test/battle/move_effect/return.c` | 3 | 0 | 0 |
| `test/battle/move_effect/frustration.c` | 3 | 0 | 0 |
| `test/battle/hold_effect/friendship_up.c` | 0 | 0 | 1 |
| `Gen3 turn order:` | 10 | 0 | 0 |
| `Gen3 faint replacement:` | 11 | 0 | 0 |
| `Shift style:` | 9 | 0 | 0 |
| `Spread restoration` | 8 | 0 | 0 |
| `Story AI` | 29 | 0 | 0 |
| `Frontier AI decision baseline` | 2 | 0 | 0 |
| `Story public-info` | 5 | 0 | 0 |
| `AI calculation context parity` | 3 | 0 | 0 |
| `AI calculation adapter parity` | 3 | 0 | 0 |
| `AI trace:` | 7 | 0 | 0 |
| `test/battle/multi_battle_whiteout.c` | 8 | 0 | 0 |
| `test/battle/evolution_tracker.c` | 8 | 0 | 0 |
| `Summary screen:` | 3 | 0 | 0 |
| `test/battle/form_change/mega_evolution.c` | 13 | 1 | 0 |
| `test/battle/status1/paralysis.c` | 7 | 1 | 0 |

Combined selection: **150 passed, 2 pre-existing failures, 1 existing TODO**, no new failures or selected-test skips. The full suite was not run.

Remaining failures are the previously baselined Mega Sableye ability-replacement message (`test/battle/form_change/mega_evolution.c:106`) and Thunder Wave avoided-attack message (`test/battle/status1/paralysis.c:110`). Neither was changed. Test registration also emits the existing unrelated Harvest, Pickup, and `special_attack_up_3` ASSUME diagnostics.

No-Mega verification passed: zero routes/forms in normal and release preprocessing, 97 routes/forms in test-only compatibility data. A release/LTO ROM was not built separately.

`DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make -j4` succeeded. The only warning was the existing linker warning `has a LOAD segment with RWX permissions`; no compiler warnings were reported.

| Region | Used | Usage |
| --- | ---: | ---: |
| EWRAM | 226,416 B | 86.37% |
| IWRAM | 28,396 B | 86.66% |
| ROM | 25,034,036 B | 74.61% |

These sizes are unchanged from Phase 3B-2D. `git diff --check` passed. Debug configuration, trainer backup, trainer parties, all AI sources, and item configuration passed their before/after checksum checks.

## Files and final Git status

- `include/config/pokemon.h`: the four approved defaults.
- `src/data/pokemon/species_info/gen_2_families.h`: Gen 3-only Skarmory EXP yield correction.
- `test/battle/exp.c`: paired trainer-multiplier expectations with explicit distribution settings.
- `test/progression_gen3.c`: compiled species data, creation friendship, evolution threshold, EV cap, and trainer EV override tests.
- `test/data/progression_gen3.h`: independent numerical oracle for all 386 original species.
- `test/battle/progression_gen3.c`: exact EXP/EV distribution, capture, simultaneous-KO, and Return/Frustration tests.
- `docs/phase3b3a_progression_verification.md`: this audit and verification record.

`git status --short`:

```
 M include/config/pokemon.h
 M src/data/pokemon/species_info/gen_2_families.h
 M test/battle/exp.c
?? docs/phase3b3a_progression_verification.md
?? src/data/trainers.party.backup
?? test/battle/progression_gen3.c
?? test/data/
?? test/progression_gen3.c
```

`test/data/` contains only the new `progression_gen3.h` oracle. The trainer backup was already untracked and is untouched. Nothing is staged, committed, or pushed. HEAD remains `2ed1a7300fff1524c4d897b02ec860db3a5b86ef`.



## Save compatibility and manual checks

No save migration or save editing was performed. Preprocessed `SaveBlock1` and `SaveBlock2` declarations are identical before and after. Pokémon storage layout, species IDs, and growth curves are unchanged.

Existing Pokémon retain stored EXP, levels, EVs, and friendship; existing evolutions are not reversed. Future EXP/EV awards use restored yields. Newly created Pokémon use restored starting friendship. An existing unevolved Pokémon at friendship 160–219 now needs 220 to satisfy a friendship evolution condition. Existing Pokémon's Return/Frustration power is unchanged until their friendship changes normally.

Manual follow-up: verify EXP bars/level-up presentation with party-wide sharing and catching, friendship evolution at the new threshold, and approved evolution-item/trade-free flows on a copied existing save. Automated battle tests drive recorded controller actions rather than the full overworld/menu flow.
