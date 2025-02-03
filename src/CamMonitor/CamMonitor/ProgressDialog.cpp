#include "stdafx.h"
#include "ProgressDialog.h"
#include "resource.h"
#include "AppDefine.h"

IMPLEMENT_DYNAMIC(CProgressDialog, CDialogEx)

CProgressDialog::CProgressDialog(INT64 nStartNo, INT64 nEndNo, FUNCPROCESSING funcProcessing, CWnd* pProcessingWnd)
	: CDialogEx(IDD_PROGRESS, pProcessingWnd)
	, m_nStartNo(nStartNo)
	, m_nEndNo(nEndNo)
	, m_funcProcessing(funcProcessing)
	, m_pProcessingWnd(pProcessingWnd)
	, m_pThread(NULL)
	, m_isShowProgress(TRUE)
	, m_bThreadExit(FALSE)
{

}

CProgressDialog::CProgressDialog(INT64 nStartNo, INT64 nEndNo, CWnd* pProcessingWnd)
	:CDialogEx(IDD_PROGRESS, pProcessingWnd)
	, m_nStartNo(nStartNo)
	, m_nEndNo(nEndNo)
	, m_pThread(NULL)
	, m_isShowProgress(FALSE)
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
	CenterWindow();

	CMenu* pMenu = GetSystemMenu(FALSE);
	pMenu->EnableMenuItem(SC_CLOSE, MF_GRAYED);

	m_progressBar.SetRange32(0, m_nEndNo - m_nStartNo + 1);
	OnUpdateProgress(0, 0);
	if (m_isShowProgress) 
	{
		m_pThread = AfxBeginThread(_Thread, (LPVOID)this, THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED);
		m_pThread->m_bAutoDelete = TRUE;
		m_pThread->ResumeThread();
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	}

	CString text;
	text.LoadString(IDS_TEXT_SAVE_FILE);
	SetWindowText(text);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CProgressDialog::OnOK()
{
	if (m_pThread != NULL)
	{
		m_bThreadExit = TRUE;
		WaitForSingleObject(m_pThread->m_hThread, 2000);
		m_pThread = NULL;
	}

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
		if (m_bThreadExit)
		{
			m_bThreadExit = FALSE;
			break;
		}

		int ret = m_funcProcessing(i, m_nStartNo, m_nEndNo, m_pProcessingWnd);
		if (ret == RET_ERROR)
		{
			return -1;
		}
		else if (ret == RET_CONTINUE_NEXT_FRAME)
		{
			PostMessage(WM_USER_UPDATE_PROGRESS, i, 0);
		}
		else if (ret == RET_CONTINUE_CURRENT_FRAME)
		{
			i--;
		}
		else if (ret == RET_FINISH)
		{
			break;
		}
		else
		{
			::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
			if (msg.message == WM_QUIT)
				break;
		}
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

void CProgressDialog::SetPos(int nPos)
{
	m_progressBar.SetPos(nPos);
}

void CProgressDialog::SetText(int top, int bottom)
{
	m_xvProgText.Format(_T("%lld / %lld"), top, bottom);
	UpdateData(FALSE);
}