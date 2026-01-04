# Translation Plan: Korean Comments to English

## Overview
Translate all Korean (Hangul) comments throughout the codebase to English to improve maintainability and accessibility for international developers.

**Total Scope:** ~3,193 lines with non-ASCII characters (primarily Korean comments)

## Encoding Challenge
**CRITICAL**: Source files have **MIXED ENCODING** issues. Three types of encoding problems exist:

### 1. CP949 Garbled as Latin-1 (~2,087 comments in Server Game.cpp)
Korean text saved in CP949 encoding but read as Latin-1/ISO-8859-1, appearing as gibberish:
- **Example:** `//¿ù¿äÀÏ` (should be "월요일" - Monday)
- **Example:** `// °ªÀ» ÂüÁ¶ÇØ¾ß ÇÑ´Ù` (should be "값을 참조해야 한다" - must reference value)
- **Example:** `// ¹«±â` (should be "무기" - weapon)
- **Pattern:** Contains characters like °, ±, ², ³, ´, µ, ¶, ·, ¸, ¹, º, À, Á, Â, Ã, Ä, Å, Æ, Ç, È, É
- **Fix Required:** Open in CP949-aware editor, re-save properly or translate directly

### 2. Proper UTF-8 Korean (~34 comments in Server Game.cpp)
Correctly encoded Korean that displays properly:
- **Example:** `//strcpy(cTxt, "인접한 클라이언트들에게 방향전환 이벤트를 알린다.` (Notify adjacent clients of direction change event)
- **Example:** Line 23786: `클라이언트 버그?` (Client bug?)
- **Pattern:** Contains Hangul characters 가-힣 that display correctly
- **Fix Required:** Direct translation to English

### 3. UTF-8 Mojibake (~10 comments in Server Game.cpp)
Double-encoded or misinterpreted UTF-8, appearing as complex gibberish:
- **Example:** `// ì¨íš§ì©íš„íšì²  ?ì²´ì©Œíš¤` (completely unreadable)
- **Example:** `// Custom-Item?íš"íšì² ?íš‰ ì©"ì§¤ì¨˜íš"`
- **Pattern:** Contains sequences like ì¨íš§, ì©íš„, íšì², ì²´ì©Œ
- **Fix Required:** May need to reconstruct original text from context or mark for manual review

### 4. Spanish Comment (1 comment)
- **Location:** Server Game.cpp:2234
- **Example:** `//copiar el nombre del char (de tamaño 20) a la data` (copy char name (size 20) to data)
- **Fix Required:** Translate Spanish to English

### Encoding Statistics for Server Game.cpp:
- **Total non-ASCII lines:** 2,668
- **CP949 garbled (Latin-1):** ~2,087 (78%)
- **Proper UTF-8 Korean:** ~34 (1.3%)
- **UTF-8 mojibake:** ~10 (0.4%)
- **Mixed/Other:** ~537 (20%)

**This requires:**
- Specialized encoding-aware text editors (e.g., VS Code with "Reopen with Encoding")
- Manual review to ensure translation accuracy
- Careful preservation of file encoding during edits
- Context-based reconstruction for mojibake comments

## Priority Classification

### Priority 1: Protocol-Critical Files (HIGH)
Files that define network communication and shared constants - most important for understanding client-server interaction.

### Priority 2: Core Game Logic (MEDIUM)
Files containing main game mechanics and systems.

### Priority 3: Supporting Systems (LOW)
Helper classes and secondary systems.

---

## Server Files (Total: ~3,118 lines)

### Priority 1: Protocol & Core Systems

#### 1. Sources/Server/Game.cpp
- **Lines with Korean:** 2,668
- **Complexity:** EXTREME (largest file in codebase)
- **Categories of comments:**
  - Player initialization and data handling
  - Guild creation/disbandment logic
  - Item purchase/transaction systems
  - NPC attack patterns and combat
  - Hacking detection warnings
  - Map configuration and event management
  - Character appearance and equipment
  - Day-of-week war period calculations
