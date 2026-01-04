// Msg.h: interface for the CMsg class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <string.h>
#include <winbase.h>

class CMsg
{
public:
	inline CMsg(char cType, char * pMsg, DWORD dwTime)
	{
		m_cType = cType;

		m_pMsg = 0;
		m_pMsg = new char [strlen(pMsg) + 1];
		ZeroMemory(m_pMsg, strlen(pMsg) + 1);
		strcpy(m_pMsg, pMsg);
		m_dwTime = dwTime;
		m_iObjectID = -1;
	}

	inline virtual ~CMsg()
	{
		if (m_pMsg != 0) delete m_pMsg;
	}

	char   m_cType;
	char * m_pMsg;
	short  m_sX, m_sY;
	DWORD  m_dwTime;

	int    m_iObjectID;

};
