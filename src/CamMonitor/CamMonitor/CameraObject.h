#pragma once

#include "PUCLIB.h"
#include "Bitmap.h"
#include "Lock.h"
#include "AppDefine.h"

struct ThreadInfo
{
	HANDLE handle;
	BOOL exit;
	CriticalLock lock;
	DWORD preUpdateTime;
	ThreadInfo() : handle(NULL), exit(FALSE), preUpdateTime(0) {}
};

class CCameraObject
{
public:
	CCameraObject();
	virtual ~CCameraObject();

	PUCRESULT OpenDevice(UINT32 nDeviceNo, UINT32 nSingleXferTimeOut, UINT32 nContinuousXferTimeOut, UINT32 nRingBufCount);
	void CloseDevice();

	BOOL IsOpened() const { return m_hDevice != NULL; }
	PUC_HANDLE GetHandle() const { return m_hDevice; }

	ACQUISITION_MODE GetAcquisitionMode() const { return m_acquisitionMode; }
	BOOL SetAcquisitionMode(ACQUISITION_MODE acquisitionMode);

	BOOL IsStartedLive() const;

	PUCRESULT StartLive();
	PUCRESULT StopLive();

	void UpdateBuffer();

	PUCRESULT SetQuantization(QUANTIZATION_MODE quantization);

	PUSHORT GetQuantization() { return m_q; }

	void SetCallbackSingle(RECIEVE_CALLBACK cb, void* arg) { m_cbSingle = cb; m_argSingle = arg; }
	void SetCallbackContinuous(RECIEVE_CALLBACK cb, void* arg) { m_cbContinuous = cb; m_argContinuous = arg; }

	CBitmapImage* GetSingleImageStart();
	void GetSingleImageEnd();

protected:
	void InitQuantization();

	static DWORD WINAPI _SingleAcquitisionThread(LPVOID pArg);
	void SingleAcquitisionThread();

	static void _ContinuousCallback(PPUC_XFER_DATA_INFO pInfo, void* pArg);
	void ContinuousCallback(PPUC_XFER_DATA_INFO pInfo);

protected:
	PUC_HANDLE m_hDevice;

	USHORT m_baseQ[PUC_Q_COUNT];
	USHORT m_q[PUC_Q_COUNT];

	ACQUISITION_MODE m_acquisitionMode;

	RECIEVE_CALLBACK m_cbSingle;
	void* m_argSingle;
	LockBuffer m_bufSingle;

	RECIEVE_CALLBACK m_cbContinuous;
	void* m_argContinuous;
	LockBuffer m_bufContinuous;

	ThreadInfo m_thInfo;

	LockImage m_lockImage;
	UINT8* m_pBuffer;

private:
	CCameraObject(const CCameraObject&);
	CCameraObject operator=(const CCameraObject&);
};



inline BOOL CCameraObject::SetAcquisitionMode(ACQUISITION_MODE acquisitionMode)
{
	if (IsStartedLive())
		return FALSE;

	m_acquisitionMode = acquisitionMode;
	return TRUE;
}

inline BOOL CCameraObject::IsStartedLive() const
{
	if (m_acquisitionMode == ACQUISITION_MODE_SINGLE)
		return m_thInfo.handle != NULL;
	else
	{
		BOOL b;
		PUC_IsXferring(m_hDevice, &b);
		return b;
	}
}
