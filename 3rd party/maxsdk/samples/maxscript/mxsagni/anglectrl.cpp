// ============================================================================
// AngleCtrl.cpp
// Copyright 1999 Discreet
// Created by Simon Feltman
// ----------------------------------------------------------------------------

#include <maxscript/maxscript.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/UI/rollouts.h>
#include <maxscript/foundation/colors.h>
#include <maxscript/compiler/parser.h>
#include <maxscript/maxwrapper/bitmaps.h>
#include <trig.h>
#include <WindowsX.h>

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

#define ANGLECTRL_WINDOWCLASS _T("ANGLECTRL_WINDOWCLASS")
#define TOOLTIP_ID 1

// ============================================================================
visible_class_s (AngleControl, RolloutControl)

class AngleControl : public RolloutControl
{
protected:
	HWND   m_hWnd;
	HWND   m_hToolTip;
	int    m_diameter;
	BOOL   m_lButtonDown;

	float  m_degrees;
	float  m_startDegrees;
	float  m_max;
	float  m_min;
	BOOL   m_dirCCW;
	BOOL   m_applyUIScaling;

	COLORREF m_color;
	HBRUSH   m_hBrush;
	MAXBitMap *m_maxBitMap;

public:
	// Static methods
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static RolloutControl* create(Value* name, Value* caption, Value** keyparms, int keyparm_count)
	{ return new AngleControl(name, caption, keyparms, keyparm_count); }

	// Property methods
	void  SetDegrees(float deg);
	float GetDegrees();

	void  SetRadians(float rad);
	float GetRadians();

	void  SetStartDegrees(float deg);
	float GetStartDegrees();

	void  SetStartRadians(float rad);
	float GetStartRadians();

	void  SetDiameter(int dia);
	int   GetDiameter();

	void  SetPoint(int x, int y, BOOL bNegative=FALSE); // x,y are ui unscaled values

	int   SetBitmap(Value *val);
	int   SetColor(Value *val);
	void  Invalidate();
	TOOLINFO* GetToolInfo();

	// Event handlers
	LRESULT LButtonDown(int xPos, int yPos, int fwKeys); // xPos,yPos are ui scaled values
	LRESULT LButtonUp(int xPos, int yPos, int fwKeys); // xPos,yPos are ui scaled values
	LRESULT MouseMove(int xPos, int yPos, int fwKeys); // xPos,yPos are ui scaled values
	LRESULT EraseBackground(HDC hDC);
	LRESULT Paint();

	// MAXScript event handlers
	void CallChangedHandler();

	AngleControl(Value* name, Value* caption, Value** keyparms, int keyparm_count);
	~AngleControl();

	classof_methods(AngleControl, RolloutControl);
	void     gc_trace();
	void     collect() { delete this; }
	void     sprin1(CharStream* s) { s->printf(_T("AngleControl:%s"), name->to_string()); }

	void     add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y);
	void     adjust_control(int& current_y) override;
	LPCTSTR  get_control_class() { return SPINNERWINDOWCLASS; }
	void     compute_layout(Rollout *ro, layout_data* pos, int& current_y);
	BOOL     handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam);

	int      num_controls() { return 2; }

	Value*   get_property(Value** arg_list, int count);
	Value*   set_property(Value** arg_list, int count);
	void     set_enable();
};

// ============================================================================
AngleControl::AngleControl(Value* name, Value* caption, Value** keyparms, int keyparm_count)
	: RolloutControl(name, caption, keyparms, keyparm_count)
{
	tag = class_tag(AngleControl);

	m_hWnd = NULL;
	m_hToolTip = NULL;
	m_diameter = 0;
	m_lButtonDown = FALSE;

	m_degrees = 0.f;
	m_startDegrees = 90.f;
	m_min = -360.0f;
	m_max = 360.0f;
	m_dirCCW = TRUE;
	m_applyUIScaling = TRUE;

	m_color = RGB(0,0,255);
	m_hBrush = NULL;
	m_maxBitMap = NULL;
}

AngleControl::~AngleControl()
{
	if(m_hBrush) DeleteObject(m_hBrush);
}

void AngleControl::gc_trace()
{
	RolloutControl::gc_trace();
	if (m_maxBitMap && m_maxBitMap->is_not_marked())
		m_maxBitMap->gc_trace();
}

