﻿#pragma once

#include "resource.h"
#include "BaseTab.h"
#include "PUCLIB.h"
#include "PUCUTIL.h"
#include "CameraObject.h"
#include <SetupAPI.h>
#include "RingBuffer.h"


class CLiveTab : public CBaseTab
{
public:
	DECLARE_DYNAMIC(CLiveTab)
	CLiveTab(CWnd* pParent);
	virtual ~CLiveTab();

	enum { IDD = IDD_LIVE };

	BOOL IsCameraOpened() const { return m_camera.IsOpened(); }
	PUC_HANDLE GetCameraHandle() const { return m_camera.GetHandle(); }

	virtual LockImage* GetLockImage() { return &m_lockImage; }
	virtual LockBuffer* GetLockTextInfo() { return &m_lockTextInfo; }

	void setSaveFilePath(CString path) { m_filepath = path; }
	CString getSaveFilePath() { return m_filepath; }
	void setSaveFolderPath(CString folder) { m_folderPath = folder; }
	CString getSaveFolderPath() { return m_folderPath; }
	void UpdateControlText();
	void setLanguage(WORD langID) { m_langID = langID; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	BOOL OpenCamera(UINT32 nDeviceNo, UINT32 nSingleXferTimeOut, UINT32 nContinuousXferTimeOut, UINT32 nRingBufCount);
	void CloseCamera();

	void StartLive();
	void StopLive();

	void StartRec();
	void StopRec();

	void WriteCIH();

	static void _SingleProc(PPUC_XFER_DATA_INFO pInfo, void* pArg);
	void SingleProc(PPUC_XFER_DATA_INFO pInfo);

	static void _ContinuousProc(PPUC_XFER_DATA_INFO pInfo, void* pArg);
	void ContinuousProc(PPUC_XFER_DATA_INFO pInfo);

	void UpdateBuffer();

	void DecodeImage(PUCHAR pBuffer);

	void ConfirmDecodePos();
	void ResetDecodePos();

	void UpdateControlState();

	void UpdateCameraSerialNo();
	void UpdateFramerateComboBox();
	void UpdateShutterFpsComboBox();
	void UpdateResoComboBox();
	void UpdateSyncInRadio();
	void UpdateSyncOutSignalRadio();
	void UpdateSyncOutDelayEdit();
	void UpdateSyncOutWidthEdit();
	void UpdateSyncOutMagComboBox();
	void UpdateLEDModeCheckBox();
	void UpdateFANCtrlCheckBox();
	void UpdateQuantization();
	void UpdateExposeTime();

	void SetExposeTime();

	void UpdateLiveUI();

private:
	CString ConvertToTimeFormat(ULONGLONG totalseconds);
	ULONGLONG CalcRecordTimeSecond();
	std::unique_ptr<RingBuffer> CreateRingBuffer(UINT32 imageSize);
	double GetAvailableRAMSpace();

private:
	CCameraObject m_camera;

	// live
	LockImage m_lockImage;
	LockBuffer m_lockTextInfo;

	// recording
	LockBuffer m_lockRec;
	FILE* m_csvFile;
	FILE* m_mdatFile;
	INT64 m_nRecFrameCount;

	// controls
	int m_xvAcqMode;
	CMFCButton m_buttonRec;
	CComboBox m_comboFramerate;
	CComboBox m_comboShutterFps;
	CComboBox m_comboReso;
	int m_xvSyncIn;
	int m_xvSyncInSignal;
	int m_xvSyncOutSignal;
	UINT m_xvSyncOutDelay;
	UINT m_xvSyncOutWidth;
	CComboBox m_comboSyncOutMag;
	BOOL m_xvLEDMode;
	BOOL m_xvFanState;
	CComboBox m_comboQuantization;
	UINT m_xvExposeOn;
	UINT m_xvExposeOff;
	CPoint m_xvDecodePos;
	CSize m_xvDecodeSize;

	CPoint m_xvDecodePosBK;
	CSize m_xvDecodeSizeBK;

	int m_decodeMode;
	BOOL m_enableDecodeGPU;

	CString m_filepath;
	CString m_folderPath;
	std::unique_ptr<RingBuffer> m_ringBuffer;
	BOOL m_isMemoryRecord;
	CString m_saveTimeStamp;

	WORD m_langID;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOpencamera();
	afx_msg void OnBnClickedResetcamera();
	afx_msg void OnBnClickedAcquisitionSingle();
	afx_msg void OnBnClickedAcquisitionContinuous();
	afx_msg void OnBnClickedRec();
	afx_msg void OnBnClickedStop();
	afx_msg void OnCbnSelchangeFramerate();
	afx_msg void OnCbnSelchangeShutterFps();
	afx_msg void OnCbnSelchangeResolution();
	afx_msg void OnBnClickedSyncIn();
	afx_msg void OnBnClickedSyncInSignal();
	afx_msg void OnBnClickedSyncOutSignal();
	afx_msg void OnEnKillfocusSyncOutDelay();
	afx_msg void OnEnKillfocusSyncOutWidth();
	afx_msg void OnCbnSelchangeSyncOutMag();
	afx_msg void OnBnClickedLEDMode();
	afx_msg void OnBnClickedFANCtrl();
	afx_msg void OnCbnSelchangeQuantization();
	afx_msg void OnEnKillfocusExposeOn();
	afx_msg void OnEnKillfocusExposeOff();
	afx_msg void OnEnKillfocusDecodePos();
	afx_msg void OnBnClickedSnapshot();
	afx_msg void OnBnClickedSaveTo();
	afx_msg void OnBnClickedResetSeqNo();
	afx_msg void OnBnClickedDecodeCPU();
	afx_msg void OnBnClickedDecodeGPU();

	afx_msg void OnBnClickedCheckAdvancedSetting();
	afx_msg void OnBnClickedRecordModeMemory();
	afx_msg void OnBnClickedRecordModeStorage();
	afx_msg void OnDeltaposSpinRecordtime(NMHDR* pNMHDR, LRESULT* pResult);
};

