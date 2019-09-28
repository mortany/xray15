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

#define I_CLIP_THING		Interface_ID(0x7f483689, 0x344762c1)
#define I_CLIP_ROOT			Interface_ID(0x657250b8, 0x228a3915)
#define I_CLIP_BRANCH		Interface_ID(0x37024b25, 0x1f521238)
#define I_CLIP_LIMB			Interface_ID(0x35c26092, 0x740e7704)
#define I_CLIP_PALM			Interface_ID(0x5c76069b, 0x175b213e)
#define I_CLIP_TAIL			Interface_ID(0x717565ee, 0x2f484558)

#define I_CLIP_HUB			Interface_ID(0x28530eaa, 0x2a525cc1)
#define I_CLIP_HUBGROUP		Interface_ID(0x568968f4, 0x5e006625)

enum ClipLayerMethod {
	LAYER_RELATIVE,
	LAYER_ABSOLUTE,
	LAYER_IGNORE,
	LAYER_RELATIVE_WORLD,
	LAYER_CATMOTION,
};

// [GB 20-Feb-2004] The colour of the catmotion layer.  Currently
// this layer is always at index 0.  And for v1.xx it will remain
// that way.  We now need this colour, because of colour modes.
#define CATMOTION_LAYER_COLOUR		RGB(0,170,90)

#define TRACK_DISPLAY_METHOD_ACTIVE			1
#define TRACK_DISPLAY_METHOD_CONTRIBUTING	2
#define TRACK_DISPLAY_METHOD_ALL			3

////////////////////////////////////////////////////////////////////////
// class LayerRange
//
// This is a basic container for storing the inclusive range of layer
// indices...  But of course you can use it for any kind of integer
// range you like.  However, it's not very well suited to ranges with
// negatives.
//
// The definition of 'empty' is the invalid range [0,-1], and the
// 'infinite' range is [0,INT_MAX].
//
// If it's really important to you, I'm sure you could template this
// class for any container you like...  Perhaps there's already one
// hidden in the dirty bowels of the C++ STL.
//
#define LAYER_RANGE_ALL			LayerRange(0,INT_MAX)
#define LAYER_RANGE_NONE		LayerRange(0,-1)

class LayerRange {
public:
	int nFirst, nLast;

	LayerRange(int first, int last) { Set(first, last); }

	inline const int& First() const { return nFirst; }
	inline const int& Last() const { return nLast; }
	inline int& First() { return nFirst; }
	inline int& Last() { return nLast; }

	inline const BOOL Contains(const int val) const { return val >= nFirst && val <= nLast; }
	inline const BOOL Empty() const { return nLast < nFirst; }

	inline void Set(const int first, const int last) { nFirst = first; nLast = last; }
	inline void SetEmpty() { Set(0, -1); }
	inline void Limit(const int first, const int last) { if (nFirst < first) nFirst = first; if (nLast > last) nLast = last; }
	inline void Limit(const LayerRange &r) { Limit(r.nFirst, r.nLast); }

	inline bool operator==(const LayerRange& r) const { return r.nFirst == nFirst && r.nLast == nLast; }
	inline bool operator!=(const LayerRange& r) const { return !operator==(r); }
};

////////////////////////////////////////////////////////////////////////
// ICATClipThing
//
class ICATClipRoot;

class ICATClipThing : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_THING; }

	virtual ICATClipThing *GetDaddy() const = 0;
	virtual ICATClipRoot *GetRoot() = 0;

	virtual void SetFlag(USHORT f) = 0;
	virtual void ClearFlag(USHORT f) = 0;
	virtual BOOL TestFlag(USHORT f) const = 0;

};

////////////////////////////////////////////////////////////////////////
// ICATClipRoot
//
class ECATParent;

