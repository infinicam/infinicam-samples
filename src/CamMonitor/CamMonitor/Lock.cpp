#include "stdafx.h"
#include "Lock.h"

LockBuffer::LockBuffer(INT nSize)
	: m_pBuffer(NULL)
{
	m_pBuffer = new BYTE[nSize];
	memset(m_pBuffer, 0, nSize);
	m_nSize = nSize;
}

LockBuffer::~LockBuffer()
{
	delete[] m_pBuffer;
}

void* LockBuffer::GetLockData()
{
	Lock();
	return m_pBuffer;
}

void LockBuffer::Create(INT nSize)
{
	delete[] m_pBuffer;
	m_pBuffer = new BYTE[nSize];
	memset(m_pBuffer, 0, nSize);
	m_nSize = nSize;
}




LockImage::LockImage()
	: m_pImage(NULL)
	, m_nWidth(0)
	, m_nHeight(0)
	, m_isColor(FALSE)
{
	m_pImage = new CBitmapImage;
}

LockImage::~LockImage()
{
	delete m_pImage;
}

void LockImage::Create()
{
	Delete();

	Lock();

	m_pImage->Create(m_nWidth, m_nHeight, m_isColor);

	Unlock();
}

void LockImage::Delete()
{
	Lock();

	m_pImage->Release();

	Unlock();
}

void* LockImage::GetLockData()
{
	Lock();
	return m_pImage;
}
