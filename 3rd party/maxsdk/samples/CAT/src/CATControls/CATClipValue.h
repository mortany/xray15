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

#include "../CATAPI.h"
#include "CatPlugins.h"

#include "CATClipHierarchy.h"
#include "CATToken.h"

#include "CATGroup.h"
#include <vector>

class ECATParent;
class CATClipValue;
class CATClipRoot;
class CATClipWeights;
class CATControl;
class CATClipValuePLCB;

extern CATClipValue* CreateClipValueController(ClassDesc2* desc, CATClipWeights* weights, ICATParentTrans* catparenttrans, BOOL loading);
extern CATClipWeights*	CreateClipWeightsController(CATClipWeights* parentweights, ICATParentTrans* catparenttrans, BOOL loading);

//////////////////////////////////////////////////////////////////////
// CATClipValue
//
//
class CATClipValue : public Control, public ILayerControlFP {
	//---> Begin Function Publishing Stuff
private:

	CATClipRoot*		m_pClipRoot;
	ICATParentTrans*	m_pCATParentTrans;

	// no copying!
	CATClipValue(const CATClipValue &ctrl);

protected:
	friend CATClipValuePLCB;
	friend class DeleteRestore;

	DWORD dwFileSaveVersion;
	Tab<Control*>	tabLayers;			// our layer values
	Control*		ctrlSetup;

	// USHORT only allows us to store 16 flags and we need more than that
	DWORD	flags;		// flags tell us various interesting things.
	USHORT	rigID;
	TSTR	newlayercallback;

	// keeps track of the panel our rollout is being displayed on
	static int flagsBegin;
	static int flagsEnd;

	// Class Interface pointer
	IObjParam *ipClip;

	Interval clipvalueValid;
	//GetSetMethod methodValid;

	//		CATClipThing*	daddy;	// parent node
	CATGroup*		layerbranch;

public:

	// This pointer will remove the dependance on Daddy and therefore the whole clip hierarchy
	// I have set up Load/Save but no actual code uses it yet
	// CATClipWeights derrives from this class so it has a weights pointer now too,
	// but it can use it to get to waddy->daddy->weights
	CATClipWeights* weights;

	void SetCATParentTrans(ICATParentTrans* pCATParentTrans);
	inline ICATParentTrans* GetCATParentTrans() { return m_pCATParentTrans; }
	inline CATClipRoot* GetRoot() { return m_pClipRoot; }
	inline const CATClipRoot* GetRoot() const { return m_pClipRoot; }
	virtual CATClipWeights* GetWeightsCtrl();// { return daddy ? daddy->weights : NULL; }
	inline int GetTrackDisplayMethod();

	CATMode GetCATMode();
	float GetCATUnits();

	virtual	void SetSetupVal(void* val) { UNREFERENCED_PARAMETER(val); };
	virtual void GetSetupVal(void* val) { UNREFERENCED_PARAMETER(val); };
	virtual void ResetSetupVal() {};

	virtual void HoldTrack() = 0;

	virtual IOResult SaveSetupval(ISave *) { return IO_OK; };
	virtual IOResult LoadSetupval(ILoad *) { return IO_OK; };

	USHORT GetRigID() { return rigID; }
	void SetRigID(USHORT id) { rigID = id; }

	// Our own functions

	void SetFlag(DWORD f, BOOL on = TRUE);// { if(on) flags |= f; else flags &= ~f; }
	void ClearFlag(DWORD f);/// { flags &= ~f; }
	BOOL TestFlag(DWORD f) const { return (flags & f) == f; }

	DWORD GetFlags() { return flags; }
	void SetFlags(DWORD f);

	virtual BOOL PasteRig(CATClipValue* pasteLayers, DWORD flags, float scalefactor)
	{
		UNREFERENCED_PARAMETER(pasteLayers); UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(scalefactor); return TRUE;
	};

	virtual float GetWeight(TimeValue t, int index, Interval& valid);

	virtual void CATMessage(TimeValue t, UINT msg, int data);

	int FindLayerIndexByAddr(Control* ptr);

	virtual void ResizeList(int n, BOOL loading = FALSE);