// ============================================================================
TOOLINFO* AngleControl::GetToolInfo()
{
	static TOOLINFO ti;
	static TSTR str;

	str.printf(_T("%f%s"), m_degrees, MaxSDK::GetResourceStringAsMSTR(IDS_DEGREE_SYMBOL));

	memset(&ti, 0, sizeof(TOOLINFO));
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hWnd;
	ti.uId = TOOLTIP_ID;
	ti.lpszText = const_cast<TCHAR*>(str.data());
	GetClientRect(m_hWnd, &ti.rect);

	return &ti;
}

// ============================================================================
void AngleControl::SetPoint(int x, int y, BOOL bNegative/*=FALSE*/)
{
	// x,y are ui unscaled values
	float rad = m_diameter * 0.5f;

	Point2 point;
	point.x = x - rad;
	point.y = y - rad;
	point = Normalize(point);
	if (!m_dirCCW) point.y = -point.y;

	float m_newDegrees;
	m_newDegrees = RadToDeg(acos(point.x));
	if (point.y > 0.f) m_newDegrees = 360.f-m_newDegrees;

	m_newDegrees -= m_startDegrees;
	if (m_newDegrees < 0.f) m_newDegrees += 360.f;

	if (m_min >= 0.f && m_max >= 0.f) bNegative=FALSE;
	if (m_min <= 0.f && m_max <= 0.f) bNegative=TRUE;
	if(bNegative) m_newDegrees -= 360.f;

	BOOL newVal = FALSE;
	if (m_newDegrees >= m_min && m_newDegrees <= m_max)
	{
		newVal = TRUE;
	}
	else // if (m_degrees != m_min && m_degrees != m_max)
	{
		// figure out if m_degrees should be m_min, m_max, or 0
		float lo_min=m_min;
		float lo_max=m_max;
		if (!bNegative && lo_min < 0.0f && lo_max > 0.0f) lo_min=0.0f;
		else if (bNegative && lo_min < 0.0f && lo_max > 0.0f) lo_max=0.0f;
		Point2 pnt_min;
		pnt_min.x = rad + cos(DegToRad(lo_min+m_startDegrees))*rad;
		pnt_min.y = sin(DegToRad(lo_min+m_startDegrees))*rad;
		if (m_dirCCW)
			pnt_min.y = rad - pnt_min.y;
		else
			pnt_min.y += rad;
		Point2 pnt_max;
		pnt_max.x = rad + cos(DegToRad(lo_max+m_startDegrees))*rad;
		pnt_max.y = sin(DegToRad(lo_max+m_startDegrees))*rad;
		if (m_dirCCW)
			pnt_max.y = rad - pnt_max.y;
		else
			pnt_max.y += rad;
		Point2 origPoint = Point2((float)x,(float)y);
		if (Length(origPoint-pnt_min) < Length(origPoint-pnt_max))
			m_newDegrees=lo_min;
		else
			m_newDegrees=lo_max;
		newVal = TRUE;
	}

	if (newVal && m_newDegrees != m_degrees)
	{
		m_degrees = m_newDegrees;
		CallChangedHandler();
		SendMessage(m_hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)GetToolInfo());
		Invalidate();
	}
}

// ============================================================================
void AngleControl::SetDegrees(float deg)
{
	m_degrees = deg;
	if (m_degrees < m_min)
		m_degrees = m_min;
	else if (m_degrees > m_max)
		m_degrees = m_max;

	SendMessage(m_hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)GetToolInfo());
	Invalidate();
	CallChangedHandler();
}

// ============================================================================
float AngleControl::GetDegrees() { return m_degrees; }

// ============================================================================
void AngleControl::SetRadians(float rad)
{
	SetDegrees(RadToDeg(rad));
}

// ============================================================================
float AngleControl::GetRadians() { return DegToRad(m_degrees); }

// ============================================================================
void AngleControl::SetStartDegrees(float deg)
{
	m_startDegrees = deg;

	for (; m_startDegrees < 0.0f; m_startDegrees+=360.0f);
	for (; m_startDegrees >= 360.0f; m_startDegrees-=360.0f);

	Invalidate();
}

// ============================================================================
float AngleControl::GetStartDegrees() { return m_startDegrees; }

// ============================================================================
void AngleControl::SetStartRadians(float rad)
{
	SetStartDegrees(RadToDeg(rad));
}

