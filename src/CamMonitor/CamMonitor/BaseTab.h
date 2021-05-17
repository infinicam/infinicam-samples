#pragma once

#include "resource.h"
#include "Bitmap.h"
#include "Lock.h"

typedef struct
{
	USHORT nSeqNo;
	UINT32 nTemp;
} TEXTINFO, *PTEXTINFO;

class CBaseTab : public CDialog
{
	DECLARE_DYNAMIC(CBaseTab)

public:
	CBaseTab(int nID, LPCTSTR name, CWnd* pParent);
	virtual ~CBaseTab();

	const CString& GetName() const { return m_name; }

	virtual LockImage* GetLockImage() = 0;
	virtual LockBuffer* GetLockTextInfo() = 0;

protected:
	CString m_name;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateView(WPARAM, LPARAM);
};
