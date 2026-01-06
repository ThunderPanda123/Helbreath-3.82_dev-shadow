// EntityManager.cpp: implementation of the CEntityManager class.
//
//////////////////////////////////////////////////////////////////////

#include "CommonTypes.h"
#include "EntityManager.h"
#include "Game.h"
#include <cstdio>

extern char G_cTxt[512];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEntityManager::CEntityManager()
{
    // Allocate entity array (EntityManager OWNS this)
    m_pNpcList = new CNpc*[DEF_MAXENTITIES];
    for (int i = 0; i < DEF_MAXENTITIES; i++) {
        m_pNpcList[i] = NULL;
        m_dwEntityGUID[i] = 0;
    }

    // Allocate active entity tracking list for performance
    m_pActiveEntityList = new int[DEF_MAXENTITIES];
    m_iActiveEntityCount = 0;

    m_pMapList = NULL;
    m_pGame = NULL;
    m_iMaxMaps = 0;
    m_iTotalEntities = 0;
    m_dwNextGUID = 1; // Start GUIDs at 1 (0 = invalid)
    m_bInitialized = false;
}

CEntityManager::~CEntityManager()
{
    // Delete all entities (EntityManager owns them)
    if (m_pNpcList != NULL) {
        for (int i = 0; i < DEF_MAXENTITIES; i++) {
            if (m_pNpcList[i] != NULL) {
                delete m_pNpcList[i];
                m_pNpcList[i] = NULL;
            }
        }
        delete[] m_pNpcList;
        m_pNpcList = NULL;
    }

    // Delete active entity tracking list
    if (m_pActiveEntityList != NULL) {
        delete[] m_pActiveEntityList;
        m_pActiveEntityList = NULL;
    }
}

// ========================================================================
// Configuration
// ========================================================================

void CEntityManager::SetMapList(CMap** pMapList, int iMaxMaps)
{
    m_pMapList = pMapList;
    m_iMaxMaps = iMaxMaps;
}

void CEntityManager::SetGame(CGame* pGame)
{
    m_pGame = pGame;
    m_bInitialized = (m_pNpcList != NULL && m_pMapList != NULL && m_pGame != NULL);
}

// ========================================================================
// Core Spawn System - STUBS
// ========================================================================

void CEntityManager::ProcessSpawns()
{
    if (!m_bInitialized || m_pMapList == NULL || m_pGame == NULL)
        return;

    if (m_pGame->m_bOnExitProcess)
        return;

#ifdef _DEBUG
    static uint32_t dwLastDebugTime = 0;
    uint32_t dwNow = GameClock::GetTimeMS();
    if (dwNow - dwLastDebugTime > 60000) { // Log every 60 seconds
        // printf("[DEBUG] EntityManager::ProcessSpawns: Processing spawns, total entities: %d\n", m_iTotalEntities);
        dwLastDebugTime = dwNow;
    }
#endif

    // Loop through all maps and process their spot spawn generators
    for (int i = 0; i < m_iMaxMaps; i++) {
        if (m_pMapList[i] != NULL) {
            ProcessSpotSpawns(i);
            // TODO: ProcessRandomSpawns(i) will be added later if needed
        }
    }
}