// ============================================================================
float AngleControl::GetStartRadians() { return DegToRad(m_startDegrees); }

// ============================================================================
void AngleControl::SetDiameter(int dia)
{
	POINT pnt = {0,0};
	m_diameter = dia;

	MapWindowPoints(m_hWnd, parent_rollout->page, &pnt, 1);
	int uiScaledDiameter = GetUIScaledValue(m_diameter);
	MoveWindow(m_hWnd, pnt.x, pnt.y, uiScaledDiameter, uiScaledDiameter, TRUE);
	SendMessage(m_hToolTip, TTM_SETTOOLINFO, 0, (LPARAM)GetToolInfo());
}

// ============================================================================
int AngleControl::GetDiameter() { return m_diameter; }

// ============================================================================
int AngleControl::SetBitmap(Value *val)
{
	if(val == nullptr || val == &undefined || val == &unsupplied)
		return 0;

	type_check(val, MAXBitMap, _T("set .bitmap"));
	m_maxBitMap = (MAXBitMap*)val;
	PBITMAPINFO bmi = m_maxBitMap->bm->ToDib(24,NULL, FALSE, TRUE);
	HDC hDC = (parent_rollout && parent_rollout->rollout_dc) ? parent_rollout->rollout_dc : GetDC(MAXScript_interface->GetMAXHWnd());
	HBITMAP hBitmap = CreateDIBitmap(hDC, &bmi->bmiHeader, CBM_INIT, bmi->bmiColors, bmi, DIB_RGB_COLORS);
	if (m_applyUIScaling)
	{
		HBITMAP scaledBitmap = MaxSDK::GetUIScaledBitmap(hBitmap);
		if (scaledBitmap)
		{
			DeleteObject(hBitmap);
			hBitmap = scaledBitmap;
		}
	}
	
	if(!(parent_rollout && parent_rollout->rollout_dc))
		ReleaseDC(MAXScript_interface->GetMAXHWnd(), hDC);

	LocalFree(bmi);
	if(hBitmap)
	{
		if(m_hBrush) DeleteObject(m_hBrush);
		m_hBrush = CreatePatternBrush(hBitmap);
		DeleteObject(hBitmap);
		Invalidate();
	}
	return 1;
}

// ============================================================================
int AngleControl::SetColor(Value *val)
{
	if(val == &unsupplied)
		m_color = RGB(0, 0, 255);
	else
		m_color = val->to_colorref();

	if(m_hBrush) DeleteObject(m_hBrush);
	m_maxBitMap = NULL;
	m_hBrush = CreateSolidBrush(m_color);
	Invalidate();
	return 1;
}

// ============================================================================
void AngleControl::Invalidate()
{
	if (m_hWnd == NULL) return;
	int uiScaledDiameter = GetUIScaledValue(m_diameter);
	RECT rect = {0, 0, uiScaledDiameter, uiScaledDiameter};
	MapWindowPoints(m_hWnd, parent_rollout->page, (POINT*)&rect, 2);
	InvalidateRect(parent_rollout->page, &rect, TRUE);
	InvalidateRect(m_hWnd, NULL, TRUE);
}

// ============================================================================
void AngleControl::CallChangedHandler()
{
	ScopedMaxScriptEvaluationContext scopedMaxScriptEvaluationContext;
	MAXScript_TLS* _tls = scopedMaxScriptEvaluationContext.Get_TLS();
	one_value_local_tls(arg);
	vl.arg = Float::intern(m_degrees);
	run_event_handler(parent_rollout, n_changed, &vl.arg, 1);
}

