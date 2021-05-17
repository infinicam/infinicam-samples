#include "stdafx.h"
#include "SaveFileDialog.h"
#include "Util.h"
#include "CamMonitor.h"

IMPLEMENT_DYNAMIC(CSaveFileDialog, CDialogEx)

CSaveFileDialog::CSaveFileDialog(CWnd* pParent)
	: CDialogEx(IDD, pParent)
	, m_xvFolderpath(_T(""))
	, m_xvFilename(_T(""))
{

}

CSaveFileDialog::~CSaveFileDialog()
{
}

void CSaveFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FOLDERPATH, m_xvFolderpath);
	DDX_Text(pDX, IDC_FILENAME, m_xvFilename);
}

BOOL CSaveFileDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	m_xvFolderpath = df.fileSaveFolderPath;
	m_xvFilename = df.fileSaveFileName;

	UpdateControlState();
	return TRUE;
}

void CSaveFileDialog::UpdateControlState()
{
	UpdateData(FALSE);
}


BEGIN_MESSAGE_MAP(CSaveFileDialog, CDialogEx)
	ON_BN_CLICKED(IDC_REFERENCE, &CSaveFileDialog::OnBnClickedReference)
	ON_BN_CLICKED(IDOK, &CSaveFileDialog::OnBnClickedOk)
END_MESSAGE_MAP()

void CSaveFileDialog::OnBnClickedReference()
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

void CSaveFileDialog::OnBnClickedOk()
{
	UpdateData(TRUE);

	CDefaultParams& df = ((CCamMonitorApp*)AfxGetApp())->GetDefaultParams();
	df.fileSaveFolderPath = m_xvFolderpath;
	df.fileSaveFileName = m_xvFilename;

	CDialogEx::OnOK();
}
