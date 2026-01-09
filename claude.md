# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Helbreath 3.82 is a classic MMORPG client-server application written in C++ for Windows. The project consists of two main components that communicate via a custom network protocol:

- **Client (Game)**: DirectX-based game client with DirectDraw rendering, DirectInput, and DirectSound
- **Server**: Multi-threaded game server managing game logic, player sessions, NPCs, and world state

## Project Structure & Module Organization

- `Sources/Client/`: C++ game client (DirectDraw/DirectInput/DirectSound)
- `Sources/Server/`: C++ game server (networking, AI, world state)
- `Dependencies/Shared/`: Headers shared between client and server (protocol IDs, shared constants)
- `Dependencies/Client/`: DirectX SDK headers and legacy libraries
- `Binaries/Game/`: Client runtime content (configs, dialogs, assets)
- `Binaries/Server/`: Server configs and data files
- `PLANS/`: Implementation plans for significant changes
- `Helbreath.sln`: Solution entry point; **always build from here** to resolve shared includes

## Build Instructions

### Building with Visual Studio

The project uses Visual Studio 2022 (v143 platform toolset) and requires Windows SDK 10.0.

**CRITICAL: Always build from the solution level, not individual project files.**
Building from the solution level ensures shared include paths (`Dependencies/Shared/`) are properly resolved.

**Build early and often:** After each logical change or file group, re-run the appropriate build to catch breakage quickly.

**Build both projects:**
```bash
# From the Helbreath-3.82 directory
msbuild Helbreath.sln /p:Configuration=Release /p:Platform=x86
# or
msbuild Helbreath.sln /p:Configuration=Debug /p:Platform=x86
```

**Build client only (from solution):**
```bash
msbuild Helbreath.sln /t:Game /p:Configuration=Release /p:Platform=x86
```

**Build server only (from solution):**
```bash
msbuild Helbreath.sln /t:Server /p:Configuration=Release /p:Platform=x86
```

**Note:** `msbuild` should be available in PATH. If using PowerShell, invoke directly: `msbuild Helbreath.sln ...`

**Output locations:**
- Client: `Debug/Game.exe` or `Release/Game.exe`
- Server: `Debug/Server.exe` or `Release/Server.exe`

**Build Logs:**
If build issues require user verification beyond agent-run builds, capture output in `server_build.log` and `client_build.log`.

**Important Notes:**
- Both projects target **Win32 (x86)** architecture only
- **DO NOT** build from project directories (`Sources/Client/` or `Sources/Server/`) as this breaks shared include path resolution
- Shared headers in `Dependencies/Shared/` (ActionID.h, DynamicObjectID.h, NetMessages.h) require solution-level builds
- When adding new source or header files, update the MSVC project (`.vcxproj`) so the files are included in the build

### Debug vs Release Builds

**Debug build characteristics:**
- No optimization
- Edit & Continue enabled
- Full debug symbols
- Runtime library: `MultiThreadedDebugDLL` (client) / `MultiThreadedDebug` (server)

**Release build characteristics:**
- Max speed optimization
- Inline expansion
- String pooling and function-level linking
- No debug info (client), limited symbols (server)
- Runtime library: `MultiThreadedDLL` (client) / `MultiThreaded` (server)

## Architecture

### Client Architecture (Sources/Client/)

The client is built around a monolithic `CGame` class (`Game.cpp`, ~1.7MB) that handles:

- **Rendering Pipeline**: DirectDraw 2D rendering with sprite system (`Sprite.h`, `Tile.h`, `MapData.h`)
- **Input Handling**: DirectInput for keyboard/mouse (`DXC_dinput.cpp`, `MouseInterface.h`)
- **Audio System**: DirectSound wrapper (`DXC_dsound.cpp`, `YWSound.h`, `SoundBuffer.h`)
- **Network Communication**: Custom socket wrapper (`XSocket.h`) with async Windows message-based events
- **Game State**: Character info, inventory, skills, magic system (`CharInfo.h`, `Item.h`, `Magic.h`, `Skill.h`)
- **UI System**: Custom dialog and menu rendering integrated into Game.cpp

