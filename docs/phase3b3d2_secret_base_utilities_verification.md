# Phase 3B-3D2: movable Secret Base utilities

Baseline: `6e72f931ad0c3aae6b7e69753733107fe6a2e4eb`, branch `emerald-enhanced-classic`. Initial verification date: 2026-11-14. Historical pre-checkpoint status and pending manual checks below are superseded by the final checkpoint section.

## Gameplay implementation

| Decoration | ID | Category | Footprint | Price |
| --- | ---: | --- | --- | ---: |
| Healing Bed | 121 | Ornaments | 2×2 | Free |
| Pokémon Storage PC (menu name STORAGE PC) | 122 | Ornaments | 1×2 | Free |

Both utilities are supplied automatically when the player opens the decoration menu in their own Secret Base. `EnsureSecretBaseUtilities` adds one missing copy of each to the existing Ornaments inventory; existing copies (including placed copies) are not duplicated. No purchase, money, badge condition, or National Dex is required. Fortree retains its original eight desk entries, and the utility definition prices are zero. The 30-slot Ornaments inventory and 16-placement limit are unchanged. If Ornaments is full, existing possessions are preserved; missing utilities are retried on the next menu opening after space is freed. Tossing a utility allows a replacement to be granted on reopening the menu.

Utilities use solid-floor placement and the existing bottom-left anchor/packed-coordinate format. `GetSecretBaseUtilityScript` matches a live saved decoration footprint, the actual utility metatile, elevation, Secret Base map type, and owned-base index 0 with a nonzero base ID. This lookup runs after object-event and background-event resolution. It has no cached interaction coordinates. Moving/removing furniture changes the matching saved slots; the old location no longer grants an interaction. Other players' bases cannot activate either utility. The fixed management PC remains independent.

The bed's script calls `HealSecretBaseParty`, which calls `HealPokemon` for each non-egg party member. HP, status and all four move PP are restored, including fainted members; PP bonuses are preserved. Eggs are left byte-for-byte unchanged. The wrapper does not invoke `HealPlayerParty`, heal boxes, recharge a gimmick, write story flags, or change the last healing location. The script uses the existing follower refresh callback after healing; no following-Pokémon implementation is changed.

The PC's script calls the existing `ShowPokemonStorageSystemPC` special, whose declaration supplies the script wait. It uses the same party/box data, menu restrictions, and transfer code as every standard storage PC. `CB2_ExitPokeStorage` returns to the field through `FieldTask_ReturnToPcMenu`; leaving that menu resumes the utility script, plays the off sound, releases objects, and ends the script to unlock field controls. The general PC menu and fixed Secret Base PC effects are not invoked. All modifications to `src/pokemon_storage_system.c` are inside `#if TESTING`. The no-longer-needed decoration-shop driver changes in `src/shop.c` and `include/test/shop.h` were removed.

## Graphics and allocation

The original shared secondary metatile table had 324 entries. Twelve new entries occupy IDs 0x344–0x34F: four bed quadrants and the two-part storage PC, each with the paired variant expected by the decoration renderer. Original table bytes and attributes are preserved exactly. New entries use the existing floor underlay and impassable/covered attributes. They do not reuse the fixed PC ID or its behavior.

`src/data/tilesets/secret_base_utilities.h` contains the original Weather Institute bed's 16 native 4bpp tiles plus eight tiles for the original Building PC. Twenty-four tiles are appended after the original 83 secondary tiles in each of the six Secret Base variants. Existing PNG atlases are unchanged. Bed graphics are copied byte-for-byte from Lab tile indices 0x107/108/117/118, 0x109/10A/119/11A, 0x127/128/137/138, and 0x129/12A/139/13A. The original Lab palette 07 is reused through palette references in all six variants and the decoration-preview palette table. Background palette slot 7 was unused by the original Secret Base metatiles; the PC uses the original Building palette 00 in previously unused background palette slot 8 (including the preview palette table). Placement previews and inventory icons use the existing metatile-derived icon path; `CopyTile` selects the secondary atlas for secondary tile indices.

