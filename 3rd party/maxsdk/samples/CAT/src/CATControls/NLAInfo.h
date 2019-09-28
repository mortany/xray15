//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2009 Autodesk, Inc.  All rights reserved.
//  Copyright 2003 Character Animation Technologies.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "ILayerInfo.h"
#include "ICATParent.h"

#include "./ICATClipHierarchy.h"
#include "../DataRestoreObj.h"
#include <SingleWeakRefMaker.h>

class CATClipRoot;
class NLAInfo;

extern INLAInfoClassDesc* GetNLAInfoDesc();
extern INLAInfoClassDesc* GetMocapLayerInfoDesc();

#define NLAINFO_CLASS_ID Class_ID(0x43bf76ac, 0x60ab5707)

// These are layer flags.  The first 3 bits store the clip layer
// method, which tells us how the clip values are interpolated.
#define LAYER_METHOD_MASK	0x07

//#define LAYER_MOCAP			(1<<3)
#define LAYER_DISABLE			(1<<4)
#define LAYER_NO_SOLO			(1<<5)
#define LAYER_LOCKED			(1<<6)

#define LAYER_RANGEUNLOCKED		(1<<7)
#define LAYER_MANUAL_ORT_IN		(1<<8)
#define LAYER_MANUAL_ORT_OUT	(1<<9)

//#define LAYER_CATMOTION			(1<<12)

// This is so we can disable the weight hierarchy
// for this layer and only use the local weights.
#define LAYER_DISABLE_WEIGHTS_HIERARCHY	(1<<13)

#define LAYER_REMOVE_DISPLACEMENT		(1<<14)
#define LAYER_DISPLAY_GHOST		(1<<15)
#define LAYER_MAKE_LOOPABLE		(1<<16)

class CATRigWriter;
class CATRigReader;
class CATClipMatrix3;
class CATClipFloat;
class NLAInfoChangeRestore;

class NLAInfo : public ILayerInfo, IDataRestoreOwner<COLORREF>, IDataRestoreOwner<TSTR>
{
protected:

	friend class NLAInfoChangeRestore;
	friend class NLAInfoEditTimeRangeRestore;

	//////////////////////////////////////////////////////////////////////////
	// Class NLAInfo
	TSTR			strName;			// The name of the layer
	DWORD			dwFlags;			// Flags describing the properties of this layer
	COLORREF		dwColour;			// The colour of the layer
	int				index;

	Interval	range;				// when using manual ranges we use this Interval
	int			ort_in, ort_out;

	Interval	loop_range;

	Control*	weight;		// weights for this layer
	Control*	timewarp;		// weight view - shows weights hierarchy for each layer
	Control*	transform;
	Interval	iTransform;
	Matrix3		tmTransform, tmInvOriginal;
	INode*		transform_node;		// Pointer to our ghost, or NULL

	MaxSDK::SingleWeakRefMaker m_root;

	int			nParentLayerCache;	// This is managed by NLAInfo

	Box3 bbox;
	Point3	 root_hub_displacement;
	Interval root_hub_displacement_valid;

public:

	DWORD dwFileVersion;

	// Be very carefull when changing this.
	// CATMotinlayers maintain 1 extra reference.
	// changing this number would stop catmotion layers from loading
	enum {
		REF_WEIGHT,
		REF_TIMING,
		REF_TRANSFORM,
		REF_TRANSFORMNODE,
		NUM_REFS
	};

	///////////////////////////////////////////////////////////////////////
	// Inherited from Control
	//

	// Construction
	void Init();
	NLAInfo(BOOL loading = FALSE);
	virtual ~NLAInfo();

	// Load/Save stuff
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//////////////////////////////////////////////////////////////////////////
	//From Animatable
	Class_ID ClassID() { return NLAINFO_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_NLAINFO); }

	void Copy(Control *ctrl) { UNREFERENCED_PARAMETER(ctrl); };
	RefTargetHandle Clone(RemapDir &remap);
	void CloneNLAInfo(NLAInfo* nlainfo, RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);
	int NumSubs() { return NUM_REFS; };
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);

	// TODO: Maintain the number or references here
	int NumRefs() { return NUM_REFS; }
	RefTargetHandle GetReference(int i);
