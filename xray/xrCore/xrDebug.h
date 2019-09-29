#ifndef xrDebugH
#define xrDebugH
#pragma once

typedef	void		crashhandler		(void);
typedef	void		on_dialog			(bool before);

class XRCORE_API	xrDebug
{
private:
	crashhandler*	handler	;
	on_dialog*		m_on_dialog;

public:
	void			_initialize			(const bool &dedicated);
	void			_destroy			();
	
public:
	crashhandler*	get_crashhandler	()							{ return handler;	};
	void			set_crashhandler	(crashhandler* _handler)	{ handler=_handler;	};

	on_dialog*		get_on_dialog		()							{ return m_on_dialog;	}
	void			set_on_dialog		(on_dialog* on_dialog)		{ m_on_dialog = on_dialog;	}

	LPCTSTR			error2string		(long  code	);

	void			gather_info			(LPCTSTR expression, LPCTSTR description, LPCTSTR argument0, LPCTSTR argument1, LPCTSTR file, int line, LPCTSTR function, LPTSTR assertion_info);
	void			fail				(LPCTSTR e1, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	#ifdef UNICODE
	void			fail				(LPCTSTR e1, const std::wstring &e2, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	#else
    void fail(LPCTSTR e1, const std::string& e2, LPCTSTR file, int line, LPCTSTR function, bool& ignore_always);
	#endif
	void			fail				(LPCTSTR e1, LPCTSTR e2, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	void			fail				(LPCTSTR e1, LPCTSTR e2, LPCTSTR e3, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	void			fail				(LPCTSTR e1, LPCTSTR e2, LPCTSTR e3, LPCTSTR e4, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	void			error				(long  code, LPCTSTR  e1, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	void			error				(long  code, LPCTSTR  e1, LPCTSTR  e2, LPCTSTR file, int line, LPCTSTR function, bool &ignore_always);
	void _cdecl		fatal				(LPCTSTR file, int line, LPCTSTR function, LPCTSTR  F,...);
	void			backend				(LPCTSTR  reason, LPCTSTR  expression, LPCTSTR argument0, LPCTSTR argument1, LPCTSTR  file, int line, LPCTSTR function, bool &ignore_always);
	void			do_exit				(const std::wstring &message);
};

// warning
// this function can be used for debug purposes only
IC	std::wstring __cdecl	make_string		(LPCTSTR format,...)
{
	va_list		args;
	va_start	(args,format);

	TCHAR		temp[4096];
	wsprintf(temp,format,args);
	
	return		std::wstring(temp);
}

extern XRCORE_API	xrDebug		Debug;

XRCORE_API void LogStackTrace	(LPCTSTR header);

#include "xrDebug_macros.h"

#endif // xrDebugH