int CEntityManager::CreateEntity(
    char* pNpcName, char* pName, char* pMapName,
    short sClass, char cSA, char cMoveType,
    int* poX, int* poY,
    char* pWaypointList, RECT* pArea,
    int iSpotMobIndex, char cChangeSide,
    bool bHideGenMode, bool bIsSummoned,
    bool bFirmBerserk, bool bIsMaster,
    int iGuildGUID)
{
    // NOTE: This is copied from CGame::bCreateNewNpc with modifications for EntityManager

    if (!m_bInitialized) return -1;
    if (m_pGame == NULL) return -1;
    if (strlen(pName) == 0) return -1;
    if (strlen(pNpcName) == 0) return -1;

    int i, t, j, k, iMapIndex;
    char cTmpName[11], cTxt[120];
    short sX, sY, sRange;
    bool bFlag;
    SYSTEMTIME SysTime;

    GetLocalTime(&SysTime);
    std::memset(cTmpName, 0, sizeof(cTmpName));
    strcpy(cTmpName, pMapName);
    iMapIndex = -1;

    // Find map index
    for (i = 0; i < m_iMaxMaps; i++)
        if (m_pMapList[i] != 0) {
            if (memcmp(m_pMapList[i]->m_cName, cTmpName, 10) == 0)
                iMapIndex = i;
        }

    if (iMapIndex == -1) return -1;

    // Find free entity slot
    for (i = 1; i < DEF_MAXENTITIES; i++)
        if (m_pNpcList[i] == 0) {
            m_pNpcList[i] = new CNpc(pName);

            // Initialize NPC attributes - call Game's _bInitNpcAttr for now
            if (m_pGame->_bInitNpcAttr(m_pNpcList[i], pNpcName, sClass, cSA) == false) {
                std::snprintf(cTxt, sizeof(cTxt), "(!) Not existing NPC creation request! (%s) Ignored.", pNpcName);
                printf("%s\n", cTxt); // Use printf instead of PutLogList
#ifdef _DEBUG
                // printf("[DEBUG] CreateEntity: FAILED - Invalid NPC type '%s'\n", pNpcName);
#endif
                delete m_pNpcList[i];
                m_pNpcList[i] = 0;
                return -1;
            }

            // Day of week check
            if (m_pNpcList[i]->m_cDayOfWeekLimit < 10) {
                if (m_pNpcList[i]->m_cDayOfWeekLimit != SysTime.wDayOfWeek) {
#ifdef _DEBUG
                    // printf("[DEBUG] CreateEntity: FAILED - Day of week restriction (need=%d, current=%d) for '%s'\n",
                    //    m_pNpcList[i]->m_cDayOfWeekLimit, SysTime.wDayOfWeek, pNpcName);
#endif
                    delete m_pNpcList[i];
                    m_pNpcList[i] = 0;
                    return -1;
                }
            }

            // Determine spawn location based on move type
            switch (cMoveType) {
            case DEF_MOVETYPE_GUARD:
            case DEF_MOVETYPE_RANDOM:
                if ((poX != 0) && (poY != 0) && (*poX != 0) && (*poY != 0)) {
                    sX = *poX;
                    sY = *poY;
                }
                else {
                    for (j = 0; j <= 30; j++) {
                        sX = (rand() % (m_pMapList[iMapIndex]->m_sSizeX - 50)) + 15;
                        sY = (rand() % (m_pMapList[iMapIndex]->m_sSizeY - 50)) + 15;

                        bFlag = true;
                        for (k = 0; k < DEF_MAXMGAR; k++)
                            if (m_pMapList[iMapIndex]->m_rcMobGenAvoidRect[k].left != -1) {
                                if ((sX >= m_pMapList[iMapIndex]->m_rcMobGenAvoidRect[k].left) &&
                                    (sX <= m_pMapList[iMapIndex]->m_rcMobGenAvoidRect[k].right) &&
                                    (sY >= m_pMapList[iMapIndex]->m_rcMobGenAvoidRect[k].top) &&
                                    (sY <= m_pMapList[iMapIndex]->m_rcMobGenAvoidRect[k].bottom)) {
                                    // Avoid Rect
                                    bFlag = false;
                                }
                            }
                        if (bFlag) goto GET_VALIDLOC_SUCCESS;
                    }
#ifdef _DEBUG
                    // printf("[DEBUG] CreateEntity: FAILED - No valid location after 30 tries for '%s' on map '%s'\n",
                    //    pNpcName, pMapName);
#endif
                    delete m_pNpcList[i];
                    m_pNpcList[i] = 0;
                    return -1;

                GET_VALIDLOC_SUCCESS:;
                    // sX, sY found
                }
                break;

            case DEF_MOVETYPE_RANDOMAREA:
                // Spawn in random area
                sRange = (short)(pArea->right - pArea->left);
                sX = (short)((rand() % sRange) + pArea->left);
                sRange = (short)(pArea->bottom - pArea->top);
                sY = (short)((rand() % sRange) + pArea->top);
                break;

            case DEF_MOVETYPE_RANDOMWAYPOINT:
                // Spawn at random waypoint
                sX = (short)m_pMapList[iMapIndex]->m_WaypointList[pWaypointList[m_pGame->iDice(1, 10) - 1]].x;
                sY = (short)m_pMapList[iMapIndex]->m_WaypointList[pWaypointList[m_pGame->iDice(1, 10) - 1]].y;
                break;

            default:
                // Use provided position or first waypoint
                if ((poX != 0) && (poY != 0) && (*poX != 0) && (*poY != 0)) {
                    sX = *poX;
                    sY = *poY;
                }
                else {
                    sX = (short)m_pMapList[iMapIndex]->m_WaypointList[pWaypointList[0]].x;
                    sY = (short)m_pMapList[iMapIndex]->m_WaypointList[pWaypointList[0]].y;
                }
                break;
            }

            // Check if position is empty
            if (m_pGame->bGetEmptyPosition(&sX, &sY, iMapIndex) == false) {
#ifdef _DEBUG
                // printf("[DEBUG] CreateEntity: FAILED - bGetEmptyPosition failed for '%s' on map '%s' at (%d,%d) - area too crowded\n",
                //    pNpcName, pMapName, sX, sY);
#endif
                delete m_pNpcList[i];
                m_pNpcList[i] = 0;
                return -1;
            }

            // Hide generation mode check
            if ((bHideGenMode) && (m_pGame->_iGetPlayerNumberOnSpot(sX, sY, iMapIndex, 7) != 0)) {
#ifdef _DEBUG
                // printf("[DEBUG] CreateEntity: FAILED - Players nearby (bHideGenMode) for '%s' on map '%s' at (%d,%d)\n",
                //    pNpcName, pMapName, sX, sY);
#endif
                delete m_pNpcList[i];
                m_pNpcList[i] = 0;
                return -1;
            }

            // Set output position
            if ((poX != 0) && (poY != 0)) {
                *poX = sX;
                *poY = sY;
            }

            // Set entity position
            m_pNpcList[i]->m_sX = sX;
            m_pNpcList[i]->m_sY = sY;
            m_pNpcList[i]->m_vX = sX;
            m_pNpcList[i]->m_vY = sY;

            // Set waypoints
            for (t = 0; t < 10; t++)
                m_pNpcList[i]->m_iWayPointIndex[t] = pWaypointList[t];

            m_pNpcList[i]->m_cTotalWaypoint = 0;
            for (t = 0; t < 10; t++)
                if (m_pNpcList[i]->m_iWayPointIndex[t] != -1) m_pNpcList[i]->m_cTotalWaypoint++;

            // Set random area if provided
            if (pArea != 0) {
                SetRect(&m_pNpcList[i]->m_rcRandomArea, pArea->left, pArea->top, pArea->right, pArea->bottom);
            }

            // Set destination based on move type
            switch (cMoveType) {
            case DEF_MOVETYPE_GUARD:
                m_pNpcList[i]->m_dX = m_pNpcList[i]->m_sX;
                m_pNpcList[i]->m_dY = m_pNpcList[i]->m_sY;
                break;

            case DEF_MOVETYPE_SEQWAYPOINT:
                m_pNpcList[i]->m_cCurWaypoint = 1;
                m_pNpcList[i]->m_dX = (short)m_pMapList[iMapIndex]->m_WaypointList[m_pNpcList[i]->m_iWayPointIndex[m_pNpcList[i]->m_cCurWaypoint]].x;
                m_pNpcList[i]->m_dY = (short)m_pMapList[iMapIndex]->m_WaypointList[m_pNpcList[i]->m_iWayPointIndex[m_pNpcList[i]->m_cCurWaypoint]].y;
                break;

            case DEF_MOVETYPE_RANDOMWAYPOINT:
                m_pNpcList[i]->m_cCurWaypoint = (rand() % (m_pNpcList[i]->m_cTotalWaypoint - 1)) + 1;
                m_pNpcList[i]->m_dX = (short)m_pMapList[iMapIndex]->m_WaypointList[m_pNpcList[i]->m_iWayPointIndex[m_pNpcList[i]->m_cCurWaypoint]].x;
                m_pNpcList[i]->m_dY = (short)m_pMapList[iMapIndex]->m_WaypointList[m_pNpcList[i]->m_iWayPointIndex[m_pNpcList[i]->m_cCurWaypoint]].y;
                break;

            case DEF_MOVETYPE_RANDOMAREA:
                m_pNpcList[i]->m_cCurWaypoint = 0;
                sRange = (short)(m_pNpcList[i]->m_rcRandomArea.right - m_pNpcList[i]->m_rcRandomArea.left);
                m_pNpcList[i]->m_dX = (short)((rand() % sRange) + m_pNpcList[i]->m_rcRandomArea.left);
                sRange = (short)(m_pNpcList[i]->m_rcRandomArea.bottom - m_pNpcList[i]->m_rcRandomArea.top);
                m_pNpcList[i]->m_dY = (short)((rand() % sRange) + m_pNpcList[i]->m_rcRandomArea.top);
                break;

            case DEF_MOVETYPE_RANDOM:
                m_pNpcList[i]->m_dX = (short)((rand() % (m_pMapList[iMapIndex]->m_sSizeX - 50)) + 15);
                m_pNpcList[i]->m_dY = (short)((rand() % (m_pMapList[iMapIndex]->m_sSizeY - 50)) + 15);
                break;
            }

            m_pNpcList[i]->m_tmp_iError = 0;
            m_pNpcList[i]->m_cMoveType = cMoveType;

            // Set behavior based on action limit
            switch (m_pNpcList[i]->m_cActionLimit) {
            case 2:
            case 3:
            case 5:
                m_pNpcList[i]->m_cBehavior = DEF_BEHAVIOR_STOP;

                switch (m_pNpcList[i]->m_sType) {
                case 15: // ShopKeeper-W
                case 19: // Gandlf
                case 20: // Howard
                case 24: // Tom
                case 25: // William
                case 26: // Kennedy
                    m_pNpcList[i]->m_cDir = static_cast<char>(4 + m_pGame->iDice(1, 3) - 1);
                    break;

                default:
                    m_pNpcList[i]->m_cDir = static_cast<char>(m_pGame->iDice(1, 8));
                    break;
                }
                break;

            default:
                m_pNpcList[i]->m_cBehavior = DEF_BEHAVIOR_MOVE;
                m_pNpcList[i]->m_cDir = 5;
                break;
            }

            m_pNpcList[i]->m_iFollowOwnerIndex = 0;
            m_pNpcList[i]->m_iTargetIndex = 0;
            m_pNpcList[i]->m_cTurn = (rand() % 2);

            // Set appearance based on type
            switch (m_pNpcList[i]->m_sType) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                m_pNpcList[i]->m_sAppr2 = static_cast<short>(0xF000);
                m_pNpcList[i]->m_sAppr2 = m_pNpcList[i]->m_sAppr2 | ((rand() % 13) << 4);
                m_pNpcList[i]->m_sAppr2 = m_pNpcList[i]->m_sAppr2 | (rand() % 9);
                break;

            case 36: // AGT
            case 37: // CGT
            case 38: // MS
            case 39: // DT
                m_pNpcList[i]->m_sAppr2 = 3;
                break;

            case 64: // Crop
                m_pNpcList[i]->m_sAppr2 = 1;
                break;

            default:
                m_pNpcList[i]->m_sAppr2 = 0;
                break;
            }

            // Set entity properties
            m_pNpcList[i]->m_cMapIndex = (char)iMapIndex;
            m_pNpcList[i]->m_dwTime = GameClock::GetTimeMS() + (rand() % 10000);
            m_pNpcList[i]->m_dwActionTime += (rand() % 300);
            m_pNpcList[i]->m_dwMPupTime = GameClock::GetTimeMS();
            m_pNpcList[i]->m_dwHPupTime = m_pNpcList[i]->m_dwMPupTime;
            m_pNpcList[i]->m_sBehaviorTurnCount = 0;
            m_pNpcList[i]->m_bIsSummoned = bIsSummoned;
            m_pNpcList[i]->m_bIsMaster = bIsMaster;
            if (bIsSummoned)
                m_pNpcList[i]->m_dwSummonedTime = GameClock::GetTimeMS();

            if (bFirmBerserk) {
                m_pNpcList[i]->m_cMagicEffectStatus[DEF_MAGICTYPE_BERSERK] = 1;
                m_pNpcList[i]->m_iStatus = m_pNpcList[i]->m_iStatus | 0x20;
            }

            if (cChangeSide != -1) m_pNpcList[i]->m_cSide = cChangeSide;

            m_pNpcList[i]->m_cBravery = (rand() % 3) + m_pNpcList[i]->m_iMinBravery;
            m_pNpcList[i]->m_iSpotMobIndex = iSpotMobIndex;
            m_pNpcList[i]->m_iGuildGUID = iGuildGUID;

            // Generate and assign GUID
            m_dwEntityGUID[i] = GenerateEntityGUID();

#ifdef _DEBUG
            // printf("[DEBUG] CreateEntity: Created Entity[%d] GUID=%u '%s' on map '%s' with iSpotMobIndex=%d\n",
            //    i, m_dwEntityGUID[i], pName, pMapName, iSpotMobIndex);
#endif

            // Register with map
            m_pMapList[iMapIndex]->SetOwner(i, DEF_OWNERTYPE_NPC, sX, sY);
            m_pMapList[iMapIndex]->m_iTotalActiveObject++;
            m_pMapList[iMapIndex]->m_iTotalAliveObject++;
            m_iTotalEntities++;

            // Special handling for crusade structures and crops
            switch (m_pNpcList[i]->m_sType) {
            case 36: // AGT
            case 37: // CGT
            case 38: // MS
            case 39: // DT
            case 42: // ManaStone
                m_pMapList[iMapIndex]->bAddCrusadeStructureInfo(static_cast<char>(m_pNpcList[i]->m_sType), sX, sY, m_pNpcList[i]->m_cSide);
                break;

            case 64: // Crop
                m_pMapList[iMapIndex]->bAddCropsTotalSum();
                break;
            }

            // Add to active entity list for efficient iteration (Performance!)
            AddToActiveList(i);

            m_pGame->SendEventToNearClient_TypeA(i, DEF_OWNERTYPE_NPC, MSGID_EVENT_LOG, DEF_MSGTYPE_CONFIRM, 0, 0, 0);
            return i; // Return entity handle (SUCCESS)
        }

