#include "stdafx.h"
#include "Bitmap.h"


CBitmapImage::CBitmapImage()
	: m_nWidth(0)
	, m_nHeight(0)
	, m_nLineBytes(0)
	, m_pBuff(NULL)
	, m_pBitmapInfo(NULL)
	, m_nBitmapInfoSize(0)
	, m_hMemDC(NULL)
	, m_hBitmap(NULL)
	, m_hOldBitmap(NULL)
	, m_bColor(FALSE)
{
}

CBitmapImage::~CBitmapImage()
{
	Release();
}

BOOL CBitmapImage::Create(LONG nWidth, LONG nHeight, BOOL isColor)
{
	INT32 nLineBytes = 0;
	INT32 nPixelBytes = 0;
	INT32 nInfoBytes = 0;
	BITMAPINFO* pBitmapInfo = NULL;
	HDC hMemDC = NULL;
	UINT8* pBits = NULL;
	HBITMAP hBitmap = NULL;
	HBITMAP hOldBitmap = NULL;
	BOOL bReturnValue = TRUE;

	if (m_pBuff != NULL && (nWidth == m_nWidth && nHeight == m_nHeight && m_bColor == isColor))
		return TRUE;

	Release();

	if (isColor)
		nLineBytes = nWidth * 3;
	else
		nLineBytes = nWidth;

	nLineBytes = nLineBytes % 4 == 0 ? nLineBytes : nLineBytes + (4 - nLineBytes % 4);

	nPixelBytes = nLineBytes * nHeight;
	nInfoBytes = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256);

	pBitmapInfo = (BITMAPINFO*)malloc(nInfoBytes);
	if (pBitmapInfo == NULL)
	{
		bReturnValue = FALSE;
		goto EXIT_LABEL;
	}
	memset(pBitmapInfo, 0, nInfoBytes);
	pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pBitmapInfo->bmiHeader.biWidth = nWidth;
	pBitmapInfo->bmiHeader.biHeight = -nHeight;
	pBitmapInfo->bmiHeader.biPlanes = 1;
	pBitmapInfo->bmiHeader.biBitCount = isColor ? 24 : 8;
	pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	pBitmapInfo->bmiHeader.biSizeImage = nPixelBytes;
	pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	pBitmapInfo->bmiHeader.biClrUsed = 256;
	pBitmapInfo->bmiHeader.biClrImportant = 0;

	for (int i = 0; i < 256; i++)
	{
		pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
		pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
		pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
	}

	hMemDC = CreateCompatibleDC(NULL);
	hBitmap = CreateDIBSection(hMemDC, pBitmapInfo, isColor ? DIB_RGB_COLORS : DIB_PAL_COLORS, (void**)&pBits, 0, 0);
	if (NULL == hBitmap)
	{
		bReturnValue = FALSE;
		goto EXIT_LABEL;
	}

	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

EXIT_LABEL:

	if (bReturnValue)
	{
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_nLineBytes = nLineBytes;
		m_pBuff = pBits;
		m_pBitmapInfo = pBitmapInfo;
		m_nBitmapInfoSize = nInfoBytes;
		m_hMemDC = hMemDC;
		m_hBitmap = hBitmap;
		m_hOldBitmap = hOldBitmap;
		m_bColor = isColor;
	}
	else
	{
		if (pBitmapInfo != NULL)
			free(pBitmapInfo);
		if (hBitmap != NULL)
			DeleteObject(hBitmap);
	}

	return bReturnValue;
}

void CBitmapImage::Release()
{
	if (m_hMemDC != NULL)
	{
		SelectObject(m_hMemDC, m_hOldBitmap);
		DeleteDC(m_hMemDC);
	}
	DeleteObject(m_hBitmap);
	free(m_pBitmapInfo);

	m_nWidth = 0;
	m_nHeight = 0;
	m_nLineBytes = 0;
	m_pBuff = NULL;
	m_pBitmapInfo = NULL;
	m_hMemDC = NULL;
	m_hBitmap = NULL;
	m_hOldBitmap = NULL;
	m_bColor = FALSE;
}

BOOL CBitmapImage::Save(LPCTSTR fileName)
{
	FILE* fp = NULL;
	errno_t error;

	error = _tfopen_s(&fp, fileName, _T("wb"));
	if (error != 0)
		return FALSE;

	BITMAPFILEHEADER h;
	memset(&h, 0, sizeof(h));
	h.bfType = ('M' << 8) | 'B';
	h.bfSize = sizeof(BITMAPFILEHEADER) + m_nBitmapInfoSize + m_pBitmapInfo->bmiHeader.biSizeImage;
	h.bfOffBits = sizeof(BITMAPFILEHEADER) + m_nBitmapInfoSize;

	fwrite(&h, sizeof(h), 1, fp);
	fwrite(m_pBitmapInfo, m_nBitmapInfoSize, 1, fp);
	fwrite(m_pBuff, m_pBitmapInfo->bmiHeader.biSizeImage, 1, fp);

	fclose(fp);

	return TRUE;
}
