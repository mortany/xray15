/**********************************************************************
*<
FILE: GraphicsWindow.cpp

DESCRIPTION: 

CREATED BY: Larry Minton

HISTORY: Created 15 April 2007

*>	Copyright (c) 2007, All Rights Reserved.
**********************************************************************/

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/maxwrapper/bitmaps.h>
#include <Graphics/IDisplayManager.h>
#include "MXSAgni.h"

#include "resource.h"

#include "IHardwareMaterial.h"
#include "IHardwareMesh.h"
#include "IHardwareShader.h"
#include "IHardwareRenderer.h"

#include "gamma.h"
#include "IColorCorrectionMgr.h"

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#include "extclass.h"

extern void DeGammaCorrectBitmap(Bitmap* map);

// ============================================================================

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include "GraphicsWindow_wraps.h"

#include "agnidefs.h"

void UIScaleValue(IPoint3 &p)
{
	p.x = MAXScript::GetUIScaledValue(p.x);
	p.y = MAXScript::GetUIScaledValue(p.y);
	p.z = MAXScript::GetUIScaledValue(p.z);
}

void UIScaleValue(IPoint2 &p)
{
	p.x = MAXScript::GetUIScaledValue(p.x);
	p.y = MAXScript::GetUIScaledValue(p.y);
}

void UIScaleValue(Point3 &p)
{
	p *= MaxSDK::GetUIScaleFactor();
}

void UIScaleValue(Box2 &rect)
{
	rect.left = MAXScript::GetUIScaledValue(rect.left);
	rect.bottom = MAXScript::GetUIScaledValue(rect.bottom);
	rect.right = MAXScript::GetUIScaledValue(rect.right);
	rect.top = MAXScript::GetUIScaledValue(rect.top);
}

void UIUnscaleValue(IPoint3 &p)
{
	p.x = MAXScript::GetValueUIUnscaled(p.x);
	p.y = MAXScript::GetValueUIUnscaled(p.y);
	p.z = MAXScript::GetValueUIUnscaled(p.z);
}

void UIUnscaleValue(Point3 &p)
{
	p /= MaxSDK::GetUIScaleFactor();
}

void UIUnscaleValue(Box2 &rect)
{
	rect.left = MAXScript::GetValueUIUnscaled(rect.left);
	rect.bottom = MAXScript::GetValueUIUnscaled(rect.bottom);
	rect.right = MAXScript::GetValueUIUnscaled(rect.right);
	rect.top = MAXScript::GetValueUIUnscaled(rect.top);
}

/*-----------------------------------GraphicsWindow-------------------------------------------*/

Value*
getDriverString_cf(Value** arg_list, int count)
{
	check_arg_count(getDriverString, 0, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new String(gw->getDriverString()));
}

Value*
isPerspectiveView_cf(Value** arg_list, int count)
{
	check_arg_count(isPerspectiveView, 0, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();		
	return gw->isPerspectiveView() ? &true_value : &false_value;	
}

Value*
setSkipCount_cf(Value** arg_list, int count)
{
	check_arg_count(setSkipCount, 1, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	gw->setSkipCount(arg_list[0]->to_int());
	return &ok;
}


Value*
setDirectXDisplayAllTriangle_cf(Value** arg_list, int count)
{
	check_arg_count(setDirectXDisplayAllTriangle, 1, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	IHardwareRenderer *phr = (IHardwareRenderer *)gw->GetInterface(HARDWARE_RENDERER_INTERFACE_ID);
	if (phr)
	{
		bool displayAllEdges = arg_list[0]->to_bool() != FALSE;
		phr->SetDisplayAllTriangleEdges(displayAllEdges);
	}
	return &ok;
}


Value*
getSkipCount_cf(Value** arg_list, int count)
{
	check_arg_count(getSkipCount, 0, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(gw->getSkipCount()));
}

Value*
querySupport_cf(Value** arg_list, int count)
{
	check_arg_count(querySupport, 01, count);
	def_spt_types();
	GraphicsWindow	*gw = MAXScript_interface->GetActiveViewExp().getGW();		
	return gw->querySupport(
		ConvertValueToID(sptTypes, elements(sptTypes), arg_list[0])) ?
		&true_value : &false_value;
}

Value*
setTransform_cf(Value** arg_list, int count)
{
	check_arg_count(setTransform, 1, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	gw->setTransform(arg_list[0]->to_matrix3());
	return &ok;
}

Value*
setPos_cf(Value** arg_list, int count)
{
	// gw.setPos <x_integer> <y_integer> <w_integer> <h_integer> applyUIScaling:<true>
	check_arg_count_with_keys(setPos, 4, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;
	int x =arg_list[0]->to_int();
	int y =arg_list[1]->to_int();
	int w =arg_list[2]->to_int();
	int h =arg_list[3]->to_int();
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
	{
		x = MAXScript::GetUIScaledValue(x);
		y = MAXScript::GetUIScaledValue(y);
		w = MAXScript::GetUIScaledValue(w);
		h = MAXScript::GetUIScaledValue(h);
	}
	gw->setPos(x,y,w,h);
	return &ok;
}

Value*
getWinSizeX_cf(Value** arg_list, int count)
{
	// gw.getWinSizeX removeUIScaling:<true>
	check_arg_count_with_keys(getWinSizeX, 0, count);
	int sizeX = MAXScript_interface->GetActiveViewExp().getGW()->getWinSizeX();
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling)
		sizeX = MAXScript::GetValueUIUnscaled(sizeX);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(sizeX));

}

Value*
getWinSizeY_cf(Value** arg_list, int count)
{
	// gw.getWinSizeY removeUIScaling:<true>
	check_arg_count_with_keys(getWinSizeY, 0, count);
	int sizeY = MAXScript_interface->GetActiveViewExp().getGW()->getWinSizeY();
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling)
		sizeY = MAXScript::GetValueUIUnscaled(sizeY);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(sizeY));
}


Value*
getWinDepth_cf(Value** arg_list, int count)
{
	check_arg_count(getWinDepth, 0, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(gw->getWinDepth()));
}

Value*
getHitherCoord_cf(Value** arg_list, int count)
{
	check_arg_count(getHitherCoord, 0, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(gw->getHitherCoord()));
}

Value*
getYonCoord_cf(Value** arg_list, int count)
{
	check_arg_count(getYonCoord, 0, count);
	GraphicsWindow *gw = MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(gw->getYonCoord()));
}

