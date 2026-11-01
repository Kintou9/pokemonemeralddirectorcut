# Phase 2E: Reproducible AI information and decision tests

## Commands and test-harness limit

```sh
DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS='Story AI fairness' -j4
DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS='Story AI decision' -j4
DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro make check TESTS='Frontier AI decision baseline' -j4
```

The battle-test build does not populate `gTrainers` for these story IDs. An
earlier version of the Phase 2E-1 tests read zero flags from that table, so
its 16 passing results established AI data preparation but **did not exercise
the intended trainer scoring profiles**. The tests now use explicit flag
expressions checked against the current `src/data/trainers.party` entries:

| Trainer ID | Test flag profile |
|---|---|
| `TRAINER_BRENDAN_ROUTE_103_TREECKO` | Basic Trainer |
| `TRAINER_ROXANNE_1` | Basic Trainer / HP Aware / Smart Switching |
| `TRAINER_NORMAN_1` | Basic Trainer / HP Aware / Smart Switching / Try To 2HKO / PP Stall Prevention |
| `TRAINER_WALLY_VR_1` | Basic Trainer / Smart Switching / Try To 2HKO |

Tests use these real profiles with synthetic parties and moves to isolate AI
behavior. They prove what an enhanced trainer *can* decide under its flags;
they do not prove that its current story party reaches every setup. Each
parameterized pair holds species, level, displayed status, moves, RNG seed
and other visible battle state fixed, except where noted below. A final action
is checked with `EXPECT_MOVE`, `EXPECT_SWITCH`, `gLastMoves`, or a recorded
cross-parameter result. Intermediate scores alone are never reported as a
decision leak.

## Confirmed final-action differences

| ID and hidden player value | Legitimately visible information held fixed | Final AI action in paired branches | Interpretation |
|---|---|---|---|
| Brendan Route 103, Speed 1/100 | First turn, same Wobbuffet species/HP; no turn order observed | Strength / Quick Attack | Exact unrevealed Speed influences move choice |
| Roxanne 1, Speed 1/100 | First turn, same species/HP; no turn order observed | Strength / Quick Attack | Same Speed effect under a Gym profile |
| Wally Victory Road 1, Speed 1/100 | First turn, same species/HP and move choices | Strength / Quick Attack | Same Speed effect under a late rival profile |
| Wally Victory Road 1, Defense 1/200 | Same species, level and HP bar | Strength / Surf | Hidden Defense influences move choice |
| Wally Victory Road 1, Special Defense 1/200 | Same species, level and HP bar | Thunderbolt / Strength | Hidden Special Defense influences move choice |
| Wally Victory Road 1, Defense IV 0/31 | Same species, level and full HP bar; stats recalculated after IV setter | Strength / Thunderbolt | Hidden IV affects a final move through actual Defense |
| Wally Victory Road 1, Defense EV 0/252 | Same species, level and full HP bar; stats recalculated after EV setter | Strength / Thunderbolt | Hidden EV affects a final move through actual Defense |
| Wally Victory Road 1, Lonely/Bold Nature | Same species, level and full HP bar; hidden nature and stats recalculated | Strength / Thunderbolt | Hidden Nature affects a final move through actual Defense |
| Norman 1, HP/max HP 40/80 vs 41/82 | Both show an exact 50% HP bar before the first turn | Dragon Rage / Yawn | Exact private HP changes the selected move at a fixed-damage KO threshold |
| Norman 1, remaining Surf PP 0/1 | Surf was used once into Protect; no damage or nonpriority turn order observed | Tackle / switch to Vaporeon | Actual remaining PP of a revealed move influences switching |
| Norman 1, no item/Oran Berry | Same full HP and no item activation or reveal | Splash / Trick | Unrevealed item presence influences move choice in this synthetic Trick setup |
| Norman 1, Magnet Pull/Sturdy | Same Magneton species; ability has not activated | Aron uses Toxic / Aron switches to Machop | Final action depends on the ability; this may be the legal trapping constraint rather than an unfair AI inference |
| Norman 1, Special Attack 1/200 | Surf revealed into Protect; no damage or nonpriority turn order observed | Tackle / switch to Vaporeon | Hidden Special Attack influences switching through damage simulation |

