#pragma once

#include "PUCLIB.h"

class CSelectCameraDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSelectCameraDialog)

public:
	CSelectCameraDialog(CWnd* pParent = nullptr);
	virtual ~CSelectCameraDialog();

	UINT32 GetDeviceNo() const { return m_nDeviceNo; }
	UINT32 GetSingleXferTimeOut() const { return m_nSingleXferTimeOut; }
	UINT32 GetContinuousXferTimeOut() const { return m_nContinuousXferTimeOut; }
	UINT32 GetRingBufCount() const { return m_nRingBufCount; }

private:
	CComboBox m_comboCamera;
	CString m_xvSingleXferTimeOut;
	CString m_xvContinuousXferTimeOut;
	CString m_xvRingBufCount;

	PUC_DETECT_INFO m_detectInfo;

	UINT32 m_nDeviceNo;
	UINT32 m_nSingleXferTimeOut;
	UINT32 m_nContinuousXferTimeOut;
	UINT32 m_nRingBufCount;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateControlState();
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
};
