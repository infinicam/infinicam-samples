#pragma once

extern BOOL SelectFolderDialog(HWND hwndOwner, LPTSTR initFolder, LPTSTR selectedFolder);
extern CString MakeUpper(LPCTSTR str);
extern BOOL CheckDiskSpace(const CString& filePath, unsigned int thresholdMB);
extern  CString GetDesktopPathCString();
