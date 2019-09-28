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

#include "FnPub/ILayerRootFP.h"
#include "ILayerInfo.h"
#include "NLAInfo.h"
#include "ICATParent.h"

#include "CATClipHierarchy.h"
#include "CATClipValue.h"

#include "ICATClipHierarchy.h"

//////////////////////////////////////////////////////////////////////////

class RootLayerChangeRestore;
class CATClipRoot;
class CATNodeControl;

#define CATCLIPROOT_CLASS_ID				Class_ID(0x742d30ac, 0x37984f09)

class ClipLayerInfo {
public:
	TSTR		strName;				// The name of the layer
	DWORD		dwFlags;				// Flags describing the properties of this layer
	COLORREF	dwLayerColour;			// The colour of the layer

	int			nParentLayerCache;		// This is managed by CATClipRoot

	ClipLayerInfo(const TSTR& name = _T(""), DWORD flags = 0) {
		strName = name;
		dwFlags = flags;
		dwLayerColour = RGB(32 + (rand() % 224), 32 + (rand() % 224), 32 + (rand() % 224));
		nParentLayerCache = -1;
	}

	ClipLayerInfo Clone() const {
		ClipLayerInfo cli(*this);

		// When cloning a clip info from another layer, we take
		// everything except the ghost object pointer and the
		// caches.
		cli.nParentLayerCache = -1;

		return cli;
	}
};

//////////////////////////////////////////////////////////////////////
// CATClipRoot
//
class CATClipRoot : public Control, public ILayerRootFP
{
	//---> Begin Function Publishing stuff
public:
	virtual FPInterfaceDesc* GetDescByID(Interface_ID id);
	//	virtual	FPInterfaceDesc* GetDesc();

	ClipLayerMethod MethodFromString(const TCHAR *method);

	virtual int AppendLayer(const MSTR& name, const TCHAR *method);
	virtual BOOL InsertLayer(const MSTR& name, int layer, const MCHAR *method);
	virtual void RemoveLayer(int layer);
	virtual void MoveLayerUp(int layer);
	virtual void MoveLayerDown(int layer);
	virtual int NumLayers() const { DbgAssert(nNumLayers == tabLayers.Count()); return nNumLayers; };

	virtual Color GetLayerColor(int index);
	virtual BOOL SetLayerColor(int index, Color *newColor);

	virtual void SelectLayer(int layer);
	virtual void SelectNone();
	virtual int GetSelectedLayer() const;

	virtual void SoloLayer(int layer);
	virtual void SoloNone() { SoloLayer(-1); }
	virtual int GetSoloLayer() const;
	virtual BOOL IsLayerSolo(int layer) { return layer == GetSoloLayer(); }

	virtual BOOL SaveClip(const MSTR& filename, TimeValue start_t, TimeValue end_t, int from_layer, int to_layer);
	virtual BOOL SavePose(const MSTR& filename);

	// this is the function that is called by script and the rollout
	virtual INode* LoadClip(const MSTR& filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY);
	virtual INode* LoadPose(const MSTR& filename, TimeValue t, BOOL scale_data, BOOL transformData, BOOL mirrorData, BOOL mirrorWorldX, BOOL mirrorWorldY);
	virtual INode* CreatePasteLayerTransformNode();

	virtual TSTR GetFileTagValue(const MSTR& filename, const MSTR& tag);

	//if we are loading from scripts, we simply force bones only
	virtual BOOL LoadHTR(const MSTR& filename, const MSTR& camfile) { return LoadHTR(filename, camfile, TRUE); }
	virtual BOOL LoadBVH(const MSTR& filename, const MSTR& camfile) { return LoadBVH(filename, camfile, TRUE); }
	virtual BOOL LoadBIP(const MSTR& filename, const MSTR& camfile) { return LoadBIP(filename, camfile, TRUE); };
	virtual BOOL LoadFBX(const MSTR& filename, const MSTR& camfile) { return LoadFBX(filename, camfile, TRUE); };

	virtual void CollapsePoseToCurLayer(TimeValue t);
	virtual BOOL CollapseTimeRangeToLayer(TimeValue start_t, TimeValue end_t, TimeValue iKeyFreq, BOOL regularplot, int numpasses, float posdelta, float rotdelta);

	virtual int  GetTrackDisplayMethod() const;
	virtual void SetTrackDisplayMethod(int n);

	void	CopyLayer(int n);
	void	PasteLayer(BOOL instance, BOOL duplicatelayerinfo = TRUE) { PasteLayer(instance, duplicatelayerinfo, -1, NULL); }

	//<--- End Function Publishing stuff

	ClassDesc2* GetClassDesc() { return GetCATClipRootDesc(); }

protected:
	friend class CATClipRootPLCB;
	friend class RootLayerChangeRestore;

	// We not longer inherit off CATClipThing
	DWORD flags;
	const static int nNumValues = 0;
	const static int nNumBranches = 0;

	int nNumLayers;
	int nSelected;
	int nSolo;

	int nNumHubGroups;
	int nNumProps;

