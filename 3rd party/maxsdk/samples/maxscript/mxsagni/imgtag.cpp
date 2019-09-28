// ============================================================================
// ImgTag.cpp
// Copyright �1999 Discreet
// Created by Simon Feltman
// ----------------------------------------------------------------------------
#include <maxscript/maxscript.h>
#include <maxscript/maxwrapper/bitmaps.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/colors.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/compiler/parser.h>
#include <maxscript/UI/rollouts.h>

#include "MXSAgni.h"

#include <maxicon.h>
#include <Qt/QMaxHelpers.h>

extern HINSTANCE g_hInst;

#include "resource.h"

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

using namespace MAXScript;

#include <maxscript\macros\define_external_functions.h>
#  include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>

#include "3dsmaxport.h"

#define IMGTAG_WINDOWCLASS _T("IMGTAG_WINDOWCLASS")
#define TOOLTIP_ID 1

// ============================================================================
visible_class_s (ImgTag, RolloutControl)

class ImgTag : public RolloutControl
{
protected:
	HWND     m_hWnd;
	HWND     m_hToolTip;
	TSTR     m_sToolTip;
	bool     m_bMouseOver;
	float    m_opacity;
	COLORREF m_transparent;
	BOOL     m_applyUIScaling;
	int      m_image_width, m_image_height;

	HBITMAP     m_hBitmap;
	Value      *m_maxBitMap;
	Value      *m_style;
	Value      *m_iconName;

public:
	// Static methods
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static RolloutControl* create(Value* name, Value* caption, Value** keyparms, int keyparm_count)
	{ return new ImgTag(name, caption, keyparms, keyparm_count); }

	ImgTag(Value* name, Value* caption, Value** keyparms, int keyparm_count);
	~ImgTag();

	int SetBitmap(Value* val);
	TOOLINFO* GetToolInfo();
	void Invalidate();

	enum button {left, middle, right};

	// Event handlers
	LRESULT ButtonDown(int xPos, int yPos, int fwKeys, button which);
	LRESULT ButtonUp(int xPos, int yPos, int fwKeys, button which);
	LRESULT ButtonDblClk(int xPos, int yPos, int fwKeys, button which);
	LRESULT MouseMove(int xPos, int yPos, int fwKeys);

	LRESULT EraseBkgnd(HDC hDC);
	LRESULT Paint(HDC hDC);

	void   run_event_handler (Value* event, int xPos, int yPos, int fwKeys, bool capture = true);

	classof_methods(ImgTag, RolloutControl);
	void     gc_trace();
	void     collect() { delete this; }
	void     sprin1(CharStream* s) { s->printf(_T("ImgTag:%s"), name->to_string()); }

	void     add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y);
	void     adjust_control(int& current_y) override;
	LPCTSTR  get_control_class() { return IMGTAG_WINDOWCLASS; }
	void     compute_layout(Rollout *ro, layout_data* pos) { }
	BOOL     handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam);

	Value*   get_property(Value** arg_list, int count);
	Value*   set_property(Value** arg_list, int count);
	void     set_enable();
};

// ============================================================================
ImgTag::ImgTag(Value* name, Value* caption, Value** keyparms, int keyparm_count)
	: RolloutControl(name, caption, keyparms, keyparm_count)
{
	tag = class_tag(ImgTag);

	m_hWnd = NULL;
	m_hToolTip = NULL;
	m_sToolTip = _T("");
	m_hBitmap = NULL;
	m_maxBitMap = NULL;
	m_iconName = NULL;
	m_bMouseOver = false;
	m_style = n_bmp_stretch;
	m_opacity = 0.f;
	m_transparent = 0;
	m_applyUIScaling = TRUE;
	m_image_width = 24;
	m_image_height = 24;
}

// ============================================================================
ImgTag::~ImgTag()
{
	if(m_hBitmap) DeleteObject(m_hBitmap);
}

