#include "stdafx.h"
#include "SaveLiveDialog.h"
#include "Util.h"
#include "CamMonitor.h"

IMPLEMENT_DYNAMIC(CSaveLiveDialog, CDialogEx)

CSaveLiveDialog::CSaveLiveDialog(CWnd* pParent)
	: CDialogEx(IDD, pParent)
	, m_xvFolderpath(_T(""))
	, m_xvFilename(_T(""))
	, m_xvSaveFileType(SAVE_FILE_TYPE_RAW)
{

}

CSaveLiveDialog::~CSaveLiveDialog()
{
}

void CSaveLiveDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FOLDERPATH, m_xvFolderpath);
	DDX_Text(pDX, IDC_FILENAME, m_xvFilename);
	DDX_Radio(pDX, IDC_SAVE_FILE_RAW, m_xvSaveFileType);
}

BOOL CSaveLiveDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	m_xvFolderpath = df.cameraSaveFolderPath;
	m_xvFilename = df.cameraSaveFileName;
	m_xvSaveFileType = df.cameraSaveFileType;

	UpdateControlState();
	return TRUE;
}

void CSaveLiveDialog::UpdateControlState()
{
	UpdateData(FALSE);
}


BEGIN_MESSAGE_MAP(CSaveLiveDialog, CDialogEx)
	ON_BN_CLICKED(IDC_REFERENCE, &CSaveLiveDialog::OnBnClickedReference)
	ON_BN_CLICKED(IDOK, &CSaveLiveDialog::OnBnClickedOk)
END_MESSAGE_MAP()

void CSaveLiveDialog::OnBnClickedReference()
{
	TCHAR initFolder[MAX_PATH] = { 0 };
	TCHAR selectedFolder[MAX_PATH] = { 0 };

	_tcscpy_s(initFolder, m_xvFolderpath);

	if (SelectFolderDialog(GetSafeHwnd(), initFolder, selectedFolder))
	{
		m_xvFolderpath = selectedFolder;
	}

	UpdateControlState();
}

void CSaveLiveDialog::OnBnClickedOk()
{
	UpdateData(TRUE);

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	df.cameraSaveFolderPath = m_xvFolderpath;
	df.cameraSaveFileName = m_xvFilename;
	df.cameraSaveFileType = (SAVE_FILE_TYPE)m_xvSaveFileType;

	CDialogEx::OnOK();
}
