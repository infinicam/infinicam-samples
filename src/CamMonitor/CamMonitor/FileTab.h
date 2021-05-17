#pragma once

#include "resource.h"
#include "BaseTab.h"
#include "cih.h"
#include "Bitmap.h"

class CFileTab : public CBaseTab
{
public:
	DECLARE_DYNAMIC(CFileTab)
	CFileTab(CWnd* pParent);
	virtual ~CFileTab();

	enum { IDD = IDD_FILE };

	BOOL IsOpened() const { return m_bOpened; }
	BOOL ReadDataFile(UINT32 nFrameNo);
	const CIH& GetCIH() const { return m_cih; }
	UINT8* GetData() const { return m_pData; }

	virtual LockImage* GetLockImage() { return &m_lockImage; }
	virtual LockBuffer* GetLockTextInfo() { return &m_lockTextInfo; }

protected:
	void UpdateControlState();

	void StartLive();
	void StopLive();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	static UINT _SaveImage(INT64 nFrameNo, CWnd* pWnd);
	UINT SaveImage(INT64 nFrameNo);

	BOOL SaveCSV(const CString& filePath);

	BOOL OpenCIH(const CString& filePath);
	void CloseCIH();

	BOOL OpenDataFile();
	BOOL ReadDataFileAndDecode(UINT32 nFrameNo);
	void CloseDataFile();

	void DecodeImage();

private:
	BOOL m_bOpened;
	BOOL m_bPlaying;

	CIH m_cih;
	UINT8* m_pData;
	LockImage m_lockImage;
	LockBuffer m_lockTextInfo;

	// controls
	CButton m_buttonPlay;
	int m_xvCurrentFrame;
	int m_xvStartFrame;
	int m_xvEndFrame;
	CSliderCtrl m_sldrFrameCtrl;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedOpen();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedForwardFrame();
	afx_msg void OnBnClickedBackwardFrame();
	afx_msg void OnBnClickedGoToStart();
	afx_msg void OnBnClickedGoToEnd();
	afx_msg void OnEnKillfocusCurrentFrame();
	afx_msg void OnEnKillfocusStartFrame();
	afx_msg void OnEnKillfocusEndFrame();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDeltaposSpinStartFrame(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinCurrentFrame(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinEndFrame(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedPostStartFrame();
	afx_msg void OnBnClickedPostEndFrame();
	afx_msg void OnBnClickedSaveFrameInfo();
	afx_msg void OnBnClickedSaveToFile();
};