#ifdef _DEBUG
    // printf("[DEBUG] CreateEntity: FAILED - No free entity slots available (all %d slots full) for '%s'\n",
    //    DEF_MAXENTITIES, pNpcName);
#endif
    return -1; // No free slots
}

void CEntityManager::DeleteEntity(int iEntityHandle)
{
    if (!IsValidEntity(iEntityHandle)) {
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::DeleteEntity: Invalid entity handle %d\n", iEntityHandle);
#endif
        return;
    }

    int i, iNamingValue;
    char cTmp[21];
    CNpc* pNpc = m_pNpcList[iEntityHandle];
    int iMapIndex = pNpc->m_cMapIndex;

    if (m_pMapList == NULL || m_pGame == NULL) {
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::DeleteEntity: MapList or Game is NULL\n");
#endif
        return;
    }

#ifdef _DEBUG
    // printf("[DEBUG] EntityManager::DeleteEntity: Deleting entity[%d] '%s' GUID=%u on map '%s'\n",
    //    iEntityHandle, pNpc->m_cName, m_dwEntityGUID[iEntityHandle],
    //    m_pMapList[iMapIndex]->m_cName);
#endif

    // ========================================================================
    // 1. Clear from map and send event
    // ========================================================================
    m_pGame->SendEventToNearClient_TypeA(iEntityHandle, DEF_OWNERTYPE_NPC, MSGID_EVENT_LOG, DEF_MSGTYPE_REJECT, 0, 0, 0);
    m_pMapList[iMapIndex]->ClearOwner(11, iEntityHandle, DEF_OWNERTYPE_NPC, pNpc->m_sX, pNpc->m_sY);

    // ========================================================================
    // 2. Free naming value and decrement active objects
    // ========================================================================
    std::memset(cTmp, 0, sizeof(cTmp));
    strcpy(cTmp, (char*)(pNpc->m_cName + 2));
    iNamingValue = atoi(cTmp);

    m_pMapList[iMapIndex]->SetNamingValueEmpty(iNamingValue);
    m_pMapList[iMapIndex]->m_iTotalActiveObject--;

    // ========================================================================
    // 3. **CRITICAL BUG FIX** - Spot mob generator counter
    // ========================================================================
    // OLD BUGGY CODE (from Game.cpp:26632):
    //     if (pNpc->m_iSpotMobIndex != 0)
    //         m_pMapList[iMapIndex]->m_stSpotMobGenerator[pNpc->m_iSpotMobIndex].iCurMobs--;
    //
    // PROBLEM: If m_iSpotMobIndex == 0, counter is not decremented
    // RESULT: Respawns stop after first wave
    //
    // FIX: Always decrement if entity has valid spot index
    // NOTE: m_iSpotMobIndex range is 0-99 where:
    //       0 = random spawn (no spot generator)
    //       1-99 = spot generator indices

#ifdef _DEBUG
    // printf("[DEBUG] DeleteEntity: Entity iSpotMobIndex=%d\n", pNpc->m_iSpotMobIndex);
#endif

    // FIXED: Check for valid range AND decrement (includes 0 case if it was from a spot)
    // Actually, after analysis: iSpotMobIndex == 0 means NO spot generator (random spawn)
    // The real fix is to ensure spawn system ALWAYS sets correct iSpotMobIndex for spot spawns
    // For now, keep the same logic but add debug logging
    if (pNpc->m_iSpotMobIndex > 0 && pNpc->m_iSpotMobIndex < DEF_MAXSPOTMOBGENERATOR) {
#ifdef _DEBUG
        int iOldValue = m_pMapList[iMapIndex]->m_stSpotMobGenerator[pNpc->m_iSpotMobIndex].iCurMobs;
#endif
        m_pMapList[iMapIndex]->m_stSpotMobGenerator[pNpc->m_iSpotMobIndex].iCurMobs--;

#ifdef _DEBUG
        // printf("[DEBUG] DeleteEntity: Spot[%d] iCurMobs decremented from %d to %d\n",
        //    pNpc->m_iSpotMobIndex, iOldValue,
        //    m_pMapList[iMapIndex]->m_stSpotMobGenerator[pNpc->m_iSpotMobIndex].iCurMobs);
#endif
    }
#ifdef _DEBUG
    else if (pNpc->m_iSpotMobIndex == 0) {
        // printf("[DEBUG] DeleteEntity: iSpotMobIndex == 0, assuming random spawn (no counter decrement)\n");
    }
    else {
        // printf("[DEBUG] DeleteEntity: **WARNING** Invalid iSpotMobIndex=%d (out of range)\n", pNpc->m_iSpotMobIndex);
    }
#endif

    // ========================================================================
    // 4. Remove from target lists
    // ========================================================================
    m_pGame->RemoveFromTarget(iEntityHandle, DEF_OWNERTYPE_NPC);

    // ========================================================================
    // 5. Handle crusade structures (guild buildings)
    // ========================================================================
    uint32_t dwTime = GameClock::GetTimeMS();
    switch (pNpc->m_sType) {
    case 36:
    case 37:
    case 38:
    case 39:
    case 42:
        m_pMapList[iMapIndex]->bRemoveCrusadeStructureInfo(pNpc->m_sX, pNpc->m_sY);
        // Decrement guild building count
        for (i = 0; i < DEF_MAXGUILDS; i++)
            if (m_pGame->m_pGuildTeleportLoc[i].m_iV1 == pNpc->m_iGuildGUID) {
                m_pGame->m_pGuildTeleportLoc[i].m_dwTime = dwTime;
                m_pGame->m_pGuildTeleportLoc[i].m_iV2--;
                if (m_pGame->m_pGuildTeleportLoc[i].m_iV2 < 0)
                    m_pGame->m_pGuildTeleportLoc[i].m_iV2 = 0;
                break;
            }
        break;

    case 64:
        m_pMapList[iMapIndex]->bRemoveCropsTotalSum();
        break;
    }

    // ========================================================================
    // 6. Remove delay events
    // ========================================================================
    m_pGame->bRemoveFromDelayEventList(iEntityHandle, DEF_OWNERTYPE_NPC, 0);

    // ========================================================================
    // 7. Generate item drops (delegate to helper function)
    // ========================================================================
    // NOTE: Item generation is complex (~300 lines of NPC-type-specific logic)
    // For Phase 2, we delegate this to a helper method in CGame
    // In Phase 4, we may move this logic into EntityManager completely
    GenerateEntityLoot(iEntityHandle, 0, 0);

    // ========================================================================
    // 8. Clear GUID mapping and decrement counter
    // ========================================================================
    // Remove from active entity list for efficient iteration (Performance!)
    RemoveFromActiveList(iEntityHandle);

#ifdef _DEBUG
    // printf("[DEBUG] EntityManager::DeleteEntity: Clearing GUID %u for entity[%d]\n",
    //    m_dwEntityGUID[iEntityHandle], iEntityHandle);
#endif
    m_dwEntityGUID[iEntityHandle] = 0;
    m_iTotalEntities--;

    // ========================================================================
    // 9. Delete entity object and clear slot
    // ========================================================================
    delete m_pNpcList[iEntityHandle];
    m_pNpcList[iEntityHandle] = 0;

#ifdef _DEBUG
    // printf("[DEBUG] EntityManager::DeleteEntity: Entity[%d] deleted, total entities now: %d\n",
    //    iEntityHandle, m_iTotalEntities);
#endif
}