**Key Client Classes:**
- `CGame`: Main game loop, rendering, input, and state management
- `XSocket`: Asynchronous socket wrapper using Windows message pump (WM_USER_GAMESOCKETEVENT)
- `CSprite`: Sprite rendering and management
- `CMapData`: Map/tile data and collision detection
- `CItem`: Item properties and handling

**Client Dependencies:**
- DirectX SDK headers and libs in `Dependencies/Client/`
- Legacy libraries: `ddraw.lib`, `dinput.lib`, `dsound.lib`, `dxguid.lib`
- Image processing: `cximage` and `jpeg` libraries

### Server Architecture (Sources/Server/)

The server uses a client-manager pattern with extensive game systems:

- **Connection Management**: `CClient` class represents connected player sessions
- **Game Logic Core**: `CGame` class (~2.1MB) orchestrates all game systems
- **Entity Systems**: NPCs (`Npc.h`), dynamic objects (`DynamicObject.h`), items, fish, minerals
- **World Management**: Multiple maps (`Map.h`), teleport locations, strategic points
- **Event System**: Delayed events (`DelayEvent.h`), quests (`Quest.h`), crusades
- **Database**: ODBC-based persistence for player accounts and characters
- **Party System**: Player grouping via `PartyManager.h`

**Key Server Classes:**
- `CGame`: Central game coordinator managing all systems
- `CClient`: Player session with inventory, stats, magic, skills, and connection state
- `CNpc`: NPC behavior, AI, and spawning
- `CMap`: Tile-based world, collision, visibility, and entity tracking
- `XSocket`: Network layer matching client protocol

**Server Limits (Game.h:52-99):**
```cpp
#define DEF_MAXCLIENTS              2000
#define DEF_MAXNPCS                 5000
#define DEF_MAXMAPS                 100
#define DEF_MAXITEMTYPES            5000
#define DEF_MAXDYNAMICOBJECTS       60000
```

### Network Protocol (NetMessages.h)

The client and server share a custom binary protocol with message IDs defined in `NetMessages.h`. Both sides must have synchronized message definitions.

**Protocol Structure:**
- Header-based framing with message type + size
- Binary packed structures
- Message IDs like `MSGID_COMMAND_MOTION (0x0FA314D5)`, `MSGID_EVENT_COMMON (0x0FA314DB)`
- Common types for game actions: item drop, equip, magic, combat, guilds, etc.

**Socket Implementation:**
- `XSocket` class uses Windows async sockets
- Event-driven with `WSAAsyncSelect`
- Buffered send/receive with block limits
- Client uses `WM_USER_GAMESOCKETEVENT`, server uses `DEF_SERVERSOCKETBLOCKLIMIT`

### Shared Systems

Both client and server share parallel implementations of:
- **Item System**: Item types, properties, crafting (`Item.h`, `BuildItem.h`)
- **Magic System**: Spell definitions and effects (`Magic.h`)
- **Skill System**: Character skills and training (`Skill.h`)
- **Action IDs**: Synchronized animation and action constants (`ActionID.h`)
- **Dynamic Object IDs**: Shared object type definitions (`DynamicObjectID.h`)

## Code Conventions

- **Indentation**: Tabs; braces use Allman style
- **Naming**: Hungarian notation (`m_` for members, `p` for pointers, `i` for int, `sz` for strings)
- **Classes**: Pascal case with `C` prefix (CGame, CClient, CNpc)
- **Headers**: Use `#pragma once` for include guards
- **Constants**: `DEF_` prefix for defines, all caps
- **Memory**: Manual new/delete, no smart pointers
- **Strings**: Mix of C-style char arrays and some C++ strings
- **Comments**: Korean comments present in server code

## Testing Guidelines

**No automated test framework is present.** All testing is manual.

- **Smoke Testing**: Validate changes by running server then client using configs in `Binaries/Server/` and content in `Binaries/Game/`
- **Build Validation**: For extensive code changes, stop once a build passes and request the user to run manual tests before proceeding further
- **Network Testing**: For network communication changes, verify message IDs, payload layout, and handler symmetry between client and server; if full verification isn't possible, note the gap explicitly
- **Protocol Changes**: When modifying protocol messages, update both client and server handlers and rebuild both

### Modifying Network Protocol

When adding or changing network messages:

1. Update `Dependencies/Shared/NetMessages.h` with new message IDs
2. Implement handler in client `Game.cpp` message processing
3. Implement handler in server `Game.cpp` message processing
4. Ensure both sides pack/unpack structures identically
5. Rebuild both client and server
6. Note any verification gaps that require manual testing

## Working with Game Data

Game content files are in `Binaries/Game/CONTENTS/`:
- `contents*.txt`: NPC dialogs, quests, and game text
- Item configs: `CItemcfg.txt`, `BItemcfg.txt`
- `badword.txt`: Chat filter

Server config files are in `Binaries/Server/`.

## Completing TODO Tasks

When you finish a TODO task from `TODO.md`:

1. **Create/Update DONE.md**: Add the completed task to a `DONE.md` file with:
   - Task title
   - Brief description of what was done

   Example:
   ```markdown
   - Replaced custom CStrTok class with C Style strtok
     - Moved to using a simpler standard of strtok to avoid having a custom version to maintain.
   ```

2. **Mark as complete in TODO.md**: Add a checkmark (✓ or ✅) to the completed task in `TODO.md`

## Planning & Documentation

Before significant changes, write a plan and place it in `PLANS/`:
- Use descriptive names (e.g., `PLAN_EntityManager.md`, `PLAN_NetworkRefactor.md`)
- Rename older generic plans to `PLAN_{ModifiedDateTimeStamp}.md` to preserve history
- Include: objectives, affected files, implementation steps, and testing notes

## Commit & Pull Request Guidelines

- **Do not create git commits in this repo**; work only with the existing working tree
- If a PR is needed, include:
  - Summary of changes
  - Affected modules
  - Test notes (manual steps to verify)
  - Screenshots only for UI changes

## Agent-Specific Instructions

**Critical Rules:**
- **Always build from `Helbreath.sln`**, never from `Sources/Client/` or `Sources/Server/` project folders
- **`HelbreathServer-main` is read-only**; do not edit files there
- **Avoid over-optimization**; implement only what is requested and necessary for correctness
- **Do not create tooling inside this repo**; place any new tooling in `C:\Users\ShadowEvil\source\Repos3`
- **Do not preserve refactored code**; when refactoring, delete the old implementation entirely. Git provides backup history if rollback is needed. Keeping "for reference" copies creates dead code and confusion.

**Workflow Guidelines:**
- If a change is large enough to warrant end-to-end verification, pause and ask the user to run the test locally
- If the next step is unclear or scope might expand, ask the user for direction before continuing
- When adding new source or header files, update the MSVC project (`.vcxproj`) so the files are included in the build
- Build early and often to catch issues quickly

## Platform Specifics

- **Windows-only**: Heavy use of Windows API (windows.h, winbase.h, mmsystem.h)
- **32-bit**: Must compile as x86/Win32
- **DirectX**: Legacy DirectX 7/8 era APIs (DirectDraw, DirectInput, DirectSound)
- **Compiler**: Requires MSVC, uses `/FORCE:UNRESOLVED` linker flag on client
- **Safe Exception Handlers**: Disabled (`ImageHasSafeExceptionHandlers=false`)

## Critical Files

- `Sources/Client/Game.cpp` (1.7MB): Core client game loop and rendering
- `Sources/Server/Game.cpp` (2.1MB): Core server logic and entity management
- `Sources/Client/Game.h`: Client constants and CGame interface
- `Sources/Server/Game.h`: Server constants and limits
- `Dependencies/Shared/NetMessages.h`: Network protocol (must match on both sides)
- `XSocket.cpp/h`: Custom async socket wrapper
- `GlobalDef.h`: Shared global definitions (different per side)

## Known Issues

**Header File Duplication**: There are currently 16 header files duplicated between `Sources/Client/` and `Sources/Server/`. Critical protocol files like `NetMessages.h`, `ActionID.h`, and `DynamicObjectID.h` should be centralized to prevent desynchronization. See `TODO.md` for the full refactoring plan.

## Notes

- The codebase is from the early 2000s with legacy coding patterns
- Large monolithic Game.cpp files handle most functionality
- No formal testing framework present
- Manual memory management throughout
- Windows message pump integration for async sockets and rendering
- See `TODO.md` for planned improvements and refactoring tasks
