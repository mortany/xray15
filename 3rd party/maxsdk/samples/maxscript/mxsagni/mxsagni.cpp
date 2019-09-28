/**********************************************************************
 *<
	FILE: MXSAgni.cpp

	DESCRIPTION: Get/Set functions for system globals defined in MXSAgni

	CREATED BY: Ravi Karra, 1998

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

 //#include "pch.h"
#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxscript/foundation/strings.h>
#include <Util/FileMutexObject.h>

#define NO_INIUTIL_USING
#include <Util\IniUtil.h> // MaxSDK::Util::WritePrivateProfileString

#include "randgenerator.h"

#include "resource.h"

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#include "MXSAgni.h"
#include "ExtClass.h"

//#include <maxscript\macros\define_external_functions.h>
#include <maxscript\macros\define_instantiation_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include "ExtFuncs.h"
#include "AssetManagement/iassetmanager.h"
#include "IViewPanelManager.h"
#include "IViewPanel.h"

#include "agnidefs.h"

using namespace MaxSDK::AssetManagement;

Value*
get_MAX_version_cf(Value** arg_list, int count)
{
	check_arg_count(MAXVersion, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	TSTR buildNumberString = MaxSDK::Util::GetMaxBuildNumber();
	int majorVersion = 0, updateVersion = 0, hotFix = 0, buildNumber = 0;
	DbgAssert(buildNumberString);
	if (!buildNumberString.isNull())
		_stscanf(buildNumberString, _T("%d.%d.%d.%d"), &majorVersion, &updateVersion, &hotFix, &buildNumber);
	vl.result = new Array(9);
	vl.result->append(Integer::intern(MAX_RELEASE));
	vl.result->append(Integer::intern(MAX_API_NUM));
	vl.result->append(Integer::intern(MAX_SDK_REV));
	vl.result->append(Integer::intern(majorVersion));
	vl.result->append(Integer::intern(updateVersion));
	vl.result->append(Integer::intern(hotFix));
	vl.result->append(Integer::intern(buildNumber));
	vl.result->append(Integer::intern(MAX_PRODUCT_YEAR_NUMBER));
	vl.result->append(new String(_T(MAX_PRODUCT_VERSION_DESC)));
	return_value_tls(vl.result);
}

Value*
get_MAX_ext_version_cf(Value** arg_list, int count)
{
	check_arg_count(getMaxExtensionVersion, 0, count);
	int extensionNumber = GetMaxExtensionVersion();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(Integer::intern(extensionNumber));
}

Value*
get_root_node()
{
	return_value (MAXRootNode::intern(MAXScript_interface->GetRootNode()));
}

Value*
set_root_node(Value* val)
{
	throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_ROOTNODE_IS_READ_ONLY));
	return &undefined;
}

Value*
set_rend_output_filename(Value* val)
{
	TSTR fileName = val->to_filename();
	AssetId assetId;
	if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
		fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

	if (fileName.Length() == 0)
		MAXScript_interface->SetRendSaveFile(FALSE);
	BitmapInfo& bi = MAXScript_interface->GetRendFileBI();

	bi.SetName(fileName);
	// LAM - 8/4/04 - defects 483429, 493861 - reload PiData
	bi.SetDevice(_T(""));

	bi.ResetPiData();
	if (fileName.Length() != 0)
	{
		if (!TheManager->ioList.Listed())
			TheManager->ListIO();
		int idx = TheManager->ioList.ResolveDevice(&bi);
		if (idx != -1)
		{
			BitmapIO *IO = TheManager->ioList.CreateDevInstance(idx);
			if (IO) {
				DWORD pisize = IO->EvaluateConfigure();
				if (pisize) {
					bi.AllocPiData(pisize);
					IO->ReadCfg();
					IO->SaveConfigure(bi.GetPiData());
				}
				delete IO;
			}
		}
	}
	return val;
}

Value*
get_rend_output_filename()
{
	BitmapInfo& bi = MAXScript_interface->GetRendFileBI();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(new String(bi.Name()));
}

Value*
get_rend_useActiveView()
{
	return MAXScript_interface11->GetRendUseActiveView() ?
		&true_value : &false_value;
}

Value*
set_rend_useActiveView(Value* val)
{
	MAXScript_interface11->SetRendUseActiveView(val->to_bool());
	return val;
}

Value*
get_rend_viewIndex()
{
	// if the rendViewId represents a viewport of the current active view panel
	// return the corresponding index, otherwise just return undefined.
	int id = MAXScript_interface14->GetRendViewID();
	IViewPanel* pPanel = GetViewPanelManager()->GetActiveViewPanel();
	if (pPanel)
	{
		const int MaxVptPerViewPanel = 4;
		for (int i = 0; i < MaxVptPerViewPanel; ++i)
		{
			ViewExp& vpt = pPanel->GetViewExpByIndex(i);
			if (vpt.IsAlive())
			{
				if (vpt.GetViewID() == id)
				{
					return_value (Integer::intern(i + 1)); // 0 based to 1 based.
				}
			}
		}
	}
	return &undefined;
}

Value*
set_rend_viewIndex(Value* val)
{
	int index = val->to_int() - 1; // 1 based to 0 based
	// convert the viewport index to viewID and set to rendViewID
	IViewPanel* pPanel = GetViewPanelManager()->GetActiveViewPanel();

	if (pPanel)
	{
		ViewExp& vpt = pPanel->GetViewExpByIndex(index);
		if (vpt.IsAlive())
		{
			MAXScript_interface14->SetRendViewID(vpt.GetViewID());
		}
	}
	return val;
}

Value*
get_rend_viewID()
{
	return_value (Integer::intern(MAXScript_interface14->GetRendViewID() + 1)); // Convert 0-based to 1-based
}

Value*
set_rend_viewID(Value* val)
{
	MAXScript_interface14->SetRendViewID(val->to_int() - 1); // Convert 1-based to 0-based
	return val;
}

Value*
get_rend_camNode()
{
	return_value (MAXRootNode::intern(MAXScript_interface8->GetRendCamNode()));
}

Value*
set_rend_camNode(Value* val)
{
	if (val == &undefined)
		MAXScript_interface8->SetRendCamNode(NULL);
	else MAXScript_interface8->SetRendCamNode(val->to_node());
	return val;
}

Value*
set_rend_useImgSeq(Value* val)
{
	MAXScript_interface8->SetRendUseImgSeq(val->to_bool());
	return val;
}

Value*
get_rend_useImgSeq()
{
	return MAXScript_interface8->GetRendUseImgSeq() ?
		&true_value : &false_value;
}

Value*
set_rend_imgSeqType(Value* val)
{
	MAXScript_interface8->SetRendImgSeqType(val->to_int());
	return val;
}

Value*
get_rend_imgSeqType()
{
	return_value (Integer::intern(MAXScript_interface8->GetRendImgSeqType()));
}

Value*
set_rend_preRendScript(Value* val)
{
	AssetUser asset = IAssetManager::GetInstance()->GetAsset(val->to_filename(), kPreRenderScript);
	MAXScript_interface8->SetPreRendScriptAsset(asset);
	return val;
}

Value*
get_rend_preRendScript()
{
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(new String(MAXScript_interface8->GetPreRendScriptAsset().GetFileName()));
}

Value*
set_rend_usePreRendScript(Value* val)
{
	MAXScript_interface8->SetUsePreRendScript(val->to_bool());
	return val;
}

Value*
get_rend_usePreRendScript()
{
	return MAXScript_interface8->GetUsePreRendScript() ? &true_value : &false_value;
}

Value*
set_rend_localPreRendScript(Value* val)
{
	MAXScript_interface8->SetLocalPreRendScript(val->to_bool());
	return val;
}

Value*
get_rend_localPreRendScript()
{
	return MAXScript_interface8->GetLocalPreRendScript() ? &true_value : &false_value;
}

Value*
set_rend_postRendScript(Value* val)
{
	AssetUser asset = IAssetManager::GetInstance()->GetAsset(val->to_filename(), kPreRenderScript);
	MAXScript_interface8->SetPostRendScriptAsset(asset);
	return val;
}

Value*
get_rend_postRendScript()
{
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(new String(MAXScript_interface8->GetPostRendScriptAsset().GetFileName()));
}

Value*
set_rend_usePostRendScript(Value* val)
{
	MAXScript_interface8->SetUsePostRendScript(val->to_bool());
	return val;
}

Value*
get_rend_usePostRendScript()
{
	return MAXScript_interface8->GetUsePostRendScript() ? &true_value : &false_value;
}

Value*
get_renderPresetMRUList()
{
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	two_typed_value_locals_tls(Array* result, Array* entry);
	int count = MAXScript_interface11->GetRenderPresetMRUListCount();
	vl.result = new Array(count);

	for (int i = 0; i < count; i++)
	{
		const TCHAR* displayName = MAXScript_interface11->GetRenderPresetMRUListDisplayName(i);
		const TCHAR* filename = MAXScript_interface11->GetRenderPresetMRUListFileName(i);
		if (displayName != nullptr && displayName[0] != _T('\0'))
		{
			vl.entry = new Array(2);
			vl.entry->append(new String(TSTR(displayName)));
			vl.entry->append(new String(TSTR(filename)));
			vl.result->append(vl.entry);
		}
	}

	return_value_tls(vl.result);
}

Value*
set_rend_useIterative(Value* val)
{
	// JOHNSON RELEASE SDK
	//MAXScript_interface11->SetRendUseIterative(val->to_bool());
	return val;
}

Value*
get_rend_useIterative()
{
	// JOHNSON RELEASE SDK
	//return MAXScript_interface11->GetRendUseIterative() ? &true_value : &false_value;
	return &undefined;
}



Value*
set_real_time_playback(Value* val)
{
	MAXScript_interface->SetRealTimePlayback(val->to_bool());
	return val;
}

Value*
get_real_time_playback()
{
	return MAXScript_interface->GetRealTimePlayback() ?
		&true_value : &false_value;
}

Value*
set_playback_loop(Value* val)
{
	MAXScript_interface->SetPlaybackLoop(val->to_bool());
	return val;
}

Value*
get_playback_loop()
{
	return MAXScript_interface->GetPlaybackLoop() ?
		&true_value : &false_value;
}

Value*
set_play_active_only(Value* val)
{
	MAXScript_interface->SetPlayActiveOnly(val->to_bool());
	return val;
}

Value*
get_play_active_only()
{
	return MAXScript_interface->GetPlayActiveOnly() ?
		&true_value : &false_value;
}

Value*
set_enable_animate_button(Value* val)
{
	MAXScript_interface->EnableAnimateButton(val->to_bool());
	return val;
}

Value*
get_enable_animate_button()
{
	return MAXScript_interface->IsAnimateEnabled() ?
		&true_value : &false_value;
}

Value*
set_animate_button_state(Value* val)
{
	MAXScript_interface->SetAnimateButtonState(val->to_bool());
	return val;
}

Value*
get_animate_button_state()
{
	return Animating() ? &true_value : &false_value;
}

Value*
set_fly_off_time(Value* val)
{
	MAXScript_interface->SetFlyOffTime(val->to_int());
	return val;
}

Value*
get_fly_off_time()
{
	return_value (Integer::intern(MAXScript_interface->GetFlyOffTime()));
}

Value*
set_xform_gizmos(Value *val)
{
	MAXScript_interface->SetUseTransformGizmo(val->to_bool());
	return val;
}

Value*
get_xform_gizmos()
{
	return MAXScript_interface->GetUseTransformGizmo() ? &true_value : &false_value;
}


Value*
set_constant_axis(Value *val)
{
	MAXScript_interface->SetConstantAxisRestriction(val->to_bool());
	return val;
}

Value*
get_constant_axis()
{
	return MAXScript_interface->GetConstantAxisRestriction() ? &true_value : &false_value;
}

Value*
set_DontRepeatRefMsg(Value *val)
{
	SetDontRepeatRefMsg(val->to_bool() == TRUE, false);
	return val;
}

Value*
get_DontRepeatRefMsg()
{
	return DontRepeatRefMsg() ? &true_value : &false_value;
}

Value*
set_InvalidateTMOpt(Value *val)
{
	SetInvalidateTMOpt(val->to_bool() == TRUE, false);
	return val;
}

Value*
get_InvalidateTMOpt()
{
	return GetInvalidateTMOpt() ? &true_value : &false_value;
}

Value* set_autoKeyDefaultKeyOn(Value *val)
{
	GetCOREInterface10()->SetAutoKeyDefaultKeyOn(val->to_bool());
	return val;
}

Value* get_autoKeyDefaultKeyOn()
{
	return GetCOREInterface10()->GetAutoKeyDefaultKeyOn() ? &true_value : &false_value;
}


Value* set_autoKeyDefaultKeyTime(Value *val)
{
	GetCOREInterface10()->SetAutoKeyDefaultKeyTime(val->to_int()*GetTicksPerFrame());
	return val;
}

Value* get_autoKeyDefaultKeyTime()
{
	return_value (Integer::intern(GetCOREInterface10()->GetAutoKeyDefaultKeyTime() / GetTicksPerFrame()));
}

Value*
set_EnableOptimizeDependentNotifications(Value *val)
{
	SetEnableOptimizeDependentNotifications(val->to_bool() == TRUE, false);
	return val;
}

Value*
get_EnableOptimizeDependentNotifications()
{
	return GetEnableOptimizeDependentNotifications() ? &true_value : &false_value;
}

Value*
set_EnableTMCache(Value *val)
{
	GetCOREInterface17()->SetEnableTMCache(val->to_bool()==TRUE,false);
	return val;
}

Value*
get_EnableTMCache()
{
	return GetCOREInterface17()->GetEnableTMCache() ? &true_value : &false_value;
}

Value*
set_ControllerIntervalOptimization(Value *val)
{
	SetControllerIntervalOptimization(val->to_bool() == TRUE, false);
	return val;
}

Value*
get_ControllerIntervalOptimization()
{
	return GetControllerIntervalOptimization() ? &true_value : &false_value;
}

// mjm - 3.1.99 - use spinner precision for edit boxes linked to slider controls
/*
Value*
set_slider_precision(Value *val)
{
	SetSliderPrecision (val->to_int());
	return val;
}

Value*
get_slider_precision()
{
	return_value (Integer::intern(GetSliderPrecision()));
}
*/

