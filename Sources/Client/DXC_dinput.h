// DXC_dinput.h: interface for the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

//#define INITGUID
#include <windows.h>
#include "dinput.h"

class DXC_dinput  
{
public:
	DXC_dinput();
	virtual ~DXC_dinput();
	void UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB);
	void SetAcquire(bool bFlag);
	bool bInit(HWND hWnd, HINSTANCE hInst);

	DIMOUSESTATE dims;
	IDirectInput *           m_pDI;
	IDirectInputDevice *     m_pMouse;
	HWND m_hWnd;
	short m_sX, m_sY, m_sZ;
};
