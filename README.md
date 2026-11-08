# eris
A top-down sci-fi arena shooter with bullet-hell enemy patterns. Survive waves of enemies that pour in through the four gates, pick up weapon drops, and take down the Overmind boss every fifth wave.

## Building

Requires Visual Studio 2026 with the "Desktop development with C++" workload. raylib 5.5 is downloaded automatically when CMake configures the project.

- **Open Folder:** open this folder in Visual Studio, pick the `x64 Debug` or `x64 Release` preset, and run `eris.exe`.
- **Solution:** `cmake --preset vs2026` generates `out/build/vs2026/eris.slnx`.

## Controls

| Input | Action |
| --- | --- |
| WASD | Move |
| Mouse / LMB (or Space) | Aim / fire |
| Arrow keys | Fire in 8 directions |
| 1–9, 0 / mouse wheel / Q, E | Switch weapon |
| Esc / P | Pause |

## Weapons

| # | Weapon | Notes |
| --- | --- | --- |
| 1 | AR-15 | Default, unlimited ammo |
| 2 | Plasma Rifle | Energy bolts that pierce 3 targets |
| 3 | Quantum Entangler | Fires an entangled pair that spirals around each other |
| 4 | Singularity Launcher | Single shot. Collapses into a black hole that pulls in enemies and swallows enemy fire |
| 5 | Flamethrower | Short-range cone, continuous damage |
| 6 | Rocket Launcher | Accelerating rockets with area damage |
| 7 | Minigun | Spins up to a very high rate of fire, slows you down |
| 8 | Dual Uzis | Fires alternately from each hand |
| 9 | Chainsaw | Continuous melee, shreds incoming bullets |
| 0 | Energy Sword | Wide melee swing that deflects enemy fire back |

Other drops: phase shield (SH), hull repair (HP), extra ship (1UP).