Value*
get_useVertexDots()
{
	return getUseVertexDots() ? &true_value : &false_value;
}


Value*
set_useVertexDots(Value *val)
{
	setUseVertexDots((val->to_bool()) ? 1 : 0);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return val;
}

Value*
get_vertexDotType()
{
	return getVertexDotType() ? &true_value : &false_value;
}


Value*
set_vertexDotType(Value *val)
{
	setVertexDotType((val->to_bool()) ? 1 : 0);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	needs_redraw_set(_tls);
	return val;
}


Value*
get_useTrackBar()
{
	return MAXScript_interface->GetKeyStepsUseTrackBar() ? &true_value : &false_value;
}


Value*
set_useTrackBar(Value *val)
{
	MAXScript_interface->SetKeyStepsUseTrackBar(val->to_bool());
	return val;
}


Value*
get_renderDisplacements()
{
	return MAXScript_interface->GetRendDisplacement() ? &true_value : &false_value;
}


Value*
set_renderDisplacements(Value *val)
{
	MAXScript_interface->SetRendDisplacement(val->to_bool());
	return val;
}


Value*
get_renderEffects()
{
	return MAXScript_interface->GetRendEffects() ? &true_value : &false_value;
}


Value*
set_renderEffects(Value *val)
{
	MAXScript_interface->SetRendEffects(val->to_bool());
	return val;
}