void ImgTag::gc_trace()
{
	RolloutControl::gc_trace();
	if (m_maxBitMap && m_maxBitMap->is_not_marked())
		m_maxBitMap->gc_trace();
	if (m_style && m_style->is_not_marked())
		m_style->gc_trace();
	if (m_iconName && m_iconName->is_not_marked())
		m_iconName->gc_trace();
}

// ============================================================================
TOOLINFO* ImgTag::GetToolInfo()
{
	static TOOLINFO ti;

	memset(&ti, 0, sizeof(TOOLINFO));
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hWnd;
	ti.uId = TOOLTIP_ID;
	ti.lpszText = const_cast<TCHAR*>(m_sToolTip.data());
	GetClientRect(m_hWnd, &ti.rect);

	return &ti;
}

// ============================================================================
LRESULT CALLBACK ImgTag::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImgTag *ctrl = DLGetWindowLongPtr<ImgTag*>(hWnd);
	if(ctrl == NULL && msg != WM_CREATE)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	if(ctrl && ctrl->m_hToolTip)
	{
		MSG ttmsg;
		ttmsg.lParam = lParam;
		ttmsg.wParam = wParam;
		ttmsg.message = msg;
		ttmsg.hwnd = hWnd;
		SendMessage(ctrl->m_hToolTip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&ttmsg);
	}

	switch(msg)
	{
	case WM_CREATE:
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			ctrl = (ImgTag*)lpcs->lpCreateParams;
			DLSetWindowLongPtr(hWnd, ctrl);
			break;
		}

	case WM_KILLFOCUS:
		if (ctrl->m_bMouseOver)
		{
			ctrl->m_bMouseOver = false;
			ctrl->run_event_handler(n_mouseout, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, false);
		}
		break;

	case WM_MOUSEMOVE:
		return ctrl->MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);

	case WM_LBUTTONDOWN:
		return ctrl->ButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, left);

	case WM_LBUTTONUP:
		return ctrl->ButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, left);

	case WM_LBUTTONDBLCLK:
		return ctrl->ButtonDblClk(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, left);

	case WM_RBUTTONDOWN:
		return ctrl->ButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, right);

	case WM_RBUTTONUP:
		return ctrl->ButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, right);

	case WM_RBUTTONDBLCLK:
		return ctrl->ButtonDblClk(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, right);

	case WM_MBUTTONDOWN:
		return ctrl->ButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, middle);

	case WM_MBUTTONUP:
		return ctrl->ButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, middle);

	case WM_MBUTTONDBLCLK:
		return ctrl->ButtonDblClk(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam, middle);

	case WM_ERASEBKGND:
		return ctrl->EraseBkgnd((HDC)wParam);

	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hWnd,&ps);
		ctrl->Paint(ps.hdc);
		EndPaint(hWnd,&ps);
		return FALSE;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return FALSE;
}

void ImgTag::run_event_handler (Value* event, int xPos, int yPos, int fwKeys, bool capture)
{
	if (capture) SetFocus(m_hWnd);
	HashTable* event_handlers= (HashTable*)parent_rollout->handlers->get(event);
	Value*   handler = NULL;
	if (event_handlers && (handler = event_handlers->get(name)) != NULL && (parent_rollout->flags & RO_CONTROLS_INSTALLED))
	{
		ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
		MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
		ScopedPushPlugin scopedPushPlugin(parent_rollout->plugin, _tls);
		one_value_local_tls(ehandler);
		vl.ehandler = handler->eval_no_wrapper();
		if (is_maxscriptfunction(vl.ehandler) && ((MAXScriptFunction*)vl.ehandler)->parameter_count == 2)
		{
			Value** args;
			value_local_array_tls(args, 2);
			if (m_applyUIScaling)
			{
				xPos = GetValueUIUnscaled(xPos);
				yPos = GetValueUIUnscaled(yPos);
			}
			args[0] = new Point2Value(xPos, yPos);
			args[1] = Integer::intern(fwKeys);
			RolloutControl::run_event_handler(parent_rollout, event, args, 2);
		}
		else
			RolloutControl::run_event_handler(parent_rollout, event, NULL, 0);
	}
	if (capture) SetCapture(m_hWnd);
}

