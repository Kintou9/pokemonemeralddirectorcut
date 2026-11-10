# Phase 3B-3C: Gen 3 items and Lilycove evolution shop

Baseline: `6e7964eae36b01ab705be078c3d0a4280c8bb8a0`, branch `emerald-enhanced-classic`. Implementation and final checkpoint verification are recorded below.

## Production changes

Only `I_HEALTH_RECOVERY`, `I_TYPE_BOOST_POWER`, and `I_LAX_INCENSE_BOOST` change from `GEN_LATEST` to `GEN_3` in `include/config/item.h`.

| Effect | Before | After |
| --- | ---: | ---: |
| Super Potion / Energy Powder | 60 HP | 50 HP |
| Hyper Potion / Energy Root | 120 HP | 200 HP |
| Fresh Water | 30 HP | 50 HP |
| Soda Pop | 50 HP | 60 HP |
| Lemonade | 70 HP | 80 HP |
| Ordinary type-boosting held items | 20% | 10% |
| Sea Incense | 20% | 5% |
| Lax Incense accuracy factor | 90% | 95% |

Healing clamps to missing HP. Existing shared branches also select 50/200 HP for Fine/Superb Remedy; no item availability was added. Sitrus remains flat 30 HP. Vitamins still work above 100 EVs, with the approved 252-per-stat/510-total bounds. No unrelated item configuration or global item-price data changed.

## Counter, stock, and local prices

The existing third-floor right clerk now calls a stock-selection subroutine in `data/maps/LilycoveCity_DepartmentStore_3F/scripts.inc`. It tests **FLAG_BADGE06_GET**, not badge count. The original seven entries retain their exact order: X Speed, X Sp. Atk, X Attack, X Defense, Dire Hit, Guard Spec., X Accuracy. Without Feather Badge, only these entries are available. With Feather Badge, the following six items are appended, followed by Fire Stone, Water Stone, Thunder Stone, Leaf Stone, Moon Stone, and Sun Stone (19 entries total). The stones use ordinary global purchase prices and normal discount handling; only the six items below receive fixed shop-local prices:

| Item | Local purchase price | Unchanged global price | Unchanged resale |
| --- | ---: | ---: | ---: |
| Metal Coat | 8,000 | 3,000 | 750 |
| King's Rock | 8,000 | 10,000 | 2,500 |
| Dragon Scale | 10,000 | 4,000 | 1,000 |
| Up-Grade (`ITEM_UPGRADE`) | 10,000 | 4,000 | 1,000 |
| DeepSeaTooth | 7,500 | 2,000 | 500 |
| DeepSeaScale | 7,500 | 2,000 | 500 |

`src/shop.c` captures the expanded stock-list identity in a transient MartInfo boolean **before** dynamic shop filtering allocates a replacement list. Every call to `SetShopItemsForSale` resets that identity, and quitting the counter clears it. No map-wide or item-global price override is used.

`GetMartBuyPrice` supplies all four normal-shop price reads: list display, initial affordability/quantity price, single-item confirmation, and quantity updates. Maximum affordable quantity and final payment continue to use the same stored total. The six overrides return before the ordinary sale calculation. Original stock retains ordinary prices and discounts. The Energy Guru discount normally cannot apply on Lilycove 3F; tests deliberately force that condition to prove that overrides remain fixed. Resale still uses the unchanged global price.

All original purchase/payment and failure handlers remain intact. Failed bag insertion does not reach the money-deduction callback. There is no stock depletion or purchase flag: repeat purchases are unlimited subject to money, available slots, and the existing 999-per-stack bag limit.

Greeting, farewell, and all original NPC text are unchanged. No new NPC, map, item ID, or persistent save field is introduced. The original stock list remains separately available.

## Evolution and acquisition preservation

Existing bag-use routes were verified without changing their implementation:

- Onix + Metal Coat -> Steelix; Scyther + Metal Coat -> Scizor.
- Poliwhirl + King's Rock -> Politoed; Slowpoke + King's Rock -> Slowking.
- Seadra + Dragon Scale -> Kingdra; Porygon + Upgrade -> Porygon2.
- Clamperl + Deep Sea Tooth -> Huntail; Clamperl + Deep Sea Scale -> Gorebyss.

