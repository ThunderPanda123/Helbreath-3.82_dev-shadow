// BuildItem.h: interface for the CBuildItem class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>

class CBuildItem
{
public:
	inline CBuildItem()
	{
		int i;

		ZeroMemory(m_cName, sizeof(m_cName));

		m_bBuildEnabled = false;
		m_iSkillLimit   = 0;
		m_iMaxSkill     = 0;

		m_iSprH     = 0;
		m_iSprFrame = 0;

		ZeroMemory(m_cElementName1, sizeof(m_cElementName1));
		ZeroMemory(m_cElementName2, sizeof(m_cElementName2));
		ZeroMemory(m_cElementName3, sizeof(m_cElementName3));
		ZeroMemory(m_cElementName4, sizeof(m_cElementName4));
		ZeroMemory(m_cElementName5, sizeof(m_cElementName5));
		ZeroMemory(m_cElementName6, sizeof(m_cElementName6));

		for (i = 0; i < 7; i++) {
			m_iElementCount[i] = 0;
			m_bElementFlag[i]  = 0;
		}
	}

	inline virtual ~CBuildItem()
	{

	}

	bool m_bBuildEnabled;
	char m_cName[21];
	int	 m_iSkillLimit;
	int  m_iMaxSkill;
	int  m_iSprH, m_iSprFrame;
	char m_cElementName1[21], m_cElementName2[21], m_cElementName3[21];
	char m_cElementName4[21], m_cElementName5[21], m_cElementName6[21];
	DWORD m_iElementCount[7];
	bool  m_bElementFlag[7];

};
