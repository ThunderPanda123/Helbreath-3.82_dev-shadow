// Player.h: interface for the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include "XSocket.h"


class CPlayer  				  
{
public:
	CPlayer();
	virtual ~CPlayer();

	class XSocket * m_pXSock;

private:



};