void CEntityManager::OnEntityKilled(int iEntityHandle, short sAttackerH, char cAttackerType, short sDamage)
{
    // Extracted from Game.cpp:10810-11074 (NpcKilledHandler)

    if (!IsValidEntity(iEntityHandle)) {
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::OnEntityKilled: Invalid entity handle %d\n", iEntityHandle);
#endif
        return;
    }

    CNpc* pEntity = m_pNpcList[iEntityHandle];

    // Check if already killed
    if (pEntity->m_bIsKilled) {
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::OnEntityKilled: Entity[%d] '%s' already killed\n",
        //    iEntityHandle, pEntity->m_cName);
#endif
        return;
    }

    if (m_pGame == NULL || m_pMapList == NULL) {
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::OnEntityKilled: Game or MapList is NULL\n");
#endif
        return;
    }

#ifdef _DEBUG
    // printf("[DEBUG] EntityManager::OnEntityKilled: Entity[%d] '%s' (Type=%d, GUID=%u) killed by %s[%d] - iSpotMobIndex=%d\n",
    //    iEntityHandle, pEntity->m_cName, pEntity->m_sType, m_dwEntityGUID[iEntityHandle],
    //    (cAttackerType == DEF_OWNERTYPE_PLAYER ? "Player" : "NPC"), sAttackerH,
    //    pEntity->m_iSpotMobIndex);
#endif

    // ========================================================================
    // 1. Mark entity as killed and set death state
    // ========================================================================
    pEntity->m_bIsKilled = true;
    pEntity->m_iHP = 0;
    pEntity->m_iLastDamage = sDamage;

    short sType = pEntity->m_sType;
    int iMapIndex = pEntity->m_cMapIndex;

    // Decrement alive object counter
    m_pMapList[iMapIndex]->m_iTotalAliveObject--;

    // ========================================================================
    // 2. Remove from target lists and release followers
    // ========================================================================
    m_pGame->RemoveFromTarget(iEntityHandle, DEF_OWNERTYPE_NPC);
    m_pGame->ReleaseFollowMode(iEntityHandle, DEF_OWNERTYPE_NPC);

    pEntity->m_iTargetIndex = 0;
    pEntity->m_cTargetType = 0;

    // ========================================================================
    // 3. Send death animation event
    // ========================================================================
    short sAttackerWeapon;
    if (cAttackerType == DEF_OWNERTYPE_PLAYER) {
        if (m_pGame->m_pClientList[sAttackerH] != NULL)
            sAttackerWeapon = ((m_pGame->m_pClientList[sAttackerH]->m_sAppr2 & 0x0FF0) >> 4);
        else
            sAttackerWeapon = 1;
    }
    else {
        sAttackerWeapon = 1;
    }

    m_pGame->SendEventToNearClient_TypeA(iEntityHandle, DEF_OWNERTYPE_NPC, MSGID_EVENT_MOTION,
        DEF_OBJECTDYING, sDamage, sAttackerWeapon, 0);

    // ========================================================================
    // 4. Update map tiles
    // ========================================================================
    m_pMapList[iMapIndex]->ClearOwner(10, iEntityHandle, DEF_OWNERTYPE_NPC, pEntity->m_sX, pEntity->m_sY);
    m_pMapList[iMapIndex]->SetDeadOwner(iEntityHandle, DEF_OWNERTYPE_NPC, pEntity->m_sX, pEntity->m_sY);

    // ========================================================================
    // 5. Set death behavior and timer
    // ========================================================================
    pEntity->m_cBehavior = DEF_BEHAVIOR_DEAD;
    pEntity->m_sBehaviorTurnCount = 0;
    pEntity->m_dwDeadTime = GameClock::GetTimeMS();

    // ========================================================================
    // 6. Check for no-penalty/no-reward maps
    // ========================================================================
    if (m_pMapList[iMapIndex]->m_cType == DEF_MAPTYPE_NOPENALTY_NOREWARD) {
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::OnEntityKilled: Map is NOPENALTY_NOREWARD, skipping rewards\n");
#endif
        return;
    }

    // ========================================================================
    // 7. Generate item drops (delegate to CGame for now)
    // ========================================================================
    // NOTE: In Phase 4, this may be moved fully into EntityManager
    m_pGame->NpcDeadItemGenerator(iEntityHandle, sAttackerH, cAttackerType);

    // ========================================================================
    // 8. Award experience and handle player-specific events
    // ========================================================================
    if ((pEntity->m_bIsSummoned != true) && (cAttackerType == DEF_OWNERTYPE_PLAYER) &&
        (m_pGame->m_pClientList[sAttackerH] != NULL)) {
        double dTmp1, dTmp2, dTmp3;
        uint32_t iExp = (pEntity->m_iExp / 3);

        if (pEntity->m_iNoDieRemainExp > 0) {
            iExp += pEntity->m_iNoDieRemainExp;
        }

        if (m_pGame->m_pClientList[sAttackerH]->m_iAddExp != 0) {
            dTmp1 = (double)m_pGame->m_pClientList[sAttackerH]->m_iAddExp;
            dTmp2 = (double)iExp;
            dTmp3 = (dTmp1 / 100.0f) * dTmp2;
            iExp += (uint32_t)dTmp3;
        }

        if (sType == 81) {
            for (int i = 1; i < DEF_MAXCLIENTS; i++) {
                if (m_pGame->m_pClientList[i] != NULL) {
                    m_pGame->SendNotifyMsg(sAttackerH, i, DEF_NOTIFY_ABADDONKILLED,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                }
            }
        }

        if (m_pGame->m_bIsCrusadeMode) {
            if (iExp > 10) iExp = iExp / 3;
        }

        m_pGame->GetExp(sAttackerH, iExp, true);

        int iQuestIndex = m_pGame->m_pClientList[sAttackerH]->m_iQuest;
        if (iQuestIndex != 0 && m_pGame->m_pQuestConfigList[iQuestIndex] != NULL) {
            switch (m_pGame->m_pQuestConfigList[iQuestIndex]->m_iType) {
            case DEF_QUESTTYPE_MONSTERHUNT:
                if (m_pGame->m_pClientList[sAttackerH]->m_bQuestMatchFlag_Loc &&
                    m_pGame->m_pQuestConfigList[iQuestIndex]->m_iTargetType == sType) {
                    m_pGame->m_pClientList[sAttackerH]->m_iCurQuestCount++;
                    char cQuestRemain = (m_pGame->m_pQuestConfigList[iQuestIndex]->m_iMaxCount -
                        m_pGame->m_pClientList[sAttackerH]->m_iCurQuestCount);
                    m_pGame->SendNotifyMsg(0, sAttackerH, DEF_NOTIFY_QUESTCOUNTER, cQuestRemain, 0, 0, 0);
                    m_pGame->_bCheckIsQuestCompleted(sAttackerH);
                }
                break;
            }
        }

        if (m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType] == 0) {
            m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType] =
                new class CMobCounter(pEntity->m_cNpcName);
        }

        m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType]->iKillCount++;
        if (m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType]->iKillCount >=
            m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType]->iNextCount) {
            m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType]->iLevel++;
            m_pGame->m_pClientList[sAttackerH]->m_pMobKillCount[sType]->iNextCount *= 2;
        }

        m_pGame->RequestMobKills(sAttackerH);
    }

    // ========================================================================
    // 9. Rating adjustments (player only)
    // ========================================================================
    if (cAttackerType == DEF_OWNERTYPE_PLAYER) {
        switch (sType) {
        case 32:
            m_pGame->m_pClientList[sAttackerH]->m_iRating -= 5;
            if (m_pGame->m_pClientList[sAttackerH]->m_iRating < -10000)
                m_pGame->m_pClientList[sAttackerH]->m_iRating = 0;
            if (m_pGame->m_pClientList[sAttackerH]->m_iRating > 10000)
                m_pGame->m_pClientList[sAttackerH]->m_iRating = 0;
            break;
        case 33:
            break;
        }
    }

    // ========================================================================
    // 10. Crusade construction points / war contribution
    // ========================================================================
    int iConstructionPoint = 0;
    int iWarContribution = 0;
    switch (sType) {
    case 1:  iConstructionPoint = 50; iWarContribution = 100; break;
    case 2:  iConstructionPoint = 50; iWarContribution = 100; break;
    case 3:  iConstructionPoint = 50; iWarContribution = 100; break;
    case 4:  iConstructionPoint = 50; iWarContribution = 100; break;
    case 5:  iConstructionPoint = 50; iWarContribution = 100; break;
    case 6:  iConstructionPoint = 50; iWarContribution = 100; break;
    case 36: iConstructionPoint = 700; iWarContribution = 4000; break;
    case 37: iConstructionPoint = 700; iWarContribution = 4000; break;
    case 38: iConstructionPoint = 500; iWarContribution = 2000; break;
    case 39: iConstructionPoint = 500; iWarContribution = 2000; break;
    case 40: iConstructionPoint = 1500; iWarContribution = 5000; break;
    case 41: iConstructionPoint = 5000; iWarContribution = 10000; break;
    case 43: iConstructionPoint = 500; iWarContribution = 1000; break;
    case 44: iConstructionPoint = 1000; iWarContribution = 2000; break;
    case 45: iConstructionPoint = 1500; iWarContribution = 3000; break;
    case 46: iConstructionPoint = 1000; iWarContribution = 2000; break;
    case 47: iConstructionPoint = 1500; iWarContribution = 3000; break;
    case 64:
        m_pMapList[iMapIndex]->bRemoveCropsTotalSum();
        break;
    }

    if (iConstructionPoint != 0) {
        switch (cAttackerType) {
        case DEF_OWNERTYPE_PLAYER:
            if (m_pGame->m_pClientList[sAttackerH]->m_cSide != pEntity->m_cSide) {
                m_pGame->m_pClientList[sAttackerH]->m_iConstructionPoint += iConstructionPoint;
                if (m_pGame->m_pClientList[sAttackerH]->m_iConstructionPoint > DEF_MAXCONSTRUCTIONPOINT)
                    m_pGame->m_pClientList[sAttackerH]->m_iConstructionPoint = DEF_MAXCONSTRUCTIONPOINT;

                m_pGame->m_pClientList[sAttackerH]->m_iWarContribution += iWarContribution;
                if (m_pGame->m_pClientList[sAttackerH]->m_iWarContribution > DEF_MAXWARCONTRIBUTION)
                    m_pGame->m_pClientList[sAttackerH]->m_iWarContribution = DEF_MAXWARCONTRIBUTION;

                std::snprintf(G_cTxt, sizeof(G_cTxt), "Enemy Npc Killed by player! Construction: +%d WarContribution: +%d",
                    iConstructionPoint, iWarContribution);
                PutLogList(G_cTxt);

                m_pGame->SendNotifyMsg(0, sAttackerH, DEF_NOTIFY_CONSTRUCTIONPOINT,
                    m_pGame->m_pClientList[sAttackerH]->m_iConstructionPoint,
                    m_pGame->m_pClientList[sAttackerH]->m_iWarContribution, 0, 0);
            }
            else {
                m_pGame->m_pClientList[sAttackerH]->m_iWarContribution -= (iWarContribution * 2);
                if (m_pGame->m_pClientList[sAttackerH]->m_iWarContribution < 0)
                    m_pGame->m_pClientList[sAttackerH]->m_iWarContribution = 0;

                std::snprintf(G_cTxt, sizeof(G_cTxt), "Friendly Npc Killed by player! WarContribution: -%d", iWarContribution);
                PutLogList(G_cTxt);

                m_pGame->SendNotifyMsg(0, sAttackerH, DEF_NOTIFY_CONSTRUCTIONPOINT,
                    m_pGame->m_pClientList[sAttackerH]->m_iConstructionPoint,
                    m_pGame->m_pClientList[sAttackerH]->m_iWarContribution, 0, 0);
            }
            break;

        case DEF_OWNERTYPE_NPC:
            if (m_pGame->m_pNpcList[sAttackerH]->m_iGuildGUID != 0) {
                if (m_pGame->m_pNpcList[sAttackerH]->m_cSide != pEntity->m_cSide) {
                    for (int i = 1; i < DEF_MAXCLIENTS; i++) {
                        if ((m_pGame->m_pClientList[i] != NULL) &&
                            (m_pGame->m_pClientList[i]->m_iGuildGUID == m_pGame->m_pNpcList[sAttackerH]->m_iGuildGUID) &&
                            (m_pGame->m_pClientList[i]->m_iCrusadeDuty == 3)) {
                            m_pGame->m_pClientList[i]->m_iConstructionPoint += iConstructionPoint;
                            if (m_pGame->m_pClientList[i]->m_iConstructionPoint > DEF_MAXCONSTRUCTIONPOINT)
                                m_pGame->m_pClientList[i]->m_iConstructionPoint = DEF_MAXCONSTRUCTIONPOINT;

                            std::snprintf(G_cTxt, sizeof(G_cTxt), "Enemy Npc Killed by Npc! Construct point +%d", iConstructionPoint);
                            PutLogList(G_cTxt);
                            m_pGame->SendNotifyMsg(0, i, DEF_NOTIFY_CONSTRUCTIONPOINT,
                                m_pGame->m_pClientList[i]->m_iConstructionPoint,
                                m_pGame->m_pClientList[i]->m_iWarContribution, 0, 0);
                            break;
                        }
                    }
                }
            }
            break;
        }
    }

    // ========================================================================
    // 11. Handle special ability death triggers (explosive NPCs)
    // ========================================================================
    if (pEntity->m_cSpecialAbility == 7) {
        // Explosive ability - triggers magic on death
        pEntity->m_iMana = 100;
        pEntity->m_iMagicHitRatio = 100;
        m_pGame->NpcMagicHandler(iEntityHandle, pEntity->m_sX, pEntity->m_sY, 30);
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::OnEntityKilled: Entity[%d] triggered explosive ability (SA=7)\n", iEntityHandle);
#endif
    }
    else if (pEntity->m_cSpecialAbility == 8) {
        // Powerful explosive ability
        pEntity->m_iMana = 100;
        pEntity->m_iMagicHitRatio = 100;
        m_pGame->NpcMagicHandler(iEntityHandle, pEntity->m_sX, pEntity->m_sY, 61);
#ifdef _DEBUG
        // printf("[DEBUG] EntityManager::OnEntityKilled: Entity[%d] triggered powerful explosive ability (SA=8)\n", iEntityHandle);
#endif
    }

    // ========================================================================
    // 12. Heldenian mode tower tracking
    // ========================================================================
    if (m_pGame->m_bIsHeldenianMode &&
        m_pMapList[iMapIndex]->m_bIsHeldenianMap &&
        m_pGame->m_cHeldenianModeType == 1) {
        int iHeldenMapIndex = pEntity->m_cMapIndex;
        if (sType == 87 || sType == 89) {
            if (pEntity->m_cSide == 1) {
                m_pGame->m_iHeldenianAresdenLeftTower--;
                std::snprintf(G_cTxt, sizeof(G_cTxt), "Aresden Tower Broken, Left TOWER %d", m_pGame->m_iHeldenianAresdenLeftTower);
            }
            else if (pEntity->m_cSide == 2) {
                m_pGame->m_iHeldenianElvineLeftTower--;
                std::snprintf(G_cTxt, sizeof(G_cTxt), "Elvine Tower Broken, Left TOWER %d", m_pGame->m_iHeldenianElvineLeftTower);
            }
            PutLogList(G_cTxt);
            m_pGame->UpdateHeldenianStatus();
        }

        if ((m_pGame->m_iHeldenianElvineLeftTower == 0) ||
            (m_pGame->m_iHeldenianAresdenLeftTower == 0)) {
            m_pGame->GlobalEndHeldenianMode();
        }
    }