// ============================================================================
LRESULT CALLBACK AngleControl::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	AngleControl *ctrl = DLGetWindowLongPtr<AngleControl*>(hWnd);
	if(ctrl == NULL && msg != WM_CREATE)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	bool inControl = false;
	if (ctrl)
	{
		POINT pt;
		GetCursorPos( &pt );
		ScreenToClient( hWnd, &pt );
		float rad = GetUIScaledValue(ctrl->m_diameter) * 0.5f;
		float x = pt.x - rad;
		float y = pt.y - rad;
		inControl = ((x*x + y*y) <= rad*rad);
	}

	if(ctrl && ctrl->m_hToolTip && (inControl || ctrl->m_lButtonDown))
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
			ctrl = (AngleControl*)lpcs->lpCreateParams;
			DLSetWindowLongPtr(hWnd, ctrl);
			break;
		}

	case WM_LBUTTONDOWN:
		return ctrl->LButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);

	case WM_LBUTTONUP:
		return ctrl->LButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);

	case WM_RBUTTONUP:
		{
			Rollout* ro = ctrl->parent_rollout;
			HashTable* event_handlers= (HashTable*)ro->handlers->get(n_rightClick);
			if (event_handlers && (event_handlers->get(ctrl->name) != NULL))
				ctrl->call_event_handler(ro, n_rightClick, NULL, 0);
			else
				ctrl->SetDegrees(0.f);
		}
		break;

	case WM_MOUSEMOVE:
		return ctrl->MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);

	case WM_ERASEBKGND:
		return ctrl->EraseBackground((HDC)wParam);

	case WM_PAINT:
		return ctrl->Paint();
	case WM_SETCURSOR:
		{
			if (reinterpret_cast<HWND>(wParam) == hWnd && LOWORD(lParam) == HTCLIENT) 
			{
				UINT mouseMsg = HIWORD(lParam); 
				if ( mouseMsg == WM_MOUSEMOVE && !ctrl->m_lButtonDown)
				{
					static HCURSOR crossCursor = LoadCursor(NULL, IDC_CROSS);
					static HCURSOR arrowCursor = LoadCursor(NULL, IDC_ARROW);
					::SetCursor(inControl ? crossCursor : arrowCursor);
				}
			}
		}
		return 0;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

// ============================================================================
LRESULT AngleControl::LButtonDown(int xPos, int yPos, int fwKeys)
{
	float rad = m_diameter * 0.5f;
	xPos = GetValueUIUnscaled(xPos);
	yPos = GetValueUIUnscaled(yPos);

	float x = xPos - rad;
	float y = yPos - rad;

	if((x*x + y*y) <= rad*rad)
	{
		m_lButtonDown = TRUE;
		::SetCursor(MAXScript_interface->GetSysCursor(SYSCUR_ROTATE));
		SetCapture(m_hWnd);

		if(fwKeys & MK_SHIFT || fwKeys & MK_CONTROL)
			SetPoint(xPos, yPos, TRUE);
		else
			SetPoint(xPos, yPos);
		return 0;
	}

	return 1;
}

// ============================================================================
LRESULT AngleControl::LButtonUp(int xPos, int yPos, int fwKeys)
{
	if(m_lButtonDown)
	{
		m_lButtonDown = FALSE;
		ReleaseCapture();
	}
	return 0;
}

// ============================================================================
LRESULT AngleControl::MouseMove(int xPos, int yPos, int fwKeys)
{

	if(m_lButtonDown && (fwKeys & MK_LBUTTON))
	{
		xPos = GetValueUIUnscaled(xPos);
		yPos = GetValueUIUnscaled(yPos);
		if(fwKeys & MK_SHIFT || fwKeys & MK_CONTROL)
			SetPoint(xPos, yPos, TRUE);
		else
			SetPoint(xPos, yPos);

		SetCapture(m_hWnd);
		POINT pnt;
		GetCursorPos(&pnt);
		SendMessage(m_hToolTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELPARAM(pnt.x, pnt.y));
	}
	return 0;
}

// ============================================================================
LRESULT AngleControl::EraseBackground(HDC hDC)
{
	return DefWindowProc(m_hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0);
}