Value*
get_showEndResult()
{
	return MAXScript_interface->GetShowEndResult() ? &true_value : &false_value;
}


Value*
set_showEndResult(Value *val)
{
	MAXScript_interface->SetShowEndResult(val->to_bool());
	return val;
}


Value*
get_skipRenderedFrames()
{
	return MAXScript_interface->GetSkipRenderedFrames() ? &true_value : &false_value;
}


Value*
set_skipRenderedFrames(Value *val)
{
	MAXScript_interface->SetSkipRenderedFrames(val->to_bool());
	return val;
}

Value*
get_spinner_wrap()
{
	return GetSpinnerWrap() ? &true_value : &false_value;
}

Value*
set_spinner_wrap(Value *val)
{
	SetSpinnerWrap(val->to_bool() ? 1 : 0);
	return val;
}

Value*
get_spinner_precision()
{
	return_value (Integer::intern(GetSpinnerPrecision()));
}

Value*
set_spinner_precision(Value *val)
{
	SetSpinnerPrecision(val->to_int());
	return val;
}

Value*
get_spinner_snap()
{
	return_value (Float::intern(GetSnapSpinValue()));
}

Value*
set_spinner_snap(Value *val)
{
	SetSnapSpinValue(val->to_float());
	return val;
}

Value*
get_spinner_useSnap()
{
	return GetSnapSpinner() ? &true_value : &false_value;
}