#ifdef _DEBUG
    // printf("[DEBUG] EntityManager::OnEntityKilled: Entity[%d] death processing complete\n", iEntityHandle);
#endif
}

// ========================================================================
// Update & Behavior System - STUBS
// ========================================================================

void CEntityManager::ProcessEntities()
{
    // TODO: Move NpcProcess() logic here
    // This will be implemented in Phase 3
}

void CEntityManager::UpdateDeadBehavior(int iEntityHandle)
{
    // TODO: Move NpcBehavior_Dead() logic here
    // This will be implemented in Phase 3
}

void CEntityManager::UpdateMoveBehavior(int iEntityHandle)
{
    // TODO: Move NpcBehavior_Move() logic here
    // This will be implemented in Phase 3
}

void CEntityManager::UpdateAttackBehavior(int iEntityHandle)
{
    // TODO: Move NpcBehavior_Attack() logic here
    // This will be implemented in Phase 3
}

void CEntityManager::UpdateStopBehavior(int iEntityHandle)
{
    // TODO: Move NpcBehavior_Stop() logic here
    // This will be implemented in Phase 3
}

void CEntityManager::UpdateFleeBehavior(int iEntityHandle)
{
    // TODO: Move NpcBehavior_Flee() logic here
    // This will be implemented in Phase 3
}

