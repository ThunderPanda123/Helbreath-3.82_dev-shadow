// DXC_dinput.cpp: implementation of the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#include "DXC_dinput.h"
#include <cstdio>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DXC_dinput::DXC_dinput()
{
	m_pDI    = 0;
	m_pMouse = 0;
	m_hWnd   = 0;
	m_sX     = 0;
	m_sY     = 0;
	m_sZ     = 0;
}

DXC_dinput::~DXC_dinput()
{
	if (m_pMouse != 0) {
		m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = 0;
	}
	if (m_pDI != 0) {
		m_pDI->Release();
        m_pDI = 0;
	}
}

bool DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
 HRESULT hr;
 DIMOUSESTATE dims;

	m_hWnd = hWnd;
	m_sX   = 400;
	m_sY   = 300;

	hr = DirectInputCreate( hInst, DIRECTINPUT_VERSION, &m_pDI, 0 );
    if (hr != DI_OK) return false;
	hr = m_pDI->CreateDevice( GUID_SysMouse, &m_pMouse, 0 );
	if (hr != DI_OK) return false;
	hr = m_pMouse->SetDataFormat( &c_dfDIMouse );
	if (hr != DI_OK) return false;
	hr = m_pMouse->SetCooperativeLevel( hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (hr != DI_OK) return false;

	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
	}

	// Clip cursor to window on initialization
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	POINT ptTopLeft = {rcClient.left, rcClient.top};
	POINT ptBottomRight = {rcClient.right, rcClient.bottom};
	ClientToScreen(m_hWnd, &ptTopLeft);
	ClientToScreen(m_hWnd, &ptBottomRight);
	RECT rcClip = {ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y};
	ClipCursor(&rcClip);

	return true;
}


void DXC_dinput::SetAcquire(bool bFlag)
{
 DIMOUSESTATE dims;

	if (m_pMouse == 0) return;
	if (bFlag == true) {
		m_pMouse->Acquire();
		m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );

		// Clip cursor to window client area to prevent it from leaving
		if (m_hWnd) {
			RECT rcClient;
			GetClientRect(m_hWnd, &rcClient);
			POINT ptTopLeft = {rcClient.left, rcClient.top};
			POINT ptBottomRight = {rcClient.right, rcClient.bottom};
			ClientToScreen(m_hWnd, &ptTopLeft);
			ClientToScreen(m_hWnd, &ptBottomRight);
			RECT rcClip = {ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y};
			ClipCursor(&rcClip);
		}
	}
	else {
		m_pMouse->Unacquire();
		// Release cursor clip when losing focus
		ClipCursor(NULL);
	}
}

void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB)
{
	// Get button states from DirectInput
	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
		return;
	}

	// In windowed NONEXCLUSIVE mode, DirectInput position tracking doesn't work reliably on multi-monitor setups
	// Use Windows cursor position instead and convert to window-relative coordinates
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);

	// Set mouse position from window-relative cursor coordinates
	m_sX = (short)pt.x;
	m_sY = (short)pt.y;

	// Handle mouse wheel from DirectInput
	if( (short)dims.lZ != 0 ) m_sZ = (short)dims.lZ;

	// Clamp to game bounds
	if (m_sX < 0) m_sX = 0;
	if (m_sY < 0) m_sY = 0;
	if (m_sX > 799) m_sX = 799;
	if (m_sY > 599) m_sY = 599;

	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = (char)dims.rgbButtons[0];
	*pRB = (char)dims.rgbButtons[1];
}