// This namespace is intended to contain global functions that implement functionality common to MAXScript and MaxPlus.
namespace ScripterBase
{
	namespace Viewport
	{
		enum GetViewportDIB_Result { Success, FAIL_ViewAccessError, FAIL_BitmapCreateFail};
		GetViewportDIB_Result GetViewportDIB(ViewExp &vpt, BitmapInfo &bi, Bitmap* &bmp, bool captureAlpha, bool gammaCorrect);
	}
}

// capture viewport as DIB. Return as BitmapInfo and Bitmap*
// Parameters:
//	ViewExp& vpt [in] - viewport to capture
//	BitmapInfo &bi [out] - BitmapInfo to store image information data
//	Bitmap* &bmp [out] - ref to Bitmap*. Bitmap created by this method, caller owns Bitmap
ScripterBase::Viewport::GetViewportDIB_Result ScripterBase::Viewport::GetViewportDIB(ViewExp &vpt, BitmapInfo &bi, Bitmap* &bmp, bool captureAlpha, bool gammaCorrect)
{
	DbgAssert(vpt.IsAlive());
	if (! vpt.IsAlive()) 
		return FAIL_ViewAccessError;

	GraphicsWindow *gw = vpt.getGW();
	DbgAssert(gw != nullptr);
	if (gw == nullptr)
		return FAIL_ViewAccessError;

	int size;
	if (!gw->getDIB(nullptr, &size))
		return FAIL_ViewAccessError;

	BITMAPINFO *bmi  = (BITMAPINFO *)malloc(size);
	BITMAPINFOHEADER *bmih = (BITMAPINFOHEADER *)bmi;
	gw->getDIB(bmi, &size);
	bi.SetWidth((WORD)bmih->biWidth);
	bi.SetHeight((WORD)bmih->biHeight);
	// To gamma-correct we need 64 bits resolution
	bi.SetType(BMM_TRUE_64);
	bi.SetGamma(1.0f); // New bitmap will be linear
	if (captureAlpha)
		bi.SetFlags(MAP_HAS_ALPHA);

	{
		TempBitmapManagerSilentMode tbmsm;
		bmp = TheManager->Create(&bi); // Make new, linear bitmap
	}
	if (bmp == nullptr)
	{
		free(bmi);
		return FAIL_BitmapCreateFail;
	}
	bmp->FromDib(bmi);

	free(bmi);

	if (gammaCorrect)
		DeGammaCorrectBitmap(bmp);
	return Success;
}

//DIB Methods
Value*
getViewportDib_cf(Value** arg_list, int count)
{
	// gw.getViewportDib captureAlpha:<bool> gammaCorrect:<bool>
	check_arg_count_with_keys(getViewportDib, 0, count);
	bool captureAlpha = key_arg_or_default(captureAlpha, &false_value)->to_bool() ? true : false;
	bool gammaCorrect = key_arg_or_default(gammaCorrect, &true_value)->to_bool() ? true : false;

	ViewExp& vpt = MAXScript_interface->GetActiveViewExp();
	BitmapInfo bi;
	Bitmap *bmp = nullptr;
	ScripterBase::Viewport::GetViewportDIB_Result res = ScripterBase::Viewport::GetViewportDIB(vpt, bi, bmp, captureAlpha, gammaCorrect);
	if (res == ScripterBase::Viewport::Success)
	{
		MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
		return_value_tls(new MAXBitMap(bi, bmp));
	}
	else if (res == ScripterBase::Viewport::FAIL_BitmapCreateFail)
		throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_BITMAP_CREATE_FAILURE));
	return &undefined;
}