Value*
set_spinner_useSnap(Value *val)
{
	SetSnapSpinner(val->to_bool() ? 1 : 0);
	return val;
}

Value*
get_logsys_quiet_mode()
{
	LogSys* thelog = MAXScript_interface->Log();
	return (thelog->GetQuietMode() ? &true_value : &false_value);
}

Value*
set_logsys_quiet_mode(Value *val)
{
	LogSys* thelog = MAXScript_interface->Log();
	thelog->SetQuietMode(val->to_bool() ? true : false);
	return val;
}

Value*
get_logsys_longevity()
{
	def_logFileLongevity_types();
	LogSys* thelog = MAXScript_interface->Log();
	return (ConvertIDToValue(logFileLongevity, elements(logFileLongevity), thelog->Longevity()));
}

Value*
set_logsys_longevity(Value *val)
{
	def_logFileLongevity_types();
	LogSys* thelog = MAXScript_interface->Log();
	int longevity = ConvertValueToID(logFileLongevity, elements(logFileLongevity), val);
	thelog->SetLongevity(longevity);
	return val;
}

Value*
get_logsys_logDays()
{
	LogSys* thelog = MAXScript_interface->Log();
	return_value (Integer::intern(thelog->LogDays()));
}

Value*
set_logsys_logDays(Value *val)
{
	LogSys* thelog = MAXScript_interface->Log();
	thelog->SetLogDays(val->to_int());
	return val;
}

