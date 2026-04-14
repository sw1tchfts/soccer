# Soccer Legends

A Fortnite-styled 5v5 soccer game built with Unreal Engine 5, targeting Steam release.

## Overview

Soccer Legends is an online multiplayer soccer game featuring a cel-shaded Fortnite-inspired art style, 5v5 team matches, AI bots, and Steam P2P networking.

## Tech Stack

- **Engine:** Unreal Engine 5.5
- **Language:** C++ base classes + Blueprints
- **Networking:** Steam P2P via SteamSockets (listen server model)
- **Platform:** Windows (Steam)

## Project Structure

```
SoccerGame.uproject          -- UE5 project file
Config/                      -- Engine, game, and input configuration
Source/SoccerGame/
  Public/
    Types/SoccerTypes.h      -- Shared enums, structs, constants
    GameModes/               -- SoccerGameMode, SoccerGameState
    Player/                  -- SoccerCharacter, SoccerPlayerController, SoccerPlayerState
    Ball/                    -- SoccerBall (physics + replication)
    Field/                   -- SoccerGoal, SoccerPitchManager
    AI/                      -- SoccerAIController
    UI/                      -- SoccerHUD
  Private/                   -- Matching .cpp implementations
Content/                     -- Blueprint assets, meshes, materials, maps (created in editor)
```

## Core Systems

### Gameplay
- Third-person character with sprint, kick (tap = pass, hold = power shot), tackle, and goalkeeper dive
- Chaos physics ball with server-authoritative simulation
- Full match flow: countdown, first half, halftime, second half, overtime, post-match
- 1-1-2-1 formation (GK, DEF, 2 MID, FWD) with position-based AI
- Stamina system for sprinting

### Networking
- Server-authoritative game state (score, timer, ball, possession)
- Client prediction for character movement via CharacterMovementComponent
- Ball replicated at 60Hz with client-side interpolation
- Server RPCs for kicks, tackles, dives
- Multicast RPCs for cosmetic effects

### AI
- Position-specific decision logic (GK, DEF, MID, FWD)
- Utility-based decisions every 0.3s
- Configurable difficulty (speed, reaction time, accuracy)
- Bots fill empty team slots automatically

## Setup

### Prerequisites
- Unreal Engine 5.5 installed via Epic Games Launcher
- Visual Studio 2022 with C++ game development workload
- Steamworks SDK (bundled with UE5)
- Git LFS enabled (`git lfs install`)

### Getting Started
1. Clone the repository: `git clone <repo-url>`
2. Run `git lfs pull` to fetch binary assets
3. Right-click `SoccerGame.uproject` and select "Generate Visual Studio project files"
4. Open the generated `.sln` in Visual Studio or double-click `.uproject` to open in UE5 Editor
5. Build and run (F5 in VS, or Play in Editor)

### First-Time Editor Setup
1. Open the project in UE5 Editor
2. Create Blueprint subclasses for each C++ class (BP_SoccerCharacter, BP_SoccerBall, etc.)
3. Set up Enhanced Input actions and mapping contexts
4. Build the stadium level (L_Stadium_01) with pitch, goals, and pitch manager
5. Create UMG widget blueprints for HUD, menus, and scoreboard

## Controls

| Input | Action |
|-------|--------|
| WASD | Move |
| Mouse | Look / Aim |
| Left Shift | Sprint |
| Space | Jump |
| Left Click (tap) | Pass |
| Left Click (hold) | Power shot |
| E | Tackle / Slide |
| Q | Goalkeeper dive |
| Tab | Scoreboard |

## Build Phases

1. **Phase 1:** Project scaffold + character movement + ball physics (offline)
2. **Phase 2:** Match rules + AI bots (offline 5v5)
3. **Phase 3:** Online multiplayer networking (Steam P2P)
4. **Phase 4:** Fortnite visual polish + UI/menus
5. **Phase 5:** Steam integration + release packaging

## License

All rights reserved.