protected:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	BOOL AssignController(Animatable *control, int subAnim);

	ParamDimension* GetParamDimension(int i);

	virtual int SubNumToRefNum(int subNum) { return subNum; }

	int	NumParamBlocks() { return 0; }
	IParamBlock2* GetParamBlock(int) { return NULL; }
	IParamBlock2* GetParamBlockByID(BlockID) { return NULL; }

	void DeleteThis() { delete this; }

	// UI functions. (GB 04-Aug-03)
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL)
	{
		UNREFERENCED_PARAMETER(ip); UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(prev);
	};
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL)
	{
		UNREFERENCED_PARAMETER(ip); UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);
	};

	/////////////////////////////////////////////////////////////////////////////////
	// These are the standard time range calls.
	BOOL SupportTimeOperations();
	virtual Interval GetTimeRange(DWORD flags);
	virtual void EditTimeRange(Interval range, DWORD flags);
	virtual void MapKeys(TimeMap *map, DWORD flags);
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags);

	int  GetORT(int type);
	void SetORT(int ort, int type);

	void GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box);

	//////////////////////////////////////////////////////////////////////////////////

	// GetValue and SetValue
	void GetValueLocalTime(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValueLocalTime(TimeValue t, void *val, int commit, GetSetMethod method);

	void Extrapolate(Interval /*range*/, TimeValue /*t*/, void * /*val*/, Interval & /*valid*/, int /*type*/) {};
	void *CreateTempValue() { return NULL; };
	void DeleteTempValue(void * /*val*/) {};
	void ApplyValue(void * /*val*/, void * /*delta*/) {};
	void MultiplyValue(void * /*val*/, float /*m*/) {};

	//////////////////////////////////////////////////////////////////////////
	// INLAInfo
	///////////////////////////////////////////////
	// NLAInfo Methods

	// This method will be called directly after creating a new layer Info controller
	// New layer types may wish to over ride this method to initialise the creation of thier own
	virtual		void Initialise(CATClipRoot* root, const TSTR& name, ClipLayerMethod method, int index);
	virtual		void PostLayerCreateCallback();
	virtual		void PreLayerRemoveCallback() { if (transform_node) DestroyTransformNode(); };

	void				SetNLARoot(CATClipRoot* pNewRoot);;
	CATClipRoot*		GetNLARoot();
	ICATParentTrans*	GetCATParentTrans();

	virtual TSTR	GetName() { return strName; };
	virtual void	SetName(TSTR str);

	int			GetIndex() { return index; };
	void		SetIndex(int id) { index = id; };

	virtual		ClipLayerMethod GetMethod() const { return (ClipLayerMethod)GetMaskedFlag(LAYER_METHOD_MASK); }
	virtual		void SetMethod(ClipLayerMethod method) { SetMaskedFlag(LAYER_METHOD_MASK, method); };

	Matrix3		GetTransform(TimeValue t, Interval &valid);
	void		SetTransform(TimeValue t, Matrix3 tm);
	Point3		GetScale(TimeValue t, Interval &valid);
	Control*	GetTransformController() { return transform; };

	void		MakeLoopable(TimeValue t, INode* node, Matrix3 &tm, BOOL edit_pos, BOOL edit_rot);
	Interval	GetLoopRange() { return loop_range; }
	void		SetLoopRange(Interval range) { loop_range = range; }

	// these flags are used internally. We only have this method so we can save out the flags in the clip saver
	DWORD GetFlags() const { return dwFlags; }
	void  SetFlags(DWORD flgs) { dwFlags = flgs; }

	void	SetFlag(DWORD flag, BOOL on = TRUE, BOOL enable_undo = TRUE);
	void	ClearFlag(DWORD flag, BOOL enable_undo = TRUE);
	BOOL	TestFlag(DWORD flag) const { return (dwFlags & flag) == flag; }

	void	SetMaskedFlag(DWORD mask, DWORD flag) { dwFlags = (dwFlags&~mask) | flag; }
	DWORD	GetMaskedFlag(DWORD mask) const { return dwFlags&mask; }
	BOOL	TestMaskedFlag(DWORD mask, DWORD flag) const { return (dwFlags & mask) == flag; }

	// Enabled and soloed layers
	void EnableLayer(BOOL bEnable = TRUE);
	BOOL LayerEnabled() { return !TestFlag(LAYER_DISABLE); }
	BOOL LayerLocked() { return !TestFlag(LAYER_LOCKED); }

	Color	GetColour() { return Color(dwColour); }
	void	SetColour(Color clr);

	// The transform node can be used by CATMotion as the path node
	virtual BOOL	ApplyAbsolute() { return GetMethod() == LAYER_ABSOLUTE; }
	virtual BOOL	CanTransform() { return GetMethod() == LAYER_ABSOLUTE; }

	BOOL	TransformNodeOn() { return transform_node ? TRUE : FALSE; };
	INode*	GetTransformNode() { return transform_node; };
	virtual INode*	CreateTransformNode();
	virtual void	DestroyTransformNode();
	virtual void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	int			GetParentLayer() { return nParentLayerCache; }
	void		SetParentLayer(int pl) { nParentLayerCache = pl; }

	float		GetWeight(TimeValue t, Interval &weightvalid) { float val = 0.0f; if (weight) weight->GetValue(t, (void*)&val, weightvalid, CTRL_ABSOLUTE); return val; }
	void		SetWeight(TimeValue t, float val);
	Control*	GetWeightsController() { return weight; }

	void					CreateTimeController();
	virtual		TimeValue	GetTime(TimeValue t);
	virtual		void		SetTime(TimeValue t, float val);
	virtual		Control*	GetTimeController() { return timewarp; }

	virtual		BOOL	SaveClip(CATRigWriter *save, int flags, Interval timerange);
	virtual		BOOL	LoadClip(CATRigReader *load, Interval range, float dScale, int flags);
	virtual		BOOL	LoadPreCAT2Weights(CATRigReader *load, Interval range, float dScale, int flags);

	virtual		INLAInfoClassDesc* GetClassDesc() { return GetNLAInfoDesc(); };

	//////////////////////////////////////////////////////////////////////////
	// Function publishing
	virtual TSTR	GetLayerType() { // SA: This appears to be used in IO so I won't globalize
		switch (GetMethod()) {
		case LAYER_ABSOLUTE:		return _T("Absolute");
		case LAYER_RELATIVE:		return _T("Relative Local");
		case LAYER_RELATIVE_WORLD:	return _T("Relative World");
		};
		return GetString(IDS_INVALID_LYR_TYPE);
	}

	virtual BOOL	IGetTransformNodeOn() { return GetTransformNode() ? TRUE : FALSE; };
	virtual void	ISetTransformNodeOn(BOOL on) { if (on) CreateTransformNode(); else DestroyTransformNode(); };

	virtual BOOL	IGetRemoveDisplacementOn() { return TestFlag(LAYER_REMOVE_DISPLACEMENT); };
	virtual void	ISetRemoveDisplacementOn(BOOL on) {
		SetFlag(LAYER_REMOVE_DISPLACEMENT, on);
		if (on) iTransform = NEVER;
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		if (GetCATParentTrans())
			GetCATParentTrans()->UpdateCharacter();
	};

	virtual INode*	IGetCATParent();

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	//////////////////////////////////////////////////////////////////////////
	// DataRestoreOwner functions
	void UpdateUI();

	virtual void OnRestoreDataChanged(COLORREF val);
	virtual void OnRestoreDataChanged(TSTR val) { UpdateUI(); }
};
