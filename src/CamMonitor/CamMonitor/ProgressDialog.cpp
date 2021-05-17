#include "stdafx.h"
#include "ProgressDialog.h"
#include "resource.h"
#include "FileTab.h"

IMPLEMENT_DYNAMIC(CProgressDialog, CDialogEx)

CProgressDialog::CProgressDialog(INT64 nStartNo, INT64 nEndNo, FUNCPROCESSING funcProcessing, CWnd* pProcessingWnd)
	: CDialogEx(IDD_PROGRESS, pProcessingWnd)
	, m_nStartNo(nStartNo)
	, m_nEndNo(nEndNo)
	, m_funcProcessing(funcProcessing)
	, m_pProcessingWnd(pProcessingWnd)
	, m_pThread(NULL)
{

}

CProgressDialog::~CProgressDialog()
{
}

void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_BAR, m_progressBar);
	DDX_Text(pDX, IDC_PROGRESS_TEXT, m_xvProgText);
}

BOOL CProgressDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CMenu* pMenu = GetSystemMenu(FALSE);
	pMenu->EnableMenuItem(SC_CLOSE, MF_GRAYED);

	m_progressBar.SetRange32(0, m_nEndNo - m_nStartNo + 1);
	OnUpdateProgress(0, 0);

	m_pThread = AfxBeginThread(_Thread, (LPVOID)this, THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED);
	m_pThread->m_bAutoDelete = TRUE;
	m_pThread->ResumeThread();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CProgressDialog::OnOK()
{
	m_pThread->PostThreadMessage(WM_QUIT, 0, 0);
	WaitForSingleObject(m_pThread->m_hThread, 2000);
	CDialogEx::OnOK();
}

UINT CProgressDialog::_Thread(LPVOID pParam)
{
	CProgressDialog* p = (CProgressDialog*)pParam;
	CString s;

	UINT result = p->MainThread();
	if (result == 0)
	{
		p->SendMessage(WM_USER_UPDATE_PROGRESS, p->m_nEndNo, 0);
		s.LoadString(IDS_COMPLETED);
		AfxMessageBox(s, MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		s.LoadString(IDS_ERROR_SAVE);
		AfxMessageBox(s, MB_OK | MB_ICONERROR);
	}

	return 0;
}

UINT CProgressDialog::MainThread()
{
	MSG msg;

	for (INT64 i = m_nStartNo; i <= m_nEndNo; i++)
	{
		if (!m_funcProcessing(i, m_pProcessingWnd))
		{
			return -1;
		}

		PostMessage(WM_USER_UPDATE_PROGRESS, i, 0);

		::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (msg.message == WM_QUIT)
			break;
	}

	return 0;
}

BEGIN_MESSAGE_MAP(CProgressDialog, CDialogEx)
	ON_MESSAGE(WM_USER_UPDATE_PROGRESS, &CProgressDialog::OnUpdateProgress)
END_MESSAGE_MAP()

LRESULT CProgressDialog::OnUpdateProgress(WPARAM wParam, LPARAM)
{
	INT64 nNo = (INT64)wParam;

	CString s;
	INT64 nIndex = nNo - m_nStartNo + 1;
	INT64 nCount = m_nEndNo - m_nStartNo + 1;

	m_progressBar.SetPos(nIndex);
	m_xvProgText.Format(_T("%lld / %lld"), nIndex, nCount);

	if (nIndex == nCount)
		s.LoadString(IDS_CLOSE);
	else
		s.LoadString(IDS_STOP);
	GetDlgItem(IDOK)->SetWindowText(s);

	UpdateData(FALSE);
	return 0;
}