	int			GetNumLayers() const;
	Control*	GetLayer(int n) const;
	void		SetLayer(int n, Control *ctrl);
	Control*	GetSelectedLayer() const;
	ClipLayerMethod GetSelectedLayerMethod();
	NLAInfo*	GetSelectedNLAInfo();

	void		InsertController(int n, BOOL makenew);
	void		RemoveController(int n);
	void		MoveControllerUp(int n);
	void		MoveControllerDown(int n);

	void		SavePostLayerCallback();
	void		LoadPostLayerCallback();

	virtual BOOL LayerHasKeys(int layer) { return (layer == -1) ? FALSE : GetLayer(layer)->NumKeys() > 0; }

	virtual ClipLayerMethod GetLayerMethod(int layer);
	//		ClipLayerMethod CurrLayerMethod() { return (nSelected < nNumLayers) ?  GetLayerMethod(nSelected) : LAYER_IGNORE;}
	float			CurrLayerWeight(TimeValue t) { Interval iv = FOREVER; return GetWeight(t, GetSelectedIndex(), iv); }

	// Use this function to get the first absolute
	// layer with a weight of 100% (Stop evaluating layers
	// that will have no effect)
	int GetFirstActiveAbsLayer(TimeValue t, LayerRange range);

	int LayerNumToRefNum(int nLayer) const { return nLayer; }
	Control* GetSelectedLayerOrDummy();
	int GetSelectedIndex() const;
	BOOL AssignToSelectedLayer(Control *ctrl);

	Matrix3 GetTransform(int index, TimeValue t);

	virtual void CreateSetupController(BOOL tf) = 0;

	virtual void SaveClip(class CATRigWriter *save, int flags, Interval timerange, int layerindex);
	virtual BOOL LoadClip(class CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	//	void SaveKeys(CATRigWriter *save, TimeValue t, Control* ctrl, IKey key, USHORT idVal, void* val, USHORT idKey, int flags);

	// New Collapsing method for CAT2, involves actually looking at existing keyframes and collapsing where nesessarry
	//	BOOL CollapseLayers(INode* node, DWORD flags, Interval timerange, Interval layerrange, int tolayer);

	BOOL RemapConstraint(Control* copyctrl, Control* pastectrl);
	virtual BOOL PasteLayer(CATClipValue* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap);

protected:
	virtual Control* ControlOrDummy(Control* ctrl) { return ctrl; }
	virtual Control* NewWhateverIAmController() = 0;

public:

	void* GetInterface(ULONG id);

	// Construction
	CATClipValue(BOOL loading = FALSE);
	~CATClipValue();

	//	void DeleteMe();

	ParamDimension* GetParamDimension(int i);

	// Copy / Clone
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	// Copy/Paste of our subanims
	BOOL CanCopySubTrack(int subNum, Interval iv, DWORD flags) { DbgAssert(tabLayers[subNum]); return tabLayers[subNum]->CanCopyTrack(iv, flags); }
	BOOL CanPasteSubTrack(int subNum, TrackClipObject *cobj, Interval iv, DWORD flags) { DbgAssert(tabLayers[subNum]); return tabLayers[subNum]->CanPasteTrack(cobj, iv, flags); }
	TrackClipObject *CopySubTrack(int subNum, Interval iv, DWORD flags) { DbgAssert(tabLayers[subNum]); return tabLayers[subNum]->CopyTrack(iv, flags); }
	void PasteSubTrack(int subNum, TrackClipObject *cobj, Interval iv, DWORD flags) { DbgAssert(tabLayers[subNum]); tabLayers[subNum]->PasteTrack(cobj, iv, flags); }

	// Load/Save stuff
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// Class stuff
	Class_ID ClassID() = 0;
	SClass_ID SuperClassID() = 0;
	void GetClassName(TSTR& s) = 0;
	void DeleteThis() = 0;

	// GB 07-11-2003: Implemented these methods to fix the problem of
	// (TimeLine-right-click -> FilterKeys -> CurrentTransform) not showing
	// any keys.  The Current Transform filter uses the currently selected
	// transform tool (eg rotation) to determine which keys to test.  We
	// need to tell it where to take our controllers from.  I think if "All
	// Transform Keys" is selected, max calls IsKeyAtTime() instead.
	virtual Control *GetPositionController() { return NULL; }
	virtual Control *GetRotationController() { return NULL; }
	virtual Control *GetScaleController() { return NULL; }
	virtual BOOL SetPositionController(Control *) { return FALSE; }
	virtual BOOL SetRotationController(Control *) { return FALSE; }
	virtual BOOL SetScaleController(Control *) { return FALSE; }

	// References and subanims
	int NumSubs();// { return nNumLayers + 1; }
	int NumRefs();// { return nNumLayers + 1; }
	int SubNumToRefNum(int subNum);// { return subNum; }

	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);
	BOOL AssignController(Animatable *control, int subAnim);
	BOOL IsReplaceable() { return FALSE; };

