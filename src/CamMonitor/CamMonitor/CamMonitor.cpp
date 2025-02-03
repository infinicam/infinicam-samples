#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "CamMonitor.h"
#include "MainFrm.h"
#include "CamMonitor.h"
#include "PUCLIB.h"
#include "Util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CCamMonitorApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CCamMonitorApp::OnAppAbout)
END_MESSAGE_MAP()

CCamMonitorApp theApp;

CCamMonitorApp::CCamMonitorApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("CamMonitor.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

BOOL CCamMonitorApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	auto langID = LoadLanguagePreference();
	SetLanguage(langID);

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(REG_CAMPANY);

	PUC_Initialize();

	m_dfParams.Read();

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// The one and only window has been initialized, so show and update it
	pFrame->ShowWindow(SW_SHOWMAXIMIZED);
	pFrame->UpdateWindow();
	return TRUE;
}


int CCamMonitorApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE);

	m_dfParams.Write();
	SaveLanguagePreference(m_currentLangID);
	return CWinApp::ExitInstance();
}

// CCamMonitorApp message handlers



CDefaultParams::CDefaultParams()
{
}

void CDefaultParams::Read()
{
	CWinApp* pApp = AfxGetApp();

	cameraSaveFolderPath = pApp->GetProfileString(REG_SECTION_DF, REG_KEY_CAMERA_SAVE_FOLDER_PATH, cameraSaveFolderPath);
	if (cameraSaveFolderPath.IsEmpty())
	{
		cameraSaveFolderPath = GetDesktopPathCString();
	}
	cameraSaveFileName = pApp->GetProfileString(REG_SECTION_DF, REG_KEY_CAMERA_SAVE_FILE_NAME, cameraSaveFileName);
	if (cameraSaveFileName.IsEmpty())
	{
		cameraSaveFileName = _T("CamMonitor");
	}
	cameraSaveFileType = (SAVE_FILE_TYPE)pApp->GetProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_SAVE_FILE_TYPE, cameraSaveFileType);

	fileOpenFolderPath = pApp->GetProfileString(REG_SECTION_DF, REG_KEY_FILE_OPEN_FOLDER_PATH, fileOpenFolderPath);
	fileSaveFolderPath = pApp->GetProfileString(REG_SECTION_DF, REG_KEY_FILE_SAVE_FOLDER_PATH, fileSaveFolderPath);
	fileSaveFileName = pApp->GetProfileString(REG_SECTION_DF, REG_KEY_FILE_SAVE_FILE_PATH, fileSaveFileName);

	cameraFramerateIndex = pApp->GetProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_FRAMERATE_INDEX, DEFAULT_FRAMERATE_INDEX);
	cameraResolutionIndex = pApp->GetProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_RESOLUTION_INDEX, DEFAULT_RESOLUTION_INDEX);
	cameraShutterSpeedIndex = pApp->GetProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_SHUTTER_INDEX, DEFAULT_SHUTTERSPEED_INDEX);

	latestSaveCIHFullPath = pApp->GetProfileString(REG_SECTION_DF, REG_KEY_LATEST_CIH_FULLPATH, latestSaveCIHFullPath);
}

void CDefaultParams::Write()
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileString(REG_SECTION_DF, REG_KEY_CAMERA_SAVE_FOLDER_PATH, cameraSaveFolderPath);
	pApp->WriteProfileString(REG_SECTION_DF, REG_KEY_CAMERA_SAVE_FILE_NAME, cameraSaveFileName);
	pApp->WriteProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_SAVE_FILE_TYPE, cameraSaveFileType);

	pApp->WriteProfileString(REG_SECTION_DF, REG_KEY_FILE_OPEN_FOLDER_PATH, fileOpenFolderPath);
	pApp->WriteProfileString(REG_SECTION_DF, REG_KEY_FILE_SAVE_FOLDER_PATH, fileSaveFolderPath);
	pApp->WriteProfileString(REG_SECTION_DF, REG_KEY_FILE_SAVE_FILE_PATH, fileSaveFileName);

	pApp->WriteProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_FRAMERATE_INDEX, cameraFramerateIndex);
	pApp->WriteProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_RESOLUTION_INDEX, cameraResolutionIndex);
	pApp->WriteProfileInt(REG_SECTION_DF, REG_KEY_CAMERA_SHUTTER_INDEX, cameraShutterSpeedIndex);

	pApp->WriteProfileString(REG_SECTION_DF, REG_KEY_LATEST_CIH_FULLPATH, latestSaveCIHFullPath);
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CCamMonitorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CCamMonitorApp::SetLanguage(WORD langID)
{
	SetThreadUILanguage(langID);
	m_currentLangID = langID;
}

void CCamMonitorApp::SaveLanguagePreference(WORD langID)
{
	CWinApp::WriteProfileInt(_T("Settings"), _T("Language"), langID);
}

WORD CCamMonitorApp::LoadLanguagePreference()
{
	return CWinApp::GetProfileInt(_T("Settings"), _T("Language"), MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT));
}