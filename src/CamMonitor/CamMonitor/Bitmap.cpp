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

CBitmapImage::CBitmapImage(const CBitmapImage& src)
	: m_nWidth(src.m_nWidth)
	, m_nHeight(src.m_nHeight)
	, m_nLineBytes(src.m_nLineBytes)
	, m_pBuff(NULL)
	, m_pBitmapInfo(NULL)
	, m_nBitmapInfoSize(0)
	, m_hMemDC(NULL)
	, m_hBitmap(NULL)
	, m_hOldBitmap(NULL)
	, m_bColor(src.m_bColor)
{
	if (src.m_pBitmapInfo)
	{
		const size_t bmiSize = sizeof(BITMAPINFOHEADER)
			+ src.m_pBitmapInfo->bmiHeader.biClrUsed * sizeof(RGBQUAD);

		m_pBitmapInfo = (BITMAPINFO*)malloc(bmiSize);
		memcpy(m_pBitmapInfo, src.m_pBitmapInfo, bmiSize);

		m_nBitmapInfoSize = (INT32)bmiSize;
	}

	if (src.m_pBuff)
	{
		const size_t bufSize = m_nLineBytes * m_nHeight;
		m_pBuff = (BYTE*)malloc(bufSize);
		memcpy(m_pBuff, src.m_pBuff, bufSize);
	}
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

BOOL CBitmapImage::Save(LPCTSTR fileName, int rotationCount)
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

	if (rotationCount == 0)
	{
		fwrite(m_pBitmapInfo, m_nBitmapInfoSize, 1, fp);
		fwrite(m_pBuff, m_pBitmapInfo->bmiHeader.biSizeImage, 1, fp);
	}
	else
	{
		auto info = GetRotatedBitmapInfo(rotationCount);
		fwrite(info, m_nBitmapInfoSize, 1, fp);
		free(info);

		int size = GetRotatedBufferSize(rotationCount);
		BYTE* pRotatedBuffer = new BYTE[size];
		memset(pRotatedBuffer, 0, size);
		rotateImageBuffer(rotationCount, pRotatedBuffer);
		fwrite(pRotatedBuffer, size, 1, fp);
		delete[] pRotatedBuffer;
	}

	fclose(fp);

	return TRUE;
}

UINT32 CBitmapImage::GetRotatedBufferSize(int rotationCount) const
{
	bool swapWH = (rotationCount & 1);
	int dstWidth = swapWH ? m_nHeight : m_nWidth;
	int dstHeight = swapWH ? m_nWidth : m_nHeight;
	int nBytesPerPixel = m_bColor ? 3 : 1;
	int dstLineBytes = dstWidth * nBytesPerPixel;
	dstLineBytes = dstLineBytes % 4 == 0 ? dstLineBytes : dstLineBytes + (4 - dstLineBytes % 4);

	return (UINT32)(dstLineBytes * dstHeight);
}

void CBitmapImage::rotateImageBuffer(int rotationCount, BYTE* pDstBuffer) const
{
	bool swapWH = (rotationCount & 1);
	int dstWidth = swapWH ? m_nHeight : m_nWidth;
	int dstHeight = swapWH ? m_nWidth : m_nHeight;
	int nBytesPerPixel = m_bColor ? 3 : 1;
	int dstLineBytes = dstWidth * nBytesPerPixel;
	dstLineBytes = dstLineBytes % 4 == 0 ? dstLineBytes : dstLineBytes + (4 - dstLineBytes % 4);

	int ax, bx, cx;
	int ay, by, cy;
	switch (rotationCount)
	{
		case 1: // 90
			ax = 0;  bx = -1; cx = dstWidth - 1;
			ay = 1;  by = 0; cy = 0;
			break;

		case 2: // 180
			ax = -1; bx = 0; cx = dstWidth - 1;
			ay = 0; by = -1; cy = dstHeight - 1;
			break;

		case 3: // 270
			ax = 0;  bx = 1;  cx = 0;
			ay = -1; by = 0;  cy = dstHeight - 1;
			break;
	}

	for (int y = 0; y < m_nHeight; ++y)
	{
		for (int x = 0; x < m_nWidth; ++x)
		{
			const BYTE* src = m_pBuff + y * m_nLineBytes + x * nBytesPerPixel;
			const int xDst = ax * x + bx * y + cx;
			const int yDst = ay * x + by * y + cy;
			BYTE* dstp = pDstBuffer + yDst * dstLineBytes + xDst * nBytesPerPixel;
			memcpy(dstp, src, nBytesPerPixel);
		}
	}
}

BITMAPINFO* CBitmapImage::GetRotatedBitmapInfo(int rotationCount) const
{
	bool swapWH = (rotationCount & 1);
	int dstWidth = swapWH ? m_nHeight : m_nWidth;
	int dstHeight = swapWH ? m_nWidth : m_nHeight;
	int nBytesPerPixel = m_bColor ? 3 : 1;
	int nLineBytes = dstWidth * nBytesPerPixel;
	nLineBytes = nLineBytes % 4 == 0 ? nLineBytes : nLineBytes + (4 - nLineBytes % 4);

	const size_t infoSize = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256);
	BITMAPINFO* pRotatedBitmapInfo = (BITMAPINFO*)malloc(infoSize);
	memset(pRotatedBitmapInfo, 0, infoSize);

	pRotatedBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pRotatedBitmapInfo->bmiHeader.biWidth = dstWidth;
	pRotatedBitmapInfo->bmiHeader.biHeight = -dstHeight;
	pRotatedBitmapInfo->bmiHeader.biPlanes = 1;
	pRotatedBitmapInfo->bmiHeader.biBitCount = m_bColor ? 24 : 8;
	pRotatedBitmapInfo->bmiHeader.biCompression = BI_RGB;
	pRotatedBitmapInfo->bmiHeader.biSizeImage = GetRotatedBufferSize(rotationCount);
	pRotatedBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	pRotatedBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	pRotatedBitmapInfo->bmiHeader.biClrUsed = 256;
	pRotatedBitmapInfo->bmiHeader.biClrImportant = 0;

	for (int i = 0; i < 256; i++)
	{
		pRotatedBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
		pRotatedBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
		pRotatedBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
	}

	return pRotatedBitmapInfo;
}