// ============================================================================
LRESULT AngleControl::Paint()
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(m_hWnd, &ps);
	SetBkMode(hDC, TRANSPARENT);

	int uiScaledDiameter = GetUIScaledValue(m_diameter);
	float rad = uiScaledDiameter * 0.5f;

	POINT start;
	start.x= rad + cos(DegToRad(m_startDegrees))*rad + 0.5f;
	start.y= sin(DegToRad(m_startDegrees))*rad + 0.5f;
	if (m_dirCCW)
		start.y = rad - start.y;
	else
		start.y += rad;

	POINT end;
	end.x = rad + cos(DegToRad(m_degrees+m_startDegrees))*rad + 0.5f;
	end.y = sin(DegToRad(m_degrees+m_startDegrees))*rad + 0.5f;
	if (m_dirCCW)
		end.y = rad - end.y;
	else
		end.y += rad;

	if(IsWindowEnabled(m_hWnd))
	{
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		SelectBrush(hDC, m_hBrush);
	}
	else
	{
		SelectObject(hDC, GetStockObject(WHITE_PEN));
		SelectBrush(hDC, GetStockObject(LTGRAY_BRUSH));
	}

	if(m_degrees != 0.f)
	{
		BOOL drawCCW = TRUE;
		if (m_degrees <= 0.f) drawCCW = FALSE;
		if (!m_dirCCW) drawCCW = !drawCCW;
		if(drawCCW)
			Pie(hDC, 0, 0, uiScaledDiameter, uiScaledDiameter, start.x, start.y, end.x, end.y);
		else 
			Pie(hDC, 0, 0, uiScaledDiameter, uiScaledDiameter, end.x, end.y, start.x, start.y);
	}

	DeleteObject(SelectObject(hDC, GetStockObject(WHITE_PEN)));
	Arc(hDC, 0, 0, uiScaledDiameter, uiScaledDiameter, 0, uiScaledDiameter, uiScaledDiameter, 0);
	MoveToEx(hDC, rad, rad, NULL);
	LineTo(hDC, start.x, start.y);
	MoveToEx(hDC, rad, rad, NULL);
	LineTo(hDC, end.x, end.y);
	DeleteObject(SelectObject(hDC, GetStockObject(BLACK_PEN)));
	Arc(hDC, 0, 0, uiScaledDiameter, uiScaledDiameter, uiScaledDiameter, 0, 0, uiScaledDiameter);

	EndPaint(m_hWnd, &ps);
	return 0;
}

// ============================================================================
visible_class_instance (AngleControl, "AngleControl")

void
	AngleControl::compute_layout(Rollout *ro, layout_data* pos, int& current_y)
{
	setup_layout(ro, pos, current_y);

	const TCHAR*   label_text = caption->eval()->to_string();
	int label_height = (_tcslen(label_text) != 0) ? ro->text_height + SPACING_BEFORE - 2 : 0;

	Value *val;
	if((val = control_param(diameter)) != &unsupplied)
		m_diameter = val->to_int();
	else if((val = control_param(width)) != &unsupplied)
		m_diameter = val->to_int();
	else if((val = control_param(height)) != &unsupplied)
		m_diameter = val->to_int();
	else
		m_diameter = 64;

	pos->width  = m_diameter;
	process_layout_params(ro, pos, current_y);
	pos->height = label_height + m_diameter;
	current_y = pos->top + pos->height;
}

void AngleControl::add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y)
{
	caption = caption->eval();

	HWND  label;
	int      left, top, width, height;
	SIZE  size;
	const TCHAR *label_text = caption->eval()->to_string();

	parent_rollout = ro;
	control_ID = next_id();
	WORD label_id = next_id();

	Value *val;
	if((val = control_param(diameter)) != &unsupplied)
		m_diameter = val->to_int();
	else if((val = control_param(width)) != &unsupplied)
		m_diameter = val->to_int();
	else if((val = control_param(height)) != &unsupplied)
		m_diameter = val->to_int();
	else
		m_diameter = 64;

	val = control_param(degrees);
	if(val != &unsupplied)
		m_degrees = val->to_float();
	else
		m_degrees = 0.f;

	val = control_param(radians);
	if(val != &unsupplied)
		m_degrees = RadToDeg(val->to_float());

	bool haveValue = false;
	val = control_param(range);
	if (val == &unsupplied)
	{
		m_min = -360.0f; m_max = 360.0f;
	}
	else if (is_point3(val))
	{
		Point3 p = val->to_point3();
		m_min = p.x; m_max = p.y; m_degrees = p.z;
		haveValue = true;
	}
	else
		throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_ANGLE_RANGE_MUST_BE_A_VECTOR), val);

	val = control_param(startDegrees);
	if(val != &unsupplied)
		SetStartDegrees(val->to_float());
	else if (!haveValue)
		SetStartDegrees(0.f);

	val = control_param(startRadians);
	if(val != &unsupplied)
		SetStartDegrees(RadToDeg(val->to_float()));

	val = control_param(dir);
	if(val != &unsupplied)
	{
		if (val != n_CW && val != n_CCW)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ANGLE_DIR_BAD_VALUE), val);
		m_dirCCW = val != n_CW;
	}

	Value* v = nullptr;
	m_applyUIScaling = bool_control_param(applyUIScaling, v, TRUE);

	val = control_param(bitmap);
	if(val != &unsupplied)
		SetBitmap(val);
	else
		SetColor(control_param(color));

	m_lButtonDown = FALSE;

	layout_data pos;
	compute_layout(ro, &pos, current_y);
	left = pos.left; top = pos.top;

	// place optional label
	int label_height = (_tcslen(label_text) != 0) ? ro->text_height + SPACING_BEFORE - 2 : 0;
	DLGetTextExtent(ro->rollout_dc, label_text, &size, true); 
	width = min(size.cx, pos.width); height = ro->text_height;
	label = CreateWindow(_T("STATIC"),
		label_text,
		WS_VISIBLE | WS_CHILD | WS_GROUP,
		GetUIScaledValue(left), GetUIScaledValue(top), 
		GetUIScaledValue(width), GetUIScaledValue(height),
		parent, (HMENU)label_id, hInstance, NULL);

	// place angle box
	top = pos.top + label_height;
	width = pos.width;

	int uiScaledDiameter = GetUIScaledValue(m_diameter);
	m_hWnd = CreateWindow(
		ANGLECTRL_WINDOWCLASS,
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_GROUP,
		GetUIScaledValue(left), GetUIScaledValue(top), 
		uiScaledDiameter, uiScaledDiameter,
		parent, (HMENU)control_ID, g_hInst, this);

	m_hToolTip = CreateWindow(
		TOOLTIPS_CLASS,
		TEXT(""), WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_hWnd, (HMENU)NULL, g_hInst, NULL);

	SendMessage(label, WM_SETFONT, (WPARAM)ro->font, 0L);
	SendMessage(m_hToolTip, TTM_ADDTOOL, 0, (LPARAM)GetToolInfo());
}