Value*
get_logsys_logSize()
{
	LogSys* thelog = MAXScript_interface->Log();
	return_value (Integer::intern(thelog->LogSize()));
}

Value*
set_logsys_logSize(Value *val)
{
	LogSys* thelog = MAXScript_interface->Log();
	thelog->SetLogSize(val->to_int());
	return val;
}

Value*
get_logsys_enabled_mode()
{
	LogSys* thelog = MAXScript_interface->Log();
	return (thelog->GetEnabledMode() ? &true_value : &false_value);
}

Value*
set_logsys_enabled_mode(Value *val)
{
	LogSys* thelog = MAXScript_interface->Log();
	thelog->SetEnabledMode(val->to_bool() ? true : false);
	return val;
}

Value*
get_maxGBufferLayers()
{
	return_value (Integer::intern(GetMaximumGBufferLayerDepth()));
}

Value*
set_maxGBufferLayers(Value *val)
{
	SetMaximumGBufferLayerDepth(val->to_int());
	return val;
}

Value*
get_playPreviewWhenDone()
{
	return MAXScript_interface19->GetPlayPreviewWhenDone() ? &true_value : &false_value;
}

Value*
set_playPreviewWhenDone(Value *val)
{
	MAXScript_interface19->SetPlayPreviewWhenDone(val->to_bool());
	return val;
}


//mcr_global_imp

// called in Parser::setup(), MXSAgni system globals & all name interns 
// are registered here
void MXSAgni_init1()
{
#	include "ExtGlbls.h"
#include <maxscript\macros\define_implementations.h>
#	include "namedefs.h"
}

