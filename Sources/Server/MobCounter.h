#pragma once
// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)
#define _WINSOCKAPI_
#include <windows.h>
#include "CommonTypes.h"
#include <string.h>

class CMobCounter
{
public:
	inline CMobCounter(char* cName)
	{
		strcpy(cNpcName, cName);
		iKillCount = 0;
		iLevel = 0;
		iNextCount = 100;
	}

	inline CMobCounter()
	{
		std::memset(cNpcName, 0, sizeof(cNpcName));
		iKillCount = 0;
		iLevel = 0;
		iNextCount = 0;
	}

	inline virtual ~CMobCounter()
	{
	}

	char cNpcName[21];
	int iKillCount, iLevel, iNextCount;
};