A native-art preview was rendered and visually inspected. Headless tests verify identical utility tile data and preview copying for all six tilesets. A host-side audit checked all 24 Secret Base layouts: original map tiles do not reference the newly appended IDs; all normal floor coordinates fit the existing four-bit coordinate fields; each layout has valid floor pairs for the bed and floor cells for the PC. Some layouts include a seventeenth boundary row/column, but their usable floor anchors remain representable. This is an allocation/layout audit, not a complete visual playthrough of every room.

## Automated coverage

`make check TESTS='Secret Base utilities:' -j4`: **14 passed, 0 failed, 0 skipped** (parameterized cases run within test titles).

Coverage includes:

- Definition IDs, free prices, categories, shapes, and unchanged original Fortree stock.
- Free grants on existing-save inventory states, repeat menu openings without duplication, no charge, placed-copy preservation, own-base gating, full inventory preservation, and retry after freeing space.
- Real placement validation and save-slot writing, collision, boundary rejection, overlap rejection, and the 16-slot limit.
- All four bed tiles, all four facing directions through the real interaction dispatcher, elevation, own/foreign/non-base maps, relocation and removal.
- Object-event and background-event precedence, and the existing fixed PC script.
- Party HP/status/all move PP restoration, fainted members, repeated healing, PP bonuses, unchanged eggs, boxes, story flags and respawn location.
- Reconstruction from copied saved placement records and identical rendering/interaction across all six tileset variants.
- Moving out of a base clears placements while retaining purchased decorations.
- Unknown decoration ID rejection/sanitization and safe foreign-decoration sprite initialization.
- Real storage menu errors for full parties and last Pokémon, script resumption and control release on exit, launch/return callback wiring for all three Pokémon operations.
- Existing storage transfer primitives for deposit, withdrawal and box-to-box movement; the real full-box early-rejection path and last-live-party-member guard.

TESTING-only drivers omit full map rendering and storage animation. They call existing production functions rather than implementing alternate placement, payment, or storage logic. Save/reload tests reconstruct saved placement records in memory; flash serialization and disk-save import are not exercised.

Fixture issues resolved during verification: initialized background state for the headless storage menu; selected menu options using the existing configurable order; calculated Pokémon stats before asserting healing/live-party behavior. A 2,076-byte map fixture initially occupied IWRAM and caused battle-runner heap assertions during broader checks. Moving that test-only fixture to EWRAM restored the affected battle tests. The incomplete pre-fix runs are not counted as successful verification.

## Save compatibility and unknown IDs

No persistent structures, original IDs, inventory capacities, map layouts, or save fields changed. Existing saves load without migration. Free grants and placements use existing arrays; no save preferences change.

Unsupported IDs are rejected by decoration inventory APIs and map drawing. Player inventories and owned-base/player-room placement lists clear unsupported IDs before menu indexing (placement coordinates are cleared with invalid placed IDs). Foreign decoration sprite initialization skips unsupported IDs, and rendering/sprite initialization rejects an out-of-range current-base index. Valid original IDs and new IDs 121–122 are retained.

This protects the updated ROM, not old ROMs. Older builds do not understand 121–122 and may index outside their decoration tables. Cross-version record mixing and loading an updated save in an older ROM are not guaranteed safe. No record-mixing protocol or multiplayer behavior was rewritten; same-version bases can display utilities, but only the owner can activate them.

## Manual checks still required

Use representative real saves to open Decorate, confirm both utilities appear under Ornaments, place both, save/reload, put away/reposition them, move bases, and open/close storage repeatedly. Inspect floor compositing, palette appearance, scrolling, and selection icons in cave/tree/shrub interiors. Complete animated deposit/withdraw/box organization and return to overworld movement. Verify follower presentation after reviving the lead Pokémon, and a received base's trainer interaction. Automated tests cover callback/data behavior, not the full graphical lifecycle or flash save system.