	// A parent layer is the first absolute layer above a relative layer.
	// An absolute layer effectively controls all relative layers below it
	// until the next absolute layer.  The parent layer is cached in the
	// vecLayers array.  By definition the parent layer of an absolute
	// layer is -1 (or a relative layer with no parent, as is the case with
	// the CATMotion layer).  The following function MUST be called whenever
	// the structure or order of the layers changes.  We always maintain a
	// valid cache of the parent layers.  Note this function must also be
	// called during Load(), as the values are not saved.
	int GetLayerParent(int layer) { return GetParentLayer(layer - 1) + 1; }
	void RefreshParentLayerCache();

public:

	void NotifyActiveLayerChangeCallbacks();

	static IObjParam *ip;
	ULONG flagsbegin;

	DWORD dwFileVersion;

	ICATParentTrans *catparenttrans;

	int	idRangesView;
	int	idGlobalWeightsView;
	int	idLocalWeightsView;
	int	idTimeWarpView;

	// this is just an idea so far
	// but this pointer is used so the UI can
	// display the weights of a branch of the clip hierarchy.
	// Global * Local = Effective
	CATClipValue* localBranchWeights;
	CATClipValue* localLayerControl;

	DWORD  nClipTreeViewID;
	void RenameTreeView();

	virtual CATClipRoot* Root() { return this; }

public:
	BOOL LayerHasContribution(int nLayer, TimeValue t, LayerRange range, CATClipWeights *localweights);

	//
	// Inherited from ICATClipRoot
	//
	Control *AsControl() { return (Control*)this; }

	void SetFlag(USHORT f) { flags |= f; }
	void ClearFlag(USHORT f) { flags &= ~f; }
	BOOL TestFlag(USHORT f) const { return (flags & f) == f; }

	void SetType(USHORT t);
	void SetFlags(USHORT f);

	// Accessors to layer information.
	int GetLayerIndex(const TCHAR* name) const;
	ClipLayerMethod GetLayerMethod(ULONG id) const;
	ClipLayerMethod GetSelectedLayerMethod() const { return GetLayerMethod(nSelected); }

	float		GetLayerWeightLocal(int id, TimeValue t) const;
	void		SetLayerWeightLocal(int id, TimeValue t, float weight);
	// used by the ClipRollout to show the actual layer weight
	float		GetCombinedLayerWeightLocal(int id, TimeValue t);
	CATClipValue* GetLayerWeightLocalControl() { return	localBranchWeights; }

	void SetMaskedLayerFlag(int id, DWORD mask, DWORD flag) { if (tabLayers[id]) tabLayers[id]->SetMaskedFlag(mask, flag); }
	DWORD GetMaskedLayerFlag(int id, DWORD mask) const { return tabLayers[id] ? tabLayers[id]->GetMaskedFlag(mask) : 0; }
	BOOL TestMaskedLayerFlag(int id, DWORD mask, DWORD flag) const { return tabLayers[id] ? tabLayers[id]->TestMaskedFlag(mask, flag) : FALSE; }

	//////////////////////////////////////////////////////////////////////////
	// this returns an internally used data structure that
	// is really none of your business. Go away...
//		ClipLayerInfo &GetLayerInfo(int id){ return vecLayers[id]; }
	NLAInfo* GetLayer(int id) { return (id >= 0 && id < tabLayers.Count()) ? tabLayers[id] : NULL; }
	const NLAInfo* GetLayer(int id) const { return const_cast<CATClipRoot*>(this)->GetLayer(id); }
	NLAInfo *GetSelectedLayerInfo() { return (nSelected >= 0 && nSelected < tabLayers.Count()) ? tabLayers[nSelected] : NULL; }

	// CATParent pointer added 18-Jul-2003 GB
	ICATParentTrans *GetCATParentTrans() const { return catparenttrans; }

	// Operations to layers
	BOOL RenameLayer(int id, const TSTR& newName);
	int AppendLayer(const TSTR& name, ClipLayerMethod method);
	BOOL InsertLayer(const TSTR& name, int at, ClipLayerMethod method);
	BOOL CloneLayer(int id, const TSTR& newName);
	//void RemoveLayer(int at);
	//void MoveLayerUp(int id);
	//void MoveLayerDown(int id);

	// Stuff to do with layer child relationships
	int GetParentLayer(int nLayer) { return (nLayer >= 0 && tabLayers[nLayer]) ? tabLayers[nLayer]->GetParentLayer() : -1; }

	// Stuff to do with the active layer.
	//int GetSelectedIndex() const { return nSelected; }
	//int Selected() const { return nSelected; }	// Derived for Fn Publishing
	//void SelectLayer(int id);
	//void SelectNone();

	//void SoloLayer(int id);
	//int GetSoloLayer() { return nSolo; }
	BOOL WeightHierarchyEnabled(int id) { return !GetLayer(id)->TestFlag(LAYER_DISABLE_WEIGHTS_HIERARCHY); }