Value*
get_INI_setting_cf(Value** arg_list, int count)
{
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	if (count == 1 || count == 2) // get sections or section keys
	{
		TSTR  fileName = arg_list[0]->to_filename();
		AssetId assetId;
		if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
			fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

		const TCHAR* section = (count == 2) ? arg_list[1]->to_string() : nullptr;

		MaxSDK::Array<TCHAR> buffer;
		DWORD charCount = MaxSDK::Util::GetPrivateProfileString(
			section, 
			nullptr,
			_T(""),
			buffer,
			fileName);

		one_typed_value_local_tls(Array* result);
		const TCHAR *bufferStartPtr = buffer.asArrayPtr();
		const TCHAR *bufferPtr = bufferStartPtr;
		vl.result = new Array(0);
		//compare with characters read to account for a possible empty section name
		while ((bufferPtr - bufferStartPtr) < charCount) {
			vl.result->append(new String(bufferPtr));
			while (*bufferPtr++);
		}
		return_value_tls(vl.result);
	}
	else
	{
		check_arg_count(getIniSetting, 3, count);
		TSTR  fileName = arg_list[0]->to_filename();
		AssetId assetId;
		if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
			fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

		const TCHAR* section = arg_list[1]->to_string();
		const TCHAR* key = arg_list[2]->to_string();

		MaxSDK::Array<TCHAR> buffer;
		MaxSDK::Util::GetPrivateProfileString(
			section,
			key,
			_T(""), buffer,
			fileName);

		return_value_tls(new String(buffer.asArrayPtr()));
	}
}

Value*
del_INI_setting_cf(Value** arg_list, int count)
{
	BOOL res = FALSE;
	if (count == 2 || count == 3) // delete section or section key
	{
		TSTR  fileName = arg_list[0]->to_filename();
		AssetId assetId;
		if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
			fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

		res = MaxSDK::Util::WritePrivateProfileString(
			arg_list[1]->to_string(),
			(count == 3) ? arg_list[2]->to_string() : NULL,
			NULL,
			fileName);
	}
	else
		check_arg_count(delIniSetting, 3, count);
	return bool_result(res);
}

BOOL forceUTF16_default = TRUE;

Value*
set_INI_force_utf16_default_cf(Value** arg_list, int count)
{
	BOOL old_value = forceUTF16_default;
	check_arg_count(setIniForceUTF16Default, 1, count);
	forceUTF16_default = arg_list[0]->to_bool();
	return bool_result(old_value);
}

Value*
set_INI_setting_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setINISetting, 4, count);

	TSTR  fileName = arg_list[0]->to_filename();
	AssetId assetId;
	if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
		fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

	Value *v;
	BOOL forceutf16 = bool_key_arg(forceutf16, v, forceUTF16_default);

	if (forceutf16)
	{
		return MaxSDK::Util::WritePrivateProfileString(
			arg_list[1]->to_string(),
			arg_list[2]->to_string(),
			arg_list[3]->to_string(),
			fileName) ? &true_value : &false_value;
	}

	MaxSDK::Util::FileMutexObject fileMutexObject(fileName);
	BOOL res = ::WritePrivateProfileString(
		arg_list[1]->to_string(),
		arg_list[2]->to_string(),
		arg_list[3]->to_string(),
		fileName);
	return bool_result(res);
}

Value*
has_INI_setting_cf(Value** arg_list, int count)
{
	TSTR val;
	if (count == 2 || count == 3) // get sections or section keys
	{
		TSTR  fileName = arg_list[0]->to_filename();
		AssetId assetId;
		if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
			fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

		MaxSDK::Array<TCHAR> buffer;
		DWORD buffCount = MaxSDK::Util::GetPrivateProfileString(
			(count == 3) ? arg_list[1]->to_string() : NULL,
			NULL,
			_T(""), buffer,
			fileName);

		const TCHAR* bufferStartPtr = buffer.asArrayPtr();
		const TCHAR* bufferPtr = bufferStartPtr;
		const TCHAR *key = arg_list[count - 1]->to_string();
		//compare with characters read to account for a possible empty section name
		while ((bufferPtr - bufferStartPtr) < buffCount) {
			if (_tcsicmp(bufferPtr, key) == 0)
				return &true_value;
			while (*bufferPtr++);
		}
	}
	else
		check_arg_count(hasINISetting, 2, count);
	return &false_value;
}

