// ItemName.h: interface for the CItemName class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>

class CItemName
{
public:
	inline CItemName()
	{
		ZeroMemory(m_cOriginName, sizeof(m_cOriginName));
		ZeroMemory(m_cName, sizeof(m_cName));
	}

	inline virtual ~CItemName()
	{
	}

	char m_cOriginName[21];
	char m_cName[34];
};