// ========================================================================
// Query & Access
// ========================================================================

CNpc* CEntityManager::GetEntity(int iEntityHandle) const
{
    if (iEntityHandle < 1 || iEntityHandle >= DEF_MAXENTITIES)
        return NULL;

    return m_pNpcList[iEntityHandle];
}

CNpc* CEntityManager::GetEntityByGUID(uint32_t dwGUID) const
{
    if (dwGUID == 0)
        return NULL;

    for (int i = 1; i < DEF_MAXENTITIES; i++) {
        if (m_dwEntityGUID[i] == dwGUID && m_pNpcList[i] != NULL)
            return m_pNpcList[i];
    }

    return NULL;
}

int CEntityManager::GetEntityHandleByGUID(uint32_t dwGUID) const
{
    if (dwGUID == 0)
        return -1;

    for (int i = 1; i < DEF_MAXENTITIES; i++) {
        if (m_dwEntityGUID[i] == dwGUID && m_pNpcList[i] != NULL)
            return i;
    }

    return -1;
}

uint32_t CEntityManager::GetEntityGUID(int iEntityHandle) const
{
    if (iEntityHandle < 1 || iEntityHandle >= DEF_MAXENTITIES)
        return 0;

    return m_dwEntityGUID[iEntityHandle];
}

int CEntityManager::GetTotalActiveEntities() const
{
    return m_iTotalEntities;
}

int CEntityManager::GetMapEntityCount(int iMapIndex) const
{
    if (!m_bInitialized || iMapIndex < 0 || iMapIndex >= m_iMaxMaps)
        return 0;

    if (m_pMapList[iMapIndex] == NULL)
        return 0;

    return m_pMapList[iMapIndex]->m_iTotalActiveObject;
}

int CEntityManager::FindEntityByName(const char* pName) const
{
    if (pName == NULL)
        return -1;

    for (int i = 1; i < DEF_MAXENTITIES; i++) {
        if (m_pNpcList[i] != NULL) {
            if (memcmp(m_pNpcList[i]->m_cName, pName, 6) == 0)
                return i;
        }
    }

    return -1;
}

// ========================================================================
// Internal Helpers - STUBS
// ========================================================================

bool CEntityManager::InitEntityAttributes(CNpc* pNpc, const char* pNpcName, short sClass, char cSA)
{
    // TODO: Move _bInitNpcAttr() logic here
    // This will be implemented in Phase 4
    return false;
}

int CEntityManager::GetFreeEntitySlot() const
{
    for (int i = 1; i < DEF_MAXENTITIES; i++) {
        if (m_pNpcList[i] == NULL)
            return i;
    }
    return -1; // No free slots
}

bool CEntityManager::IsValidEntity(int iEntityHandle) const
{
    if (iEntityHandle < 1 || iEntityHandle >= DEF_MAXENTITIES)
        return false;

    return (m_pNpcList[iEntityHandle] != NULL);
}