- **Sample lines requiring translation:**
  - Line 426: `//m_pClientList[iClientH]->m_bIsInitComplete °ªÀ» ÂüÁ¶ÇØ¾ß ÇÑ´Ù.`
  - Line 954: `//strcpy(cTxt, "인접한 클라이언트들에게 방향전환 이벤트를 알린다. ********** TEST SERVER MODE **********");`
  - Line 3723-3729: Day of week comments (`//¿ù¿äÀÏ`, `//È­¿äÀÏ`, etc.)
  - Line 4713: `// ÀÌ¹Ì ÃÊ±âÈ­ µÈ Ä³¸¯ÅÍÀÇ µ¥ÀÌÅÍÀÌ´Ù.`
  - Line 12961+: Guild creation/disbandment comments (multiple sequential lines)
- **Note:** This file alone represents 83.5% of all Korean comments in the codebase
- **Recommendation:** Break into sections for translation (e.g., 500 lines at a time)

#### 2. Sources/Server/Client.h
- **Lines with Korean:** 108
- **Priority:** HIGH (defines client session structure)
- **Expected content:** Member variable comments, client state documentation

#### 3. Sources/Server/Item.h
- **Lines with Korean:** 64
- **Priority:** HIGH (item system definitions)
- **Expected content:** Item attribute comments, type definitions

#### 4. Sources/Server/Npc.h
- **Lines with Korean:** 62
- **Priority:** HIGH (NPC behavior and AI)
- **Expected content:** NPC state comments, behavior flags

### Priority 2: Network & Communication

#### 5. Dependencies/Shared/NetMessages.h
- **Lines with Korean:** 23
- **Priority:** MEDIUM-HIGH (shared protocol definitions)
- **Expected content:** Message ID explanations
- **Note:** This was recently centralized from Server (10 lines) + Client versions

#### 6. Sources/Server/XSocket.cpp
- **Lines with Korean:** 48
- **Priority:** MEDIUM (network implementation)
- **Expected content:** Socket operation comments

#### 7. Sources/Server/XSocket.h
- **Lines with Korean:** 18
- **Priority:** MEDIUM (socket interface)
- **Expected content:** Socket state and method comments

### Priority 3: Game Systems

#### 8. Sources/Server/Quest.h
- **Lines with Korean:** 31
- **Priority:** MEDIUM (quest system)

#### 9. Sources/Server/Game.h
- **Lines with Korean:** 30
- **Priority:** MEDIUM (game class interface)

#### 10. Sources/Server/Map.cpp
- **Lines with Korean:** 20
- **Priority:** MEDIUM (map implementation)

#### 11. Sources/Server/Client.cpp
- **Lines with Korean:** 20
- **Priority:** MEDIUM (client implementation)

#### 12. Sources/Server/Map.h
- **Lines with Korean:** 12
- **Priority:** LOW (map definitions)

#### 13. Sources/Server/Misc.h
- **Lines with Korean:** 9
- **Priority:** LOW (utility functions)

#### 14-20. Smaller Files (≤5 lines each)
- **Tile.h** - 5 lines
- **Fish.h** - 5 lines
- **StrategicPoint.h** - 3 lines
- **Skill.h** - 3 lines
- **BuildItem.h** - 3 lines
- **Wmain.cpp** - 2 lines
- **Magic.h** - 2 lines
- **Total:** 23 lines combined
- **Priority:** LOW (minor supporting files)

---

## Client Files (Total: ~54 lines)

### Priority 1: Core Client Logic

#### 1. Sources/Client/Game.cpp
- **Lines with Korean:** 44
- **Priority:** HIGH (main client game loop)
- **Expected content:** UI logic, rendering comments, client-side game state

#### 2. Sources/Client/Item.h
- **Lines with Korean:** 9
- **Priority:** MEDIUM (client item display)
- **Expected content:** Item rendering, UI display comments

#### 3. Sources/Client/Game.h
- **Lines with Korean:** 1
- **Priority:** LOW (single comment)

---

## Encoding Corruption Examples by File

### Most Affected Files (Garbled Comments)

