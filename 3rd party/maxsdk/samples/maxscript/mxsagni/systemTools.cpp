#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include "MXSAgni.h"

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

using namespace MAXScript;

// ============================================================================

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include "systemTools_wraps.h"
// -----------------------------------------------------------------------------------------


Value* systemTools_IsDebugging_cf (Value** arg_list, int count)
{
	check_arg_count (IsDebugging, 0, count);
	return bool_result ( IsDebugging() );
}

Value* systemTools_DebugPrint_cf(Value** arg_list, int count)
{
	check_arg_count(DebugPrint, 1, count);
	const MCHAR* message = arg_list[0]->to_string();
	DebugPrint(_T("%s\n"), message);
	return &ok;
}

Value* systemTools_NumberOfProcessors_cf (Value** arg_list, int count)
{
	check_arg_count (NumberOfProcessors, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern (NumberOfProcessors()) );
}

Value* systemTools_GetScreenWidth_cf (Value** arg_list, int count)
{
	// <int> systemTools.GetScreenWidth removeUIScaling:<true>
	check_arg_count_with_keys (GetScreenWidth, 0, count);
	Value* tmp;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, tmp, TRUE); 
	int width = GetScreenWidth();
	if (removeUIScaling)
		width = GetValueUIUnscaled(width);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern (width) );
}

Value* systemTools_GetScreenHeight_cf (Value** arg_list, int count)
{
	// <int> systemTools.GetScreenHeight removeUIScaling:<true>
	check_arg_count_with_keys (GetScreenHeight, 0, count);
	Value* tmp;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, tmp, TRUE); 
	int height = GetScreenHeight();
	if (removeUIScaling)
		height = GetValueUIUnscaled(height);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern (height) );
}

Value* systemTools_IsAeroEnabled_cf (Value** arg_list, int count)
{
	check_arg_count (IsAeroEnabled, 0, count);
	return bool_result ( IsVistaAeroEnabled() );
}

Value*
	systemTools_getEnvVar_cf(Value** arg_list, int count)
{
	// systemTools.getEnvVariable <string>
	check_arg_count(systemTools.getEnvVariable, 1, count);
	const TCHAR *varName = arg_list[0]->to_string();
	DWORD bufSize = GetEnvironmentVariable(varName,NULL,0);
	if (bufSize == 0) return &undefined;
	TSTR varVal;
	varVal.Resize(bufSize+1);
	GetEnvironmentVariable(varName,varVal.dataForWrite(),bufSize+1);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( new String(varVal) );
}

Value*
	systemTools_setEnvVar_cf(Value** arg_list, int count)
{
	// systemTools.setEnvVariable <string> <string>
	check_arg_count(systemTools.setEnvVariable, 2, count);
	const TCHAR *varName = arg_list[0]->to_string();
	const TCHAR *varVal = NULL;
	if (arg_list[1] != &undefined)
		varVal = arg_list[1]->to_string();
	BOOL res = SetEnvironmentVariable(varName,varVal);
	return bool_result(res);
}

Value*
	systemTools_getSystemMetrics_cf(Value** arg_list, int count)
{
	// systemTools.getSystemMetrics <int>
	check_arg_count(systemTools.getSystemMetrics, 1, count);
	int which = arg_list[0]->to_int();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern(GetSystemMetrics(which)) );
}

Value*
	systemTools_getmaxstdio_cf(Value** arg_list, int count)
{
	// <int> systemTools.getmaxstdio()
	// Returns the number of simultaneously open files permitted at the stdio level (i.e. via fopen).
	check_arg_count(systemTools.getmaxstdio, 0, count);
	int maxstdio = _getmaxstdio();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls ( Integer::intern(maxstdio) );
}

Value*
	systemTools_setmaxstdio_cf(Value** arg_list, int count)
{
	// <int> systemTools.setmaxstdio <int newmax>
	// Sets a maximum for the number of simultaneously open files at the stdio level (i.e. via fopen).
	// The valid range for newmax is 512 to 2048.
	// Returns new maximum if successful; –1 otherwise. Fails if reducing max and have active file handle 
	// in one of the slots above newmax, or if memory allocation occurring when increasing maximum fails.
	// Note: due to a defect in the c runtime, it is currently unsafe to decrease the maximum. Attempts 
	// to decrease the maximum via this method will be a no-op, and -1 is returned.
	check_arg_count(systemTools.setmaxstdio, 1, count);
	int  new_maxstdio= arg_list[0]->to_int();
	range_check(new_maxstdio, 512, 2048, _T("systemTools.setmaxstdio arg"));
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	// because of a c runtime bug, it is unsafe to decrease the max allowed handles.
	int old_maxstdio = _getmaxstdio();
	if (old_maxstdio > new_maxstdio)
		return_value_tls(Integer::intern(-1));
	int maxstdio = _setmaxstdio(new_maxstdio);
	return_value_tls ( Integer::intern(maxstdio) );
}


Value* EnumDisplayDevices_cf(Value** arg_list, int count)
{
	// systemTools.EnumDisplayDevices  removeUIScaling:<true>
	check_arg_count_with_keys(EnumDisplayDevices, 0, count);
	Value* tmp;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, tmp, TRUE);
	DISPLAY_DEVICE device;
	device.cb = sizeof(DISPLAY_DEVICE);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	two_typed_value_locals_tls(Array* result, Array* subResult);
	vl.result = new Array(0);
	for (int i = 0; ; i++)
	{
		DWORD dwFlags = EDD_GET_DEVICE_INTERFACE_NAME;
		if (!::EnumDisplayDevices(NULL, i, &device, 0))
			break;
		vl.subResult = new Array(0);
		vl.subResult->append(new String(device.DeviceName));
		vl.subResult->append(new String(device.DeviceString));
		vl.subResult->append(Integer::intern(device.StateFlags));
		vl.subResult->append(new String(device.DeviceID));
		vl.subResult->append(new String(device.DeviceKey));
		vl.result->append(vl.subResult);
		// If it's not connected to the desktop, then ignore it.
		if ((device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
			continue;
		// Get the display mode settings of this device.
		DEVMODE devMode;
		if (!::EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &devMode))
			continue;
		if (removeUIScaling)
		{
			devMode.dmPosition.x = GetValueUIUnscaled(devMode.dmPosition.x);
			devMode.dmPosition.y = GetValueUIUnscaled(devMode.dmPosition.y);
			devMode.dmPelsWidth = GetValueUIUnscaled(devMode.dmPelsWidth);
			devMode.dmPelsHeight = GetValueUIUnscaled(devMode.dmPelsHeight);
		}
		vl.subResult->append(Integer::intern(devMode.dmPosition.x));
		vl.subResult->append(Integer::intern(devMode.dmPosition.y));
		vl.subResult->append(Integer::intern(devMode.dmPelsWidth));
		vl.subResult->append(Integer::intern(devMode.dmPelsHeight));
	}
	return_value_tls(vl.result);
}