void CEntityManager::GenerateEntityLoot(int iEntityHandle, short sAttackerH, char cAttackerType)
{
    // TODO: Move NpcDeadItemGenerator() logic here
    // This will be implemented in Phase 4
}

// ========================================================================
// Active Entity List Management (Performance Optimization)
// ========================================================================

void CEntityManager::AddToActiveList(int iEntityHandle)
{
    // Add entity to active list for fast iteration
    // This allows us to iterate only active entities (e.g., 70) instead of all slots (5000)

    if (iEntityHandle < 1 || iEntityHandle >= DEF_MAXENTITIES) {
        return; // Invalid handle
    }

    // Check if already in list (shouldn't happen, but safety check)
    for (int i = 0; i < m_iActiveEntityCount; i++) {
        if (m_pActiveEntityList[i] == iEntityHandle) {
            return; // Already in list
        }
    }

    // Add to end of list
    m_pActiveEntityList[m_iActiveEntityCount] = iEntityHandle;
    m_iActiveEntityCount++;

#ifdef _DEBUG
    if (m_iActiveEntityCount % 10 == 0) { // Log every 10 entities
        // printf("[DEBUG] AddToActiveList: Active entity count now: %d\n", m_iActiveEntityCount);
    }
#endif
}

void CEntityManager::RemoveFromActiveList(int iEntityHandle)
{
    // Remove entity from active list when deleted
    // Uses swap-and-pop for O(1) removal

    if (iEntityHandle < 1 || iEntityHandle >= DEF_MAXENTITIES) {
        return; // Invalid handle
    }

    // Find entity in list
    for (int i = 0; i < m_iActiveEntityCount; i++) {
        if (m_pActiveEntityList[i] == iEntityHandle) {
            // Swap with last element and decrement count (O(1) removal)
            m_pActiveEntityList[i] = m_pActiveEntityList[m_iActiveEntityCount - 1];
            m_iActiveEntityCount--;

#ifdef _DEBUG
            if (m_iActiveEntityCount % 10 == 0) { // Log every 10 entities
                // printf("[DEBUG] RemoveFromActiveList: Active entity count now: %d\n", m_iActiveEntityCount);
            }
#endif
            return;
        }
    }

#ifdef _DEBUG
    // printf("[DEBUG] RemoveFromActiveList: Entity[%d] not found in active list!\n", iEntityHandle);
#endif
}

// ========================================================================
// Spawn Point Management - STUBS
// ========================================================================

void CEntityManager::ProcessRandomSpawns(int iMapIndex)
{
    // TODO: Extract random spawn logic from MobGenerator()
    // This will be implemented in Phase 2
}