void AngleControl::adjust_control(int& current_y)
{
	if (parent_rollout == nullptr || parent_rollout->page == nullptr)
		return;

	WORD angle_id = control_ID;
	WORD label_id = angle_id + 1;

	int   left, top, width, height;

	HWND label = GetDlgItem(parent_rollout->page, label_id);
	TSTR  label_text;
	if (label)
		label_text = GetWindowText(label);

	int label_height = (!label_text.isNull()) ? parent_rollout->text_height + SPACING_BEFORE - 2 : 0;

	layout_data pos;
	compute_layout(parent_rollout, &pos, current_y);

	// fit controls
	left = pos.left; top = pos.top;
	SIZE  size;
	DLGetTextExtent(parent_rollout->rollout_dc, label_text, &size, true);
	width = min(size.cx, pos.width); height = parent_rollout->text_height;

	if (label)
	{
		InvalidateRect(label, NULL, TRUE);
		SetWindowPos(label, NULL, GetUIScaledValue(left), GetUIScaledValue(top), GetUIScaledValue(width), GetUIScaledValue(height), SWP_NOZORDER);
	}

	HWND angle = GetDlgItem(parent_rollout->page, angle_id);
	if (angle)
	{
		top = pos.top + label_height;
		width = pos.width;
		int uiScaledDiameter = GetUIScaledValue(m_diameter);

		InvalidateRect(angle, NULL, TRUE);
		SetWindowPos(angle, NULL, GetUIScaledValue(left), GetUIScaledValue(top), uiScaledDiameter, uiScaledDiameter, SWP_NOZORDER);
	}
}

// ============================================================================
BOOL AngleControl::handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

// ============================================================================
Value* AngleControl::get_property(Value** arg_list, int count)
{
	Value* prop = arg_list[0];

	if(prop == n_diameter ||
		prop == n_width ||
		prop == n_height)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (Integer::intern(GetDiameter()));
		else
			return &undefined;
	}
	else if(prop == n_degrees)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (Float::intern(GetDegrees()));
		else
			return &undefined;
	}
	else if(prop == n_radians)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (Float::intern(GetRadians()));
		else
			return &undefined;
	}
	else if (prop == n_range)
	{
		if(parent_rollout && parent_rollout->page)
			return new Point3Value (m_min, m_max, GetDegrees());
		else
			return &undefined;
	}
	else if(prop == n_startDegrees)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (Float::intern(GetStartDegrees()));
		else
			return &undefined;
	}
	else if(prop == n_startRadians)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (Float::intern(GetStartRadians()));
		else
			return &undefined;
	}
	else if(prop == n_dir)
	{
		if(parent_rollout && parent_rollout->page)
			return m_dirCCW ? n_CCW : n_CW;
		else
			return &undefined;
	}
	else if(prop == n_color)
	{
		if(parent_rollout && parent_rollout->page)
			return_value (ColorValue::intern(AColor(m_color)));
		else
			return &undefined;
	}
	else if(prop == n_bitmap)
	{
		if(parent_rollout && parent_rollout->page)
			return m_maxBitMap ? (Value*)m_maxBitMap : &undefined;
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

	return RolloutControl::get_property(arg_list, count);
}

