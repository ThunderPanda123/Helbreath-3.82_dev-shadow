// GuildsMan.h: interface for the CGuildsMan class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>

class CGuildsMan
{
public:

	inline CGuildsMan()
	{
		ZeroMemory(m_cName, sizeof(m_cName));
	}

	inline virtual ~CGuildsMan()
	{
	}

	char m_cName[11];
	int  m_iRank;
};