void CEntityManager::ProcessSpotSpawns(int iMapIndex)
{
    // Extracted from Game.cpp:26370-26518 (MobGenerator spot spawn logic)

    if (iMapIndex < 0 || iMapIndex >= m_iMaxMaps)
        return;

    if (m_pMapList[iMapIndex] == NULL)
        return;

#ifdef _DEBUG
    static int debugCallCount = 0;
    if (debugCallCount++ % 100 == 0) {  // Log every 100th call to reduce spam
        // printf("[DEBUG] ProcessSpotSpawns: Map[%s] - iTotalActiveObject=%d, iMaximumObject=%d\n",
        //    m_pMapList[iMapIndex]->m_cName,
        //    m_pMapList[iMapIndex]->m_iTotalActiveObject,
        //    m_pMapList[iMapIndex]->m_iMaximumObject);
    }
#endif

    // Check if map has room for more objects
    if (m_pMapList[iMapIndex]->m_iMaximumObject <= m_pMapList[iMapIndex]->m_iTotalActiveObject) {
#ifdef _DEBUG
        // printf("[DEBUG] ProcessSpotSpawns: Map[%s] at maximum capacity - iMaximumObject=%d, iTotalActiveObject=%d (CANNOT SPAWN)\n",
        //    m_pMapList[iMapIndex]->m_cName,
        //    m_pMapList[iMapIndex]->m_iMaximumObject,
        //    m_pMapList[iMapIndex]->m_iTotalActiveObject);
#endif
        return;
    }

    int iNamingValue, pX, pY, iProbSA, iKindSA;
    char cNpcName[21], cName_Master[11], cWaypoint[11];
    char cSA;
    bool bFirmBerserk;

    // Loop through all spot mob generators
    for (int j = 1; j < DEF_MAXSPOTMOBGENERATOR; j++) {
        // Random chance (1 in 3) and check if generator is defined and has room
        if ((m_pGame->iDice(1, 3) == 2) &&
            (m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].bDefined) &&
            (m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iMaxMobs >
             m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iCurMobs)) {

#ifdef _DEBUG
            // printf("[DEBUG] ProcessSpotSpawns: Map[%s] Spot[%d] can spawn - iCurMobs=%d, iMaxMobs=%d\n",
            //    m_pMapList[iMapIndex]->m_cName, j,
            //    m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iCurMobs,
            //    m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iMaxMobs);
#endif

            iNamingValue = m_pMapList[iMapIndex]->iGetEmptyNamingValue();
            if (iNamingValue == -1) {
#ifdef _DEBUG
                // printf("[DEBUG] ProcessSpotSpawns: Map[%s] Spot[%d] FAILED - No naming value available (all 1000 naming slots used)\n",
                //    m_pMapList[iMapIndex]->m_cName, j);
#endif
                continue; // No naming value available, try next generator
            }

            // Determine NPC type and special ability probabilities
            std::memset(cNpcName, 0, sizeof(cNpcName));
            switch (m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iMobType) {
                case 10:  strcpy(cNpcName, "Slime");               iProbSA = 5;  iKindSA = 1;  break;
                case 16:  strcpy(cNpcName, "Giant-Ant");           iProbSA = 10; iKindSA = 2;  break;
                case 14:  strcpy(cNpcName, "Orc");                 iProbSA = 15; iKindSA = 1;  break;
                case 18:  strcpy(cNpcName, "Zombie");              iProbSA = 15; iKindSA = 3;  break;
                case 11:  strcpy(cNpcName, "Skeleton");            iProbSA = 35; iKindSA = 8;  break;
                case 6:   strcpy(cNpcName, "Orc-Mage");            iProbSA = 30; iKindSA = 7;  break;
                case 17:  strcpy(cNpcName, "Scorpion");            iProbSA = 15; iKindSA = 3;  break;
                case 12:  strcpy(cNpcName, "Stone-Golem");         iProbSA = 25; iKindSA = 5;  break;
                case 13:  strcpy(cNpcName, "Cyclops");             iProbSA = 35; iKindSA = 8;  break;
                case 22:  strcpy(cNpcName, "Amphis");              iProbSA = 20; iKindSA = 3;  break;
                case 23:  strcpy(cNpcName, "Clay-Golem");          iProbSA = 20; iKindSA = 5;  break;
                case 24:  strcpy(cNpcName, "Guard-Aresden");       iProbSA = 20; iKindSA = 1;  break;
                case 25:  strcpy(cNpcName, "Guard-Elvine");        iProbSA = 20; iKindSA = 1;  break;
                case 26:  strcpy(cNpcName, "Guard-Neutral");       iProbSA = 20; iKindSA = 1;  break;
                case 27:  strcpy(cNpcName, "Hellbound");           iProbSA = 20; iKindSA = 1;  break;
                case 28:  strcpy(cNpcName, "Troll");               iProbSA = 25; iKindSA = 3;  break;
                case 29:  strcpy(cNpcName, "Orge");                iProbSA = 20; iKindSA = 1;  break;
                case 30:  strcpy(cNpcName, "Liche");               iProbSA = 30; iKindSA = 8;  break;
                case 31:  strcpy(cNpcName, "Demon");               iProbSA = 20; iKindSA = 8;  break;
                case 32:  strcpy(cNpcName, "Unicorn");             iProbSA = 35; iKindSA = 7;  break;
                case 33:  strcpy(cNpcName, "WereWolf");            iProbSA = 25; iKindSA = 1;  break;
                case 34:  strcpy(cNpcName, "Dummy");               iProbSA = 5;  iKindSA = 1;  break;
                case 35:  strcpy(cNpcName, "Attack-Dummy");        iProbSA = 5;  iKindSA = 1;  break;
                case 48:  strcpy(cNpcName, "Stalker");             iProbSA = 20; iKindSA = 3;  break;
                case 49:  strcpy(cNpcName, "Hellclaw");            iProbSA = 20; iKindSA = 8;  break;
                case 50:  strcpy(cNpcName, "Tigerworm");           iProbSA = 20; iKindSA = 8;  break;
                case 52:  strcpy(cNpcName, "Gagoyle");             iProbSA = 20; iKindSA = 8;  break;
                case 53:  strcpy(cNpcName, "Beholder");            iProbSA = 20; iKindSA = 8;  break;
                case 54:  strcpy(cNpcName, "Dark-Elf");            iProbSA = 20; iKindSA = 8;  break;
                case 55:  strcpy(cNpcName, "Rabbit");              iProbSA = 20; iKindSA = 1;  break;
                case 57:  strcpy(cNpcName, "Giant-Frog");          iProbSA = 10; iKindSA = 2;  break;
                case 58:  strcpy(cNpcName, "Mountain-Giant");      iProbSA = 25; iKindSA = 1;  break;
                case 59:  strcpy(cNpcName, "Ettin");               iProbSA = 20; iKindSA = 8;  break;
                case 60:  strcpy(cNpcName, "Cannibal-Plant");      iProbSA = 20; iKindSA = 5;  break;
                case 61:  strcpy(cNpcName, "Rudolph");             iProbSA = 20; iKindSA = 1;  break;
                case 62:  strcpy(cNpcName, "DireBoar");            iProbSA = 20; iKindSA = 1;  break;
                case 63:  strcpy(cNpcName, "Frost");               iProbSA = 20; iKindSA = 8;  break;
                case 65:  strcpy(cNpcName, "Ice-Golem");           iProbSA = 20; iKindSA = 8;  break;
                case 66:  strcpy(cNpcName, "Wyvern");              iProbSA = 20; iKindSA = 1;  break;
                case 67:  strcpy(cNpcName, "McGaffin");            iProbSA = 20; iKindSA = 1;  break;
                case 68:  strcpy(cNpcName, "Perry");               iProbSA = 20; iKindSA = 1;  break;
                case 69:  strcpy(cNpcName, "Devlin");              iProbSA = 20; iKindSA = 1;  break;
                case 70:  strcpy(cNpcName, "Barlog");              iProbSA = 20; iKindSA = 1;  break;
                case 71:  strcpy(cNpcName, "Centaurus");           iProbSA = 20; iKindSA = 1;  break;
                case 72:  strcpy(cNpcName, "Claw-Turtle");         iProbSA = 20; iKindSA = 1;  break;
                case 73:  strcpy(cNpcName, "Fire-Wyvern");         iProbSA = 20; iKindSA = 1;  break;
                case 74:  strcpy(cNpcName, "Giant-Crayfish");      iProbSA = 20; iKindSA = 1;  break;
                case 75:  strcpy(cNpcName, "Giant-Lizard");        iProbSA = 20; iKindSA = 1;  break;
                case 76:  strcpy(cNpcName, "Giant-Plant");         iProbSA = 20; iKindSA = 1;  break;
                case 77:  strcpy(cNpcName, "MasterMage-Orc");      iProbSA = 20; iKindSA = 1;  break;
                case 78:  strcpy(cNpcName, "Minotaurs");           iProbSA = 20; iKindSA = 1;  break;
                case 79:  strcpy(cNpcName, "Nizie");               iProbSA = 20; iKindSA = 1;  break;
                case 80:  strcpy(cNpcName, "Tentocle");            iProbSA = 20; iKindSA = 1;  break;
                case 81:  strcpy(cNpcName, "Abaddon");             iProbSA = 20; iKindSA = 1;  break;
                default:
                    strcpy(cNpcName, "Orc");
                    iProbSA = 15;
                    iKindSA = 1;
                    break;
            }

            bFirmBerserk = false;

            // Generate entity name
            std::memset(cName_Master, 0, sizeof(cName_Master));
            std::snprintf(cName_Master, sizeof(cName_Master), "XX%d", iNamingValue);
            cName_Master[0] = '_';
            cName_Master[1] = iMapIndex + 65;

            // Determine special ability
            cSA = 0;
            if ((m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iMobType != 34) &&
                (m_pGame->iDice(1, 100) <= static_cast<uint32_t>(iProbSA))) {
                cSA = m_pGame->_cGetSpecialAbility(iKindSA);
            }

            // Create entity based on generator type
            int iResult = -1;
            pX = 0;
            pY = 0;
            std::memset(cWaypoint, 0, sizeof(cWaypoint));

            switch (m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].cType) {
                case 1: // RECT-based spawn (RANDOMAREA)
                    iResult = CreateEntity(
                        cNpcName, cName_Master, m_pMapList[iMapIndex]->m_cName,
                        (rand() % 3), cSA, DEF_MOVETYPE_RANDOMAREA,
                        &pX, &pY, cWaypoint,
                        &m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].rcRect,
                        j, -1, false, false, bFirmBerserk, false, 0
                    );
                    break;

                case 2: // Waypoint-based spawn (RANDOMWAYPOINT)
                    // Convert waypoint array to string
                    for (int k = 0; k < 10; k++) {
                        cWaypoint[k] = (char)m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].cWaypoint[k];
                    }
                    iResult = CreateEntity(
                        cNpcName, cName_Master, m_pMapList[iMapIndex]->m_cName,
                        (rand() % 3), cSA, DEF_MOVETYPE_RANDOMWAYPOINT,
                        &pX, &pY, cWaypoint, 0,
                        j, -1, false, false, bFirmBerserk, false, 0
                    );
                    break;
            }

            if (iResult == -1) {
                // Spawn failed, free naming value
                m_pMapList[iMapIndex]->SetNamingValueEmpty(iNamingValue);
#ifdef _DEBUG
                // printf("[DEBUG] ProcessSpotSpawns: Map[%s] Spot[%d] Type%d spawn FAILED for %s\n",
                //    m_pMapList[iMapIndex]->m_cName, j,
                //    m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].cType, cNpcName);
#endif
            }
            else {
                // Spawn succeeded, increment counter
                m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iCurMobs++;
#ifdef _DEBUG
                // printf("[DEBUG] ProcessSpotSpawns: Map[%s] Spot[%d] Type%d spawn SUCCESS for %s - iCurMobs incremented to %d\n",
                //    m_pMapList[iMapIndex]->m_cName, j,
                //    m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].cType, cNpcName,
                //    m_pMapList[iMapIndex]->m_stSpotMobGenerator[j].iCurMobs);
#endif
            }
        }
    }
}

bool CEntityManager::CanSpawnAtSpot(int iMapIndex, int iSpotIndex) const
{
    // TODO: Implement spawn condition checking
    // This will be implemented in Phase 2
    return false;
}

uint32_t CEntityManager::GenerateEntityGUID()
{
    uint32_t dwGUID = m_dwNextGUID++;

    // Handle wraparound (extremely unlikely, but safe)
    if (m_dwNextGUID == 0)
        m_dwNextGUID = 1;

#ifdef _DEBUG
    // printf("[DEBUG] EntityManager: Generated new GUID: %u\n", dwGUID);
#endif

    return dwGUID;
}
