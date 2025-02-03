#include "stdafx.h"
#include "Util.h"

static int CALLBACK cb(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
	return 0;
}

BOOL SelectFolderDialog(HWND hwndOwner, LPTSTR initFolder, LPTSTR selectedFolder)
{
	LPMALLOC pMalloc;
	BROWSEINFO browsInfo;
	ITEMIDLIST* pIDlist;
	BOOL ret = FALSE;

	if (FAILED(SHGetMalloc(&pMalloc)))
		return FALSE;

	memset(&browsInfo, NULL, sizeof(browsInfo));
	browsInfo.hwndOwner = hwndOwner;
	browsInfo.pidlRoot = NULL;
	browsInfo.pszDisplayName = initFolder;
	browsInfo.lpszTitle = NULL;
	browsInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	browsInfo.lpfn = cb;
	browsInfo.lParam = (LPARAM)initFolder;
	browsInfo.iImage = (int)NULL;

	pIDlist = SHBrowseForFolder(&browsInfo);
	if (NULL != pIDlist)
	{
		SHGetPathFromIDList(pIDlist, selectedFolder);
		ret = TRUE;
	}
	else
		ret = FALSE;

	pMalloc->Free(pIDlist);
	pMalloc->Release();

	return ret;
}

CString MakeUpper(LPCTSTR str)
{
	CString s;

	for (size_t i = 0; i < _tcslen(str); i++)
	{
		if (str[i] >= 'a' && str[i] <= 'z')
			s.AppendChar(str[i] - 0x20);
		else
			s.AppendChar(str[i]);
	}
	return s;
}

BOOL CheckDiskSpace(const CString& filePath, unsigned int thresholdMB)
{
	ULARGE_INTEGER freeBytesAvailable;

	if (GetDiskFreeSpaceEx(filePath, &freeBytesAvailable, NULL, NULL))
	{
		ULONGLONG thresholdBytes = static_cast<ULONGLONG>(thresholdMB) * 1024 * 1024;
		if (freeBytesAvailable.QuadPart < thresholdBytes)
		{
			return FALSE;
		}

		return TRUE;
	}
	else
		return FALSE;
}

inline CString GetDesktopPathCString()
{
	TCHAR desktopPath[MAX_PATH] = { 0 };
	HRESULT result = SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, SHGFP_TYPE_CURRENT, desktopPath);

	if (SUCCEEDED(result))
	{
		return desktopPath;
	}
	else
	{
		return _T("");
	}
}