// ============================================================================
LRESULT ImgTag::ButtonDown(int xPos, int yPos, int fwKeys, button which)
{
	switch (which)
	{
	case left:
		run_event_handler (n_mousedown, xPos, yPos, fwKeys);
		run_event_handler (n_lbuttondown, xPos, yPos, fwKeys);
		break;
	case middle:
		run_event_handler (n_mbuttondown, xPos, yPos, fwKeys);
		break;
	case right:
		run_event_handler (n_rightClick, xPos, yPos, fwKeys);
		run_event_handler (n_rbuttondown, xPos, yPos, fwKeys);
		break;
	}
	return TRUE;
}

// ============================================================================
LRESULT ImgTag::ButtonUp(int xPos, int yPos, int fwKeys, button which)
{
	switch (which)
	{
	case left:
		run_event_handler (n_mouseup, xPos, yPos, fwKeys);
		run_event_handler (n_click, xPos, yPos, fwKeys);
		run_event_handler(n_lbuttonup, xPos, yPos, fwKeys);
		break;
	case middle:
		run_event_handler(n_mbuttonup, xPos, yPos, fwKeys);
		break;
	case right:
		run_event_handler(n_rbuttonup, xPos, yPos, fwKeys);
		break;
	}
	return TRUE;
}

// ============================================================================
LRESULT ImgTag::ButtonDblClk(int xPos, int yPos, int fwKeys, button which)
{
	switch (which)
	{
	case left:
		run_event_handler(n_dblclick, xPos, yPos, fwKeys);
		run_event_handler(n_lbuttondblclk, xPos, yPos, fwKeys);
		break;
	case middle:
		run_event_handler(n_mbuttondblclk, xPos, yPos, fwKeys);
		break;
	case right:
		run_event_handler(n_rbuttondblclk, xPos, yPos, fwKeys);
		break;
	}
	return TRUE;
}

// ============================================================================
LRESULT ImgTag::MouseMove(int xPos, int yPos, int fwKeys)
{
	if(m_bMouseOver)
	{
		RECT rect;
		POINT pnt = {xPos,yPos};
		GetClientRect(m_hWnd, &rect);
		if(!PtInRect(&rect, pnt))
		{
			m_bMouseOver = false;
			ReleaseCapture();
			run_event_handler(n_mouseout, xPos, yPos, fwKeys, false);
		}
	}
	else
	{
		m_bMouseOver = true;
		run_event_handler(n_mouseover, xPos, yPos, fwKeys, false);
		SetCapture(m_hWnd);
	}

	return TRUE;
}

// ============================================================================
LRESULT ImgTag::EraseBkgnd(HDC hDC)
{
	return 1;
}

