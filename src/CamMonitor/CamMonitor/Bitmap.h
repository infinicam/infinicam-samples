#pragma once

class CBitmapImage
{
public:
	CBitmapImage();
	virtual ~CBitmapImage();

	HDC GetDC() const { return m_hMemDC; }
	INT32 GetWidth() const { return m_nWidth; }
	INT32 GetHeight() const { return m_nHeight; }
	LONG GetLineByte() const { return m_nLineBytes; }
	UINT8* GetBuffer() const { return m_pBuff; }
	BITMAPINFO* GetBitmapInfo() const { return m_pBitmapInfo; }
	BOOL IsColor() const { return m_bColor; }

	BOOL Create(LONG nWidth, LONG nHeight, BOOL color);
	BOOL IsEmpty() const { return m_pBuff == NULL; }
	void Release();

	void FillBlack() { memset(m_pBuff, 0x00, m_nLineBytes * m_nHeight); }

	BOOL Save(LPCTSTR fileName);

private:
	CBitmapImage(const CBitmapImage&);
	CBitmapImage operator=(const CBitmapImage&);

protected:
	INT32 m_nWidth;
	INT32 m_nHeight;
	INT32 m_nLineBytes;
	UINT8* m_pBuff;
	BITMAPINFO* m_pBitmapInfo;
	INT32 m_nBitmapInfoSize;
	HDC m_hMemDC;
	HBITMAP m_hBitmap;
	HBITMAP m_hOldBitmap;
	BOOL m_bColor;
};