Tests exercise the actual party-menu evolution callback for all eight valid cases and invalid Wobbuffet controls. Valid use removes exactly one of two bag items and schedules the correct evolution target; invalid use removes none. They stop before rendering the evolution animation. Evolution methods, held effects, original rewards/wild held items, and the Scanner exchange are untouched.

## Regression methodology

Initial item characterization before production edits: four expected failing test titles (field/battle healing, type boosts, Lax accuracy), two passing titles (vitamin caps and evolution lookup). Fixture setup was corrected to calculate field Pokemon stats before reading HP. Later test-driver corrections handled scroll bounds, explicit dismissal of error messages, and freeing an invalid-use text printer; no production assertions were weakened. The damage comparison uses explicit Sp. Atk/Sp. Def and the same forced maximum damage roll for all three item variants, avoiding comparisons between different random rolls.

New automated coverage:

- Seven `Gen3 items:` titles: original healing quantities/clamping in battle and field, type multipliers, controlled Lax Incense accuracy trials, vitamin caps, eight evolution routes, and actual evolution-item consumption callbacks.
- Six `Lilycove shop:` titles: actual script badge selection with other badges set, exact stock/order, filtered-list price context, unchanged global/resale values, discounts, all six items bought twice in two-item quantities with and without sale activation, insufficient funds, cancellation at confirmation, full Bag, money-limited quantity, exact-funds purchase, and opening a different shop afterward.

The `TESTING`-only shop driver and `include/test/shop.h` initialize the real list/quantity/confirmation/payment tasks and send button inputs. They omit map rendering and overworld transitions; they do not reimplement transactions. Script tests stop at the actual pokemart effect boundary and inspect its stock pointer. The tests use representative in-memory badge states, not imported disk save files.

Commands: `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS='<filter>' -j4`.

Selected suites: **258 passed, 3 pre-existing failures, 4 assumption skips, 1 existing TODO (266 total)**. Counts are test titles; parameterized cases execute within those titles. The full repository suite was not run.

| Selection | Passed | Failed | Assumption skips | TODO |
| --- | ---: | ---: | ---: | ---: |
| `Gen3 items:` | 7 | 0 | 0 | 0 |
| `Lilycove shop:` | 6 | 0 | 0 | 0 |
| `test/battle/item_effect/restore_hp.c` | 11 | 0 | 0 | 0 |
| `test/battle/hold_effect/type_power.c` | 2 | 0 | 0 | 0 |
| `test/battle/hold_effect/restore_hp.c` | 5 | 0 | 2 | 0 |
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

The three failures are unchanged from the documented breeding baseline:

- Eiscue/Mimikyu/Eject Button: `test/battle/spread_moves.c:324`, unmatched Ice Face popup because Razor Leaf is special under the existing Gen 3 split. Prior checkpoint reproduction and diagnosis are recorded in `phase3b3b1_breeding_verification.md`.
- Mega Sableye ability-replacement test: `test/battle/form_change/mega_evolution.c:118`, unmatched Celebrate message. Normal-game Mega configuration is unchanged; the test uses compatibility support.
- Thunder Wave miss-message test: `test/battle/status1/paralysis.c:119`, unmatched avoided-attack message.

Four assumption skips: spread tests requiring later Cotton Spore/Surf targets (lines 580/643), and held-berry tests requiring percentage-based Sitrus healing (lines 61/121). The latter condition already failed with the pre-existing `I_SITRUS_BERRY_HEAL = GEN_3`. The existing Soothe Bell TODO remains. Name-filter registration prints existing unrelated Harvest, Pickup, and special_attack_up_3 assumption diagnostics; these are not additional selected-test failures.

The new shop suite was rerun after strengthening the test driver's context-leak check: closing the test UI leaves the old context intact, so opening the next counter must reset it through the real production setter.

`DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make -j4` succeeded (exit 0). Only the existing linker warning remains: `pokeemerald.elf has a LOAD segment with RWX permissions`. No compiler warnings were emitted.