// ============================================================================
LRESULT ImgTag::Paint(HDC hDC)
{
	SetBkMode(hDC, TRANSPARENT);

	if(!m_hBitmap)
		return TRUE;

	BITMAP bmi;
	GetObject(m_hBitmap, sizeof(BITMAP), &bmi);
	int width  = bmi.bmWidth;
	int height = bmi.bmHeight;

	RECT rect;
	GetClientRect(m_hWnd, &rect);

	HDC hMemDC = CreateCompatibleDC(hDC);
	SelectObject(hMemDC, m_hBitmap);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = (BYTE)((1.f - m_opacity) * 255.f);
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// quick patch to fix build breakage. Need to fix..
	// bf.AlphaFormat = AC_SRC_NO_PREMULT_ALPHA|AC_DST_NO_ALPHA;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	bf.AlphaFormat = 0x03;

	if(m_style == n_bmp_tile)
	{
		for(int x = 0; x < rect.right; x+=width)
			for(int y = 0; y < rect.bottom; y+=height) 
			{
				TransparentBlt(hDC, x, y, width, height, hMemDC, 0, 0, width, height, m_transparent);
				AlphaBlend(hDC, x, y, width, height, hMemDC, 0, 0, width, height, bf);
			}
	}
	else if(m_style == n_bmp_center)
	{
		TransparentBlt(hDC, (rect.right-width)>>1, (rect.bottom-height)>>1,
			width, height, hMemDC, 0, 0, width, height, m_transparent);
		AlphaBlend(hDC, (rect.right-width)>>1, (rect.bottom-height)>>1,
			width, height, hMemDC, 0, 0, width, height, bf);
	}
	else /* n_stretch */
	{
		TransparentBlt(hDC, 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, width, height, m_transparent);
		AlphaBlend(hDC, 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, width, height, bf);
	}

	DeleteDC(hMemDC);

	return FALSE;
}

// ============================================================================
void ImgTag::Invalidate()
{
	if (parent_rollout != NULL && parent_rollout->page != NULL && m_hWnd != NULL)
	{
		RECT rect;
		GetClientRect(m_hWnd, &rect);
		MapWindowPoints(m_hWnd, parent_rollout->page, (POINT*)&rect, 2);
		InvalidateRect(parent_rollout->page, &rect, TRUE);
		InvalidateRect(m_hWnd, NULL, TRUE);
	}
}

// ============================================================================
int ImgTag::SetBitmap(Value* val)
{
	if(val == nullptr || val == &undefined || val == &unsupplied)
	{
		if(m_hBitmap) DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
		m_maxBitMap = NULL;
	}
	else if (is_string(val))
	{
		m_iconName = val;
		int height = m_applyUIScaling ? MAXScript::GetUIScaledValue(m_image_height) : m_image_height;
		int width = m_applyUIScaling ? MAXScript::GetUIScaledValue(m_image_width) : m_image_width;

		QIcon icon = MaxSDK::LoadMaxMultiResIcon( m_iconName->to_filename() );
		HBITMAP new_map = QtHelpers::HBITMAPFromQIcon(icon, width, height);
		if (new_map)
		{
			if(m_hBitmap) 
				DeleteObject(m_hBitmap);
			m_hBitmap = new_map;
			m_maxBitMap = NULL;
		}
	}
	else
	{
		HWND hWnd = MAXScript_interface->GetMAXHWnd();

		MAXBitMap* mbm = (MAXBitMap*)val;
		type_check(mbm, MAXBitMap, _T("set .bitmap"));
		m_maxBitMap = val;
		m_iconName = NULL;

		HDC hDC = GetDC(hWnd);
		PBITMAPINFO bmi = mbm->bm->ToDib(32, NULL, FALSE, TRUE);
		if(m_hBitmap) 
			DeleteObject(m_hBitmap);
		m_hBitmap = CreateDIBitmap(hDC, &bmi->bmiHeader, CBM_INIT, bmi->bmiColors, bmi, DIB_RGB_COLORS);
		
		if (m_applyUIScaling)
		{
			HBITMAP scaledBitmap = MaxSDK::GetUIScaledBitmap(m_hBitmap);
			if (scaledBitmap)
			{
				DeleteObject(m_hBitmap);
				m_hBitmap = scaledBitmap;
			}
		}
		LocalFree(bmi);
		ReleaseDC(hWnd, hDC);
	}

	Invalidate();
	return 1;
}

// ============================================================================
visible_class_instance (ImgTag, "ImgTag")

