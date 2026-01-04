#pragma once
#include <windows.h>
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



