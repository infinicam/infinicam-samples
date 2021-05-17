#pragma once

#include "resource.h"

class CTabCtrlEx : public CTabCtrl
{
	DECLARE_DYNAMIC(CTabCtrlEx)

public:
	CTabCtrlEx() : m_tabCurrent(0) {}
	virtual ~CTabCtrlEx();

	void AddTab(const CString& sTitle, CDialog* pTab);

	int GetCurrentTabIndex() const { return m_tabCurrent; }
	CDialog* GetCurrentTab() const { return m_tabPages[m_tabCurrent]; }
	CDialog* GetTab(int indexTab) const { return m_tabPages[indexTab]; }

private:
	void SetRectangle();
	void ChangeCurrentTab(int indexTab);

	vector<CDialog*> m_tabPages;
	int m_tabCurrent;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};





class CTabDlgBar : public CDialogBar
{
public:
	DECLARE_DYNAMIC(CTabDlgBar)
	CTabDlgBar();
	virtual ~CTabDlgBar();

	enum { IDD = IDD_OPERATION };

	void AddTab(int nID, const CString& sTitle, CDialog* pTab);

	CDialog* GetActiveTab() const { return m_tabCtrl.GetCurrentTab(); }

protected:
	CTabCtrlEx m_tabCtrl;

	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
};