	// Importing motion data...
	BOOL LoadHTR(TSTR filename, TSTR camfile = _T(""), BOOL quiet = FALSE);
	BOOL LoadBVH(TSTR filename, TSTR camfile = _T(""), BOOL quiet = FALSE);
	BOOL LoadBIP(TSTR filename, TSTR camfile = _T(""), BOOL quiet = FALSE);
	BOOL LoadFBX(TSTR filename, TSTR camfile = _T(""), BOOL quiet = FALSE);

	// Register/unregister callbacks
	void RegisterActiveLayerChangeCallback(CATActiveLayerChangeCallback *pCallBack);
	void UnRegisterActiveLayerChangeCallback(CATActiveLayerChangeCallback *pCallBack);

	void RefreshLayerRollout();

	// The layers manage thier own setup values and need to know setupmode
	CATMode GetCATMode() { return (catparenttrans ? catparenttrans->GetCATMode() : SETUPMODE); };

public:
	//
	// Inherited from CATClipThing
	//
	void Init();

	// Functions that traverse the hierarchy.
	void CATMessages(TimeValue t, UINT msg, int data, DWORD flags = CLIP_NOTIFY_ALL);

	// Used to implement the Max default range functions on a particular layer,
	// since we override the real ones to do weird things.
	Interval GetLayerTimeRange(int index, DWORD flags);
	void EditLayerTimeRange(int index, Interval range, DWORD flags);
	void MapLayerKeys(int index, TimeMap *map, DWORD flags);
	void SetLayerORT(int index, int ort, int type);

public:
	//
	// Inherited from Control
	//
	BaseInterface* GetInterface(Interface_ID id);

	// Construction
	CATClipRoot(BOOL loading = FALSE);
	virtual ~CATClipRoot();

	// Load/Save stuff
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// UI functions. (GB 04-Aug-03)
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	void BeginEditLayers(IObjParam *ip, ULONG flags, CATClipValue* layerctrl = NULL, Animatable *prev = NULL);
	void EndEditLayers(IObjParam *ip, ULONG flags, CATClipValue* layerctrl = NULL, Animatable *next = NULL);

	void ShowLayerRollouts(BOOL tf);

	// Even though this is defined in CATClipThing, by multiple-inheriting
	// Control through CATClipThing and ECATClipRoot, it thinks that it
	// has not been defined....
	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	// Class stuff
	Class_ID ClassID() { return CATCLIPROOT_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATCLIPROOT); }
	void DeleteThis() { delete this; }

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	// GetValue and SetValue
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(method);
		*((float*)val) = 0.0f; valid.SetInfinite();
	}
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit); UNREFERENCED_PARAMETER(method);
	}

	// These are the standard time range calls.
	virtual Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return NEVER; };
	virtual void EditTimeRange(Interval range, DWORD flags) { UNREFERENCED_PARAMETER(range); UNREFERENCED_PARAMETER(flags); };
	virtual void MapKeys(TimeMap *map, DWORD flags) { UNREFERENCED_PARAMETER(map); UNREFERENCED_PARAMETER(flags); };

	RefTargetHandle Clone(RemapDir& remap);

public:
	//
	// all new Layer Root stuff for CAT V2.0
	//
	BOOL upgraded;
	Tab<NLAInfo*> tabLayers;

	virtual int NumRefs();// { return nNumValues + nNumBranches + nNumLayers; }
	virtual int NumSubs() { return nNumValues + nNumBranches + nNumLayers; }

	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	///////////////////////////////////////////////////////////////
	// CAT2 Stuff
	// Very useful methods for the undo system
	void	ResizeList(int n, BOOL loading = FALSE);
	void	InsertController(int n, ClipLayerMethod method);
	void	RemoveController(int n);
	void	MoveControllerUp(int n);
	void	MoveControllerDown(int n);

	// UI functions
	void ShowRangesView();
	void ShowGlobalWeightsView();
	void ShowLocalWeightsView(CATClipValue *localweights);
	void ShowTimeWarpView();
	void CloseRangesView();
	void CloseGlobalWeightsView();
	void CloseLocalWeightsView();
	void CloseTimeWarpView();
	void UpdateUI(BOOL refresh_trackviews = FALSE, BOOL due_to_undo = FALSE);

	void PreResetCleanup();

	// CAT2 Upgrade stuff
	Point3 GetLayerScale(int i)const { UNREFERENCED_PARAMETER(i);  return Point3(1.0f, 1.0f, 1.0f); }

	float GetWeight(TimeValue t, int id, Interval& valid) { return tabLayers[id] ? tabLayers[id]->GetWeight(t, valid) : 0.0f; }

	BOOL	IsLayerCopied();
	void PasteLayer(BOOL instance, BOOL duplicatelayerinfo = TRUE, int targetLayerIdx = -1, CATCharacterRemap* pRemap = NULL);
	void	AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	virtual DWORD GetFileSaveVersion() {
		if (dwFileVersion > 0)return dwFileVersion;
		if (catparenttrans)
			return catparenttrans->GetFileSaveVersion();

		return 0;
	}

#if (defined(_DEBUG) || defined(_HYBRID))
	// This bool is used to disable the consistency check
	// in CATClipValue::NumLayers when resizing our layer count
	bool bIsModifyingLayers;
#endif

private:
	CATNodeControl* GetRootHub();
};
