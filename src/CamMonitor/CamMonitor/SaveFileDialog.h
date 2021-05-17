#pragma once

#include "resource.h"

class CSaveFileDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSaveFileDialog)

public:
	CSaveFileDialog(CWnd* pParent);
	virtual ~CSaveFileDialog();

	enum { IDD = IDD_SAVE_FILE };

	virtual BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateControlState();

	// Controls
	CString m_xvFolderpath;
	CString m_xvFilename;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedReference();
	afx_msg void OnBnClickedOk();
};