Protected trainer/AI/battle/Frontier/progression/breeding/evolution/story/encounter/Summary/follower/shiny/Mega/Phase 4 systems are unchanged. Debug Mode and `src/data/trainers.party.backup` are untouched.

## Broader regression results before the free-availability follow-up

Commands use `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro`:

- `make check TESTS='Secret Base utilities:' -j4`
- `make check TESTS='<selection below>' -j4` for each of the 28 baseline selections.
- `git diff --check`
- `make -j4`
- `git status --short`

Corrected baseline rerun: **258 passed, 3 documented pre-existing failures, 4 assumption skips, 1 TODO (266 total)**. Including the 13 utility tests: **271 passed, 3 existing failures, 4 skips, 1 TODO (279 total)**. No new unexplained regression remains. The full repository suite was not run.

| Selection | Passed | Failed | Total |
| --- | ---: | ---: | ---: |
| `Gen3 items:` | 7 | 0 | 7 |
| `Lilycove shop:` | 6 | 0 | 6 |
| `test/battle/item_effect/restore_hp.c` | 11 | 0 | 11 |
| `test/battle/hold_effect/type_power.c` | 2 | 0 | 2 |
| `test/battle/hold_effect/restore_hp.c` | 5 | 0 | 7 |
| `Gen3 breeding:` | 12 | 0 | 12 |
| `test/daycare.c` | 38 | 0 | 38 |
| `Gen3 progression:` | 9 | 0 | 9 |
| `test/battle/exp.c` | 9 | 0 | 9 |
| `test/battle/move_effect/return.c` | 3 | 0 | 3 |
| `test/battle/move_effect/frustration.c` | 3 | 0 | 3 |
| `test/battle/hold_effect/friendship_up.c` | 0 | 0 | 1 |
| `Gen3 turn order:` | 10 | 0 | 10 |
| `Gen3 faint replacement:` | 11 | 0 | 11 |
| `Shift style:` | 9 | 0 | 9 |
| `Spread restoration` | 8 | 0 | 8 |
| `test/battle/spread_moves.c` | 27 | 1 | 30 |
| `Story AI` | 29 | 0 | 29 |
| `Frontier AI decision baseline` | 2 | 0 | 2 |
| `Story public-info` | 5 | 0 | 5 |
| `AI calculation context parity` | 3 | 0 | 3 |
| `AI calculation adapter parity` | 3 | 0 | 3 |
| `AI trace:` | 7 | 0 | 7 |
| `test/battle/multi_battle_whiteout.c` | 8 | 0 | 8 |
| `test/battle/evolution_tracker.c` | 8 | 0 | 8 |
| `Summary screen:` | 3 | 0 | 3 |
| `test/battle/form_change/mega_evolution.c` | 13 | 1 | 14 |
| `test/battle/status1/paralysis.c` | 7 | 1 | 8 |

Unchanged failures:

- `test/battle/spread_moves.c:324`: Eiscue/Mimikyu/Eject Button unmatched Ice Face popup, the previously diagnosed Gen 3 physical/special fixture mismatch.
- `test/battle/form_change/mega_evolution.c:118`: Mega Sableye ability-replacement fixture, unmatched Celebrate message.
- `test/battle/status1/paralysis.c:119`: Thunder Wave avoided-attack message mismatch.

Skips: two spread targeting assumptions and two percentage-Sitrus assumptions. Existing Soothe Bell TODO remains. Registration also prints the existing unrelated Harvest/Pickup/special_attack_up_3 assumption diagnostics. Corrected-run logs contain neither heap assertions nor empty selections.

`make -j4` and `git diff --check` passed. Existing linker warning: `pokeemerald.elf has a LOAD segment with RWX permissions`. No compiler warnings in the final build.

| Region | Used | Utilization | Change from baseline |
| --- | ---: | ---: | ---: |
| EWRAM | 226,420 B | 86.37% | unchanged |
| IWRAM | 28,396 B | 86.66% | unchanged |
| ROM | 25,040,020 B | 74.63% | +6,240 B |

## Free-availability follow-up (2026-11-14)