**Server Game.cpp - 2,087 CP949 garbled comments:**
- Line 426: `//m_pClientList[iClientH]->m_bIsInitComplete °ªÀ» ÂüÁ¶ÇØ¾ß ÇÑ´Ù.`
- Line 3723-3729: Day of week comments all garbled (`//¿ù¿äÀÏ`, `//È­¿äÀÏ`, `//¼ö¿äÀÏ`, `//¸ñ¿äÀÏ`, `//±Ý¿äÀÏ`, `//Åä¿äÀÏ`, `//ÀÏ¿äÀÏ`)
- Line 3903: `//wsprintf(G_cTxt, "(!) ÇØÅ· ¿ëÀÇÀÚ(%s) ¼Óµµ Á¶ÀÛ"` (Hacking suspect speed manipulation)
- Line 4713: `// ÀÌ¹Ì ÃÊ±âÈ­ µÈ Ä³¸¯ÅÍÀÇ µ¥ÀÌÅÍÀÌ´Ù.` (Already initialized character data)
- Line 8631: `// ¹«±â` (weapon)
- Line 8632: `// ¹æÆÐ` (shield)
- Line 11669: `// ¹ÂÅØ½º°¡ »ý¼ºµÇ¾î ÀÖ´Ù.` (Mutex is created)
- Line 12961+: Multiple guild-related comments in garbled CP949

**Server Client.h - 108 lines:**
- All member variable comments likely garbled

**Server Item.h - 64 lines:**
- Item attribute and type definition comments

**Server Npc.h - 62 lines:**
- NPC behavior and state comments

**Client Game.cpp - 44 lines:**
- All non-ASCII comments are garbled (no proper UTF-8 Korean found in Client)

### Files with Readable UTF-8 Korean

**Server Game.cpp - 34 proper UTF-8 comments:**
- Line 954: `//strcpy(cTxt, "인접한 클라이언트들에게 방향전환 이벤트를 알린다. ********** TEST SERVER MODE **********");`
- Line 23786: `::MessageBox(0, "Å¬¶óÀÌ¾ðÆ® ¹ö±×? - 클라이언트 버그?","debug"` (Mix of garbled and proper UTF-8)
- These can be translated directly without encoding conversion

### Mojibake Pattern (Requires Context Reconstruction)

**Server Game.cpp - 10 UTF-8 mojibake comments:**
- Line 1700: `// ì¨íš§ì©íš„íšì²  ?ì²´ì©Œíš¤`
- Line 1711: `// ì¨íš§ì©íš„íšì² ì¨ì§ ì¨˜ì¨ì¨€ì©ì¨‹ì§  ì©"ì§•ì¨Œì§±ì§¸ì§• ì¨"íš©ì¨©ì²µíš‰íš©ì¨ˆíš¢ì¨ì±• íšì§ì§¸íš‡íš‰íš—ì¨ˆíš¢.`
- Line 1778, 1834: `// Custom-Item?íš"íšì² ?íš‰ ì©"ì§¤ì¨˜íš"`
- Line 20719+: Multiple crusade-related mojibake comments
- **Action:** May need Korean speaker to reconstruct or mark as "Unable to decode - context suggests: [description]"

### No Encoding Issues (Client has minimal non-ASCII)

**Client files are much cleaner:**
- Client Game.cpp: 44 lines (all CP949 garbled)
- Client Item.h: 9 lines (likely garbled)
- Client Game.h: 1 line (minimal)
- **Note:** Client codebase has almost no proper UTF-8 Korean, only CP949 garbled text

---

## Implementation Strategy

### Phase 1: Quick Wins (Estimated: 1-2 hours)
Translate small files to build momentum and identify patterns.

**Target files (27 lines total):**
1. Sources/Client/Game.h (1 line)
2. Sources/Server/Wmain.cpp (2 lines)
3. Sources/Server/Magic.h (2 lines)
4. Sources/Server/BuildItem.h (3 lines)
5. Sources/Server/Skill.h (3 lines)
6. Sources/Server/StrategicPoint.h (3 lines)
7. Sources/Server/Fish.h (5 lines)
8. Sources/Server/Tile.h (5 lines)

