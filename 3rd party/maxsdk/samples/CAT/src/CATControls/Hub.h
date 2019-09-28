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

// Transform Controller for the hub Rig Node. Hubs hold together limbs, tails and spines.

#pragma once

#include "CATNodeControl.h"
#include "CATGroup.h"
#include "FnPub/IHubFP.h"

class SpineData2;
class TailData2;

/////////////////////////////////////////////////////
// Hub
// Superclass for Pelvis and Ribcage trans controllers.
// Makes limbs just a little bit simpler(which is  a good thing)

class Hub : public CATNodeControl, public CATGroup, public IHubFP
{
private:
	friend class HubPLCB;

	//////////////////////////////////////////////////////////////////////////

	Matrix3 m_tmInSpineTarget;
	Quat mqReachCache;

	// pointers to our controls.
	CATClipValue *layerDangleRatio;
	IParamBlock2 *pblock;

	Box3 bbox;

public:
	// our subanim list
	enum PARAMLIST {
		PBLOCK,
		LAYERTRANS,
		LAYERDANGLERATIO,
		WEIGHTS,
		NUMPARAMS
	};
	// our subanim list
	enum SUBLIST {
		SUBTRANS,
		SUBDANGLE,
		SUBWEIGHTS,
		NUMSUBS
	};

	// our parameter block list.
	enum PBLIST {
		hubparams
	};

	// pblock enum
	enum {
		PB_CATPARENT,

		PB_NODE_DEPRECATED,
		PB_NAME,
		PB_COLOUR,

		PB_TMSETUP_DEPRECATED,
		PB_SCALE_DEPRECATED,
		PB_CLIPHIERARCHY_DEPRECATED,
		PB_CATHIERARCHY_DEPRECATED,

		PB_INSPINE,
		PB_SPINE_TAB,
		PB_TAIL_TAB,

		PB_LIMB_TAB,
		PB_GHOSTPARENT_DEPRECATED,
		PB_PBOBJ_DEPRECATED,
		PB_HUBFLAGS_DEPRECATED,
	};