The user requested immediate decoration-menu availability rather than late-game purchases. Updated acquisition as described above, replaced the two obsolete purchase tests with free-grant tests, and removed unused decoration-shop test hooks. The current 13 utility tests passed; the six Lilycove shop tests were rerun separately. The 28-selection broader baseline above predates this narrow acquisition change and was not repeated for it. `make -j4` passed; memory usage in the table is updated for this follow-up, with the same existing RWX warning. The playtest ROM was refreshed without replacing its save.

## Exact changed files and final status

The following status lists every changed/new implementation, asset, test and documentation file. `src/data/trainers.party.backup` was already untracked and is excluded from the work. Nothing is staged, committed or pushed.

```text
 M data/maps/FortreeCity_DecorationShop/scripts.inc
 M data/scripts/shared_secret_base.inc
 M data/tilesets/secondary/secret_base/metatile_attributes.bin
 M data/tilesets/secondary/secret_base/metatiles.bin
 M include/constants/decorations.h
 M include/constants/metatile_labels.h
 M include/secret_base.h
 M src/data/decoration/header.h
 M src/data/decoration/tiles.h
 M src/data/tilesets/graphics.h
 M src/data/tilesets/headers.h
 M src/decoration.c
 M src/decoration_inventory.c
 M src/field_control_avatar.c
 M src/pokemon_storage_system.c
 M src/secret_base.c
?? docs/phase3b3d2_secret_base_utilities_verification.md
?? include/test/secret_base.h
?? src/data/tilesets/secret_base_utilities.h
?? src/data/trainers.party.backup
?? test/secret_base_utilities.c
```

## Bed rest presentation follow-up

The Healing Bed now uses Emerald's out-of-center rest presentation: `fadescreenswapbuffers FADE_TO_BLACK`, healing fanfare and wait, party-only healing/follower refresh, then `fadescreenswapbuffers FADE_FROM_BLACK`. The final two-line message reads “You and your POKéMON / are fully rested!” No nurse routine, box healing, or respawn update is called. The 13 utility tests, `git diff --check`, and `make -j4` passed; the existing RWX warning remains. This script-only change still needs visual confirmation of the fade/jingle/message in the graphical emulator. The playtest ROM was refreshed without changing its save.

## Original Weather Institute bed follow-up (2026-11-15)

Per the screenshot request, the bed now reuses the original Weather Institute artwork and colors with its actual 2×2 footprint. Interaction first says “There's a bed… / Let's take a rest.” After dismissing that prompt, the existing fade/jingle/heal/fade-back sequence runs and confirms that the player and Pokémon are fully rested.

The footprint test now explicitly rejects a bed overlapping the management PC, then verifies valid placement after clearing that test obstacle. A new reload test ensures the expanded bed cannot overwrite the fixed PC or draw outside a layout. An incompatible placement from a prior narrow-bed playtest is left owned and put-awayable but not newly drawn over an obstacle; put it away and reposition it. Reenter the base to reconstruct the map if an older save's cached map view still shows old tiles.

Current utility suite: **14 passed, 0 failed, 0 skipped**. A host audit confirmed valid 2×2 floor spaces in all 24 layouts and byte-identical bed graphics to the original Weather Institute source. `git diff --check` and `make -j4` passed, with the existing RWX warning only. Current memory figures are in the table above. The broader 28-selection results predate this presentation/footprint follow-up. Graphical confirmation of the bed, colors and rest sequence is still an in-game check. The test ROM was refreshed; its save was not replaced. No commit or push.

## Matching original PC artwork (2026-11-15)

The movable Storage PC now uses the original Building terminal artwork shown above the Weather Institute bed in the supplied screenshot. Tiles 0x15/16/25/26/35/36 and blank lower tiles are copied byte-for-byte from the existing atlas; the original Building palette 00 is referenced in background slot 8 for all Secret Base variants and the preview table. Original Secret Base metatiles did not use slot 8. The PC now has a 1×2 footprint, and both parts resolve to the standard storage script. The existing graphics reload safeguard also protects incompatible earlier PC placements; put away and reposition an older PC and reenter the base if its cached map view is stale.

