#include "stdafx.h"
#include "CameraObject.h"
#include "AppDefine.h"

#include <string>

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

CCameraObject::CCameraObject()
	: m_nDeviceNo(0)
	, m_hDevice(NULL)
	, m_acquisitionMode(ACQUISITION_MODE_SINGLE)
	, m_cbSingle(NULL)
	, m_argSingle(NULL)
	, m_bufSingle(0)
	, m_cbContinuous(NULL)
	, m_argContinuous(NULL)
	, m_bufContinuous(0)
	, m_pBuffer(NULL)
{
	memset(m_baseQ, 0, sizeof(m_baseQ));
	memset(m_q, 0, sizeof(m_q));
}

CCameraObject::~CCameraObject()
{
	CloseDevice();
}


PUCRESULT CCameraObject::OpenDevice(UINT32 nDeviceNo, UINT32 nSingleXferTimeOut, UINT32 nContinuousXferTimeOut, UINT32 nRingBufCount)
{
	PUC_HANDLE hDevice = NULL;
	PUCRESULT result = PUC_SUCCEEDED;

	CloseDevice();

	result = PUC_OpenDevice(nDeviceNo, &hDevice);
	if (PUC_CHK_FAILED(result))
		return result;

	result = PUC_SetXferTimeOut(hDevice, nSingleXferTimeOut, nContinuousXferTimeOut);
	if (PUC_CHK_FAILED(result))
		return result;

	result = PUC_SetRingBufferCount(hDevice, nRingBufCount);
	if (PUC_CHK_FAILED(result))
		return result;

	m_nDeviceNo = nDeviceNo;
	m_hDevice = hDevice;

	InitQuantization();
	UpdateBuffer();

	return result;
}

void CCameraObject::CloseDevice()
{
	if (m_hDevice)
	{
		PUC_CloseDevice(m_hDevice);
		m_hDevice = NULL;
	}

	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}

	m_nDeviceNo = 0;
}

void CCameraObject::UpdateBuffer()
{
	UINT32 nBufSize = 0;
	PUC_GetXferDataSize(m_hDevice, &nBufSize);

	m_bufSingle.Create(nBufSize);
	m_bufContinuous.Create(nBufSize);

	if (m_pBuffer)
		delete[] m_pBuffer;
	m_pBuffer = new UINT8[nBufSize];

	UINT32 w, h;
	PUC_COLOR_TYPE colorType;
	PUC_GetResolution(m_hDevice, &w, &h);
	PUC_GetColorType(m_hDevice, &colorType);
	m_lockImage.SetWidth(w);
	m_lockImage.SetHeight(h);
	m_lockImage.SetColor(colorType != PUC_COLOR_MONO);
	m_lockImage.Create();
}

PUCRESULT CCameraObject::StartLive()
{
	PUCRESULT result = PUC_SUCCEEDED;
	DWORD nID;

	if (!IsOpened())
		return PUC_SUCCEEDED;

	m_thInfo.hThread = AfxBeginThread((AFX_THREADPROC)_SingleAcquitisionThread, (LPVOID)this);
	if (m_acquisitionMode == ACQUISITION_MODE_CONTINUOUS)
	{
		result = PUC_BeginXferData(m_hDevice, _ContinuousCallback, this);
	}

	return result;
}

PUCRESULT CCameraObject::StopLive()
{
	PUCRESULT result = PUC_SUCCEEDED;

	if (!IsOpened())
		return PUC_SUCCEEDED;

	// live‚ªstart‚³‚ê‚Ä‚¢‚È‚¢
	if (!m_thInfo.hThread)
		return PUC_SUCCEEDED;

	m_thInfo.lock.Lock();
	m_thInfo.exit = TRUE;
	m_thInfo.lock.Unlock();
	WaitForSingleObject(m_thInfo.hThread->m_hThread, INFINITE);
	m_thInfo.hThread->m_hThread = NULL;
	m_thInfo.hThread = NULL;
	m_thInfo.exit = FALSE;

	if (m_acquisitionMode == ACQUISITION_MODE_CONTINUOUS)
	{
		result = PUC_EndXferData(m_hDevice);
	}

	return result;
}

