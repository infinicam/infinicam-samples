#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "AppDefine.h"

class CDefaultParams
{
public:
	CDefaultParams();

	void Read();
	void Write();

	CString cameraSaveFolderPath;
	CString cameraSaveFileName;
	SAVE_FILE_TYPE cameraSaveFileType;

	CString fileOpenFolderPath;
	CString fileSaveFolderPath;
	CString fileSaveFileName;
	CString latestSaveCIHFullPath;

	int cameraFramerateIndex;
	int cameraResolutionIndex;
	int cameraShutterSpeedIndex;
};

class CCamMonitorApp : public CWinApp
{
public:
	CCamMonitorApp();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

	CDefaultParams& GetDefaultParams() { return m_dfParams; }

	void SetLanguage(WORD langID);
	void SaveLanguagePreference(LANGID langID);
	WORD LoadLanguagePreference();

private:
	CDefaultParams m_dfParams;
	WORD m_currentLangID;

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CCamMonitorApp theApp;