	//
	// constructors.
	//
	Hub(BOOL loading = FALSE);
	~Hub();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return HUB_CLASS_ID; }
	int NumSubs() { return NUMSUBS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	void GetClassName(TSTR& s);

	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);
	void PostCloneNode();

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//
	// from class ReferenceMaker?
	//
	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	// Before passing to CATNodeControl, we may need to groom our SetValue
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// For the procedural spine sub-objet selection
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	//
	//	Class Hub
	//
	void		ISetInheritsRot(BOOL b);
	void		ISetInheritsPos(BOOL b);

	bool ApplyReach(TimeValue t, Interval& ivValid, Matrix3 &tmSpineTip, Matrix3& tmSpineResult, const Point3& p3HubScale);
	void CacheReachRotations(Tab<float> weights, Tab<Ray> &shiftDists, const Point3& p3OldHubPos, const Point3& p3NewHubPos);
	void ResetReachCache();
	void ApplyReachCache(Matrix3& tmWorld);

	void		KeyFreeform(TimeValue t, ULONG flags = KEY_ALL);

	// this is called by the limb when it
	// wants to update the hub as well
	void Update();

	//////////////////////////////////////////////////////////////////////////
	// CATGroup
	CATControl* AsCATControl() { return this; }

	Color GetGroupColour() { return pblock->GetColor(PB_COLOUR);; };
	void  SetGroupColour(Color clr) { pblock->SetValue(PB_COLOUR, 0, clr); };

	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT	  GetRigID() { return idHub; };
	CATGroup* GetGroup() { return this; };

	int NumLayerControllers() { return 2 + (layerDangleRatio ? 1 : 0) + NumLayerFloats(); };
	CATClipValue* GetLayerController(int i) {
		if (i == 0)	return (CATClipValue*)weights;
		if (i == 1)	return GetLayerTrans();
		if (i == 2) { if (layerDangleRatio) { return layerDangleRatio; } }
		return GetLayerFloat(i - (layerDangleRatio ? 3 : 2));
	};
	int		NumChildCATControls();
	CATControl*	GetChildCATControl(int i);
	CATControl*	GetParentCATControl();

	virtual INode*	GetBoneByAddress(TSTR address);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	void SetNode(INode* n);

	ICATParentTrans* FindCATParentTrans();

	CATNodeControl* FindParentCATNodeControl();

	virtual int				NumChildCATNodeControls();
	virtual CATNodeControl*	GetChildCATNodeControl(int i);

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	TSTR		GetName() { return pblock->GetStr(PB_NAME); };
	void		SetName(TSTR newname, BOOL quiet = FALSE);

	virtual void SetLayerTransFlag(DWORD f, BOOL tf);

	void UpdateUserProps();

	void UpdateUI();

	// Hubs do not offset their children
	void ApplyChildOffset(TimeValue t, Matrix3& tmChild) const { UNUSED_PARAM(tmChild); UNUSED_PARAM(t); };

	// Hubs on procedural spines do not inherit rotation from the final link in the
	// spine.
	void CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	// A hub may need to include reach (forces) from the limbs
	void CalcWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid);

	// Don't let the ribcage drift from the end of the spine.
	void ImposeLimits(const Matrix3 &tmOrigParent, Matrix3 &tmParent, Matrix3 &tmWorld, Point3& p3LocalScale);

	//////////////////////////////////////////////////////////////////////////
	// IHub
	virtual BOOL SaveClip(CATRigWriter* save, int flags, Interval timerange, int layerindex);
	virtual BOOL LoadClip(CATRigReader* load, Interval range, int layerindex, float dScale, int flags);

	virtual Matrix3	ApproxCharacterTransform(TimeValue t);

	virtual void	GettmOrient(TimeValue t, Matrix3& tm);
	virtual float	GetDangleRatio(TimeValue t);
	virtual Control* GetDangleCtrl() { return layerDangleRatio; };

	virtual BOOL LoadRig(CATRigReader *load);
	virtual BOOL SaveRig(CATRigWriter *save);

	virtual void Initialise(ICATParentTrans* catparent, BOOL loading, SpineData2* ctrlInSpine = NULL);

	SpineData2*		GetInSpine();
	void			SetInSpine(SpineData2* sp);

	void			SetSpine(int id, SpineData2 *spine);
	int				FindSpine(SpineData2 *spine);
	void			InsertSpine(int id, SpineData2* spine);
	SpineData2*		GetSpine(int id);

	virtual CatAPI::ISpine*	GetInISpine();
	virtual CatAPI::ISpine*	GetISpine(int id);
	virtual CatAPI::ISpine*	AddSpine(int numbones = DEFAULT_NUM_CAT_BONES, BOOL loading = FALSE);
	virtual int		GetNumSpines();
	virtual void	RemoveSpine(int id, BOOL deletenodes = TRUE);

	virtual int		GetNumTails() { return pblock->Count(PB_TAIL_TAB); }
	virtual ITail*	GetITail(int id);
	virtual ITail*	AddTail(int numbones = DEFAULT_NUM_CAT_BONES, BOOL loading = FALSE);
	virtual void	RemoveTail(int id, BOOL deletenodes = TRUE);

	TailData2*		GetTail(int id);
	void			SetTail(int id, TailData2 *aux);
	void			InsertTail(int id, TailData2 *tail);
	int				FindTail(ITail *tail) { for (int i = 0; i < GetNumTails(); i++) { if (GetITail(i) == tail) return i; }	return -1; }

	virtual CatAPI::ILimb*	GetILimb(int id);
	virtual CatAPI::ILimb*	AddArm(BOOL bCollarbone = TRUE, BOOL bPalm = TRUE);
	virtual CatAPI::ILimb*	AddLeg(BOOL bCollarbone = FALSE, BOOL bAnkle = TRUE);
	virtual int		GetNumLimbs();
	virtual void	RemoveLimb(int id, BOOL deletenodes = TRUE);

	LimbData2*	GetLimb(int id);
	void		SetLimb(int id, ReferenceTarget* limb);
	LimbData2*	AddLimb(BOOL loading = FALSE, ULONG flags = 0);
	int			FindLimb(LimbData2* limb) { for (int i = 0; i < GetNumLimbs(); i++) { if (GetLimb(i) == limb) return i; }	return -1; }

	virtual BOOL GetPinHub() { return TestCCFlag(CCFLAG_FB_IK_LOCKED); };
	virtual void SetPinHub(BOOL b) { SetCCFlag(CCFLAG_FB_IK_LOCKED, b); };

	BaseInterface* GetInterface(Interface_ID id);
};