// ============================================================================
Value* AngleControl::set_property(Value** arg_list, int count)
{
	Value* val = arg_list[0];
	Value* prop = arg_list[1];

	if (prop == n_text || prop == n_caption)
	{
		const TCHAR* text = val->to_string();
		caption = val->get_heap_ptr();
		if (parent_rollout != NULL && parent_rollout->page != NULL)
			set_text(text, GetDlgItem(parent_rollout->page, control_ID +1), n_left);
	}  
	else if(prop == n_diameter ||
		prop == n_width ||
		prop == n_height)
	{
		if(parent_rollout && parent_rollout->page)
			SetDiameter(val->to_int());
	}
	else if(prop == n_degrees)
	{
		if(parent_rollout && parent_rollout->page)
			SetDegrees(val->to_float());
	}
	else if(prop == n_radians)
	{
		if(parent_rollout && parent_rollout->page)
			SetRadians(val->to_float());
	}
	else if (prop == n_range)
	{
		if(parent_rollout && parent_rollout->page)
		{
			Point3 p;
			if (is_point3(val))
				p = val->to_point3();
			else
				throw TypeError (MaxSDK::GetResourceStringAsMSTR(IDS_ANGLE_RANGE_MUST_BE_A_VECTOR), val);
			m_min = p.x; m_max = p.y; 
			SetDegrees(m_degrees = p.z);
		}
	}
	else if(prop == n_startDegrees)
	{
		if(parent_rollout && parent_rollout->page)
			SetStartDegrees(val->to_float());
	}
	else if(prop == n_startRadians)
	{
		if(parent_rollout && parent_rollout->page)
			SetStartRadians(val->to_float());
	}
	else if(prop == n_dir)
	{
		if (val != n_CW && val != n_CCW)
			throw RuntimeError (MaxSDK::GetResourceStringAsMSTR(IDS_ANGLE_DIR_BAD_VALUE), val);
		if(parent_rollout && parent_rollout->page)
			m_dirCCW = val != n_CW;
		Invalidate();
	}
	else if(prop == n_color)
	{
		if(parent_rollout && parent_rollout->page)
			SetColor(val);
	}
	else if(prop == n_bitmap)
	{
		if(parent_rollout && parent_rollout->page)
			SetBitmap(val);
	}
	else if (prop == n_applyUIScaling)
	{
		if(parent_rollout && parent_rollout->page)
		{
			BOOL newVal = val->to_bool();
			if (newVal != m_applyUIScaling)
			{
				m_applyUIScaling = newVal;
				SetBitmap(m_maxBitMap);
			}
		}
	}
	else
		return RolloutControl::set_property(arg_list, count);

	return val;
}

// ============================================================================
void AngleControl::set_enable()
{
	if(parent_rollout != NULL && parent_rollout->page != NULL)
	{
		EnableWindow(m_hWnd, enabled);
		Invalidate();
	}
}

// ============================================================================
void AngleCtrlInit()
{
	static BOOL registered = FALSE;
	if(!registered)
	{
		WNDCLASSEX wcex;
		wcex.style         = CS_HREDRAW | CS_VREDRAW;
		wcex.hInstance     = g_hInst;
		wcex.hIcon         = NULL;
		wcex.hCursor       = NULL;
		wcex.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
		wcex.lpszMenuName  = NULL;
		wcex.cbClsExtra    = 0;
		wcex.cbWndExtra    = 0;
		wcex.lpfnWndProc   = AngleControl::WndProc;
		wcex.lpszClassName = ANGLECTRL_WINDOWCLASS;
		wcex.cbSize        = sizeof(WNDCLASSEX);
		wcex.hIconSm       = NULL;

		if(!RegisterClassEx(&wcex))
			return;
		registered = TRUE;
	}

	install_rollout_control(Name::intern(_T("Angle")), AngleControl::create);
}
