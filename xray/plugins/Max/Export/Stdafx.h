//----------------------------------------------------
// file: stdafx.h
//----------------------------------------------------
#ifndef __INCDEF_STDAFX_H_
#define __INCDEF_STDAFX_H_

#pragma once  

#pragma warning(push)
#pragma warning (disable:4995)
#include "maxtypes.h"
#include "max.h"


#include "../../../xrCore/xrCore.h"

#define _BCL

#undef _MIN
#undef _MAX
#define _MIN(a,b)		(a)<(b)?(a):(b)
#define _MAX(a,b)		(a)>(b)?(a):(b)
template <class T>
T min(T a, T b) { return _MIN(a,b); }
template <class T>
T max(T a, T b) { return _MAX(a,b); }
using std::string;
#undef _MIN
#undef _MAX

#define FLT_MAX flt_max

#ifdef FLT_MIN
#undef FLT_MIN
#endif

#define FLT_MIN flt_max

#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <sys\utime.h>

#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "stdmat.h"
#include "UTILAPI.H"

// CS SDK
#ifdef _MAX_EXPORT
#	include "phyexp.h"
#	include "bipexp.h"
#endif

#include <d3d9types.h>

#define ENGINE_API
#define ECORE_API

enum TMsgDlgType { mtWarning, mtError, mtInformation, mtConfirmation, mtCustom };
enum TMsgDlgBtn { mbYes, mbNo, mbOK, mbCancel, mbAbort, mbRetry, mbIgnore, mbAll, mbNoToAll, mbYesToAll, mbHelp };
typedef TMsgDlgBtn TMsgDlgButtons[mbHelp];

#include <string>
#include <codecvt>

#define AnsiString string
DEFINE_VECTOR(AnsiString,AStringVec,AStringIt);

//#include "clsid.h"
//#include "Engine.h"
//#include "Properties.h"
#include "..\..\Shared\ELog.h"

#define THROW R_ASSERT(0)

#ifdef _MAX_EXPORT
	#define _EDITOR_FILE_NAME_ "max_export"
#else
	#ifdef _MAX_MATERIAL
		#define _EDITOR_FILE_NAME_ "max_material"
	#endif
#endif

#define GAMEMTL_NONE		u32(-1)
#define _game_data_ "$game_data$"

#pragma warning (default:4995)

#endif /*_INCDEF_STDAFX_H_*/

static IC LPCSTR StringFromUTF8(const wchar_t* in)
{
	std::wstring wstr = in;
	static std::locale locale("");
	xr_string result(wstr.size(), '\0');
	std::use_facet<std::ctype<wchar_t>>(locale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
	return result.c_str();
}

static IC string StringFromUTF8(const char* in)
{
	static std::locale locale("");
	using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
	auto wstr = wcvt{}.from_bytes(in);
	string result(wstr.size(), '\0');
	std::use_facet<std::ctype<wchar_t>>(locale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
	return result;
}

static IC string StringFromUTF8(const char* in, const std::locale& locale)
{
	using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
	auto wstr = wcvt{}.from_bytes(in);
	string result(wstr.size(), '\0');
	std::use_facet<std::ctype<wchar_t>>(locale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
	return result;
}

static IC string StringFromUTF8_convert(const wchar_t* in)
{
	std::wstring wstr = in;
	static std::locale locale("");
	string result(wstr.size(), '\0');
	std::use_facet<std::ctype<wchar_t>>(locale).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
	return result;
}

static IC xr_string StringToUTF8(const char* in, const std::locale& locale)
{
	using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
	std::wstring wstr(xr_strlen(in), L'\0');
	std::use_facet<std::ctype<wchar_t>>(locale).widen(in, in + xr_strlen(in), &wstr[0]);
	std::string result = wcvt{}.to_bytes(wstr.data(), wstr.data() + wstr.size());
	return result.data();
}

static IC std::string UTF8_to_CP1251(std::string const& utf8)
{
	if (!utf8.empty())
	{
		int wchlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), NULL, 0);
		if (wchlen > 0 && wchlen != 0xFFFD)
		{
			std::vector<wchar_t> wbuf(wchlen);
			MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), &wbuf[0], wchlen);
			std::vector<char> buf(wchlen);
			WideCharToMultiByte(1251, 0, &wbuf[0], wchlen, &buf[0], wchlen, 0, 0);

			return std::string(&buf[0], wchlen);
		}
	}
	return std::string();
}