	virtual BOOL IsAnimated();

	// Control-specific stuff that we don't care too much about
	// but should keep in mind.
	int IsKeyable();
	void CopyKeysFromTime(TimeValue src, TimeValue dst, DWORD flags);
	void AddNewKey(TimeValue t, DWORD flags);
	BOOL IsKeyAtTime(TimeValue t, DWORD flags);
	BOOL GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt);
	int GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags);
	int GetKeySelState(BitArray &sel, Interval range, DWORD flags);

	BOOL IsLeaf() { return FALSE; }
	virtual BOOL CanCopyAnim() { return TRUE; }

	// Miscellaneous subanim stuff that we don't care about
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	//		ParamDimension* GetParamDimension(int i) { return defaultDim; }

	// We override these methods to do special things with
	// key ranges.  This allows us to use the track view as
	// a non-linear animation editor.  Isn't that cool?!
	Interval GetLayerTimeRange(int index, DWORD flags);
	void EditLayerTimeRange(int index, Interval range, DWORD flags);
	void MapLayerKeys(int index, TimeMap *map, DWORD flag);
	void SetLayerORT(int index, int ort, int type);

	virtual Interval GetTimeRange(DWORD flags);
	virtual void EditTimeRange(Interval range, DWORD flags);
	virtual void MapKeys(TimeMap *map, DWORD flags);

	// TODO: Give these an actual CATClip implementation as well...
	virtual void BeginEditLayers(int nLayer, IObjParam *ip, ULONG flags, int catmode, Animatable *prev = NULL);
	virtual void EndEditLayers(int nLayer, IObjParam *ip, ULONG flags, int catmode, Animatable *next = NULL);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	// From control -- for apparatus manipulation
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box);
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void ActivateSubobjSel(int level, XFormModes& modes);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all);
	void ClearSelection(int selLevel);
	int SubObjectIndex(CtrlHitRecord *hitRec);
	void SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert);

	void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node);
	void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node);
	void SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);
	void SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin = FALSE);
	void SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin = FALSE);

	////////////////////////////////////////////////////////////////////////////
	// Published Functions
public:
	FPInterfaceDesc* GetDescByID(Interface_ID id);
	virtual BaseInterface* GetInterface(Interface_ID id);

	// ILayerControlFP
	//		virtual void		MakeLayerSetupValue(int i){};

	virtual Control*	GetLayerRoot();
	virtual Control*	GetLocalWeights();
	//virtual Control*	IGetCATControl();

	virtual BOOL		GetUseSetupController() { return (ctrlSetup ? TRUE : FALSE); };
	virtual void		SetUseSetupController(BOOL tf) { CreateSetupController(tf); };
	virtual Control*	GetSetupController() { return ctrlSetup; };

	virtual BOOL	GetRelativeBone() { return TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE); };
	virtual void	SetRelativeBone(BOOL tf) { SetFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE, tf); };

	virtual Control*	GetSelectedLayerCtrl();

	virtual void	BakeCurrentLayerSettings() { SavePostLayerCallback(); };

	virtual int				IGetNumLayers() { return GetNumLayers(); };
	virtual Tab <Control*>	IGetLayerCtrls() { return tabLayers; };
};
