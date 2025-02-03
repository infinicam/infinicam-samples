#pragma once


typedef UINT (*FUNCPROCESSING)(INT64, INT64, INT64, CWnd*);

class CProgressDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CProgressDialog)

public:
	CProgressDialog(INT64 nStartNo, INT64 nEndNo, CWnd* pProcessingWnd);
	CProgressDialog(INT64 nStartNo, INT64 nEndNo, FUNCPROCESSING funcProcessing, CWnd* pProcessingWnd);
	virtual ~CProgressDialog();

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void SetPos(int nPos);
	void SetText(int top, int bottom);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	static UINT _Thread(LPVOID pParam);
	UINT MainThread();

	INT64 m_nStartNo;
	INT64 m_nEndNo;

	CWinThread* m_pThread;
	FUNCPROCESSING m_funcProcessing;
	CWnd* m_pProcessingWnd;

	// Controls
	CProgressCtrl m_progressBar;
	CString m_xvProgText;

	BOOL m_isShowProgress;
	BOOL m_bThreadExit;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUpdateProgress(WPARAM wParam, LPARAM);
};
