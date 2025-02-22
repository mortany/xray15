/**********************************************************************
 *<
	FILE:			MXSCurveCtl.h

	DESCRIPTION:	MaxScript Curve Control extension DLX

	CREATED BY:		Ravi Karra

	HISTORY:		Created on 5/12/99

 *>	Copyright � 1997, All Rights Reserved.
 **********************************************************************/

#pragma once

#include "resource.h"
#include "icurvctl.h"

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include <maxscript/foundation/functions.h>
#include <maxscript/macros/local_class.h>
#include <maxscript\macros\generic_class.h>

#include <maxscript/UI/rollouts.h>

#pragma push_macro("ScripterExport")

#undef ScripterExport
#define ScripterExport

extern	HINSTANCE hInstance;
#define elements(_array) (sizeof(_array)/sizeof((_array)[0]))

DECLARE_LOCAL_GENERIC_CLASS(CCRootClassValue, CurveCtlGeneric);

class MAXCurve;
class MAXCurveCtl;

/* -------------------- CCRootClassValue  ------------------- */
// base class for all classes declared in MXSCurveCtl
local_visible_class (CCRootClassValue)
class CCRootClassValue : public MAXWrapper
{
public:
						CCRootClassValue() { }

	ValueMetaClass*		local_base_class() { return class_tag(CCRootClassValue); }
						classof_methods (CCRootClassValue, Value);
	void				collect() { delete this; }
	void				sprin1(CharStream* s) { s->puts(_T("CCRootClassValue\n")); }
	void				gc_trace() { Value::gc_trace(); }
#	define				is_ccroot(v) ((DbgVerify(!is_sourcepositionwrapper(v)), (v))->tag == class_tag(CCRootClassValue))

#include <maxscript\macros\local_abstract_generic_functions.h>
#	include "curvepro.h"
};

/* -------------------- CurvePointValue  ------------------- */
local_applyable_class (CurvePointValue)
class CurvePointValue : public Value
{
public:
	CurvePoint	cp;
	ICurve		*curve;
	ICurveCtl*	cctl;
	int			pt_index;

				CurvePointValue(CurvePoint pt);
				CurvePointValue(ICurveCtl*	cctl, ICurve *crv, int ipt);
	static Value* intern(ICurveCtl*	cctl, ICurve *crv, int ipt);
	void		sprin1(CharStream* s);
				classof_methods(CurvePointValue, Value);
	void		collect() { delete this; }
#	define		is_curvepoint(v) ((DbgVerify(!is_sourcepositionwrapper(v)), (v))->tag == class_tag(CurvePointValue))
	CurvePoint	to_curvepoint() { return cp; }

	Value*		get_property(Value** arg_list, int count);
	Value*		set_property(Value** arg_list, int count);

	//internal
	void		SetFlag(DWORD mask, Value* val) { 
					if(val->to_bool()) cp.flags|=(mask); else cp.flags &= ~(mask); }
};

/* -------------------- CurvePointsArray  ------------------- */
local_visible_class (CurvePointsArray)
class CurvePointsArray : public MAXWrapper
{
public:
	ICurve		*curve;
	ICurveCtl*	cctl;
				CurvePointsArray(ICurveCtl*	cctl, ICurve* crv);
	static Value* intern(ICurveCtl*	cctl, ICurve *crv);
				classof_methods (CurvePointsArray, Value);
	const TCHAR*		class_name() { return _T("CurvePointsArray"); }
	BOOL		_is_collection() { return 1; }
	void		collect() { delete this; }
	void		sprin1(CharStream* s);	
	
	/* operations */
#include <maxscript\macros\local_implementations.h>
#	include <maxscript\protocols\arrays.inl>
	Value*		map(node_map& m);

	/* built-in property accessors */

	def_property ( count );
};

/* -------------------- MAXCurve ------------------- */
local_visible_class (MAXCurve)
class MAXCurve : public CCRootClassValue // Should derive from MAXWrapper
{
public:
	ICurve*		curve;
	ICurveCtl*	cctl;

	// curve Properties
	COLORREF	color, dcolor;
	int			width, dwidth, style, dstyle;

				MAXCurve(ICurve* icurve, ICurveCtl* icctl=NULL);
	static Value* intern(ICurve* icurve, ICurveCtl* icctl=NULL);

	ValueMetaClass* local_base_class() { return class_tag(CCRootClassValue); } // local base class in this class's plug-in
				classof_methods(MAXCurve, MAXWrapper);
	const TCHAR*		class_name() { return _T("MAXCurve"); }
	void		collect() { delete this; }
	void		sprin1(CharStream* s);
	void		gc_trace();
#	define		is_curve(v) ((DbgVerify(!is_sourcepositionwrapper(v)), (v))->tag == class_tag(MAXCurve))

	Value*		get_property(Value** arg_list, int count);
	Value*		set_property(Value** arg_list, int count);
	void		Update();

	// operations 
#include <maxscript\macros\local_implementations.h>
#	include "curvepro.h"
};

/* -------------------- MAXCurveCtl  ------------------- */
class MSResourceMakerCallback : public ResourceMakerCallback, public ReferenceTarget
{
	MAXCurveCtl *mcc;
	public:
				MSResourceMakerCallback(MAXCurveCtl *cc);
				~MSResourceMakerCallback();
		void	ResetCallback(int curvenum, ICurveCtl *pCCtl);
		virtual void GetClassName(MSTR& s) { s = _M("MSResourceMakerCallback"); } // from Animatable
		RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, 
					PartID& partID, RefMessage message, BOOL propagate ) { return REF_SUCCEED; }
		void*	GetInterface(ULONG id) {		
					return (id==I_RESMAKER_INTERFACE) ? this : ReferenceMaker::GetInterface(id);
					}
};

visible_class_s (MAXCurveCtl, RolloutControl)

class MAXCurveCtl : public RolloutControl
{
public:	
	TypedSingleRefMaker<ICurveCtl> cctl;
	HWND		hFrame;
	DWORD		rcmFlags, uiFlags;
	bool		popup;
	float		xvalue;
	
				MAXCurveCtl(Value* name, Value* caption, Value** keyparms, int keyparm_count);
				~MAXCurveCtl();

    static RolloutControl* create(Value* name, Value* caption, Value** keyparms, int keyparm_count)
							{ return new MAXCurveCtl (name, caption, keyparms, keyparm_count); }
	
	ValueMetaClass* local_base_class() { return class_tag(CCRootClassValue); } // local base class in this class's plug-in
	void		gc_trace();
				classof_methods (MAXCurveCtl, RolloutControl);
	void		collect() { delete this; }
	void		sprin1(CharStream* s) { s->printf(_T("MAXCurveCtl:%s"), name->to_string()); }

	void		add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y);
	void		adjust_control(int& current_y) override;
	LPCTSTR		get_control_class() { return _T(""); }
	void		compute_layout(Rollout *ro, layout_data* pos) { }
	BOOL		handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam);
	int			num_controls() { return 2; }

	Value*		get_property(Value** arg_list, int count);
	Value*		set_property(Value** arg_list, int count);
	void		set_enable();
#include <maxscript\macros\local_implementations.h>
	use_local_generic(zoom,	"zoom");
};

#pragma pop_macro("ScripterExport")