Value*
get_file_version_cf(Value** arg_list, int count)
{
	check_arg_count(getFileVersion, 1, count);

	DWORD	tmp;
	TSTR	fileName = arg_list[0]->to_filename();
	AssetId assetId;
	if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
		fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

	LPCTSTR	file = fileName;
	DWORD	size = GetFileVersionInfoSize(file, &tmp);
	if (!size) return &undefined;

	BYTE*	data = (BYTE*)malloc(size);
	if (data && GetFileVersionInfo(file, NULL, size, data))
	{
		UINT len;
		VS_FIXEDFILEINFO *qbuf;
		TCHAR buf[256];
		if (VerQueryValue(data, _T("\\"), (void**)&qbuf, &len))
		{
			DWORD fms = qbuf->dwFileVersionMS;
			DWORD fls = qbuf->dwFileVersionLS;
			DWORD pms = qbuf->dwProductVersionMS;
			DWORD pls = qbuf->dwProductVersionLS;

			free(data);
			_stprintf(buf, _T("%i,%i,%i,%i\t\t%i,%i,%i,%i"),
				HIWORD(pms), LOWORD(pms), HIWORD(pls), LOWORD(pls),
				HIWORD(fms), LOWORD(fms), HIWORD(fls), LOWORD(fls));

			MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
			return_value_tls(new String(buf));
		}
	}
	free(data);
	return &undefined;
}

Value*
gen_class_id_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(genClassID, 0, count);

	CharStream* out = thread_local(current_stdout);
	//Generating a unique ClassID
	ulong a = mxs_rand();
	ulong b = ClassIDRandGenerator->rand();

	Value *tmp;
	if (bool_key_arg(returnValue, tmp, FALSE) == FALSE)
	{

		a &= 0x7fffffff; // don't want highest order bit set, outputting as hex,
		b &= 0x7fffffff; // if convert that hex to number, MXS makes it a float
		out->printf(_T("#(0x%x, 0x%x)\n"), a, b);
		return &ok;
	}

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array(2);
	vl.result->append(Integer64::intern(a));
	vl.result->append(Integer64::intern(b));

	return_value_tls(vl.result);
}

Value*
gen_guid_cf(Value** arg_list, int count)
{
	// <string>genGUID noBrackets:<bool> -- defaults false
	check_arg_count_with_keys(genGUID, 0, count);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(String* result);

	GUID guid;
	CoCreateGuid(&guid);

	LPOLESTR psz = NULL;
	StringFromCLSID(guid, &psz);
	TSTR identifier = TSTR::FromOLESTR(psz);
	CoTaskMemFree(psz);

	Value *tmp;
	if (bool_key_arg(noBrackets, tmp, FALSE) == TRUE)
		identifier = identifier.Substr(1, identifier.length() - 2);

	vl.result = new String(identifier);

	return_value_tls(vl.result);
}

Value*
logsys_getNetLogFileName_cf(Value** arg_list, int count)
{
	check_arg_count(getNetLogFile, 0, count);
	const TCHAR* val = MAXScript_interface->Log()->NetLogName();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(new String(val));
}


Value*
logsys_logEntry_cf(Value** arg_list, int count)
{
	// logsystem.logEntry <string> <enum>
	// logsystem.logEntry <string> error:<bool> warning:<bool> info:<bool> debug:<bool> broadcast:<bool>
	int pos_arg_count = count_with_keys();
	DWORD type = 0;
	if (pos_arg_count == 2)
	{
		def_logEntry_types();
		type = ConvertValueToID(logEntry, elements(logEntry), arg_list[1]);
	}
	else
	{
		check_arg_count_with_keys(logEntry, 1, count);
		Value *tmp;
		if (bool_key_arg(error, tmp, FALSE)) type |= SYSLOG_ERROR;
		if (bool_key_arg(warning, tmp, FALSE)) type |= SYSLOG_WARN;
		if (bool_key_arg(info, tmp, FALSE)) type |= SYSLOG_INFO;
		if (bool_key_arg(debug, tmp, FALSE)) type |= SYSLOG_DEBUG;
		if (type == 0) type = SYSLOG_DEBUG;
		if (bool_key_arg(broadcast, tmp, FALSE)) type |= SYSLOG_BROADCAST;
	}
	MAXScript_interface->Log()->LogEntry(type, NO_DIALOG, NULL, _T("%s\n"), arg_list[0]->to_string());
	return &ok;
}

