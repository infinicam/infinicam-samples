#pragma once

#include "resource.h"

class CSaveLiveDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSaveLiveDialog)

public:
	CSaveLiveDialog(CWnd* pParent);
	virtual ~CSaveLiveDialog();

	enum { IDD = IDD_SAVE_LIVE };

	virtual BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateControlState();

	// Controls
	CString m_xvFolderpath;
	CString m_xvFilename;
	int m_xvSaveFileType;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedReference();
	afx_msg void OnBnClickedOk();
};
