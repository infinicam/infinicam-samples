#include "stdafx.h"
#include "TabDlgBar.h"

IMPLEMENT_DYNAMIC(CTabCtrlEx, CTabCtrl)

CTabCtrlEx::~CTabCtrlEx()
{
	for (auto ite = m_tabPages.begin(); ite != m_tabPages.end(); ite++)
		delete* ite;
	m_tabPages.clear();
}

void CTabCtrlEx::AddTab(const CString& sTitle, CDialog* pTab)
{
	InsertItem((int)m_tabPages.size(), sTitle);
	m_tabPages.push_back(pTab);

	SetRectangle();
}

BEGIN_MESSAGE_MAP(CTabCtrlEx, CTabCtrl)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CTabCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	CTabCtrl::OnLButtonDown(nFlags, point);

	int tabCurrentFocus = GetCurFocus();
	ChangeCurrentTab(tabCurrentFocus);
}

void CTabCtrlEx::ChangeCurrentTab(int indexTab)
{
	SetCurSel(indexTab);
	if (m_tabCurrent != indexTab)
	{
		m_tabPages[m_tabCurrent]->ShowWindow(SW_HIDE);
		m_tabCurrent = indexTab;
		m_tabPages[m_tabCurrent]->ShowWindow(SW_SHOW);
		m_tabPages[m_tabCurrent]->SetFocus();
	}
}

void CTabCtrlEx::SetRectangle()
{
	CRect tabRect;
	CRect itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX = itemRect.left;
	nY = itemRect.bottom + 1;
	nXc = tabRect.right - itemRect.left - 1;
	nYc = tabRect.bottom - nY - 1;

	m_tabPages[0]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for (int i = 1; i < (int)m_tabPages.size(); i++)
	{
		m_tabPages[i]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
	}
}




IMPLEMENT_DYNAMIC(CTabDlgBar, CDialogBar)

CTabDlgBar::CTabDlgBar()
{
}

CTabDlgBar::~CTabDlgBar()
{
}

void CTabDlgBar::AddTab(int nID, const CString& sTitle, CDialog* pTab)
{
	pTab->Create(nID, &m_tabCtrl);
	m_tabCtrl.AddTab(sTitle, pTab);
}

void CTabDlgBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_CTRL, m_tabCtrl);
}

BEGIN_MESSAGE_MAP(CTabDlgBar, CDialogBar)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
END_MESSAGE_MAP()

LRESULT CTabDlgBar::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = HandleInitDialog(wParam, lParam);
	if (!UpdateData(FALSE))
	{
		TRACE0("Warning: UpdateData failed during dialog init.n");
	}

	return ret;
}
