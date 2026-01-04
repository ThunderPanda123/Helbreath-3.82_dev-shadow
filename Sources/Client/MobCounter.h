#pragma once
#include <windows.h>
class CMobCounter
{
public:
	inline CMobCounter()
	{
		ZeroMemory(cNpcName, sizeof(cNpcName));
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