class ICATClipHubGroup;
class Hub;
class CATActiveLayerChangeCallback;
class NLAInfo;
class ICATClipRoot;
//
//
//class ICATClipRoot : public BaseInterface
//{
//public:
//	Interface_ID GetID() { return I_CLIP_ROOT; }
//
//	virtual void SetFlag(USHORT f) = 0;
//	virtual void ClearFlag(USHORT f) = 0;
//	virtual BOOL TestFlag(USHORT f) const = 0;
//
//	virtual Control *AsControl() = 0;
//
//	// Accessors to layer information.
//	virtual NLAInfo*	GetLayer(int id) = 0;
//	virtual int			NumLayers() const = 0;
//	virtual int			GetLayerIndex(const TCHAR* name) const = 0;
//	virtual Control*	GetLayerWeightLocalControl()=0;
//
//
//	virtual ECATParent *GetCATParent() const = 0;
//	virtual void		SetCATParent(ECATParent *parent) = 0;
//	virtual class ICATParentTrans	*GetCATParentTrans() const=0;
//
//	// Operations to layers
//	virtual int		AppendLayer(const TSTR& name, ClipLayerMethod method) = 0;
//	virtual BOOL	InsertLayer(const TSTR& name, int at, ClipLayerMethod method) = 0;
//	virtual void	RemoveLayer(int at) = 0;
//	virtual int		Selected() const = 0;
//	virtual void	SelectLayer(int id) = 0;
//
//	// Update the UI
//	virtual void RefreshLayerRollout() = 0;
//
//	// Importing motion data...
//	virtual BOOL LoadHTR(TSTR filename, TSTR camfile=_T(""), BOOL quiet=FALSE) = 0;
//	virtual BOOL LoadBVH(TSTR filename, TSTR camfile=_T(""), BOOL quiet=FALSE) = 0;
//	virtual BOOL LoadBIP(TSTR filename, TSTR camfile=_T(""), BOOL quiet=FALSE) = 0;
//	virtual BOOL LoadFBX(TSTR filename, TSTR camfile=_T(""), BOOL quiet=FALSE) = 0;
//
//	// Register/unregister callbacks
//	virtual void RegisterActiveLayerChangeCallback(CATActiveLayerChangeCallback *pCallBack) = 0;
//	virtual void UnRegisterActiveLayerChangeCallback(CATActiveLayerChangeCallback *pCallBack) = 0;
//
//	virtual int  GetTrackDisplayMethod()=0;
//	virtual void SetTrackDisplayMethod(int n)=0;
//
//	// this is so CATParent can do all the layer operations
//	virtual ClassDesc2* GetClassDesc()=0;
//
//	virtual void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)=0;
//
//	// UI functions
//	virtual void ShowRangesView()=0;
//	virtual void ShowGlobalWeightsView()=0;
////	virtual void ShowLocalWeightsView(CATClipValue *localweights)=0;
//	virtual void ShowTimeWarpView()=0;
//	virtual void CloseRangesView()=0;
//	virtual void CloseGlobalWeightsView()=0;
//	virtual void CloseLocalWeightsView()=0;
//	virtual void CloseTimeWarpView()=0;
//	virtual void UpdateUI(BOOL refresh_trackviews=FALSE, BOOL due_to_undo=FALSE)=0;
//
//	virtual void BeginEditLayers(IObjParam *ip, ULONG flags, class CATClipValue* layerctrl=NULL, Animatable *prev=NULL)=0;
//	virtual void EndEditLayers(IObjParam *ip, ULONG flags, class CATClipValue* layerctrl=NULL, Animatable *next=NULL)=0;
//
//	virtual BOOL LayerHasContribution(int nLayer, TimeValue t, class LayerRange range, class CATClipWeights *localweights)=0;
//
//	virtual void PreResetCleanup()=0;
//};
//
//

////////////////////////////////////////////////////////////////////////
// ICATClipBranch
//
class ICATClipBranch : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_BRANCH; }

	virtual Control *AsControl() = 0;
	virtual Control *GetWeights() = 0;

	virtual Color GetActiveLayerColour(TimeValue t) = 0;
	virtual Color GetBlendedLayerColour(TimeValue t) = 0;
};

////////////////////////////////////////////////////////////////////////
// ICATClipPalm
//
class PalmTrans2;

class ICATClipPalm : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_PALM; }

	virtual Control *AsControl() = 0;
	virtual Control *GetWeights() = 0;
	virtual void SetOwner(PalmTrans2* pPalm) = 0;

	virtual int		GetNumDigits() const = 0;
	virtual int		GetNumDigitSegs(int nDigit) const = 0;
	virtual void	SetNumDigitSegs(int nDigit, int nNumSegs) = 0;

	virtual void InsertDigit(int nNumSegs, int nDigit = -1) = 0;
	virtual void RemoveDigit(int nDigit) = 0;
	// ST - 09/10/03 Lets try for an undo :-)
	virtual void RestoreDigit(int numSegs, int index) = 0;
	virtual void RedoDigit(int numSegs, int index) = 0;

	virtual Control *GetPalmRot() = 0;
	virtual Control *GetIKTargetTM() = 0;
	virtual Control *GetTargetAlign() = 0;
	virtual Control *GetPivotPos() = 0;
	virtual Control *GetPivotRot() = 0;
	virtual Control *GetDigitSegRot(int nDigit, int nSeg) = 0;
	virtual Control *GetIKFKRatio() = 0;

	virtual BOOL IsLeft() const = 0;
	virtual void SetIsLeft(BOOL bIsLeft) = 0;
};

////////////////////////////////////////////////////////////////////////
// ICATClipLimb
//
class LimbData2;

