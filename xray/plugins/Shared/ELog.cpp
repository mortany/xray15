//----------------------------------------------------
// file: NetDeviceELog.cpp
//----------------------------------------------------

//#include "Stdafx.h"
#include "Stdafx.h"
#pragma hdrstop

#include "ELog.h"
#ifdef _MAX_EXPORT
	#include "..\Max\Export\NetDeviceLog.h"
	void ELogCallback(LPCTSTR txt)
	{
 		if (0!=txt[0]){
			if (txt[0]=='!')EConsole.print(mtError,txt+1);
			else			EConsole.print(mtInformation,txt);
		}
	}
#endif
//----------------------------------------------------
CLog ELog;
//----------------------------------------------------

int CLog::DlgMsg (TMsgDlgType mt, TMsgDlgButtons btn, LPCTSTR _Format, ...)
{
    in_use = true;
	TCHAR buf[4096];
	va_list l;
	va_start( l, _Format );
	wvsprintf( buf, _Format, l );
	va_end(l);

	int res=0;
#ifdef _MAX_PLUGIN
	switch(mt){
	case mtError:		MessageBox(0, buf, LPCWSTR("Error 1"),		MB_OK|MB_ICONERROR);		break;
	case mtInformation: MessageBox(0, buf, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	default:			MessageBox(0, buf, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	}
#endif

    Msg(mt, buf);

    in_use = false;

    return res;
}


int CLog::DlgMsg (TMsgDlgType mt, LPCTSTR _Format, ...)
{
    in_use = true;
	TCHAR buf[4096];
	va_list l;
	va_start(l, _Format);
	wvsprintf(buf, _Format, l);
	va_end(l);

    int res=0;

#ifdef _MAX_PLUGIN
	switch(mt){
	case mtError:		MessageBox(0, buf, LPCWSTR("Error 1"),		MB_OK|MB_ICONERROR);		break;
	case mtInformation: MessageBox(0, buf, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	default:			MessageBox(0, buf, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	}
#endif

    Msg(mt,buf);

    in_use = false;
    
    return res;
}

void CLog::Msg(TMsgDlgType mt, LPCTSTR _Format, ...)
{

	TCHAR buf[4096];
	va_list l;
	va_start(l, _Format);
	wvsprintf(buf, _Format, l);
	va_end(l);

#ifdef _MAX_EXPORT
	EConsole.print(mt,buf);
#endif
	::LogExecCB = FALSE;
    ::Msg		(buf);
	::LogExecCB	= TRUE;
}
//----------------------------------------------------