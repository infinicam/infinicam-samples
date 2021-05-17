#pragma once

#include "Bitmap.h"

class LockBase
{
public:
	LockBase() {}
	virtual ~LockBase() {}
	virtual bool Lock() = 0;
	virtual void Unlock() = 0;
};


class CriticalLock : public LockBase
{
public:
	CriticalLock() { InitializeCriticalSection(&m_cs); }
	virtual ~CriticalLock() { Unlock(); DeleteCriticalSection(&m_cs); }
	virtual bool Lock() { EnterCriticalSection(&m_cs); return true; }
	virtual void Unlock() { LeaveCriticalSection(&m_cs); }
private:
	CRITICAL_SECTION m_cs;
};


class LockBuffer : public CriticalLock
{
public:
	LockBuffer(INT nSize);
	virtual ~LockBuffer();

	void* GetLockData();
	INT GetSize() const { return m_nSize; }

	void Create(INT nSize);

private:
	void* m_pBuffer;
	INT m_nSize;
};

class LockImage : public CriticalLock
{
public:
	LockImage();
	virtual ~LockImage();

	void Create();
	void Delete();

	void* GetLockData();

public:
	void SetWidth(UINT32 nWidth) { m_nWidth = nWidth; }
	void SetHeight(UINT32 nHeight) { m_nHeight = nHeight; }
	void SetColor(BOOL isColor) { m_isColor = isColor; }

private:
	CBitmapImage* m_pImage;
	UINT32 m_nWidth;
	UINT32 m_nHeight;
	BOOL m_isColor;
};


