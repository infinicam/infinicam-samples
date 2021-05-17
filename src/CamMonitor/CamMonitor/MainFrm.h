#pragma once

#include "ChildView.h"
#include "TabDlgBar.h"
#include "BaseTab.h"

class CMainFrame : public CFrameWnd
{
public:
	CMainFrame();
	virtual ~CMainFrame();
	DECLARE_DYNAMIC(CMainFrame)

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL DestroyWindow();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CBaseTab* GetActiveTab() { return dynamic_cast<CBaseTab*>(m_wndTabDlgBar.GetActiveTab()); }

protected:
	CToolBar m_wndToolBar;
	//CStatusBar m_wndStatusBar;
	CChildView* m_wndView;
	CTabDlgBar m_wndTabDlgBar;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
};


