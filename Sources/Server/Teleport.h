// Teleport.h: interface for the CTeleport class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>

class CTeleport  
{
public:
	inline CTeleport()
	{

		ZeroMemory(m_cTeleportNpcName, sizeof(m_cTeleportNpcName));
		ZeroMemory(m_cSourceMap, sizeof(m_cSourceMap));
		ZeroMemory(m_cTargetMap, sizeof(m_cTargetMap));
		ZeroMemory(m_cTeleportSide, sizeof(m_cTeleportSide));

		m_sDestinationX = -1;
		m_sDestinationY = -1;
		m_sTeleportCost = -1;
		m_sTeleportMinLevel = 0;
		m_sTeleportMaxLevel = 0;
		m_bTeleportHunt = FALSE;
		m_bTeleportNtrl = FALSE;
		m_bTeleportCrmnl = FALSE;
	}

	inline virtual ~CTeleport()
	{

	}

	char  m_cTeleportNum;
	char  m_cTeleportNpcName[21];								// The npc with teleport ability.
	char  m_cSourceMap[10];										// The map the teleport npc is located in.
	char  m_cTargetMap[10];											// The map the player will be teleported to.
	short m_sDestinationX, m_sDestinationY;						// The destination coordinates on the map.
	short m_sTeleportCost;										// The amount of gold paid to teleport.
	short m_sTeleportMinLevel, m_sTeleportMaxLevel;				// The required level range for the player.
	char  m_cTeleportSide[7];										// The side the player must be for a successful teleport.
			// both
			// elvine
			// aresden
	BOOL m_bTeleportHunt, m_bTeleportNtrl, m_bTeleportCrmnl;	// Check if player is a Civilian, Neutral, or Criminal.
};
