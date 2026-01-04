# TODO

## ✅ 1. Centralize Shared Header Files Between Server and Client

**Priority:** High
**Complexity:** Medium
**Impact:** Reduces duplication, prevents protocol desynchronization bugs
**Status:** ✅ **COMPLETED** (2026-01-03)

### Problem
Currently, there are 16 header files that exist in both `Sources/Client/` and `Sources/Server/` directories. Many of these files define protocol-critical constants that must remain synchronized between client and server. Having duplicate copies creates maintenance issues and risks introducing bugs when one side is updated but the other is not.

### Files to Centralize

Create a new `Dependencies/Shared/` directory and move the following protocol-critical headers:

#### Protocol Headers (Must be identical)
- **NetMessages.h** - Network message IDs and types (CRITICAL - currently has minor whitespace differences)
- **ActionID.h** - Character actions and animations (currently has differences - needs reconciliation)
- **DynamicObjectID.h** - Dynamic object type IDs (nearly identical, minor whitespace)

#### Utility Headers (Currently identical or should be)
- **StrTok.h** - String tokenizer utility
- **Msg.h** - Message handling
- **Misc.h** - Miscellaneous utilities
- **XSocket.h** - Socket wrapper class

### Files That Should NOT be Centralized

The following files have intentionally different implementations between client and server:

- **Game.h** - Different class structures and constants for client vs server
- **GlobalDef.h** - Client has config settings, server has crusade structures
- **BuildItem.h** - Different member variables in CBuildItem class
- **Item.h** - Different constants and structures
- **Magic.h** - Different magic type definitions
- **Skill.h** - Different skill structures
- **Tile.h** - Likely different implementations
- **MobCounter.h** - May have different requirements
- **resource.h** - Different resource IDs

### Implementation Steps

1. **Create shared directory structure:**
   ```
   Helbreath-3.82/Dependencies/Shared/
   ```

2. **For each file to centralize:**
   - Compare client and server versions using diff
   - If differences exist, reconcile them (favor the more complete/correct version)
   - Move the canonical version to `Dependencies/Shared/`
   - Update both .vcxproj files to add the Shared include path
   - Update both .vcxproj files to remove the duplicate .h/.cpp files from their projects
   - Delete the duplicate files from Client and Server directories

3. **Update project include paths:**

   In `Client.vcxproj` and `Server.vcxproj`, add to the `<IncludePath>` sections:
   ```xml
   $(SolutionDir)Dependencies\Shared;
   ```

4. **Test thoroughly:**
   - Build both client and server
   - Verify network communication still works
   - Test all shared functionality

### Notes

- Start with the most critical files first: **NetMessages.h**, **ActionID.h**, **DynamicObjectID.h**
- Before centralizing ActionID.h, need to resolve differences:
  - Client has `DEF_TOTALCHARACTERS 120`, server has implicit default
  - Client has `DEF_TOTALACTION 15`, server has `10`
  - Client has `DEF_OBJECTDEAD 101`, server doesn't
- Document any intentional differences that remain in separate files
- Consider adding build-time checks to ensure protocol constants match

### Risks

- Breaking existing build configurations
- Accidentally introducing protocol mismatches during reconciliation
- References in code that use relative paths to headers

### Benefits

- Single source of truth for protocol definitions
- Easier to maintain consistency
- Reduces repository size
- Clearer separation of concerns (shared vs client-specific vs server-specific)

---

## ✅ 2. Replace Custom StrTok Class with Standard strtok

**Priority:** Medium
**Complexity:** Low
**Impact:** Reduces custom code, uses standard library
**Status:** ✅ **COMPLETED** (2026-01-01)

### Problem
The codebase uses a custom `CStrTok` class when the standard C library `strtok()` or C++ alternatives would suffice. This adds unnecessary code to maintain.

### Implementation Steps

1. **Identify all usage of CStrTok:**
   - Search for `CStrTok` instances in both Client and Server
   - Document the usage patterns

2. **Replace with standard functions:**
   - Use `strtok()` or `strtok_s()` (safer version) for C-style strings
   - Consider `std::stringstream` or `std::string::find()` for more complex parsing

3. **Remove files:**
   - Delete `Sources/Client/StrTok.h` and `StrTok.cpp`
   - Delete `Sources/Server/StrTok.h` and `StrTok.cpp`
   - Update both .vcxproj files to remove these files

4. **Update includes:**
   - Remove `#include "StrTok.h"` from all source files
   - Add `<string.h>` where needed for strtok

### Benefits
- Less custom code to maintain
- Better understood standard library functions
- Slightly smaller codebase

---

## ✅ 3. Convert Misc Class to Static Header-Only File

**Priority:** Medium
**Complexity:** Low
**Impact:** Easier usage, no linking required
**Status:** ✅ **COMPLETED** (2026-01-01)

### Problem
The Misc class likely contains utility functions that would be better served as inline or static functions in a header-only file, eliminating the need for separate compilation.

### Implementation Steps

1. **Analyze Misc class:**
   - Review `Misc.h` and `Misc.cpp` in both Client and Server
   - Identify if functions can be made inline/static

2. **Convert to header-only:**
   - Move implementations from `.cpp` to `.h`
   - Make functions `static inline` or place in namespace
   - Ensure no external dependencies that would cause linking issues

3. **Update build:**
   - Remove `Misc.cpp` from both .vcxproj files
   - Keep `Misc.h` (potentially in Shared directory if identical)

4. **Test compilation:**
   - Ensure no multiple definition errors
   - Verify all functions still work correctly

### Benefits
- Simpler to use (just include the header)
- Potential for better inlining/optimization
- Reduced object file count

---

## 4. ✅ Remove Old-Style Operator new/delete Overrides