**Deliverable:** Clean 8 files, document common translation patterns

### Phase 2: Protocol & Communication (Estimated: 4-6 hours)
Critical for understanding client-server interaction.

**Target files (89 lines total):**
1. Dependencies/Shared/NetMessages.h (23 lines)
2. Sources/Server/XSocket.h (18 lines)
3. Sources/Server/XSocket.cpp (48 lines)

**Deliverable:** Fully documented network layer

### Phase 3: Core Headers (Estimated: 6-8 hours)
Important class definitions and interfaces.

**Target files (286 lines total):**
1. Sources/Server/Client.h (108 lines)
2. Sources/Server/Item.h (64 lines)
3. Sources/Server/Npc.h (62 lines)
4. Sources/Server/Quest.h (31 lines)
5. Sources/Server/Map.h (12 lines)
6. Sources/Server/Misc.h (9 lines)

**Deliverable:** Clear documentation of core data structures

### Phase 4: Implementation Files (Estimated: 6-8 hours)
Supporting logic and game systems.

**Target files (143 lines total):**
1. Sources/Client/Game.cpp (44 lines)
2. Sources/Server/Game.h (30 lines)
3. Sources/Server/Map.cpp (20 lines)
4. Sources/Server/Client.cpp (20 lines)
5. Sources/Client/Item.h (9 lines)

**Deliverable:** Translated implementation comments

### Phase 5: The Big One (Estimated: 40-60 hours)
The massive Server Game.cpp file - requires dedicated effort and breaking into sections.

**Target file:**
- Sources/Server/Game.cpp (2,668 lines)

**Recommended Approach:**
1. **Divide into logical sections** (10-15 sections):
   - Player initialization (lines 1-1000)
   - Item handling (lines 1001-2000)
   - Guild management (lines 2001-3000)
   - Combat systems (lines 3001-4000)
   - NPC logic (lines 4001-5000)
   - Continue by functional area...

2. **Translation workflow per section:**
   - Extract section to temporary file
   - Use CP949-aware editor (VS Code with Korean encoding)
   - Translate in small batches (100-200 lines)
   - Use Google Translate for first pass
   - Manual review for technical accuracy
   - Test compile after each section

3. **Create glossary** during translation:
   - Common Korean game terms → English equivalents
   - Technical terms specific to Helbreath
   - Reuse translations for consistency

**Deliverable:** Fully English-commented Game.cpp with glossary document

---

## Translation Tools & Resources

### Recommended Tools
1. **Visual Studio Code**
   - Extensions: "Korean Language Pack", "Encoding Converter"
   - Settings: Configure to preserve CP949 encoding

2. **Google Translate / DeepL**
   - First-pass automatic translation
   - Copy Korean text, translate, manually refine

3. **Notepad++ with Korean Plugin**
   - Handles CP949 encoding natively
   - Good for quick edits

### Translation Workflow
```
1. Open file in CP949-aware editor
2. Identify Korean comment block
3. Copy Korean text to translator
4. Get English translation
5. Refine for technical accuracy
6. Replace Korean with English
7. Preserve code formatting
8. Test compile
9. Commit section by section
```

### Common Korean Game Terms Reference

| Korean | English |
|--------|---------|
| 클라이언트 | Client |
| 서버 | Server |
| 아이템 | Item |
| 캐릭터 | Character |
| 길드 | Guild |
| 경험치 | Experience |
| 공격 | Attack |
| 방어 | Defense |
| 마법 | Magic |
| 스킬 | Skill |
| 맵 | Map |
| NPC | NPC |
| 퀘스트 | Quest |
| 인벤토리 | Inventory |
| 장착 | Equip |
| 해킹 | Hacking |
| 초기화 | Initialize |
| 생성 | Create |
| 삭제 | Delete |
| 요청 | Request |
| 응답 | Response |
| 전송 | Send/Transmit |
| 수신 | Receive |

---

## Progress Tracking

### Completion Checklist

