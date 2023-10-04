#include "stdafx.h"
#include "PUCLIB.h"

// Note:Switch decoding to multi threading
// #define MULTITHREAD_DECODE

void SaveBitmap(UINT32 nWidth, UINT32 nHeight, UINT32 nLineBytes, UINT8* pBuf)
{
	UINT32 nInfoBytes;
	BITMAPINFO* pBitmapInfo;
	DWORD nPixelBytes;
	BITMAPFILEHEADER fileHeader;
	FILE* fp;

	nInfoBytes = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256);
	pBitmapInfo = (BITMAPINFO*)new BYTE[nInfoBytes];
	nPixelBytes = nLineBytes * nHeight;

	memset(pBitmapInfo, 0, nInfoBytes);
	pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfo->bmiHeader.biWidth = nWidth;
	pBitmapInfo->bmiHeader.biHeight = -(INT32)nHeight;
	pBitmapInfo->bmiHeader.biPlanes = 1;
	pBitmapInfo->bmiHeader.biBitCount = 8;
	pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	pBitmapInfo->bmiHeader.biSizeImage = nPixelBytes;
	pBitmapInfo->bmiHeader.biClrUsed = 256;

	for (int i = 0; i < 256; i++)
	{
		pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
		pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
		pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
	}

	memset(&fileHeader, 0, sizeof(fileHeader));
	fileHeader.bfType = ('M' << 8) + 'B';
	fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + nInfoBytes + nPixelBytes;
	fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + nInfoBytes;

	_tfopen_s(&fp, _T("test.bmp"), _T("wb"));
	fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	fwrite(pBitmapInfo, nInfoBytes, 1, fp);
	fwrite(pBuf, nPixelBytes, 1, fp);
	fclose(fp);

	delete[] pBitmapInfo;
}

void SaveSingleImage(PUC_HANDLE hDevice)
{
	PUCRESULT result = PUC_SUCCEEDED;
	UINT32 nDataSize = 0;
	PUC_XFER_DATA_INFO xferData = { 0 };
	UINT32 nWidth, nHeight, nLineBytes;
	USHORT q[PUC_Q_COUNT];
	UINT8* pDecodeBuf = NULL;

	result = PUC_GetXferDataSize(hDevice, &nDataSize);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_GetXferDataSize error:%u\n"), result);
		goto EXIT_LABEL;
	}

	xferData.pData = new UINT8[nDataSize];

	result = PUC_GetSingleXferData(hDevice, &xferData);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_GetSingleXferData error:%u\n"), result);
		goto EXIT_LABEL;
	}

	result = PUC_GetResolution(hDevice, &nWidth, &nHeight);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_GetResolution error:%u\n"), result);
		goto EXIT_LABEL;
	}

	for (UINT32 i = 0; i < PUC_Q_COUNT; i++)
	{
		result = PUC_GetQuantization(hDevice, i, &q[i]);
		if (PUC_CHK_FAILED(result))
		{
			_tprintf(_T("PUC_GetQuantization error:%u\n"), result);
			goto EXIT_LABEL;
		}
	}

	nLineBytes = nWidth % 4 == 0 ? nWidth : nWidth + (4 - nWidth % 4);
	pDecodeBuf = new UINT8[nLineBytes * nHeight];


#ifdef MULTITHREAD_DECODE
	result = PUC_DecodeDataMultiThread(pDecodeBuf, 0, 0, nWidth, nHeight, nLineBytes, xferData.pData, q, 8);
#else
	result = PUC_DecodeData(pDecodeBuf, 0, 0, nWidth, nHeight, nLineBytes, xferData.pData, q);
#endif // MULTITHREAD_DECODE

	
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_DecodeData error:%u\n"), result);
		goto EXIT_LABEL;
	}

	SaveBitmap(nWidth, nHeight, nLineBytes, pDecodeBuf);

EXIT_LABEL:
	if (xferData.pData)
		delete[] xferData.pData;
	if (pDecodeBuf)
		delete[] pDecodeBuf;
}

int _tmain(int argc, _TCHAR* argv[])
{
	PUCRESULT result = PUC_SUCCEEDED;
	PUC_DETECT_INFO detectInfo = { 0 };
	PUC_HANDLE hDevice = NULL;

	result = PUC_Initialize();
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_Initialize error:%u\n"), result);
		goto EXIT_LABEL;
	}

	result = PUC_DetectDevice(&detectInfo);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_DetectDevice error:%u\n"), result);
		goto EXIT_LABEL;
	}
	if (detectInfo.nDeviceCount == 0)
	{
		_tprintf(_T("device count : 0\n"));
		goto EXIT_LABEL;
	}

	result = PUC_OpenDevice(detectInfo.nDeviceNoList[0], &hDevice);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_OpenDevice error:%u\n"), result);
		goto EXIT_LABEL;
	}

	result = PUC_SetFramerateShutter(hDevice, 1000, 2000);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_SetFramerateShutter error:%u\n"), result);
		goto EXIT_LABEL;
	}

	result = PUC_SetResolution(hDevice, 1246, 800);
	if (PUC_CHK_FAILED(result))
	{
		_tprintf(_T("PUC_SetResolution error:%u\n"), result);
		goto EXIT_LABEL;
	}

	SaveSingleImage(hDevice);

EXIT_LABEL:
	if (hDevice)
	{
		result = PUC_CloseDevice(hDevice);
		if (PUC_CHK_FAILED(result))
		{
			_tprintf(_T("PUC_CloseDevice error:%u\n"), result);
		}
	}
	return 0;
}

