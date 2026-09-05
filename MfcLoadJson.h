#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif
#include "resource.h"		// main symbols

class CMfcLoadJsonApp : public CWinApp
{
public:
	CMfcLoadJsonApp();

	virtual BOOL InitInstance();

protected:
	DECLARE_MESSAGE_MAP()
};

extern CMfcLoadJsonApp theApp;