**Phase 1 - Quick Wins (27 lines):**
- [ ] Sources/Client/Game.h (1)
- [ ] Sources/Server/Wmain.cpp (2)
- [ ] Sources/Server/Magic.h (2)
- [ ] Sources/Server/BuildItem.h (3)
- [ ] Sources/Server/Skill.h (3)
- [ ] Sources/Server/StrategicPoint.h (3)
- [ ] Sources/Server/Fish.h (5)
- [ ] Sources/Server/Tile.h (5)

**Phase 2 - Protocol (89 lines):**
- [ ] Dependencies/Shared/NetMessages.h (23)
- [ ] Sources/Server/XSocket.h (18)
- [ ] Sources/Server/XSocket.cpp (48)

**Phase 3 - Core Headers (286 lines):**
- [ ] Sources/Server/Client.h (108)
- [ ] Sources/Server/Item.h (64)
- [ ] Sources/Server/Npc.h (62)
- [ ] Sources/Server/Quest.h (31)
- [ ] Sources/Server/Map.h (12)
- [ ] Sources/Server/Misc.h (9)

**Phase 4 - Implementation (143 lines):**
- [ ] Sources/Client/Game.cpp (44)
- [ ] Sources/Server/Game.h (30)
- [ ] Sources/Server/Map.cpp (20)
- [ ] Sources/Server/Client.cpp (20)
- [ ] Sources/Client/Item.h (9)

**Phase 5 - The Big One (2,668 lines):**
- [ ] Sources/Server/Game.cpp Section 1 (lines 1-300)
- [ ] Sources/Server/Game.cpp Section 2 (lines 301-600)
- [ ] Sources/Server/Game.cpp Section 3 (lines 601-900)
- [ ] Sources/Server/Game.cpp Section 4 (lines 901-1200)
- [ ] Sources/Server/Game.cpp Section 5 (lines 1201-1500)
- [ ] Sources/Server/Game.cpp Section 6 (lines 1501-1800)
- [ ] Sources/Server/Game.cpp Section 7 (lines 1801-2100)
- [ ] Sources/Server/Game.cpp Section 8 (lines 2101-2400)
- [ ] Sources/Server/Game.cpp Section 9 (lines 2401-end)

### Total Estimated Effort
- **Phase 1:** 1-2 hours
- **Phase 2:** 4-6 hours
- **Phase 3:** 6-8 hours
- **Phase 4:** 6-8 hours
- **Phase 5:** 40-60 hours
- **Total:** 57-84 hours (~1.5 to 2 work weeks)

---

## Success Criteria

- [ ] All Korean comments translated to English
- [ ] Technical accuracy verified (no mistranslations that change meaning)
- [ ] Code still compiles after all changes
- [ ] Glossary document created for future reference
- [ ] DONE.md updated with completion details
- [ ] TODO.md Task #6 marked as complete

---

## Risk Mitigation

1. **Encoding corruption:**
   - Always use CP949-aware editors
   - Test compile frequently
   - Commit small batches to Git

2. **Translation errors:**
   - Have Korean speaker review critical sections
   - Cross-reference with game behavior for context
   - When uncertain, keep original Korean as secondary comment

3. **Massive scope (Game.cpp):**
   - Break into small sections
   - Take breaks to avoid burnout
   - Prioritize functional areas that need documentation most

4. **Time investment:**
   - This is a long-term task (60-80 hours)
   - Consider doing in parallel with other work
   - Phases 1-4 can be done relatively quickly for 80% coverage

---

## Notes

- **CP949 Encoding:** Korean Windows codepage - must be preserved
- **Spanish comments found:** Line 2234 in Server Game.cpp has Spanish comment about "nombre del char" - also translate
- **Existing UTF-8 Korean:** Line 954 in Server Game.cpp has proper UTF-8 Korean that displays correctly
- **Mixed encoding:** Some files may have mix of CP949 and UTF-8 Korean
- **Original TODO.md note:** Translation task was marked as "In Progress" with encoding challenges documented

**Related TODO:** Task #6 - Translate Non-English Comments to English (Partially Complete)
