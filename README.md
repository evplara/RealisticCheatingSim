# Realistic Cheating Simulator
Play on itch.io !!!
https://koolaid77.itch.io/realistic-cheating-sim

<img width="1279" height="959" alt="Screenshot 2026-03-07 221713" src="https://github.com/user-attachments/assets/cb8169b7-851b-4dbc-818a-3604f822c6b9" />

Top-down classroom stealth roguelike using Raylib.

## Controls

- `Enter`: start run / continue screens
- `Esc`: quit from main menu, close ability mode
- `W A S D`: move one tile, hold to keep moving
- `Space` (hold): peek nearby student answers, or submit when on submit tile with full quiz progress
- `Tab`: toggle ability mode pause
- `1 2 3`: select unlocked ability in ability mode
- `4`: use `Distraction` in ability mode if you have weekly charges
- `Enter` (in ability mode): cast selected ability
- `F3`: debug overlay

## Gameplay Loop

- Each run has 10 escalating weeks.
- Fill quiz progress to `100`, then reach and hold on `SUBMIT`.
- If teacher sees cheating actions for at least `0.2s`, run ends (permadeath).
- If teacher directly spots you from 1 tile away, the run ends immediately.
- After each cleared week, draft 1 of 3 random rewards.
- The reward pool mixes 3 stat upgrades with 3 ability unlocks.
- Abilities start locked and only unlock if you choose them.

## Run Upgrades

- `Long Reach`: +1 peek range
- `Quiet Hands`: -20% suspicion gained on peeks
- `Quick Study`: +3 quiz progress per peek

## Tricks

- `Distraction`: starts with 1 weekly charge to force the teacher to face away

## Attributes

Caught sound effect, "caught.wav" from https://opengameart.org/content/wrong-choice-caught by DavidW

Licensed under CC-BY 4.0

Abiltiy Sound Effect, "ability.wav" from https://opengameart.org/content/7-assorted-sound-effects-menu-level-up sound effect 3 "Item Pickup" by Joth

Peek sound effect, "peek.wav" from https://opengameart.org/node/132692 by NachtmahrTV 
