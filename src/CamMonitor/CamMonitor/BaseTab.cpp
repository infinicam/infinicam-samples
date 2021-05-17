#include "stdafx.h"
#include "BaseTab.h"
#include "ChildView.h"
#include "AppDefine.h"

IMPLEMENT_DYNAMIC(CBaseTab, CDialog)

CBaseTab::CBaseTab(int nID, LPCTSTR name, CWnd* pParent)
	: CDialog(nID, pParent)
	, m_name(name)
{
}

CBaseTab::~CBaseTab()
{
}

void CBaseTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CBaseTab, CDialog)
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
	ON_MESSAGE(WM_USER_UPDATE_VIEW, &CBaseTab::OnUpdateView)
END_MESSAGE_MAP()

LRESULT CBaseTab::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = HandleInitDialog(wParam, lParam);
	if (!UpdateData(FALSE))
	{
		TRACE0("Warning: UpdateData failed during dialog init.n");
	}

	return ret;
}

LRESULT CBaseTab::OnUpdateView(WPARAM, LPARAM)
{
	CChildView* pView = (CChildView*)GET_VIEW();
	if (!pView)
		return -1;

	pView->Invalidate(FALSE);
	return 0;
}

BOOL CBaseTab::PreTranslateMessage(MSG* pMsg)
{
	if (WM_KEYDOWN == pMsg->message)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			pMsg->wParam = VK_TAB;
			break;
		case VK_ESCAPE:
			return FALSE;
		default:
			break;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}