class ICATClipLimb : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_LIMB; }

	virtual Control *AsControl() = 0;
	virtual Control *GetWeights() = 0;

	virtual void SetOwner(LimbData2* pLimbData) = 0;
	virtual LimbData2 *GetLimbData() = 0;

	virtual int GetNumSegments() const = 0;
	virtual void SetNumSegments(int numSegs) = 0;

	virtual Control *GetSwivelAngle() = 0;
	virtual Control *GetBendAngle() = 0;
	//		virtual Control *GetIKTargetTM() = 0;

	virtual Control *GetCollarRot() = 0;
	virtual Control *GetSegmentRot(int nSeg) = 0;
	//		virtual Control *GetPalmRot() = 0;

	virtual ICATClipPalm *GetPalm() = 0;

	// IsArm() and IsLeg() are used to test the tri-state nature
	// of CAT limbs.  If both are true, the limb is an arm acting
	// as a leg (on quadripeds, for example).
	virtual BOOL IsArm() const = 0;
	virtual BOOL IsLeg() const = 0;
	virtual BOOL IsLeft() const = 0;

	virtual void SetIsLeg(BOOL bIsLeg) = 0;
	virtual void SetIsLeft(BOOL bIsLeft) = 0;
};

////////////////////////////////////////////////////////////////////////
// ICATClipHub
//
class Hub;

class ICATClipHub : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_HUB; }

	virtual Control *AsControl() = 0;
	virtual Control *GetWeights() = 0;
	virtual void SetOwner(Hub* pHub) = 0;

	virtual ICATClipHubGroup *GetHubGroup() = 0;

	virtual Control *GetHubTM() = 0;

};

////////////////////////////////////////////////////////////////////////
// ICATClipTail
//
class TailData;
class ICATClipTail : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_TAIL; }

	virtual Control *AsControl() = 0;
	virtual Control *GetWeights() = 0;
	virtual void SetOwner(TailData* pTail) = 0;
	virtual TailData* GetOwner() = 0;

	virtual int GetNumSegments() const = 0;
	virtual void SetNumSegments(int numSegs) = 0;

	virtual Control *GetSegmentRot(int nSeg) = 0;

};

////////////////////////////////////////////////////////////////////////
// ICATClipHubGroup
//
class ICATClipHubGroup : public BaseInterface
{
public:
	Interface_ID GetID() { return I_CLIP_HUBGROUP; }

	virtual Control *AsControl() = 0;
	virtual Control *GetWeights() = 0;
	virtual Hub *GetCATHub() = 0;

	virtual int NumArms() const = 0;
	virtual int NumLegs() const = 0;
	//		virtual int NumHeads() const = 0;
	virtual int NumTails() const = 0;

	virtual ICATClipLimb* AddLimb(LimbData2* pLimb, int nInsertAt = -1) = 0;
	virtual ICATClipTail* AddTail(TailData* pTail, int nInsertAt = -1) = 0;

	virtual void RemoveLimb(LimbData2* pLimbData) = 0;
	virtual void RemoveTail(TailData * pTailData) = 0;

	virtual void RemoveArm(int nArm) = 0;
	virtual void RemoveLeg(int nLeg) = 0;
	//		virtual void RemoveHead(int nTail) = 0;
	virtual void RemoveTail(int nTail) = 0;

	virtual ICATClipHub *GetHub() = 0;
	virtual ICATClipLimb *GetArm(int nArm) = 0;
	virtual ICATClipLimb *GetLeg(int nLeg) = 0;
	//		virtual ICATClipHead *GetHead(int nHead) = 0;
	virtual ICATClipTail *GetTail(int nTail) = 0;

	virtual Control *GetDangle() = 0;

	virtual void FlagAsRibcageGroup() = 0;
	virtual void FlagAsPelvisGroup() = 0;
	virtual void FlagAsHeadGroup() = 0;
	virtual BOOL IsRibcageGroup() = 0;
	virtual BOOL IsPelvisGroup() = 0;
	virtual BOOL IsHeadGroup() = 0;

	virtual int NumChildHubGroups() = 0;
	virtual ICATClipHubGroup* GetChildHubGroup(int nChild) = 0;
	virtual ICATClipHubGroup* GetParentHubGroup() = 0;
};

#define GetClipThingInterface(ctrl) ((ICATClipThing*)((BaseInterface*)ctrl)->GetInterface(I_CLIP_THING))
#define GetClipHubGroupInterface(ctrl) ((ICATClipHubGroup*)((BaseInterface*)ctrl)->GetInterface(I_CLIP_HUBGROUP))
#define GetClipHubInterface(ctrl) ((ICATClipHub*)((BaseInterface*)ctrl)->GetInterface(I_CLIP_HUB))
#define GetClipLimbInterface(ctrl) ((ICATClipLimb*)((BaseInterface*)ctrl)->GetInterface(I_CLIP_LIMB))
#define GetClipTailInterface(ctrl) ((ICATClipTail*)((BaseInterface*)ctrl)->GetInterface(I_CLIP_TAIL))
#define GetClipPalmInterface(ctrl) ((ICATClipPalm*)((BaseInterface*)ctrl)->GetInterface(I_CLIP_PALM))

#define GetClipBranchInterface(ctrl) ((ICATClipBranch*)(ctrl)->AsControl()->GetInterface(I_CLIP_BRANCH))
