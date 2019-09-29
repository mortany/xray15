//----------------------------------------------------
// file: NetDeviceLog.h
//----------------------------------------------------

#ifndef _INCDEF_NETDEVICELOG_H_
#define _INCDEF_NETDEVICELOG_H_

// -------
#define NLOG_CONSOLE_OUT
// -------

class CExportConsole{
protected:

	bool m_Valid;
	HANDLE m_hThread;
	DWORD m_ThreadId;


public:

	HWND m_hParent;
	HWND m_hWindow;
	HINSTANCE m_hInstance;

	CRITICAL_SECTION m_CSection;

	bool m_Enter;
	TCHAR m_EnterBuffer[256];

	class _ConsoleMsg{
	public:
		TCHAR buf[1024];
		_ConsoleMsg(LPCTSTR b){ wcscpy_s(buf,b); }
	};
		
	std::list<_ConsoleMsg> m_Messages;

	float fMaxVal, fStatusProgress;
public:

	bool Init( HINSTANCE _Inst, HWND _Window );
	void Clear();

	void print	(TMsgDlgType mt, const TCHAR *buf);

	bool valid();

	void ProgressStart(float max_val, LPCTSTR text=0);
	void ProgressEnd();
	void ProgressInc();
	void ProgressUpdate(float val);

	void StayOnTop	(BOOL flag);

	CExportConsole();
	~CExportConsole();

	static INT_PTR CALLBACK ConsoleDialogProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp);
};

extern CExportConsole EConsole;

#endif /*_INCDEF_NETDEVICELOG_H_*/

