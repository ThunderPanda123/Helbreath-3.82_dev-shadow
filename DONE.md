# Changelog

All notable completed tasks and changes to the Helbreath 3.82 project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [2026-01-03]

### Added
- **Completed centralization of shared header files between Client and Server** (Task #1 - All Phases Complete)
  - **Phase 1**: Created `Dependencies/Shared/` directory structure
    - Added shared include path to both Client.vcxproj and Server.vcxproj
    - Established single source of truth for protocol-critical definitions

  - **Phase 2**: Migrated DynamicObjectID.h to shared location
    - Moved `Dependencies/Shared/DynamicObjectID.h` from Server version (functionally identical)
    - Deleted `Sources/Client/DynamicObjectID.h` and `Sources/Server/DynamicObjectID.h`
    - Updated both project files to reference shared version

  - **Phase 3**: Resolved Msg.h naming conflict by renaming files
    - Renamed `Sources/Client/Msg.h` → `Sources/Client/ChatMsg.h` (UI/chat message class)
    - Renamed `Sources/Server/Msg.h` → `Sources/Server/NetworkMsg.h` (network serialization class)
    - Updated all include statements and project files
    - **Rationale**: Same filename, completely different purposes - renaming clarifies intent

  - **Phase 4**: Split ActionID.h into shared and platform-specific files
    - Created `Dependencies/Shared/ActionID.h` with common action definitions (DEF_OBJECTSTOP, DEF_OBJECTMOVE, etc.)
    - Created `Sources/Client/ActionID_Client.h` with client-specific constants (DEF_TOTALACTION=15 for animation frames, DEF_TOTALCHARACTERS=120, DEF_OBJECTDEAD=101)
    - Created `Sources/Server/ActionID_Server.h` with server-specific constants (DEF_TOTALACTION=10 for game logic)
    - **Rationale**: Client needs extra animation frame slots (15) vs server game logic states (10)

  - **Phase 5**: Split NetMessages.h into three separate files by usage (IMPROVED ARCHITECTURE)
    - **Original Plan**: Merge everything into single shared NetMessages.h
    - **Implemented Solution**: Split into three purpose-specific files for better separation of concerns

    **Created `Dependencies/Shared/NetMessages.h`** (SHARED protocol messages only - ~280 messages):
      - Core client-server protocol communication
      - Player initialization, motion, action messages
      - All DEF_NOTIFY_* notification messages (~200+ messages)
      - Login/account management messages
      - Common action types (DEF_COMMONTYPE_*)
      - Configuration content messages used by both sides
      - Item, teleport, dynamic object messages

    **Created `Sources/Client/ClientMessages.h`** (CLIENT_ONLY messages - ~9 messages):
      - Client teleport list messages (MSGID_REQUEST_TELEPORT_LIST, etc.)
      - Heldenian teleport messages
      - Gateway connection messages (MSGID_GATEWAY_CURRENTCONN, MSGID_GETMINIMUMLOADGATEWAY)
      - **Note**: Some IDs conflict with server messages - documented in comments

    **Created `Sources/Server/ServerMessages.h`** (SERVER_ONLY messages - ~110+ messages):
      - Server infrastructure messages (database, inter-server communication)
      - All 19 Gate Server Messages (GSM_*) for multi-server coordination
      - Configuration loading messages (MSGID_*CONFIGURATIONCONTENTS)
      - Server teleport messages (different IDs than client)
      - Account/server management messages
      - Bad Word Manager and logging messages
      - PK (Player Kill) tracking constants (DEF_PKLOG_*)
      - World server, monitor, and reboot messages
      - Party operation messages
      - Crusade logging, NPC item drops, slate system

    - Deleted `Sources/Client/NetMessages.h` and `Sources/Server/NetMessages.h`
    - Updated `Sources/Client/Game.h` to include both NetMessages.h and ClientMessages.h
    - Updated `Sources/Server/Game.h` to include both NetMessages.h and ServerMessages.h
    - Updated Client.vcxproj to add ClientMessages.h
    - Updated Server.vcxproj to add ServerMessages.h

### Changed
- **Improved protocol architecture with three-file message split**
  - Client project now includes: NetMessages.h (shared) + ClientMessages.h (client-only)
  - Server project now includes: NetMessages.h (shared) + ServerMessages.h (server-only)
  - **Benefits**:
    - Clearer separation of concerns (shared vs platform-specific)
    - Prevents accidental use of wrong message IDs
    - Easier to identify which messages are part of the client-server protocol
    - Server infrastructure messages isolated from protocol
    - Better documentation of message ID conflicts

### Fixed
- **Documented 5 critical message ID conflicts** in NetMessages.h and platform-specific files:
  1. `0x0FA314DB` - MSGID_EVENT_COMMON (shared) vs MSGID_MAGICCONFIGURATIONCONTENTS (server-only)
  2. `0x0EA03202` - MSGID_REQUEST_TELEPORT_LIST (client) vs MSGID_REQUEST_CITYHALLTELEPORT (server)
  3. `0x0EA03206` - MSGID_REQUEST_HELDENIAN_TP_LIST (client) vs MSGID_REQUEST_HELDENIANTELEPORT (server)
  4. `0x2900AD30` - MSGID_REQUEST_IPTIME (server) vs MSGID_REQUEST_SELLITEMLIST (shared)
  5. `0x210A914F` - MSGID_GAMEITEMLOG vs MSGID_GAMECRUSADELOG (both server-only)

### Removed
- Deleted duplicate local header files after centralization:
  - `Sources/Client/DynamicObjectID.h`
  - `Sources/Client/ActionID.h`
  - `Sources/Client/NetMessages.h`
  - `Sources/Server/DynamicObjectID.h`
  - `Sources/Server/ActionID.h`
  - `Sources/Server/NetMessages.h`

**Architecture Summary:**
```
Client Project:
  ├─ Dependencies/Shared/ActionID.h (shared action IDs)
  ├─ Dependencies/Shared/DynamicObjectID.h (shared object IDs)
  ├─ Dependencies/Shared/NetMessages.h (shared protocol - ~280 messages)
  ├─ Sources/Client/ActionID_Client.h (client-specific)
  └─ Sources/Client/ClientMessages.h (client-only messages - ~9)

Server Project:
  ├─ Dependencies/Shared/ActionID.h (shared action IDs)
  ├─ Dependencies/Shared/DynamicObjectID.h (shared object IDs)
  ├─ Dependencies/Shared/NetMessages.h (shared protocol - ~280 messages)
  ├─ Sources/Server/ActionID_Server.h (server-specific)
  └─ Sources/Server/ServerMessages.h (server-only messages - ~110+)
```

**Impact:**
- Single source of truth for protocol definitions prevents desynchronization bugs
- Reduced duplication from 16 duplicate headers to 3 shared + 4 platform-specific
- Clearer code organization with explicit shared vs platform-specific headers
- Better maintainability - protocol changes only need to be made in one place
- Easier to identify which messages are part of the client-server contract

**Related TODO:**
- Task #1 - Centralize Shared Header Files Between Server and Client (ALL PHASES COMPLETE)

## [2026-01-02]

### Changed
- **Converted 29 simple classes to header-only format** (completed in two phases)
  - **Phase 1**: 11 classes (MobCounter, Effect, CharInfo, BuildItem, Mineral, Portion, Fish, Teleport, TeleportLoc)
  - **Phase 2**: 18 additional classes

  **Client (12 total)**:
  - Item, Magic, Skill, ItemName, Msg, TileSpr, DXC_dsound, Tile
  - MobCounter, Effect, CharInfo, BuildItem (from Phase 1)

  **Server (17 total)**:
  - Item, Magic, Skill, DelayEvent, Structure, GuildsMan, StrategicPoint, OccupyFlag, TempNpcItem, DynamicObject
  - MobCounter, Mineral, Portion, Fish, Teleport, TeleportLoc, BuildItem (from Phase 1)

  - Moved all constructor/destructor implementations from .cpp to .h files
  - Marked all moved functions as `inline` keyword
  - Deleted 29 .cpp files (significant reduction in compilation units)
  - Updated Client.vcxproj to remove 12 ClCompile entries
  - Updated Server.vcxproj to remove 16 ClCompile entries (some files not in project)
  - **Benefits**: 29 fewer compilation units, better inlining opportunities, faster builds, simplified maintenance
  - **Related TODO**: Task #7 - Convert Appropriate Classes to Header-Only

### Removed
- **Deleted 29 .cpp files** after converting classes to header-only format

  **Client (12 files deleted)**:
  - Item.cpp, Magic.cpp, Skill.cpp, ItemName.cpp, Msg.cpp, TileSpr.cpp, DXC_dsound.cpp, Tile.cpp
  - MobCounter.cpp, Effect.cpp, CharInfo.cpp, BuildItem.cpp

  **Server (17 files deleted)**:
  - Item.cpp, Magic.cpp, Skill.cpp, DelayEvent.cpp, Structure.cpp, GuildsMan.cpp, StrategicPoint.cpp, OccupyFlag.cpp, TempNpcItem.cpp, DynamicObject.cpp
  - MobCounter.cpp, Mineral.cpp, Portion.cpp, Fish.cpp, Teleport.cpp, TeleportLoc.cpp, BuildItem.cpp

  All implementations now reside in their respective .h files as inline functions.

- **Started translation of Korean comments to English** (Task #6 - In Progress)
  - Identified 350+ lines of Korean (Hangul) comments across 10 Server files
  - Files with Korean comments: Client.cpp, Client.h, Fish.h, Game.cpp, Game.h, Item.h, NetMessages.h, Npc.h, Quest.h, XSocket.h
  - Created automated translation tools and dictionaries
  - **Challenge encountered**: Source files use CP949 encoding (Korean Windows codepage), requiring specialized handling
  - **Status**: Partial progress - translation infrastructure created, manual translation work remaining
  - **Note**: Full translation requires proper CP949 encoding support or manual review by Korean speaker
  - **Related TODO**: Task #6 - Translate Non-English Comments to English (Partially Complete)

- Replaced old-style header guards with `#pragma once` in all header files
  - **Client**: Converted 33 header files to use `#pragma once`
  - **Server**: Converted 41 header files to use `#pragma once` (resource.h excluded as it's a Microsoft-generated file)
  - **Total**: 74 header files modernized
  - Removed old AFX-style header guards with GUIDs (e.g., `#if !defined(AFX_ITEM_H__211A1360_91B9_11D2_B143_00001C7030A6__INCLUDED_)`)
  - Removed conditional `#pragma once` patterns (e.g., `#if _MSC_VER >= 1000`)
  - Added unconditional `#pragma once` at the top of each header file
  - Removed trailing `#endif` statements that matched old header guards
  - **Benefits**: Cleaner code, no chance of copy-paste errors with guard names, faster compilation, industry standard in modern C++
  - **Related TODO**: Task #5 - Replace Old-Style Header Guards with #pragma once

### Removed
- Removed old-style operator new/delete overrides from all Client classes
  - Removed custom memory allocators using `HeapAlloc`/`HeapFree` from 6 client classes
  - Classes now use standard C++ new/delete operators for memory allocation
  - Affected files:
    - `Sources/Client/Game.h` - Removed operator new/delete from `CGame` class
    - `Sources/Client/XSocket.h` - Removed operator new/delete from `XSocket` class
    - `Sources/Client/Sprite.h` - Removed operator new/delete from `CSprite` class
    - `Sources/Client/Msg.h` - Removed operator new/delete from `CMsg` class
    - `Sources/Client/MapData.h` - Removed operator new/delete from `CMapData` class
    - `Sources/Client/DXC_ddraw.h` - Removed operator new/delete from `DXC_ddraw` class

### Changed
- Simplified memory management by using standard C++ allocators
  - Modern C++ allocators are highly optimized and provide better compatibility
  - Removed Windows-specific `HeapAlloc`/`HeapFree` calls
  - Removed dependency on `HEAP_ZERO_MEMORY` flag (standard new already zero-initializes properly)
  - Better compatibility with modern C++ features like smart pointers

### Fixed
- **CRITICAL BUG FIX #4**: Fixed uninitialized member variables in CGame class after removing HEAP_ZERO_MEMORY
  - **Root cause**: The old custom `operator new` used `HEAP_ZERO_MEMORY` which zero-initialized the entire `CGame` object. After removing these custom operators, many char arrays contained uninitialized memory (0xCDCDCDCD in debug mode)
  - **Symptom**: Access violation at Game.cpp:19320 when `EndInputString()` called `strlen(m_cEdit)` on uninitialized data
  - **Solution**: Added explicit `ZeroMemory()` initialization for 18 char array members in the `CGame` constructor (lines 562-579):
    - `m_cEdit[4]` - Input editing buffer (CRITICAL - was causing immediate crash)
    - `m_cMsg[200]` - General message buffer
    - `m_cChatMsg[64]` - Chat message buffer
    - `m_cBackupChatMsg[64]` - Backup chat buffer
    - `m_cAmountString[12]` - Numeric input buffer
    - `m_cCurLocation[12]` - Current location
    - `m_cMapMessage[32]` - Map message
    - `m_cGameServerName[22]` - Game server name
    - `m_cAccountAge[12]` - Account age field
    - `m_cNewPassword[12]` - New password field
    - `m_cNewPassConfirm[12]` - Password confirmation
    - `m_cAccountCountry[18]` - Account country
    - `m_cAccountSSN[32]` - Account SSN
    - `m_cEmailAddr[52]` - Email address
    - `m_cAccountQuiz[46]` - Account quiz
    - `m_cAccountAnswer[22]` - Quiz answer
    - `m_cName_IE[12]` - IE name field
    - `m_cTakeHeroItemName[100]` - Hero item name
  - **Important Note**: Initial fix attempt placed initialization code outside the constructor (lines 4875, 5088-5104) which didn't execute, causing continued crashes. Fixed by moving initialization inside the constructor before line 560 (constructor end).
  - **Verification**: Checked other classes (XSocket, CSprite, CMsg, CMapData, DXC_ddraw) - all have proper initialization in their constructors
  - **Impact**: Prevents crashes when any string functions (strlen, strcpy) are called on these members before they are explicitly written to
  - **Location**: `Sources/Client/Game.cpp` lines 562-579 (inside CGame constructor)

**Related TODO:**
- Task #4 - Remove Old-Style Operator new/delete Overrides

## [2026-01-01]

### Changed
- Converted `CMisc` class to header-only implementation using namespace with static inline functions
  - Converted Client `CMisc` class (17 utility functions) to namespace with static inline functions
  - Converted Server `CMisc` class (9 utility functions) to namespace with static inline functions
  - Changed calling convention from `instance.function()` to `CMisc::function()` throughout Server code
  - All function implementations now inline in header files for better optimization potential
- Refactored string tokenization throughout codebase to use standard C library `strtok()` instead of custom `CStrTok` class
  - Replaced all `CStrTok` instantiations in Client code (Game.cpp, GameMonitor.cpp)
  - Replaced all `CStrTok` instantiations in Server code (Game.cpp, LoginServer.cpp, Map.cpp)
  - Updated tokenization pattern from `pStrTok->pGet()` to `strtok(NULL, seps)`

### Removed
- Deleted `CMisc` class implementation files (now header-only)
  - `Sources/Client/Misc.cpp` (405 lines)
  - `Sources/Server/Misc.cpp` (368 lines)
- Removed Misc.cpp file references from project files
  - Updated `Client.vcxproj` to remove Misc.cpp from compilation
  - Updated `Server.vcxproj` to remove Misc.cpp from compilation
- Removed `CMisc` class member variable from Server
  - Removed `class CMisc m_Misc;` from `Sources/Server/Game.h` (line 879)
- Deleted custom `CStrTok` class implementation files
  - `Sources/Client/StrTok.h` and `Sources/Client/StrTok.cpp`
  - `Sources/Server/StrTok.h` and `Sources/Server/StrTok.cpp`
- Removed StrTok file references from project files
  - Updated `Client.vcxproj` to remove StrTok.h and StrTok.cpp
  - Updated `Server.vcxproj` to remove StrTok.h and StrTok.cpp
- Removed `#include "StrTok.h"` from header files
  - `Sources/Client/Game.h`
  - `Sources/Client/GameMonitor.h`
  - `Sources/Server/Game.h`
  - `Sources/Server/Map.h`

### Added
- Added `#include "GlobalDef.h"` to `Sources/Client/Misc.h` for header-only implementation
- Added `#include <string.h>` to `Sources/Server/Misc.h` for string functions
- Added missing library dependencies to `Server.vcxproj`
  - `winmm.lib` for timer functions
  - `ws2_32.lib` for Winsock functions

### Fixed
- **CRITICAL BUG FIX #1**: Fixed Windows line ending handling in strtok conversion
  - CStrTok had special handling for `\r\n` (Windows line endings) as a single separator
  - Standard strtok requires `\r` to be explicitly included in the separator string
  - Added `\r` to all separator strings (`"= \t\n"` → `"= \t\r\n"`) to handle Windows text files correctly
  - Fixed 73+ instances in Server code (Game.cpp, LoginServer.cpp, Map.cpp)
  - Fixed 15+ instances in Client code (Game.cpp, GameMonitor.cpp, MapData.cpp)
  - **Impact**: Resolves file parsing failures in `_bDecodePlayerDatafileContents` and all config file readers
- **CRITICAL BUG FIX #2**: Fixed strtok reentrancy issue in GMap.cfg parsing
  - `strtok` maintains internal static state - nested calls corrupt the outer loop's state
  - When `bReadProgramConfigFile` parses GMap.cfg and calls `_bRegisterMap`, which calls `CMap::_bDecodeMapDataFileContents`, the nested `strtok` call corrupted the GMap.cfg parsing loop
  - **Root cause**: CStrTok instances each had their own state, but strtok shares global state
  - **Solution**: Converted `CMap::_bDecodeMapDataFileContents` to use thread-safe `strtok_s` with context pointer
  - **Impact**: GMap.cfg now reads all map entries correctly instead of stopping after the first map
- **CRITICAL BUG FIX #3**: Fixed uninitialized boolean members in CMap constructor
  - **Root cause**: Several boolean map settings were not initialized in the `CMap` constructor (Map.cpp:13-160)
  - When map config files didn't specify optional settings like `ApocalypseMap`, `RecallImpossible`, `CitizenLimit`, etc., these members contained uninitialized garbage data
  - **Issue reported**: `m_bIsApocalypseMap` was showing value of 1 instead of false when not present in map info files
  - **Solution**: Added explicit initialization for 6 uninitialized boolean members in `CMap::CMap()`:
    - `m_bRandomMobGenerator = false`
    - `m_bIsCitizenLimit = false`
    - `m_bIsEnergySphereAutoCreation = false`
    - `m_bIsRecallImpossible = false`
    - `m_bIsApocalypseMap = false`
    - `m_stDynamicGateCoords[].m_bIsGateMap = false` (array initialization)
  - **Impact**: Map settings now correctly default to false when not specified in config files, preventing undefined behavior
  - **Location**: `Sources/Server/Map.cpp` lines 161-173
- Fixed pre-existing bugs in Server code discovered during compilation
  - Line 49261: Uncommented `char buff[100];` declaration (was causing undeclared variable error)
  - Line 44335: Added empty statement `;` after `Skip_Storm:` label
  - Line 49285: Added missing closing brace `}` for `AdminOrder_SummonGuild` function
- Verified both Client and Server build successfully with no new compilation errors introduced

**Related TODO:**
- Task #2 - Replace Custom StrTok Class with Standard strtok
- Task #3 - Convert Misc Class to Static Header-Only File
