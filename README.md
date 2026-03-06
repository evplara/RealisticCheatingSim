# School 7DRL

Top-down classroom stealth roguelike prototype using Raylib.

## Build

```powershell
mingw32-make
.\game.exe
```

## Controls

- `Enter`: start run / continue screens
- `Esc`: quit from main menu, close ability mode
- `W A S D`: move one tile
- `Space` (hold): peek nearby student answers, or submit when on submit tile with full quiz progress
- `Tab`: toggle ability mode pause
- `1 2 3`: select unlocked ability in ability mode
- `Enter` (in ability mode): cast selected ability
- `F3`: debug overlay

## Gameplay Loop

- Each run has 3 escalating weeks.
- Fill quiz progress to `100`, then reach and hold on `SUBMIT`.
- If teacher sees cheating actions for at least `0.2s`, run ends (permadeath).
- Abilities unlock by week:
- Week 1: `Whisper`
- Week 2: `Fake Cough`
- Week 3: `Mirror`

## Audio

Placeholder audio files live in `resources/audio/`.
If files are missing or invalid, gameplay continues without audio cues.