PUCRESULT CCameraObject::SetQuantization(QUANTIZATION_MODE quantization)
{
	PUCRESULT result = PUC_SUCCEEDED;

	switch (quantization)
	{
	case IMAGE_QUALITY_NORMAL:
		for (int i = 0; i < PUC_Q_COUNT; i++)
		{
			m_q[i] = m_baseQ[i] * 2;
			PUC_SetQuantization(m_hDevice, i, m_q[i]);
		}
		break;
	case IMAGE_QUALITY_LOW:
		for (int i = 0; i < PUC_Q_COUNT; i++)
		{
			m_q[i] = m_baseQ[i] * 4;
			PUC_SetQuantization(m_hDevice, i, m_q[i]);
		}
		break;
	case IMAGE_QUALITY_HIGH:
		for (int i = 0; i < PUC_Q_COUNT; i++)
		{
			m_q[i] = m_baseQ[i] * 1;
			PUC_SetQuantization(m_hDevice, i, m_q[i]);
		}
		break;
	}

	return result;
}

void CCameraObject::InitQuantization()
{
	USHORT nVal;
	for (int i = 0; i < PUC_Q_COUNT; i++)
	{
		PUC_GetQuantization(m_hDevice, i, &nVal);
		m_baseQ[i] = m_q[i] = nVal;
	}
}

DWORD WINAPI CCameraObject::_SingleAcquitisionThread(LPVOID pArg)
{
	CCameraObject* pObj = (CCameraObject*)pArg;
	pObj->SingleAcquitisionThread();
	return 0;
}

void CCameraObject::SingleAcquitisionThread()
{
	PUCRESULT result = PUC_SUCCEEDED;
	PUC_XFER_DATA_INFO xferData = { 0 };

	m_thInfo.preUpdateTime = timeGetTime();

	while (1)
	{
		m_thInfo.lock.Lock();
		if (m_thInfo.exit)
		{
			m_thInfo.lock.Unlock();
			break;
		}
		m_thInfo.lock.Unlock();

		DWORD nEndTime = timeGetTime();
		if ((nEndTime - m_thInfo.preUpdateTime) <= UPDATE_IMG_INTERVAL)
			continue;

		// ***Lock***
		xferData.pData = (UINT8*)m_bufSingle.GetLockData();
		result = PUC_GetSingleXferData(m_hDevice, &xferData);
		if (PUC_CHK_FAILED(result))
		{
			TRACE(_T("PUC_GetSingleXferData error %d\n"), result);
		}
		else
		{
			if (m_cbSingle)
				m_cbSingle(&xferData, m_argSingle);
		}
		m_bufSingle.Unlock();
		
		string seqNo = "seqNo : " + std::to_string(xferData.nSequenceNo) + "\n";
		//OutputDebugString(s2ws(seqNo).c_str());

		// ***Unlock***

		m_thInfo.preUpdateTime = nEndTime;
	}
}

CBitmapImage* CCameraObject::GetSingleImageStart()
{
	PUCRESULT result = PUC_SUCCEEDED;
	PUC_XFER_DATA_INFO xferData = { 0 };

	xferData.pData = m_pBuffer;
	result = PUC_GetSingleXferData(m_hDevice, &xferData);
	if (PUC_CHK_FAILED(result))
		return NULL;

	// ***Lock***
	CBitmapImage* pImage = (CBitmapImage*)m_lockImage.GetLockData();
	pImage->FillBlack();
	result = PUC_DecodeData(
		pImage->GetBuffer(),
		0,
		0,
		pImage->GetWidth(),
		pImage->GetHeight(),
		pImage->GetLineByte(),
		xferData.pData,
		(PUSHORT)m_q);
	if (PUC_CHK_FAILED(result))
	{
		m_lockImage.Unlock();
		// ***Unlock***
		return NULL;
	}

	return pImage;
}

void CCameraObject::GetSingleImageEnd()
{
	m_lockImage.Unlock();
	// ***Unlock***
}

void CCameraObject::_ContinuousCallback(PPUC_XFER_DATA_INFO pInfo, void* pArg)
{
	CCameraObject* pObj = (CCameraObject*)pArg;
	pObj->ContinuousCallback(pInfo);
}




void CCameraObject::ContinuousCallback(PPUC_XFER_DATA_INFO pInfo)
{
	// ***Lock***
	UINT8* pBuffer = (UINT8*)m_bufContinuous.GetLockData();
	PUC_XFER_DATA_INFO xferData = { 0 };
	memcpy(pBuffer, pInfo->pData, pInfo->nDataSize);
	xferData.pData = pBuffer;
	xferData.nDataSize = pInfo->nDataSize;
	xferData.nSequenceNo = pInfo->nSequenceNo;

	
	string seqNo = "seqNo : " + std::to_string(xferData.nSequenceNo) + "\n";
	//OutputDebugString(s2ws(seqNo).c_str());
	

	if (m_cbContinuous)
		m_cbContinuous(&xferData, m_argContinuous);
	m_bufContinuous.Unlock();
	// ***Unlock***
}