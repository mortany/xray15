#include "VertexPaint.h"


HINSTANCE hInstance;


BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
	{
      if( fdwReason == DLL_PROCESS_ATTACH )
      {
         MaxSDK::Util::UseLanguagePackLocale();
         hInstance = hinstDLL;				// Hang on to this DLL's instance handle.
         DisableThreadLibraryCalls(hInstance);
      }
			
	return (TRUE);
	}

__declspec( dllexport ) const TCHAR* LibDescription()
	{
	return GetString(IDS_LIBDESCRIPTION);
	}


__declspec( dllexport ) int LibNumberClasses()
	{
	return 1;
	}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
	{
	switch(i) {
		case 0: return GetVertexPaintDesc();
		default: return 0;
		}
	}

__declspec( dllexport ) ULONG LibVersion()
	{
	return VERSION_3DSMAX;
	}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

TCHAR *GetString(int id)
	{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
	}