The 14 utility tests, `git diff --check`, and `make -j4` passed. The existing RWX linker warning remains. Memory figures above are current. Broader baseline results were not rerun for this graphics/footprint change. The native PC tile preview was inspected; the in-game appearance and storage transition still need visual playtesting. Test ROM refreshed without replacing the save. No commit or push.


## Follow-up: Fly to the player's Secret Base

Requested behavior implemented in `src/secret_base.c`, `include/secret_base.h`,
`src/region_map.c`, and `include/region_map.h`, with four deterministic tests in
`test/secret_base_fly.c`. This follow-up also updates this report.

The ordinary Pokémon Fly map displays a selectable, blinking SECRET BASE marker
on the owner's route. It lands one tile south of the actual entrance, using the
normal Fly animation and warp path. Normal Fly badge, party move, and departure
map restrictions remain in place. This does not add flight from indoors or a
PokéNav shortcut. Existing town destinations are unchanged.

The destination is derived from saved owner slot 0 and original map background
events each time: moving the base moves the destination, packing up removes it,
and mixed-record friends' bases never become destinations. Zero/unknown IDs fail
closed. Every one of the 75 entrances has an in-bounds, collision-free landing
coordinate. Entry from outdoors preserves normal dynamic exit-warp setup; no
respawn location, save fields, flags, or decoration inventory are added/changed.
The marker uses the nearest matching route cell to the entrance's scaled map
position, including irregularly shaped routes.

Verification after the final Fly edit:

- `make check TESTS='Secret Base' -j4`: 18 passed, 0 failed, 0 skipped, 0 TODO
  (4 Fly tests plus all 14 existing utility tests). The 75-location test also
  checks the actual selected warp via `SetFlyDestination`/`ApplyCurrentWarp`.
- Independently: 4/4 Fly tests and 14/14 utility tests passed.
- `git diff --check`: passed.
- `make -j4`: passed. Existing linker LOAD-segment RWX warning only.
- EWRAM 226,420 B (86.37%); IWRAM 28,396 B (86.66%);
  ROM 25,040,980 B (74.63%).
- The runner prints unrelated discovery-time ASSUME diagnostics for Harvest,
  Pickup, and special_attack_up_3; none of the selected tests failed or skipped.
- Broader battle baseline above was not rerun for this isolated field-map change.
- No commit or push. Existing utility work, local Debug Mode, and trainer backup
  remain intact. The playtest ROM was refreshed without changing its `.sav`.

Remaining manual verification: load the updated ROM normally and Continue (avoid
an older emulator save state), use Pokémon-menu Fly outdoors, select SECRET BASE,
check the arrival animation, enter/exit, and verify the marker after moving the
base and after saving/reloading. Automated tests verify destination selection and
state behavior, not rendered animation or emulator save-file round-trips.


## Final Phase 3B-3D checkpoint (2026-11-15)

The player confirmed successful in-game testing of the Healing Bed and Pokémon
Storage PC, Fly arrival at the base, destination updates after establishing a
new base, disappearance after packing up, and persistence after saving/reloading.
These confirmations supersede the corresponding pending manual checks above.

Final review includes only Secret Base utilities, decoration graphics and safety
checks, Fly integration, associated test drivers/tests, and this documentation.
The Fortree script change only exports its existing inventory label for regression
testing; stock is unchanged. Both binary tables preserve their complete original
byte prefixes and append only utility entries. No Debug Mode changes, trainer
backup, ROMs, or generated build artifacts are included.

Final checkpoint results: combined Secret Base selection **18 passed, 0 failed,
0 skipped, 0 TODO** (14 utility tests and 4 Fly tests); `make -j4` passed.
Existing RWX linker warning only. EWRAM 226,420 B, IWRAM 28,396 B, ROM
25,040,980 B. The staged diff must pass `git diff --cached --check` before commit.
No subsequent development phase is part of this checkpoint.
