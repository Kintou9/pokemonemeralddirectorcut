# Gen 3 spread damage and summary screen checkpoint

Verified November 3, 2026, against baseline `fc1d03dfcb`.

## Completed behavior

- Gen 3 spread damage uses occupied opposing positions for both-opponents moves, including positions protected, immune, or fainted during the same attack. All-field moves remain unreduced.
- Gen 3 spread rounding halves the base term before the final +2 and preserves the physical minimum.
- Summary Skills page cycles Stats / numeric IVs / numeric EVs with A.
- Overworld debug menu is enabled as a Start-menu entry.

## Verification

Commands used `DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro`.

| Command | Result |
| --- | --- |
| `make check TESTS='Spread restoration' -j4` | 8/8 passed, including AI/live parity and Gen 4 controls |
| `make check TESTS='Summary screen:' -j4` | 3/3 passed, including formatting, nature colors, column widths, and non-mutating cycling |
| `make check TESTS='test/battle/damage_formula.c' -j4` | 5 passed, 5 failed, 1 assumption failed; identical results on baseline |
| `make -j4` | ROM build succeeded |
| `git diff --check` | Passed |

Baseline was tested in an isolated filesystem copy with tracked changes restored to HEAD and the two new test sources removed. The damage-formula failures reproduced exactly:

- Gen5+ damage: expected 196, actual 292.
- Gen6+ Muscle Band/critical hit: expected 324, actual 326.
- Punching Glove: expected 204, actual 162.
- Gem damage: expected 240, actual 192.
- Transistor: unmatched animation.
- Marshadow/Mawile: type-matchup assumption failed.

All four existing spread-move fainting regression tests passed in both builds. The broader suite is not green; the failures above remain follow-up work. Full-suite testing and interactive visual inspection were not performed. Headless runs also emitted unrelated registration-time ASSUME diagnostics for Harvest, Pickup, and special_attack_up_3.