void ImgTag::add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y)
{
	caption = caption->eval();

	const TCHAR *text = caption->eval()->to_string();
	control_ID = next_id();
	parent_rollout = ro;

	Value *val = control_param(tooltip);
	if(val != &unsupplied)
		m_sToolTip = val->to_string();

	val = control_param(style);
	if(val != &unsupplied)
		m_style = val;

	Value* v = nullptr;
	m_applyUIScaling = bool_control_param(applyUIScaling, v, TRUE);

	val = control_param(iconSize);
	if (val != &unsupplied)
	{
		Point2 p = val->to_point2();
		m_image_width = (int)p.x;
		m_image_height = (int)p.y;
	}

	val = control_param(bitmap);
	if(val != &unsupplied)
		SetBitmap(val);

	val = control_param(iconName);
	if(val != &unsupplied)
		SetBitmap(val);

	val = control_param(transparent);
	if(val != &unsupplied)
		m_transparent = val->to_colorref();
	else
		m_transparent = RGB(0,0,0);

	val = control_param(opacity);
	if(val != &unsupplied)
	{
		m_opacity = val->to_float();
		if(m_opacity < 0.f) m_opacity = 0.f;
		if(m_opacity > 1.f) m_opacity = 1.f;
	}

	layout_data pos;
	setup_layout(ro, &pos, current_y);

	if(m_maxBitMap)
	{
		MAXBitMap *mbm = (MAXBitMap*)m_maxBitMap;
		pos.width = mbm->bi.Width();
		pos.height = mbm->bi.Height();
		// xPos and yPos coming in are 'true' screen space. If we are applying UI scaling,  then
		// we want to convert from screen space to 100% scaling space, and we call GetValueUIUnscaled. 
		// If not applying UI scaling, we want to remain in screen space.
		if (!m_applyUIScaling)
		{
			pos.width = GetValueUIUnscaled(pos.width);
			pos.height = GetValueUIUnscaled(pos.height);
		}
	}
	else if (m_iconName)
	{
		BITMAP bmi;
		GetObject(m_hBitmap, sizeof(BITMAP), &bmi);
		pos.width = GetValueUIUnscaled(bmi.bmWidth);
		pos.height = GetValueUIUnscaled(bmi.bmHeight);
	}

	process_layout_params(ro, &pos, current_y);

	m_hWnd = CreateWindow(
		IMGTAG_WINDOWCLASS,
		text,
		WS_VISIBLE | WS_CHILD | WS_GROUP,
		GetUIScaledValue(pos.left), GetUIScaledValue(pos.top), GetUIScaledValue(pos.width), GetUIScaledValue(pos.height),
		parent, (HMENU)control_ID, g_hInst, this);

	SendDlgItemMessage(parent, control_ID, WM_SETFONT, (WPARAM)ro->font, 0L);

	m_hToolTip = CreateWindow(
		TOOLTIPS_CLASS,
		TEXT(""), WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_hWnd, (HMENU)NULL, g_hInst, NULL);

	SendMessage(m_hToolTip, TTM_ADDTOOL, 0, (LPARAM)GetToolInfo());
}

void ImgTag::adjust_control(int& current_y)
{
	if (parent_rollout == nullptr || parent_rollout->page == nullptr)
		return;

	layout_data pos;
	setup_layout(parent_rollout, &pos, current_y);

	if (m_maxBitMap)
	{
		MAXBitMap *mbm = (MAXBitMap*)m_maxBitMap;
		pos.width = mbm->bi.Width();
		pos.height = mbm->bi.Height();
		// xPos and yPos coming in are 'true' screen space. If we are applying UI scaling,  then
		// we want to convert from screen space to 100% scaling space, and we call GetValueUIUnscaled. 
		// If not applying UI scaling, we want to remain in screen space.
		if (!m_applyUIScaling)
		{
			pos.width = GetValueUIUnscaled(pos.width);
			pos.height = GetValueUIUnscaled(pos.height);
		}
	}
	else if (m_iconName)
	{
		BITMAP bmi;
		GetObject(m_hBitmap, sizeof(BITMAP), &bmi);
		pos.width = GetValueUIUnscaled(bmi.bmWidth);
		pos.height = GetValueUIUnscaled(bmi.bmHeight);
	}

	process_layout_params(parent_rollout, &pos, current_y);

	HWND hwnd = GetDlgItem(parent_rollout->page, control_ID);
	if (hwnd)
	{
		InvalidateRect(hwnd, NULL, TRUE);
		SetWindowPos(hwnd, NULL, GetUIScaledValue(pos.left), GetUIScaledValue(pos.top), GetUIScaledValue(pos.width), GetUIScaledValue(pos.height), SWP_NOZORDER);
	}
}

