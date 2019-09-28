//----------------------------------------------------
// file: NetDeviceELog.cpp
//----------------------------------------------------

#include "Stdafx.h"
#pragma hdrstop

#include "ELog.h"
#ifdef _MAX_EXPORT
	#include "..\Max\Export\NetDeviceLog.h"
	void ELogCallback(LPCSTR txt)
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

int CLog::DlgMsg (TMsgDlgType mt, TMsgDlgButtons btn, LPCSTR _Format, ...)
{
    in_use = true;
	char buf[4096];
	va_list l;
	va_start( l, _Format );
	vsprintf( buf, _Format, l );

	wchar_t text_wchar[4096];

	size_t outSize;

	mbstowcs_s(&outSize, text_wchar, buf, 4096);

	int res=0;
#ifdef _MAX_PLUGIN
	switch(mt){
	case mtError:		MessageBox(0, text_wchar,LPCWSTR("Error"),		MB_OK|MB_ICONERROR);		break;
	case mtInformation: MessageBox(0, text_wchar, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	default:			MessageBox(0, text_wchar, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	}
#endif

    Msg(mt, buf);

    in_use = false;

    return res;
}


int CLog::DlgMsg (TMsgDlgType mt, LPCSTR _Format, ...)
{
    in_use = true;
	char buf[4096];
	va_list l;
	va_start( l, _Format );
	vsprintf( buf, _Format, l );

	wchar_t text_wchar[4096];

	size_t outSize;

	mbstowcs_s(&outSize, text_wchar, buf, 4096);

    int res=0;

#ifdef _MAX_PLUGIN
	switch(mt){
	case mtError:		MessageBox(0, text_wchar, LPCWSTR("Error"),		MB_OK|MB_ICONERROR);		break;
	case mtInformation: MessageBox(0, text_wchar, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	default:			MessageBox(0, text_wchar, LPCWSTR("Information"),	MB_OK|MB_ICONINFORMATION);	break;
	}
#endif

    Msg(mt,buf);

    in_use = false;
    
    return res;
}

void CLog::Msg(TMsgDlgType mt, LPCSTR _Format, ...)
{
	std::locale::global(std::locale(""));

	//string m_buf = StringFromUTF8(_Format, locale);

	//char cstr[m_buf.size() + 1];
	//strcpy(cstr, m_buf.c_str());	// or pass &s[0]

	//Listener* listener = the_listener;



	char buf[4096];

	//wchar_t text_wchar[4096];

	//size_t outSize;

	//mbstowcs_s(&outSize, text_wchar, buf, 4096);

	strcpy(buf, "Resume");	// or pass &s[0]

	//va_list l;
	//va_start(l, buf);
	//vsprintf(buf, buf, l);

#ifdef _MAX_EXPORT
	EConsole.print(mt,buf);
#endif
	::LogExecCB = FALSE;
    ::Msg		(buf);
	::LogExecCB	= TRUE;
}
//----------------------------------------------------