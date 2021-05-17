#include "stdafx.h"
#include "CameraObject.h"
#include "AppDefine.h"

CCameraObject::CCameraObject()
	: m_hDevice(NULL)
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
}

void CCameraObject::UpdateBuffer()
{
	UINT32 nBufSize = 0;
	PUC_GetXferDataSize(m_hDevice, PUC_DATA_COMPRESSED , &nBufSize);

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

	m_thInfo.handle = CreateThread(NULL, 0, _SingleAcquitisionThread, this, 0, &nID);

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

	m_thInfo.lock.Lock();
	m_thInfo.exit = TRUE;
	m_thInfo.lock.Unlock();
	WaitForSingleObject(m_thInfo.handle, INFINITE);
	m_thInfo.handle = NULL;
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
	if (m_cbContinuous)
		m_cbContinuous(&xferData, m_argContinuous);
	m_bufContinuous.Unlock();
	// ***Unlock***
}