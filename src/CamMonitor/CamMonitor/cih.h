#pragma once

#include "AppDefine.h"
#include "PUCLIB.h"

// Camera Info Header

#define CIH_KEY_FRAMERATE		_T("framerate")
#define CIH_KEY_SHUTTER_FPS		_T("shutterFps")
#define CIH_KEY_EXPOSE_ON		_T("exposeon")
#define CIH_KEY_EXPOSE_OFF		_T("exposeoff")
#define CIH_KEY_WIDTH			_T("width")
#define CIH_KEY_HEIGHT			_T("height")
#define CIH_KEY_FRAME_SIZE		_T("framesize")
#define CIH_KEY_FRAME_COUNT		_T("framecount")
#define CIH_KEY_QUANTIZATION	_T("quantization")
#define CIH_KEY_FILE_TYPE		_T("filetype")
#define CIH_KEY_COLOR_TYPE		_T("colortype")

class CIH
{
public:
	CIH();

	BOOL Read(const CString& filePath);
	BOOL Write(const CString& filePath);

	void Clear();

	// write data
	UINT32 m_framerate;
	UINT32 m_shutterFps;
	UINT32 m_exposeOn;
	UINT32 m_exposeOff;
	UINT32 m_width;
	UINT32 m_height;
	UINT32 m_frameSize;
	UINT32 m_frameCount;
	USHORT m_quantization[64];
	SAVE_FILE_TYPE m_filetype;
	PUC_COLOR_TYPE m_colortype;

	// not write data
	CString m_filepath;
	CString m_dirpath;
	CString m_filename;

private:
	CIH(const CIH&);
	CIH operator=(const CIH&);
};