// ============================================================================
BOOL ImgTag::handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

// ============================================================================
Value* ImgTag::get_property(Value** arg_list, int count)
{
	Value* prop = arg_list[0];

	if(prop == n_width)
	{
		if(parent_rollout && parent_rollout->page)
		{
			HWND hWnd = GetDlgItem(parent_rollout->page, control_ID);
			RECT rect;
			GetWindowRect(hWnd, &rect);
			MapWindowPoints(NULL, parent_rollout->page, (LPPOINT)&rect, 2);
			return_value (Integer::intern(GetValueUIUnscaled(rect.right-rect.left)));
		}
		else return &undefined;
	}
	else if(prop == n_height)
	{
		if(parent_rollout && parent_rollout->page)
		{
			HWND hWnd = GetDlgItem(parent_rollout->page, control_ID);
			RECT rect;
			GetWindowRect(hWnd, &rect);
			MapWindowPoints(NULL, parent_rollout->page, (LPPOINT)&rect, 2);
			return_value (Integer::intern(GetValueUIUnscaled(rect.bottom-rect.top)));
		}
		else return &undefined;
	}
	else if(prop == n_tooltip)
	{
		if(parent_rollout && parent_rollout->page)
			return new String(m_sToolTip);
		else
			return &undefined;
	}
	else if(prop == n_style)
	{
		if(parent_rollout && parent_rollout->page)
			return m_style;
		else
			return &undefined;
	}
	else if(prop == n_bitmap)
	{
		if(parent_rollout && parent_rollout->page)
			return m_maxBitMap ? m_maxBitMap : &undefined;
		else
			return &undefined;
	}
	else if(prop == n_opacity)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (Float::intern(m_opacity));
		else
			return &undefined;
	}
	else if(prop == n_transparent)
	{
		if(parent_rollout && parent_rollout->page)
			return new ColorValue(m_transparent);
		else
			return &undefined;
	}
	else if (prop == n_applyUIScaling)
	{
		if(parent_rollout && parent_rollout->page)
			return bool_result( m_applyUIScaling );
		else
			return &undefined;
	}
	else if (prop == n_iconName)
	{
		if(parent_rollout && parent_rollout->page)
			return m_iconName ? m_iconName : &undefined;
		else
			return &undefined;
}
	else if (prop == n_iconSize)
	{
		if (parent_rollout && parent_rollout->page)
			return_value(new Point2Value(m_image_width, m_image_height));
		else
			return &undefined;
	}

	return RolloutControl::get_property(arg_list, count);
}