Value*
resetUpdateRect_cf(Value** arg_list, int count)
{
	check_arg_count(resetUpdateRect, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	gw->resetUpdateRect();
	return &ok;
}

Value*
enlargeUpdateRect_cf(Value** arg_list, int count)
{
	// gw.enlargeUpdateRect ( <Box2> | #whole ) applyUIScaling:<true>
	check_arg_count_with_keys(enlargeUpdateRect, 1, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Value* val = arg_list[0];
	if (val == n_whole)
		gw->enlargeUpdateRect(NULL);
	else
	{
		Box2 rect = val->to_box2();
		Value* v = nullptr;
		BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
		if (applyUIScaling && !rect.IsEmpty())
			UIScaleValue(rect);
		gw->enlargeUpdateRect( &rect );
	}
	return &ok;
}

Value*
getUpdateRect_cf(Value** arg_list, int count)
{
	// gw.getUpdateRect removeUIScaling:<true>
	check_arg_count_with_keys(getUpdateRect, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Rect			rect;	
	BOOL res = gw->getUpdateRect(&rect);
	if (!res)
	{
		rect.SetEmpty();
		rect.SetWH(gw->getWinSizeX(),gw->getWinSizeY());
	}
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling && !rect.IsEmpty())
		UIUnscaleValue(rect);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Box2Value(rect));
}

Value*
updateScreen_cf(Value** arg_list, int count)
{
	check_arg_count(updateScreen, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();		
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	gw->updateScreen();
	return &ok;
}

Value*
setRndLimits_cf(Value** arg_list, int count)
{
	check_arg_count(setRndLimits, 1, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_render_types();
	Array			*arr	= (Array*)arg_list[0];
	DWORD			lim		=0;

	type_check(arr, Array, _T("setRndLimits"));
	for (int i=0; i < arr->size; i++)
		lim |= ConvertValueToID(renderTypes, elements(renderTypes), arr->data[i]);
	gw->setRndLimits(lim);
	return &ok;
}

Value*
getRndLimits_cf(Value** arg_list, int count)
{
	check_arg_count(getRndLimits, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_render_types();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);	
	DWORD			lim		= gw->getRndLimits();
	vl.result = new Array(3);
	for (int i=0; i < elements(renderTypes); i++)
		if ((renderTypes[i].id) & lim) 
			vl.result->append(renderTypes[i].val);	
	return_value_tls (vl.result);
}

Value*
getRndMode_cf(Value** arg_list, int count)
{
	check_arg_count(getRndMode, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_render_types();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(Array* result);
	DWORD			mode	= gw->getRndMode();
	vl.result = new Array(3);
	for (int i=0; i < elements(renderTypes); i++)
		if ((renderTypes[i].id) & mode) 
			vl.result->append(renderTypes[i].val);		
	return_value_tls (vl.result);
}

Value*
getMaxLights_cf(Value** arg_list, int count)
{
	check_arg_count(getMaxLights, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	int				n		= gw->getMaxLights();	
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Integer::intern(n));
}

Value*
hTransPoint_cf(Value** arg_list, int count)
{
	// gw.hTransPoint <point3> removeUIScaling:<true>
	check_arg_count_with_keys(hTransPoint, 1, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Point3			in		= arg_list[0]->to_point3();
	IPoint3			out;
	gw->hTransPoint(&in, &out);
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling)
		UIUnscaleValue(out);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point3Value((float)out.x, (float)out.y, (float)out.z));
}

Value*
wTransPoint_cf(Value** arg_list, int count)
{
	// gw.wTransPoint <point3> removeUIScaling:<true>
	check_arg_count_with_keys(wTransPoint, 1, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Point3			in		= arg_list[0]->to_point3();
	IPoint3			out;
	gw->wTransPoint(&in, &out);
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling)
		UIUnscaleValue(out);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point3Value((float)out.x, (float)out.y, (float)out.z));
}

Value*
transPoint_cf(Value** arg_list, int count)
{
	// gw.transPoint <point3> removeUIScaling:<true>
	check_arg_count_with_keys(transPoint, 1, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Point3			in		= arg_list[0]->to_point3();
	Point3			out;
	gw->transPoint(&in, &out);
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling)
		UIUnscaleValue(out);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point3Value(out.x, out.y, out.z));
}

Value*
getFlipped_cf(Value** arg_list, int count)
{
	check_arg_count(getFlipped, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	BOOL			res		= gw->getFlipped();	
	return (res) ? &true_value : &false_value;
}

Value*
setColor_cf(Value** arg_list, int count)
{
	check_arg_count(setColor, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_color_types();
	AColor			col		= arg_list[1]->to_acolor();
	ColorType clrType = (ColorType)ConvertValueToID(colorTypes, elements(colorTypes), arg_list[0]);
	gw->setColor(clrType, col.r, col.g, col.b);
	return &ok;
}

Value*
clearScreen_cf(Value** arg_list, int count)
{
	// gw.clearScreen <Box2> useBkg:<boolean> applyUIScaling:<true>
	check_arg_count_with_keys(clearScreen, 1, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Box2			rect	= arg_list[0]->to_box2();
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(rect);
	gw->clearScreen(&rect, key_arg_or_default(useBkg, &false_value)->to_bool());
	return &ok;
}

Value*
hText_cf(Value** arg_list, int count)
{
	// gw.hText <point3> <string> color:<color> applyUIScaling:<true>
	check_arg_count_with_keys(hText, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Value*			col_val = key_arg(color);
	IPoint3 pos = to_ipoint3(arg_list[0]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(pos);
	Point3 clr = (col_val == &unsupplied) ? Point3(1, 0, 0) : col_val->to_point3() / 255.0f;
	const TCHAR* text = arg_list[1]->to_string();
	gw->setColor(TEXT_COLOR, clr);
	gw->hText(&pos, text);	
	return &ok;
}

Value*
hMarker_cf(Value** arg_list, int count)
{
	// gw.hMarker <point3> <marker_name> color:<color> applyUIScaling:<true>
	check_arg_count_with_keys(hMarker, 2, count);	
	GraphicsWindow* gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_marker_types();
	Value*			col_val = key_arg(color);	
	MarkerType		mt = (MarkerType)ConvertValueToID(markerTypes, elements(markerTypes), arg_list[1]);
	IPoint3 pos = to_ipoint3(arg_list[0]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(pos);
	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	Point3 clr = (col_val == &unsupplied) ? Point3(1, 0, 0) : col_val->to_point3() / 255.0f;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->setColor(LINE_COLOR, clr);
	gw->hMarker(&pos, mt);	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
hPolyline_cf(Value** arg_list, int count)
{
	// gw.hPolyline <vertex_point3_array> <isClosed_boolean> rgb:<color_array> applyUIScaling:<true>
	check_arg_count_with_keys(hPolyline, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();				 
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	type_check(arg_list[0], Array, _T("hPolyline"));
	Array*			pts_val	= (Array*)arg_list[0];
	int				ct		= pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	MaxSDK::Array<IPoint3> points(ct);
	MaxSDK::Array<Point3> colors;
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	for (int i=0; i < ct; i++)
	{
		points[i] = to_ipoint3(pts_val->data[i]);
		if (applyUIScaling)
			UIScaleValue(points[i]);
	}

	if (key_arg(rgb) != &unsupplied) {
		type_check(key_arg(rgb), Array, _T("hPolyline"));
		Array* col_val = (Array*)key_arg(rgb);		 		
		if (ct != col_val->size)
			throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
		colors.setLengthUsed(ct);
		for (int i=0; i < ct; i++)
			colors[i] = col_val->data[i]->to_point3()/255.f;
	}

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	BOOL			closed = arg_list[1]->to_bool();
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->hPolyline(ct, points.asArrayPtr(), colors.asArrayPtr(), closed, NULL);	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
hPolygon_cf(Value** arg_list, int count)
{
	// gw.hPolygon <vertex_point3_array> <color_array> <uvw_point3_array> applyUIScaling:<true>
	check_arg_count_with_keys(hPolygon, 3, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("hPolygon"));
	Array			*pts_val = (Array*)arg_list[0], 
		*col_val = (Array*)arg_list[1],
		*uvw_val = (Array*)arg_list[2];	
	int				ct		 = pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	if (col_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
	if (uvw_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_UVW_ARRAY_SIZE));

	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);

	MaxSDK::Array<IPoint3> pts(ct);
	MaxSDK::Array<Point3> col(ct);
	MaxSDK::Array<Point3> uvw(ct);

	for (int i=0; i < ct; i++) {
		pts[i] = to_ipoint3(pts_val->data[i]);
		if (applyUIScaling)
			UIScaleValue(pts[i]);
		col[i] = col_val->data[i]->to_point3()/255.f;
		uvw[i] = uvw_val->data[i]->to_point3();
	}	

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->hPolygon(ct, pts.asArrayPtr(), col.asArrayPtr(), uvw.asArrayPtr());	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
triStrip_cf(Value** arg_list, int count)
{
	// gw.triStrip <vertex_point3_array> <color_array> <uvw_point3_array>
	check_arg_count_with_keys(triStrip, 3, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("triStrip"));
	Array			*pts_val = (Array*)arg_list[0], 
		*col_val = (Array*)arg_list[1],
		*uvw_val = (Array*)arg_list[2];	
	int				ct		 = pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed

	if (col_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
	if (uvw_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_UVW_ARRAY_SIZE));

	MaxSDK::Array<Point3> pts(ct);
	MaxSDK::Array<Point3> col(ct);
	MaxSDK::Array<Point3> uvw(ct);

	for (int i=0; i < ct; i++) {
		pts[i] = pts_val->data[i]->to_point3();
		col[i] = col_val->data[i]->to_point3()/255.f;
		uvw[i] = uvw_val->data[i]->to_point3();
	}	

	gw->triStrip(ct, pts.asArrayPtr(), col.asArrayPtr(), uvw.asArrayPtr());
	return &ok;
}

Value*
hTriStrip_cf(Value** arg_list, int count)
{
	// gw.hTriStrip <vertex_point3_array> <color_array> <uvw_point3_array> applyUIScaling:<true>
	check_arg_count_with_keys(hTriStrip, 3, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("hTriStrip"));
	Array			*pts_val = (Array*)arg_list[0], 
		*col_val = (Array*)arg_list[1],
		*uvw_val = (Array*)arg_list[2];	
	int				ct		 = pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	if (col_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
	if (uvw_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_UVW_ARRAY_SIZE));

	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);

	MaxSDK::Array<IPoint3> pts(ct);
	MaxSDK::Array<Point3> col(ct);
	MaxSDK::Array<Point3> uvw(ct);

	for (int i=0; i < ct; i++) {
		pts[i] = to_ipoint3(pts_val->data[i]);
		if (applyUIScaling)
			UIScaleValue(pts[i]);
		col[i] = col_val->data[i]->to_point3()/255.f;
		uvw[i] = uvw_val->data[i]->to_point3();
	}	

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->hTriStrip(ct, pts.asArrayPtr(), col.asArrayPtr(), uvw.asArrayPtr());
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
wText_cf(Value** arg_list, int count)
{
	// gw.wText <point3> <string> color:<color> applyUIScaling:<true>
	check_arg_count_with_keys(wText, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Value*			col_val = key_arg(color);
	IPoint3 pos = to_ipoint3(arg_list[0]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(pos);
	Point3 clr = (col_val == &unsupplied) ? Point3(1, 0, 0) : col_val->to_point3() / 255.0f;
	const TCHAR* text = arg_list[1]->to_string();
	gw->setColor(TEXT_COLOR, clr);
	gw->wText(&pos, text);	
	return &ok;
}

Value*
wMarker_cf(Value** arg_list, int count)
{
	// gw.wMarker <point3> <marker_name> color:<color> applyUIScaling:<true>
	check_arg_count_with_keys(wMarker, 2, count);	
	GraphicsWindow* gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_marker_types();
	Value*			col_val = key_arg(color);	
	MarkerType		mt = (MarkerType)ConvertValueToID(markerTypes, elements(markerTypes), arg_list[1]);
	IPoint3 pos = to_ipoint3(arg_list[0]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(pos);
	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	Point3 clr = (col_val == &unsupplied) ? Point3(1, 0, 0) : col_val->to_point3() / 255.0f;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->setColor(LINE_COLOR, clr);
	gw->wMarker(&pos, mt);	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
wPolyline_cf(Value** arg_list, int count)
{
	// gw.wPolyline <vertex_point3_array> <isClosed_boolean> rgb:<color_array> applyUIScaling:<true>
	check_arg_count_with_keys(wPolyline, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();				 
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	type_check(arg_list[0], Array, _T("wPolyline"));
	Array*			pts_val	= (Array*)arg_list[0];
	int				ct		= pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	MaxSDK::Array<IPoint3> points(ct);
	MaxSDK::Array<Point3> colors;
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	for (int i=0; i < ct; i++)
	{
		points[i] = to_ipoint3(pts_val->data[i]);
		if (applyUIScaling)
			UIScaleValue(points[i]);
	}

	if (key_arg(rgb) != &unsupplied) {
		type_check(key_arg(rgb), Array, _T("hPolyline"));
		Array* col_val = (Array*)key_arg(rgb);		 		
		if (ct != col_val->size)
			throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
		colors.setLengthUsed(ct);
		for (int i=0; i < ct; i++)
			colors[i] = col_val->data[i]->to_point3()/255.f;
	}

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	BOOL			closed = arg_list[1]->to_bool();
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->wPolyline(ct, points.asArrayPtr(), colors.asArrayPtr(), closed, NULL);	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
wPolygon_cf(Value** arg_list, int count)
{
	// gw.wPolygon <vertex_point3_array> <color_array> <uvw_point3_array> applyUIScaling:<true>
	check_arg_count_with_keys(wPolygon, 3, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("wPolygon"));
	Array			*pts_val = (Array*)arg_list[0], 
		*col_val = (Array*)arg_list[1],
		*uvw_val = (Array*)arg_list[2];	
	int				ct		 = pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	if (col_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
	if (uvw_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_UVW_ARRAY_SIZE));

	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);

	MaxSDK::Array<IPoint3> pts(ct);
	MaxSDK::Array<Point3> col(ct);
	MaxSDK::Array<Point3> uvw(ct);

	for (int i=0; i < ct; i++) {
		pts[i] = to_ipoint3(pts_val->data[i]);
		if (applyUIScaling)
			UIScaleValue(pts[i]);
		col[i] = col_val->data[i]->to_point3()/255.f;
		uvw[i] = uvw_val->data[i]->to_point3();
	}	

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->wPolygon(ct, pts.asArrayPtr(), col.asArrayPtr(), uvw.asArrayPtr());	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
wTriStrip_cf(Value** arg_list, int count)
{
	// gw.wTriStrip <vertex_point3_array> <color_array> <uvw_point3_array> applyUIScaling:<true>
	check_arg_count_with_keys(wTriStrip, 3, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("wTriStrip"));
	Array			*pts_val = (Array*)arg_list[0], 
		*col_val = (Array*)arg_list[1],
		*uvw_val = (Array*)arg_list[2];	
	int				ct		 = pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	if (col_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
	if (uvw_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_UVW_ARRAY_SIZE));

	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);

	MaxSDK::Array<IPoint3> pts(ct);
	MaxSDK::Array<Point3> col(ct);
	MaxSDK::Array<Point3> uvw(ct);

	for (int i=0; i < ct; i++) {
		pts[i] = to_ipoint3(pts_val->data[i]);
		if (applyUIScaling)
			UIScaleValue(pts[i]);
		col[i] = col_val->data[i]->to_point3()/255.f;
		uvw[i] = uvw_val->data[i]->to_point3();
	}	

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->wTriStrip(ct, pts.asArrayPtr(), col.asArrayPtr(), uvw.asArrayPtr());
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
wRect_cf(Value** arg_list, int count)
{
	// gw.wRect <box2> <color> applyUIScaling:<true>
	check_arg_count_with_keys(wRect, 2, count);
	GraphicsWindow* gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Box2 rect = arg_list[0]->to_box2();
	Point3 color = arg_list[1]->to_point3()/255.f;

	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(rect);

	IPoint3* pts = new IPoint3[2]; 
	pts[0]=IPoint3((int)(rect.left),0.f,0.f);
	pts[1]=IPoint3((int)(rect.right),0.f,0.f);
	gw->setColor(LINE_COLOR, color);
	for (int j = rect.top; j <= rect.bottom; j++)
	{
		pts[0].y=pts[1].y=j;
		gw->wPolyline(2, pts, NULL, FALSE, NULL);	
	}
	delete [] pts;
	return &ok;
}

Value*
hRect_cf(Value** arg_list, int count)
{
	// gw.hRect <box2> <color> applyUIScaling:<true>
	check_arg_count_with_keys(hRect, 2, count);
	GraphicsWindow* gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Box2 rect = arg_list[0]->to_box2();
	Point3 color = arg_list[1]->to_point3()/255.f;

	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(rect);

	IPoint3* pts = new IPoint3[2]; 
	pts[0]=IPoint3((int)(rect.left),0.f,0.f);
	pts[1]=IPoint3((int)(rect.right),0.f,0.f);
	gw->setColor(LINE_COLOR, color);
	for (int j = rect.top; j <= rect.bottom; j++)
	{
		pts[0].y=pts[1].y=j;
		gw->hPolyline(2, pts, NULL, FALSE, NULL);	
	}
	delete [] pts;
	return &ok;
}

Value*
text_cf(Value** arg_list, int count)
{
	// gw.text <point3> <string> color:<color>
	check_arg_count_with_keys(text, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	Value*			col_val = key_arg(color);
	Point3 pos = arg_list[0]->to_point3();
	Point3 clr = (col_val == &unsupplied) ? Point3(1, 0, 0) : col_val->to_point3() / 255.0f;
	const TCHAR* text = arg_list[1]->to_string();
	gw->setColor(TEXT_COLOR, clr);
	gw->text(&pos, text);	
	return &ok;
}

Value*
marker_cf(Value** arg_list, int count)
{
	// gw.marker <point3> <marker_name> color:<color>
	check_arg_count_with_keys(marker, 2, count);	
	GraphicsWindow* gw = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	def_marker_types();
	Value*			col_val = key_arg(color);	
	MarkerType		mt = (MarkerType)ConvertValueToID(markerTypes, elements(markerTypes), arg_list[1]);
	Point3 pos = arg_list[0]->to_point3();
	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	Point3 clr = (col_val == &unsupplied) ? Point3(1, 0, 0) : col_val->to_point3() / 255.0f;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->setColor(LINE_COLOR, clr);
	gw->marker(&pos, mt);	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
polyline_cf(Value** arg_list, int count)
{
	// gw.polyline <vertex_point3_array> <isClosed_boolean> rgb:<color_array>
	check_arg_count_with_keys(polyline, 2, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();				 
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	type_check(arg_list[0], Array, _T("Polyline"));
	Array*			pts_val	= (Array*)arg_list[0];
	int				ct		= pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	MaxSDK::Array<Point3> points(ct+1);  // one extra element per sdk
	MaxSDK::Array<Point3> colors;
	for (int i=0; i < ct; i++)
		points[i] = pts_val->data[i]->to_point3();

	if (key_arg(rgb) != &unsupplied) {
		type_check(key_arg(rgb), Array, _T("hPolyline"));
		Array* col_val = (Array*)key_arg(rgb);		 		
		if (ct != col_val->size)
			throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
		colors.setLengthUsed(ct);
		for (int i=0; i < ct; i++)
			colors[i] = col_val->data[i]->to_point3()/255.f;
	}

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	BOOL			closed = arg_list[1]->to_bool();
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->polyline(ct, points.asArrayPtr(), colors.asArrayPtr(), closed, NULL);	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
polygon_cf(Value** arg_list, int count)
{
	// gw.polygon <vertex_point3_array> <color_array> <uvw_point3_array>
	check_arg_count_with_keys(polygon, 3, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("Polygon"));
	Array			*pts_val = (Array*)arg_list[0], 
		*col_val = (Array*)arg_list[1],
		*uvw_val = (Array*)arg_list[2];	
	int				ct		 = pts_val->size; if (!ct) return &undefined; // Return if an empty array is passed
	if (col_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_RGB_ARRAY_SIZE));
	if (uvw_val->size != ct)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_INVALID_UVW_ARRAY_SIZE));

	MaxSDK::Array<Point3> pts(ct+1);  // one extra element per sdk
	MaxSDK::Array<Point3> col(ct+1);
	MaxSDK::Array<Point3> uvw(ct+1);

	for (int i=0; i < ct; i++) {
		pts[i] = pts_val->data[i]->to_point3();
		col[i] = col_val->data[i]->to_point3()/255.f;
		uvw[i] = uvw_val->data[i]->to_point3();
	}	

	DWORD			lim		= gw->getRndLimits();
	BOOL			resetLimit = lim & GW_Z_BUFFER;
	if (resetLimit) gw->setRndLimits(lim & ~GW_Z_BUFFER);
	gw->polygon(ct, pts.asArrayPtr(), col.asArrayPtr(), uvw.asArrayPtr());	
	if (resetLimit) gw->setRndLimits(lim);
	return &ok;
}

Value*
startTriangles_cf(Value** arg_list, int count)
{
	check_arg_count(startTriangles, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();		
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	gw->startTriangles();
	return &ok;
}

Value*
endTriangles_cf(Value** arg_list, int count)
{
	check_arg_count(endTriangles, 0, count);
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();		
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	gw->endTriangles();
	return &ok;
}

Value*
triangle_cf(Value** arg_list, int count)
{
	// gw.triangle <vertex_point3_array> <color_array>
	check_arg_count(triangle, 2, count);
	GraphicsWindow	*gw		 = MAXScript_interface->GetActiveViewExp().getGW();
	if (MaxSDK::Graphics::IsRetainedModeEnabled() && gw->querySupport(GW_SPT_NUM_LIGHTS) == 0)
		return &undefined;

	for (int i=0; i < count; i++)
		type_check(arg_list[i], Array, _T("triangle"));

	Array			*pts_val = (Array*)arg_list[0], 
	*col_val = (Array*)arg_list[1];
	if (col_val->size != 3)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_RGB_ARRAY_SIZE_NOT_3));
	if (pts_val->size != 3)
		throw RuntimeError(MaxSDK::GetResourceStringAsMSTR(IDS_RK_VERT_ARRAY_SIZE_NOT_3));

	MaxSDK::Array<Point3> pts(3);  // one extra element per sdk
	MaxSDK::Array<Point3> col(3);
	for (int i=0; i < 3; i++) {
		pts[i] = pts_val->data[i]->to_point3();
		col[i] = col_val->data[i]->to_point3()/255.f;
	}	
	gw->triangle(pts.asArrayPtr(), col.asArrayPtr());
	return &ok;
}

Value*
getTextExtent_gw_cf(Value** arg_list, int count)
{
	// gw.getTextExtent <string> removeUIScaling:<true>
	check_arg_count_with_keys(getTextExtent, 1, count);
	const TCHAR			*text	= arg_list[0]->to_string();
	GraphicsWindow	*gw		= MAXScript_interface->GetActiveViewExp().getGW();	
	SIZE size;
	gw->getTextExtents(text, &size);
	Value* v;
	BOOL removeUIScaling = bool_key_arg(removeUIScaling, v, TRUE); 
	if (removeUIScaling)
	{
		size.cx = MAXScript::GetValueUIUnscaled(size.cx);
		size.cy = MAXScript::GetValueUIUnscaled(size.cy);
	}
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point2Value((float)size.cx, (float)size.cy));
}

/*-----------------------------------ViewExp-------------------------------------------*/

Value*
NonScalingObjectSize_cf(Value** arg_list, int count)
{
	check_arg_count(NonScalingObjectSize, 0, count);
	float ret = MAXScript_interface->GetActiveViewExp().NonScalingObjectSize();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Float::intern(ret));
}

Value*
GetPointOnCP_cf(Value** arg_list, int count)
{
	// <point3>gw.getPointOnCP <pixel_coord_point2> applyUIScaling:<true>
	check_arg_count_with_keys(GetPointOnCP, 1, count);
	IPoint2 val = to_ipoint2(arg_list[0]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(val);
	Point3 ret = MAXScript_interface->GetActiveViewExp().GetPointOnCP(val);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point3Value(ret));
}

Value*
GetCPDisp_cf(Value** arg_list, int count)
{
	// <float>gw.getCPDisp <base_point3> <dir_point3> <start_pixel_coord_point2> <end_pixel_coord_point2> applyUIScaling:<true>
	check_arg_count_with_keys(GetCPDisp, 4, count);
	Point3 v1 = arg_list[0]->to_point3();
	Point3 v2 = arg_list[1]->to_point3();
	IPoint2 v3 = to_ipoint2(arg_list[2]);
	IPoint2 v4 = to_ipoint2(arg_list[3]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
	{
		UIScaleValue(v3);
		UIScaleValue(v4);
	}
	float ret = MAXScript_interface->GetActiveViewExp().GetCPDisp(v1,v2,v3,v4);		
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Float::intern(ret));
}

Value*
GetVPWorldWidth_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(GetVPWorldWidth, 1, count);
	Point3 val = arg_list[0]->to_point3();
	float ret = MAXScript_interface->GetActiveViewExp().GetVPWorldWidth(val);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Float::intern(ret));
}

Value*
MapCPToWorld_cf(Value** arg_list, int count)
{
	check_arg_count(MapCPToWorld, 1, count);

	Point3 val = arg_list[0]->to_point3();
	Point3	ret = MAXScript_interface->GetActiveViewExp().MapCPToWorld(val);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point3Value(ret));
}

Value*
SnapPoint_cf(Value** arg_list, int count)
{
	// <point3>gw.snapPoint <pixel_coord_point2> snapType:<snapType_name> snapPlane:<matrix3> applyUIScaling:<true>
	check_arg_count_with_keys(SnapPoint, 1, count);
	def_snap_types();
	IPoint2 out;
	IPoint2 loc = to_ipoint2(arg_list[0]);
	Value* v = nullptr;
	BOOL applyUIScaling = bool_key_arg(applyUIScaling, v, TRUE);
	if (applyUIScaling)
		UIScaleValue(loc);

	Value	*val = key_arg(snapType);
	int		flags = (val == &unsupplied) ? 0 : ConvertValueToID(snapTypes, elements(snapTypes), val);

	Value *snapPlane = key_arg(snapPlane);
	Matrix3* plane = NULL;
	if (snapPlane != &unsupplied) {
		if (!snapPlane->is_kind_of(class_tag(Matrix3Value)))
			throw TypeError (_T("snapPlane requires a Matrix3 value"), snapPlane);

		Matrix3Value* mv = static_cast<Matrix3Value*>(snapPlane);
		plane = new Matrix3(mv->m);
	}
	Point3	ret = MAXScript_interface->GetActiveViewExp().SnapPoint(loc, out, plane, flags);
	if (plane != NULL)
		delete plane;
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (new Point3Value(ret));
}

Value* 
SnapLength_cf(Value** arg_list, int count)
{
	check_arg_count(SnapLength, 1, count);
	float len = arg_list[0]->to_float();
	float ret = MAXScript_interface->GetActiveViewExp().SnapLength(len);
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Float::intern(ret));
}

Value*
IsPerspView_cf(Value** arg_list, int count)
{
	check_arg_count(IsPerspView, 0, count);
	Value* ret = MAXScript_interface->GetActiveViewExp().IsPerspView() ? &true_value : &false_value;
	return ret;
}

Value*
GetFocalDist_cf(Value** arg_list, int count)
{
	check_arg_count(GetFocalDist, 0, count);
	float ret = MAXScript_interface->GetActiveViewExp().GetFocalDist();
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	return_value_tls (Float::intern(ret));
}