Value*
logsys_logName_cf(Value** arg_list, int count)
{
	check_arg_count(logName, 1, count);
	TSTR  fileName = arg_list[0]->to_filename();
	AssetId assetId;
	if (IAssetManager::GetInstance()->StringToAssetId(fileName, assetId))
		fileName = IAssetManager::GetInstance()->GetAsset(assetId).GetFileName();

	MAXScript_interface->Log()->SetSessionLogName(fileName);
	const TCHAR * val = MAXScript_interface->Log()->GetSessionLogName();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls( new String(val) );
}

Value*
logsys_getLogName_cf(Value** arg_list, int count)
{
	// <string>logsystem.getLogName()
	check_arg_count(getLogName, 0, count);
	const TCHAR * val = MAXScript_interface->Log()->GetSessionLogName();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls(new String(val));
}

Value*
logsys_saveState_cf(Value** arg_list, int count)
{
	check_arg_count(saveState, 0, count);
	MAXScript_interface->Log()->SaveState();
	return &ok;
}

Value*
logsys_loadState_cf(Value** arg_list, int count)
{
	check_arg_count(loadState, 0, count);
	MAXScript_interface->Log()->LoadState();
	return &ok;
}

Value*
logsys_setLogLevel_cf(Value** arg_list, int count)
{
	// logsystem.setLogLevel <enum> <bool>
	// logsystem.setLogLevel error:<bool> warning:<bool> info:<bool> debug:<bool> 
	int pos_arg_count = count_with_keys();
	DWORD logTypes = MAXScript_interface->Log()->LogTypes();
	DWORD type = 0;
	if (pos_arg_count == 2)
	{
		def_logEntry_types();
		type = ConvertValueToID(logEntry, elements(logEntry), arg_list[0]);
		if (arg_list[1]->to_bool())
			logTypes |= type;
		else
			logTypes &= ~type;
	}
	else
	{
		check_arg_count_with_keys(setLogLevel, 0, count);
		Value *tmp = _get_key_arg(arg_list, count, n_error);
		if (tmp != &unsupplied)
		{
			if (tmp->to_bool())
				logTypes |= SYSLOG_ERROR;
			else
				logTypes &= ~SYSLOG_ERROR;
		}
		tmp = _get_key_arg(arg_list, count, n_warning);
		if (tmp != &unsupplied)
		{
			if (tmp->to_bool())
				logTypes |= SYSLOG_WARN;
			else
				logTypes &= ~SYSLOG_WARN;
		}
		tmp = _get_key_arg(arg_list, count, n_info);
		if (tmp != &unsupplied)
		{
			if (tmp->to_bool())
				logTypes |= SYSLOG_INFO;
			else
				logTypes &= ~SYSLOG_INFO;
		}
		tmp = _get_key_arg(arg_list, count, n_debug);
		if (tmp != &unsupplied)
		{
			if (tmp->to_bool())
				logTypes |= SYSLOG_DEBUG;
			else
				logTypes &= ~SYSLOG_DEBUG;
		}
	}
	MAXScript_interface->Log()->SetLogTypes(logTypes);
	return &ok;
}

Value*
logsys_getLogLevel_cf(Value** arg_list, int count)
{
	// <bool>logsystem.getLogLevel <enum> 
	check_arg_count(getLogLevel, 1, count);
	DWORD logTypes = MAXScript_interface->Log()->LogTypes();
	def_logEntry_types();
	DWORD	type = ConvertValueToID(logEntry, elements(logEntry), arg_list[0]);
	return (logTypes & type) ? &true_value : &false_value;
}


Value*
GetOptimizeDependentNotificationsStatistics_cf(Value** arg_list, int count)
{
	check_arg_count(GetOptimizeDependentNotificationsStatistics, 0, count);
	ULONGLONG numNotifySkipped;
	ULONGLONG numNotifyNotSkipped;
	GetOptimizeDependentNotificationsStatistics(numNotifySkipped, numNotifyNotSkipped);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	vl.result = new Array(2);
	vl.result->append(Integer64::intern(numNotifySkipped));
	vl.result->append(Integer64::intern(numNotifyNotSkipped));
	return_value_tls(vl.result);
}

Value*
ResetOptimizeDependentNotificationsStatistics_cf(Value** arg_list, int count)
{
	check_arg_count(ResetOptimizeDependentNotificationsStatistics, 0, count);
	ResetOptimizeDependentNotificationsStatistics();
	return &ok;
}