// ============================================================================
Value* ImgTag::set_property(Value** arg_list, int count)
{
	Value* val = arg_list[0];
	Value* prop = arg_list[1];

	if(prop == n_bitmap)
	{
		if(parent_rollout && parent_rollout->page)
			SetBitmap(val);
	}
	else if (prop == n_iconName)
	{
		if(parent_rollout && parent_rollout->page)
			SetBitmap(val);
	}
	else if (prop == n_iconSize)
	{
		Point2 p = val->to_point2();
		m_image_width = (int)p.x;
		m_image_height = (int)p.y;
		if (m_iconName)
			SetBitmap(m_iconName);
	}
	else if(prop == n_width)
	{
		if(parent_rollout && parent_rollout->page)
		{
			int width = val->to_int();
			HWND  hWnd = GetDlgItem(parent_rollout->page, control_ID);
			RECT  rect;
			GetWindowRect(hWnd, &rect);
			MapWindowPoints(NULL, parent_rollout->page,  (LPPOINT)&rect, 2);
			SetWindowPos(hWnd, NULL, rect.left, rect.top, GetUIScaledValue(width), rect.bottom-rect.top, SWP_NOZORDER);
			SendMessage(m_hToolTip, TTM_SETTOOLINFO, 0, (LPARAM)GetToolInfo());
		}
	}
	else if(prop == n_height)
	{
		if(parent_rollout && parent_rollout->page)
		{
			int height = val->to_int();
			HWND  hWnd = GetDlgItem(parent_rollout->page, control_ID);
			RECT  rect;
			GetWindowRect(hWnd, &rect);
			MapWindowPoints(NULL, parent_rollout->page,  (LPPOINT)&rect, 2);
			SetWindowPos(hWnd, NULL, rect.left, rect.top, rect.right-rect.left, GetUIScaledValue(height), SWP_NOZORDER);
			SendMessage(m_hToolTip, TTM_SETTOOLINFO, 0, (LPARAM)GetToolInfo());
		}
	}
	else if(prop == n_tooltip)
	{
		if(parent_rollout && parent_rollout->page)
		{
			m_sToolTip = val->to_string();
			SendMessage(m_hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)GetToolInfo());
		}
	}
	else if(prop == n_style)
	{
		if(parent_rollout && parent_rollout->page)
		{
			if(val == n_bmp_tile ||
				val == n_bmp_stretch ||
				val == n_bmp_center)
			{
				m_style = val;
				Invalidate();
			}
		}
	}
	else if(prop == n_opacity)
	{
		if(parent_rollout && parent_rollout->page)
		{
			m_opacity = val->to_float();
			if(m_opacity < 0.f) m_opacity = 0.f;
			if(m_opacity > 1.f) m_opacity = 1.f;
			Invalidate();
		}
	}
	else if(prop == n_transparent)
	{
		if(parent_rollout && parent_rollout->page)
		{
			m_transparent = val->to_colorref();
			Invalidate();
		}
	}
	else if (prop == n_text || prop == n_caption) // not displayed
	{
		const TCHAR *text = val->to_string(); // will throw error if not convertable
		caption = val->get_heap_ptr();
	}
	else if (prop == n_applyUIScaling)
	{
		if(parent_rollout && parent_rollout->page)
		{
			BOOL newVal = val->to_bool();
			if (newVal != m_applyUIScaling)
			{
				m_applyUIScaling = newVal;
				if (m_iconName)
					SetBitmap(m_iconName);
				else
					SetBitmap(m_maxBitMap);
			}
		}
	}
	else
		return RolloutControl::set_property(arg_list, count);

	return val;
}

// ============================================================================
void ImgTag::set_enable()
{
	if(parent_rollout != NULL && parent_rollout->page != NULL)
	{
		EnableWindow(m_hWnd, enabled);
		InvalidateRect(m_hWnd, NULL, TRUE);
	}
}

// ============================================================================
void ImgTagInit()
{
	static BOOL registered = FALSE;
	if(!registered)
	{
		WNDCLASSEX wcex;
		wcex.cbSize        = sizeof(WNDCLASSEX);
		wcex.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc   = ImgTag::WndProc;
		wcex.cbClsExtra    = 0;
		wcex.cbWndExtra    = 0;
		wcex.hInstance     = g_hInst;
		wcex.hIcon         = NULL;
		wcex.hIconSm       = NULL;
		wcex.hCursor       = LoadCursor(0, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName  = NULL;
		wcex.lpszClassName  = IMGTAG_WINDOWCLASS;

		if(!RegisterClassEx(&wcex))
			return;
		registered = TRUE;
	}

	install_rollout_control(Name::intern(_T("ImgTag")), ImgTag::create);
}

