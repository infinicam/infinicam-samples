#include "stdafx.h"
#include "cih.h"

CIH::CIH()
{
	Clear();
}

BOOL CIH::Read(const CString& filePath)
{
	FILE* fp = NULL;
	errno_t error;
	TCHAR* pTmpBuf = new TCHAR[0xFFFF];
	CString sTmp, key, val;
	int n;

	error = _tfopen_s(&fp, filePath, _T("r"));
	if (error != 0)
	{
		delete[] pTmpBuf;
		return FALSE;
	}

	while (_fgetts(pTmpBuf, 0xFFFF / sizeof(TCHAR), fp) != NULL)
	{
		sTmp = pTmpBuf;
		n = sTmp.Find(_T(':'), 0);
		if (n < 0)
			continue;

		key = sTmp.Left(n);
		val = sTmp.Mid(n + 1, sTmp.GetLength() - n);
		if (val.Right(1) == _T("\n"))
			val = val.Left(val.GetLength() - 1);

		if (key == CIH_KEY_FRAMERATE)
		{
			m_framerate = _ttoi(val);
		}
		else if (key == CIH_KEY_SHUTTER_FPS)
		{
			m_shutterFps = _ttoi(val);
		}
		else if (key == CIH_KEY_EXPOSE_ON)
		{
			m_exposeOn = _ttoi(val);
		}
		else if (key == CIH_KEY_EXPOSE_OFF)
		{
			m_exposeOff = _ttoi(val);
		}
		else if (key == CIH_KEY_WIDTH)
		{
			m_width = _ttoi(val);
		}
		else if (key == CIH_KEY_HEIGHT)
		{
			m_height = _ttoi(val);
		}
		else if (key == CIH_KEY_FRAME_SIZE)
		{
			m_frameSize = _ttoi(val);
		}
		else if (key == CIH_KEY_FRAME_COUNT)
		{
			m_frameCount = _ttoi(val);
		}
		else if (key == CIH_KEY_QUANTIZATION)
		{
			_tcscpy_s(pTmpBuf, 0xFFFF, val);
			LPTSTR p = pTmpBuf;
			int ns = 0, count = 0, index = 0;
			for (int i = 0; i < val.GetLength(); i++)
			{
				if (p[i] == _T(' '))
				{
					p[i] = NULL;
					if (count == 0)
					{
						ns++;
						continue;
					}
					m_quantization[index++] = _ttoi(&pTmpBuf[ns]);
					count = 0;
					ns = i + 1;
				}
				else
				{
					count++;
				}
			}
			if (count != 0)
			{
				m_quantization[index++] = _ttoi(&pTmpBuf[ns]);
			}
		}
		else if (key == CIH_KEY_FILE_TYPE)
		{
			m_filetype = (SAVE_FILE_TYPE)_ttoi(val);
		}
		else if (key == CIH_KEY_COLOR_TYPE)
		{
			m_colortype = (PUC_COLOR_TYPE)_ttoi(val);
		}
	}

	m_filepath = filePath;
	m_dirpath = m_filepath.Left(m_filepath.ReverseFind(_T('\\')));
	m_filename = m_filepath.Right(m_filepath.GetLength() - m_filepath.ReverseFind(_T('\\')) - 1);
	m_filename = m_filename.Left(m_filename.ReverseFind(_T('.')));

	fclose(fp);
	delete[] pTmpBuf;
	return TRUE;
}

BOOL CIH::Write(const CString& filePath)
{
	FILE* fp = NULL;
	errno_t error;
	CString s;

	error = _tfopen_s(&fp, filePath, _T("w"));
	if (error != 0)
		return FALSE;

	s.Format(_T("%s:%u\n"), CIH_KEY_FRAMERATE, m_framerate);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_SHUTTER_FPS, m_shutterFps);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_EXPOSE_ON, m_exposeOn);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_EXPOSE_OFF, m_exposeOff);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_WIDTH, m_width);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_HEIGHT, m_height);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_FRAME_SIZE, m_frameSize);
	_fputts(s, fp);
	s.Format(_T("%s:%u\n"), CIH_KEY_FRAME_COUNT, m_frameCount);
	_fputts(s, fp);

	s.Format(_T("%s:"), CIH_KEY_QUANTIZATION);
	_fputts(s, fp);
	int qCount = sizeof(m_quantization) / sizeof(USHORT);
	for (int i = 0; i < qCount; i++)
	{
		if (i == (qCount - 1))
			s.Format(_T("%d"), m_quantization[i]);
		else
			s.Format(_T("%d "), m_quantization[i]);
		_fputts(s, fp);
	}
	_fputts(_T("\n"), fp);

	s.Format(_T("%s:%d\n"), CIH_KEY_FILE_TYPE, m_filetype);
	_fputts(s, fp);

	s.Format(_T("%s:%d\n"), CIH_KEY_COLOR_TYPE, m_colortype);
	_fputts(s, fp);

	fclose(fp);
	return TRUE;
}

void CIH::Clear()
{
	// write data
	UINT32 m_framerate = 0;
	UINT32 m_width = 0;
	UINT32 m_height = 0;
	UINT32 m_frameSize = 0;
	UINT32 m_frameCount = 0;
	memset(m_quantization, 0, sizeof(m_quantization));
	m_filetype = SAVE_FILE_TYPE_RAW;
	m_colortype = PUC_COLOR_MONO;

	// not write data
	m_filepath = _T("");
	m_dirpath = _T("");
	m_filename = _T("");
}