The HP test's Dragon Rage score also changes from 111 to 103 with the same
visible 50% bar; the separate Yawn test establishes the actual action change.
The PP pair starts at Surf PP 1/2, uses Surf once, then compares 0/1 remaining.
The item pair uses Trick and Splash solely to isolate the item-read branch;
neither is a claim about Norman's current party moves. Magnet Pull and Sturdy
are both valid Magneton abilities in this configuration. The trapping effect
has not been revealed before the AI's first switch decision, but a trapped
Pokémon must still obey the game rule. This pair needs a separate probe that
distinguishes an AI preference from a switch the engine forbids.

## Paired controls with no demonstrated final-action difference

| ID and varied value | Observed final action | Limit |
|---|---|---|
| Norman 1, Attack 1/200 after Brick Break is revealed into Protect | Tackle in both branches | Incoming-damage simulation can use hidden Attack, but this setup did not turn it into a choice change |
| Norman 1, reserve Zigzagoon HP 1/20, both healthy | Same chosen move in both branches | No reserve-HP decision effect shown; the healthy reserve is never switched in |
| Norman 1, no item/Oran Berry with Knock Off/Tackle | Knock Off in both branches | The Trick pair above is the positive item case |
| Norman 1, HP 40/80 vs 41/82 with Dragon Rage/Swords Dance | Dragon Rage in both branches | The KO score changes, but this particular alternative is not competitive |

`test/battle/ai/story_hidden_information.c` also retains paired state and
damage-simulation checks for exact Speed, Defense, Special Defense, Attack,
Special Attack, IV, EV, Nature, PP, item ID, ability and reserve status. The
reserve status pair there varies healthy/fainted. In an actual battle the
party indicator may expose a faint; that pair is **not** a clean observational
equivalence test. The new reserve-HP pair above keeps both reserves healthy.

An unchanged action in one setup does not establish that the variable cannot
affect decisions elsewhere. No final-action test yet proves use of healthy
reserve HP or hidden Attack. No target-selection leak is claimed. The
synthetic pairs do not test actual story scripts, healing-item use or full
trainer parties in game.

## Frontier regression baseline

`GetAiFlags` in `src/battle_ai_main.c` uses
`CHECK_BAD_MOVE | CHECK_VIABILITY | TRY_TO_FAINT` for non-Factory Frontier
battles. `test/battle/ai/story_decision_hidden_info.c` fixes this profile and
RNG to assert a final Quick Attack choice and a replacement switch to party
slot 3. The older synthetic Frontier-profile damage test remains in
`story_hidden_information.c`. These paths share AI data preparation, damage
simulation, move scoring and switch helpers with story trainers.

The recorded-battle harness injects AI flags and synthetic parties. It cannot
reproduce Frontier trainer selection, Battle Factory's separate flags,
facility rules or generated teams through these tests. Full Frontier behavior
remains an in-game or dedicated facility-harness regression requirement before
changing shared AI functions.

## Narrow correction candidates for later approval

- **Speed and exact HP:** for story-trainer decision scoring, use observed
  turn-order bounds and visible HP-bar intervals for priority/KO evaluation.
  Keep actual battle execution calculations unchanged.
- **Stats, IVs, EVs and Nature:** use species/level and observed-damage
  estimates when story AI simulates damage. Proven final-action cases are
  Defense, Special Defense, Defense IV, Defense EV, Nature and Special Attack.
  Hidden Attack remains a simulation dependency without a proven action change.
- **PP:** for story AI switching, track observed uses of a move and treat
  unknown starting PP conservatively. The zero/one remaining Surf pair proves
  a switch difference.
- **Item:** gate story scoring on recorded reveals or a conservative unknown
  item state. The Trick pair proves an item-presence decision difference.
- **Ability:** defer a production correction until a test separates the AI's
  preemptive preference from the engine's legal Magnet Pull switch constraint.
  Preserve trapping rules and known effects once shown.
- **Reserve state:** defer a production change until a clean final-action
  reproducer exists. Keep the visible party count and observed faints distinct
  from private reserve HP, moves, items and abilities.

Each candidate should be gated to approved story trainer IDs or an explicit
story-only AI mode, with a non-Factory Frontier and Factory regression check.
No production correction is implemented in this phase.
