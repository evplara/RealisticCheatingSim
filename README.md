# Realistic Cheating Simulator
Play on itch.io !!!
https://koolaid77.itch.io/realistic-cheating-sim

<img width="1279" height="959" alt="Screenshot 2026-03-07 221713" src="https://github.com/user-attachments/assets/cb8169b7-851b-4dbc-818a-3604f822c6b9" />

Top-down classroom stealth roguelike using Raylib.

## Controls

- `Enter`: start run / continue screens
- `Esc`: quit from main menu
- `W A S D`: move one tile, hold to keep moving
- `Space` (hold): peek nearby student answers, or submit when on submit tile with full quiz progress
- `1 2 3`: cast unlocked abilities instantly during gameplay
- `4`: use `Distraction` if you have weekly charges
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


## Ability Guide

- `Whisper`: unlockable reward ability on `1`. Instantly copies an answer from the nearest adjacent student, consuming that student for the rest of the week. Gives more quiz progress from `Helpful` students and much less from `Snitchy` students, but also adds suspicion. Cooldown: `12s`.
- `Fake Cough`: unlockable reward ability on `2`. Only works if the teacher is not already looking at you. It pulls the teacher into an `Investigate` state at your current tile for `2.5s`, buying room elsewhere but adding a big suspicion spike. Cooldown: `18s`.
- `Mirror`: unlockable reward ability on `3`. Activates for `5s` and lets your normal peek reach students within `2` tiles even through one blocker. Useful for reading around desks or walls without repositioning. Cooldown: `20s`.
- `Distraction`: used with `4`. You start each week with `1` charge, and the matching reward adds `+1` more weekly charge. It instantly turns the teacher to face away from you, spends one charge, and adds a small amount of suspicion.

## Attributes

Caught sound effect, "caught.wav" from https://opengameart.org/content/wrong-choice-caught by DavidW

Licensed under CC-BY 4.0

Abiltiy Sound Effect, "ability.wav" from https://opengameart.org/content/7-assorted-sound-effects-menu-level-up sound effect 3 "Item Pickup" by Joth

Peek sound effect, "peek.wav" from https://opengameart.org/node/132692 by NachtmahrTV 