| Region | Used | Usage | Change from checkpoint |
| --- | ---: | ---: | ---: |
| EWRAM | 226,420 B | 86.37% | +4 B |
| IWRAM | 28,396 B | 86.66% | unchanged |
| ROM | 25,033,780 B | 74.61% | +192 B |

`git diff --check` and whitespace checks for every new file passed. The six shop tests passed again after the test-driver refinement. No production changes followed the broader suite run.

Exact files changed:

- `include/config/item.h`: three approved defaults.
- `data/maps/LilycoveCity_DepartmentStore_3F/scripts.inc`: badge branch and appended stock list; original dialogue preserved.
- `src/shop.c`: transient counter identity, unified buy-price helper, reset on exit/new counter, and test-only driver.
- `include/test/shop.h`: test-driver declarations, not used in normal ROM builds.
- `test/battle/items_gen3.c`: battle healing, multiplier, and accuracy regressions.
- `test/items_gen3.c`: field healing, vitamins, evolution routes and consumption callbacks.
- `test/shop_lilycove.c`: stock/script and actual shop menu transaction regressions.
- `docs/phase3b3c_items_shop_verification.md`: this report.

Pre-checkpoint implementation `git status --short`:

```text
 M data/maps/LilycoveCity_DepartmentStore_3F/scripts.inc
 M include/config/item.h
 M src/shop.c
?? docs/phase3b3c_items_shop_verification.md
?? include/test/shop.h
?? src/data/trainers.party.backup
?? test/battle/items_gen3.c
?? test/items_gen3.c
?? test/shop_lilycove.c
```

The trainer backup was already untracked and is untouched. This status records the implementation before the approved Git checkpoint.


## Save compatibility and manual checks

No migration is needed. Existing saves with Feather Badge immediately see the expanded list; those without it keep original stock. Item IDs, stack storage, money storage, and badge flags are unchanged. The pricing boolean is transient, not saved. Existing held items adopt the approved restored effects when used in battle.

Recommended in-game checks: load real saves before/after Winona, speak to the clerk, scroll through all 19 entries, buy/sell/cancel/leave/reenter, open another counter, save/reload, and complete one evolution animation for each route. Automated callback checks cover consumption and target scheduling, not animation presentation.

Protected trainer/AI/Frontier/story/encounter/breeding/EXP/evolution data, Summary Screen, Mega configuration, following Pokemon, shiny odds, Debug Mode, and trainer backup were not edited. No Phase 4 or Secret Base functionality was added.

### Evolution-stone stock follow-up

Appended Fire Stone, Water Stone, Thunder Stone, Leaf Stone, Moon Stone, and Sun Stone in the requested order. The Feather Badge stock now contains 19 entries; pre-badge stock remains seven. Extended the existing stock and price tests to cover all six stones and ordinary discount handling. Focused `Lilycove shop` regression rerun: 6 passed, 0 failed. `git diff --check` and `make -j4` passed. Build usage remains EWRAM 226,420 B, IWRAM 28,396 B, ROM 25,033,780 B; the existing RWX LOAD-segment linker warning remains. Broader regression results above precede this stock-only follow-up.

## Final checkpoint verification (2026-11-10)

Reran all 28 selections listed above against the complete 19-item implementation: **258 passed, 3 pre-existing failures, 4 assumption skips, 1 TODO; 266 total**. All seven new item tests and all six shop tests passed. Existing evolution, progression, breeding/daycare, Gen 3 battle, Shift/Set, and AI checks retain the results in the table. The three failing assertions match the previously documented files, lines, and messages; no new unexplained regressions appeared.

Verified both stock lists have exactly seven and 19 unique entries respectively, in the requested order. The left clerk and remaining NPC scripts compare identically with HEAD; no other counter scripts changed. Six trade-evolution overrides retain approved fixed prices, while all stones use global prices. No global price or resale data changed.

Final `git diff --check` and `make -j4` passed. Memory usage and the existing RWX linker warning remain as listed above. Only the eight implementation/testing/documentation files listed above are approved for staging; the trainer backup, Debug Mode, and generated artifacts are excluded. Manual presentation and real-save checks remain recommended as described above.