**Priority:** Medium
**Complexity:** Medium
**Impact:** Modernizes code, removes custom memory management
**Status:** COMPLETED

### Problem
Many classes override `operator new` and `operator delete` to use `HeapAlloc`/`HeapFree` instead of the standard allocator. This was likely a premature optimization and adds complexity.

### Example to Remove:
```cpp
void * operator new (size_t size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
};

void operator delete(void * mem)
{
    HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, mem);
};
```

### Implementation Steps

1. **Find all instances:**
   ```bash
   grep -r "operator new" Sources/
   grep -r "HeapAlloc" Sources/
   ```

2. **For each class with custom allocators:**
   - Remove the operator new/delete overrides
   - Test that the class still works with standard new/delete
   - Watch for any HEAP_ZERO_MEMORY dependencies (may need explicit memset)

3. **Update constructors if needed:**
   - If code relied on HEAP_ZERO_MEMORY zeroing memory, add explicit initialization

4. **Performance test:**
   - Verify no significant performance regression
   - Modern allocators are highly optimized

### Risks
- Classes may have relied on zero-initialization from HEAP_ZERO_MEMORY
- Potential performance impact (though likely negligible)

### Benefits
- Simpler, more maintainable code
- Better compatibility with modern C++ features (smart pointers, etc.)
- Removes Windows-specific memory management

---

## 5. ✅ Replace Old-Style Header Guards with #pragma once

**Priority:** Low
**Complexity:** Low
**Impact:** Cleaner, more maintainable headers
**Status:** COMPLETED

### Problem
All header files use old-style include guards with long, randomly-generated GUIDs. Modern compilers universally support `#pragma once`, which is simpler and less error-prone.

### Example to Replace:
```cpp
#if !defined(AFX_ITEM_H__211A1360_91B9_11D2_B143_00001C7030A6__INCLUDED_)
#define AFX_ITEM_H__211A1360_91B9_11D2_B143_00001C7030A6__INCLUDED_
// ... content ...
#endif
```

**Replace with:**
```cpp
#pragma once
// ... content ...
```

### Implementation Steps

1. **Automated replacement script:**
   ```bash
   # For each .h file:
   # - Remove #if !defined(...) line
   # - Remove #define ... line
   # - Add #pragma once at top
   # - Remove trailing #endif
   ```

2. **Update all ~77 header files** in Sources/Client and Sources/Server

3. **Verify compilation:**
   - Build both projects
   - Ensure no missing includes or circular dependency issues

### Benefits
- Cleaner, more readable headers
- No chance of copy-paste errors with guard names
- Slightly faster compilation (compiler optimization)
- Industry standard in modern C++

---

## 6. ⚠️ Translate Non-English Comments to English (In Progress)

**Priority:** Low
**Complexity:** High (requires translation)
**Impact:** Better code maintainability for international developers
**Status:** PARTIALLY COMPLETED - Encoding challenges with CP949

### Problem
There are Korean and possibly Spanish comments throughout the codebase, making it difficult for non-Korean/Spanish speakers to understand the code.

### Implementation Steps

1. **Identify non-English comments:**
   - Search for Korean characters (Hangul: \uAC00-\uD7AF)
   - Search for Spanish-specific patterns
   - Document locations and count

2. **Translation approach:**
   - Use translation tools (Google Translate, DeepL) as first pass
   - Review translations for technical accuracy
   - Native speakers should review if possible

3. **Priority order:**
   - Start with protocol-critical files (NetMessages.h, Game.h)
   - Then core game logic files
   - Finally utility and helper files

4. **Update incrementally:**
   - Translate and commit by subsystem
   - Keep original as comment if meaning unclear

### Example:
```cpp
// Before:
// 유저가 아이템을 획득할 때 호출됨

// After:
// Called when user acquires an item
```

### Benefits
- More accessible to international developers
- Easier code review and maintenance
- Better documentation

---

## 7. ✅ Convert Appropriate Classes to Header-Only

**Priority:** Low
**Complexity:** Medium
**Impact:** Simplifies build, potential for better optimization
**Status:** COMPLETED

### Problem
Some classes with `.h` and `.cpp` files may be simple enough to be header-only, reducing compilation units and potentially improving performance through inlining.

### Candidates to Review

Look for classes with:
- Only simple getters/setters
- Small inline-able functions
- No large static data
- Template classes (should always be header-only)

**Likely candidates:**
- `BuildItem` - Simple data holder
- `CharInfo` - Character information structure
- `Effect` - Effect data
- `Curse` - Curse data
- Small utility classes

### Implementation Steps

1. **Analyze each class:**
   - Count lines of code in .cpp
   - Check for static/global variables
   - Verify no complex dependencies

2. **For header-only conversion:**
   - Move implementations from .cpp to .h
   - Mark functions as `inline`
   - Add appropriate `static` for helper functions

3. **Update build files:**
   - Remove .cpp from .vcxproj
   - Keep .h file

4. **Test compilation:**
   - Watch for multiple definition errors
   - Verify no increase in binary size (minor increase acceptable)

### Classes to KEEP as separate .cpp:
- `CGame` (massive implementation)
- `CClient` (server-side client management)
- Large managers and systems

### Benefits
- Fewer compilation units
- Better optimization opportunities
- Simpler file structure
- Faster incremental builds (in some cases)

---

## Implementation Priority Order

1. **High Priority:** Task #1 - Centralize shared headers (prevents bugs)
2. **Medium Priority:** Tasks #2, #3, #4 - Code modernization
3. **Low Priority:** Tasks #5, #6, #7 - Code quality improvements

**Recommendation:** Complete tasks in order, testing thoroughly after each change.
