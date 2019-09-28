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

#include "CATHierarchy.h"
#include "CATHierarchyLeaf.h"

#include "CATMotionPresets.h"
#include "CATHierarchyFunctions.h"
#include "CATRigPresets.h"

 //
class ECATParent;
class ICATParentTrans;

#include <vector>

#define CATHIERARCHYROOT_CLASS_ID		Class_ID(0x507514e5, 0x30b07fad)
#define CATHIERARCHYROOT_INTERFACE		Interface_ID(0x28741055, 0xe820f7)
#define GetICATHierarchyRootInterface(cd) \
			((ICATHierarchyRoot*)cd->GetInterface(CATHIERARCHYROOT_INTERFACE))
enum {
	CATMOTION_FOOTSTEPS_CHANGED,
	CATMOTION_FOOTSTEPS_CREATE,
	CATMOTION_FOOTSTEPS_REMOVE,
	CATMOTION_FOOTSTEPS_RESET,
	CATMOTION_FOOTSTEPS_SNAP_TO_GROUND,
	CATMOTION_PATHNODE_SET
};
//****************************************************************

class ICATHierarchyRoot : public FPMixinInterface {	// PathConstraint now extends this class

public:

	enum {
		fnShowCATWindow,
		fnLoadPreset,
		fnSavePreset,
		fnAddLayer,
		fnRemoveLayer,
//		fnSetLayerName,

		fnCreateFootprints,
		fnResetFootprints,
		fnRemoveFootprints,
		fnSnapToGround,

		fnUpdateDistCovered,
		fnUpdateSteps,

//		propGetWalkOnSpot,
//		propSetWalkOnSpot,
		propGetFootStepMasks,
		propSetFootStepMasks,

		propGetManualUpdates,
		propSetManualUpdates
	}; // ids for the two published functions

	BEGIN_FUNCTION_MAP	// Describe my functions
		VFN_0(fnShowCATWindow, ICreateCATWindow);
		FN_2(fnLoadPreset, TYPE_BOOL, ILoadPreset, TYPE_TSTR, TYPE_BOOL);
		FN_1(fnSavePreset, TYPE_BOOL, ISavePreset, TYPE_TSTR);
		VFN_0(fnAddLayer, AddLayer);
		VFN_1(fnRemoveLayer, RemoveLayer, TYPE_INT);
//		VFN_2(fnSetLayerName,				SetLayerName,		TYPE_INT,	TYPE_TSTR);
//		VFN_1(fmSetWalkOnSpot,				SetWalkOnSpot,	TYPE_BOOL);
		VFN_0(fnCreateFootprints, ICreateFootPrints);
		VFN_1(fnResetFootprints, IResetFootPrints, TYPE_BOOL);
		VFN_1(fnRemoveFootprints, IRemoveFootPrints, TYPE_BOOL);
		FN_2(fnSnapToGround, TYPE_BOOL, ISnapToGround, TYPE_INODE, TYPE_BOOL);

		VFN_0(fnUpdateDistCovered, IUpdateDistCovered);
		VFN_0(fnUpdateSteps, IUpdateSteps);

//		PROP_FNS(propGetWalkOnSpot,		GetisWalkOnSpot,	propSetWalkOnSpot,		SetisWalkOnSpot,	TYPE_BOOL);
		PROP_FNS(propGetFootStepMasks, GetisStepMasks, propSetFootStepMasks, SetisStepMasks, TYPE_BOOL);
		PROP_FNS(propGetManualUpdates, GetisManualUpdates, propSetManualUpdates, SetisManualUpdates, TYPE_BOOL);

	// Flag the end of the description
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	//prototypes of published functions
	virtual void ICreateCATWindow() = 0;

	virtual BOOL ILoadPreset(const TSTR& filename, BOOL bNewLayer) = 0;
	virtual BOOL ISavePreset(const TSTR& filename) = 0;

	virtual int  AddLayer() = 0;
	virtual void RemoveLayer(int index) = 0;
	virtual void SetLayerName(const int index, const TSTR& name) = 0;

	virtual void ICreateFootPrints() = 0;
	virtual void IResetFootPrints(BOOL bOnlySelected) = 0;
	virtual void IRemoveFootPrints(BOOL bOnlySelected) = 0;
	virtual BOOL ISnapToGround(INode *ground, BOOL bOnlySelected) = 0;

	virtual void IUpdateDistCovered() = 0;
	virtual void IUpdateSteps() = 0;

	// properties

//		virtual BOOL GetisWalkOnSpot() const =0;
//		virtual void SetisWalkOnSpot(BOOL bWalkOnSpot)=0;
	virtual BOOL GetisStepMasks() const = 0;
	virtual void SetisStepMasks(BOOL bStepMasks) = 0;

	virtual BOOL GetisManualUpdates() const = 0;
	virtual void SetisManualUpdates(BOOL bManualUpdates) = 0;

};

//////////////////////////////////////////////////////////////////////
// CATHierarchyRoot
//
// Represents a the Root of our CAT hierarchy.
//

#define CATMOTIONFLAG_WALKONSPOT		(1<<1)
#define CATMOTIONFLAG_MANUALRANGES		(1<<2)
#define CATMOTIONFLAG_STEPMASKS			(1<<3)
#define CATMOTIONFLAG_MANUALUPDATES		(1<<4)

class CATHierarchyBranch2;
class NLAInfo;

class CATHierarchyRoot : public CATHierarchyBranch, public ICATHierarchyRoot
{
private:

	TimeChangeCallback *currWindowPane;
	INode* nodeGround;
	ICustButton* btnPath;
	ICustButton* btnWalkOnSpot;

	CATHierarchyBranch2* branchCopy;

public:
	// Switch to let notifies
	// from path node cause upates
	BOOL bCanUpdate;

	/* CATWindow undo method thingies */
	TimeChangeCallback** GetCurWindowPtrAddr() { return &currWindowPane; }
	void SetCurWindowPtr(TimeChangeCallback *ptr) { currWindowPane = ptr; }
	ICustButton **GetCurPathButton() { return &btnPath; };
	ICustButton **GetWalkOnSpotButton() { return &btnWalkOnSpot; }
	void SetPathButtons(ICustButton *btnPath, ICustButton *btnWalk) { this->btnPath = btnPath; btnWalkOnSpot = btnWalk; }
	void EnableWalkButton(BOOL onOff) { if (btnWalkOnSpot) btnWalkOnSpot->Enable(onOff); }
	/**/
	// Class stuff
	Class_ID ClassID() { return CATHIERARCHYROOT_CLASS_ID; }
	void GetClassName(TSTR& s);
	void DeleteThis() { delete this; }

	// Construction
	CATHierarchyRoot(BOOL loading = FALSE);
	~CATHierarchyRoot();
	void Init();

	/*
	 *	Parameter Block stuff
	 */
	enum CATRootPB {
		PB_BRANCHTAB,
		PB_BRANCHNAMESTAB,
		PB_EXPANDABLE,
		PB_BRANCHPARENT,
		PB_BRANCHINDEX,
		PB_BRANCHOWNER,

		PB_WEIGHTS,
		PB_CATPARENT,

		PB_PATH_NODE,
		PB_GROUND_NODE,
		PB_PATH_FACING,
		PB_MAX_STEP_LENGTH,
		PB_MAX_STEP_TIME,
		PB_CATMOTION_FLAGS,
		PB_KA_LIFTING,
		PB_ACTIVELEAF,

		PB_START_TIME,
		PB_END_TIME,

		// CAT2 stuff
		PB_STEP_EASE,
		PB_DISTCOVERED,
		PB_CATMOTIONHUB,
		PB_NLAINFO,

		// Walk Mode Stuff
		PB_DIRECTION,
		PB_GRADIENT,
		PB_WALKMODE
	};

	/*
	 *	pblock Accessors
	 */
	 //////////////////////////////////////////////////////////////////////////
	 // CATMotion Flags
	void SetFlag(USHORT f) { pblock->SetValue(PB_CATMOTION_FLAGS, 0, pblock->GetInt(PB_CATMOTION_FLAGS) | f); }
	void ClearFlag(USHORT f) { pblock->SetValue(PB_CATMOTION_FLAGS, 0, pblock->GetInt(PB_CATMOTION_FLAGS) & ~f); }
	BOOL TestFlag(USHORT f) const { return (pblock->GetInt(PB_CATMOTION_FLAGS) & f) == f; }

	virtual INode*	GetGroundNode() const { return pblock->GetINode(PB_GROUND_NODE); };
	virtual INode*	GetPathNode() const { return pblock->GetINode(PB_PATH_NODE); };
	void			SetPathNode(INode* node) { pblock->SetValue(PB_PATH_NODE, 0, node); }
	virtual TimeValue	GetMaxStepTime(TimeValue t) const { return (TimeValue)(pblock->GetFloat(PB_MAX_STEP_TIME, t) * GetTicksPerFrame()); };
	virtual float	GetMaxStepDist(TimeValue t);

	virtual float	GetDirection(TimeValue t) const { return pblock->GetFloat(PB_DIRECTION, t); };
	virtual float	GetGradient(TimeValue t) const { return pblock->GetFloat(PB_GRADIENT, t); };

	virtual float	GetKALifting(TimeValue t) const { return pblock->GetFloat(PB_KA_LIFTING, t); };;
	virtual void 	SetKALifting(TimeValue t, float val) const { pblock->SetValue(PB_KA_LIFTING, t, val); };;
	virtual Control* GetKALiftingCtrl() { return pblock->GetControllerByID(PB_KA_LIFTING); };
	virtual float	GetFWRatio(TimeValue t) const { return pblock->GetFloat(PB_PATH_FACING, t); };

	virtual BOOL	GetisStepMasks() const { return TestFlag(CATMOTIONFLAG_STEPMASKS); };
	void			SetisStepMasks(BOOL bStepMasks);//{ bStepMasks ? SetFlag(CATMOTIONFLAG_STEPMASKS) : ClearFlag(CATMOTIONFLAG_STEPMASKS); }

	virtual BOOL	GetisManualUpdates() const { return TestFlag(CATMOTIONFLAG_MANUALUPDATES); };
	void			SetisManualUpdates(BOOL bManualUpdates) { bManualUpdates ? SetFlag(CATMOTIONFLAG_MANUALUPDATES) : ClearFlag(CATMOTIONFLAG_MANUALUPDATES); }

	// Our own functions
	CATHierarchyLeaf* GetWeights() { return (CATHierarchyLeaf*)pblock->GetControllerByID(PB_WEIGHTS); };

	virtual ECATParent*			GetCATParent();
	virtual ICATParentTrans*	GetCATParentTrans();

	TimeValue GetStartTime() { return (TimeValue)pblock->GetFloat(PB_START_TIME) * GetTicksPerFrame(); };
	void  SetStartTime(float dStartTime);

	TimeValue GetEndTime() { return (TimeValue)pblock->GetFloat(PB_END_TIME) * GetTicksPerFrame(); };
	void  SetEndTime(float dEndTime);
	Interval  GetCATMotionRange() { return Interval(GetStartTime(), GetEndTime()); };

	virtual void Initialise(Control* clipRoot, NLAInfo *layerinfo, int index);
	CATHierarchyBranch2 *GetGlobalsBranch();
	CATHierarchyBranch2 *GetLimbPhasesBranch();

	CATHierarchyRoot*	GetCATRoot() { return this; };

	virtual BOOL SaveClip(CATRigWriter *save, int flags, Interval range);
	virtual BOOL LoadClip(CATRigReader *load, Interval range, int flags);

	/*
	 *	UI Methods
	 */
	HWND hCATWnd;
	// this pints to out track view window throughout CATWindow
	// TODO - Remove this pointer (we have no reason to store it)
	ITreeViewOps *pCATTreeView;

	RECT rcGraphRect;
	IOffScreenBuf* iGraphBuff;
	// Values used to scale the graph
	float minGraphY, maxGraphY;

	Tab <CATHierarchyBranch2*> tabGraphingBranches;

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);
	// CATWindow Graph painting methods
	//void ICreateCATWindow();
	HWND CreateCATWindow();
	void UnlinkCATWindow() { hCATWnd = NULL; }
	virtual void ICreateCATWindow() { hCATWnd = CreateCATWindow(); };
	virtual void IDestroyCATWindow() { if (hCATWnd) DestroyWindow(hCATWnd); };

	void AddGraphingBranch(CATHierarchyBranch* ctrlGraphingBranch);
	void ClearGraphingBranchs() { tabGraphingBranches.Resize(0); tabGraphingBranches.SetCount(0); }

	void SetGraphRect(const RECT& rcRect) { rcGraphRect = rcRect; }

	void GetYRange(float &minY, float &maxY);
	void DrawGraph(CATHierarchyBranch* ctrlActiveBranch, int nSelectedKey, HDC hGraphDC, const RECT& rcGraphRect);

	/*
	 *	Subanim Methods
	 */
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:
	int SubNumToRefNum(int subNum) { return subNum; }

	int NumSubs() { return 2 + GetNumBranches(); }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);
	BOOL AssignController(Animatable *control, int subAnim) { UNREFERENCED_PARAMETER(control); UNREFERENCED_PARAMETER(subAnim);   return FALSE; };

	// Control-specific stuff that we don't care too much about
	// but should keep in mind.
	int IsKeyable() { return FALSE; }
	BOOL IsLeaf() { return FALSE; }
	BOOL CanCopyAnim() { return TRUE; }
	// Copy / Clone
	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); };
	RefTargetHandle Clone(RemapDir& remap);

	// Miscellaneous subanim stuff that we don't care about
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	ParamDimension* GetParamDimension(int) { return defaultDim; }

	/*
	 *	Hierarchy functions
	 */
	 // GetValue and SetValue
	void GetValue(TimeValue, void *val, Interval &, GetSetMethod)
	{
		*(float*)val = 0.0f;
	};
	void SetValue(TimeValue, void *val, int commit, GetSetMethod)
	{
		UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit);
	};

	int GetActiveLayer() { return GetWeights()->GetActiveLayer(); }
	void SetActiveLayer(int index) { NotifyLeaves(CAT_LAYER_SELECT, index); }

	CATHierarchyBranch2 *GetCopiedBranch() { return branchCopy; }
	void SetCopiedBranch(CATHierarchyBranch2* branch) { this->branchCopy = branch; }

	void NotifyLeaves(UINT msg, int &data);
	void CATMotionMessage(UINT msg, int data = 0);
	void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	virtual int	 GetNumLayers() { return GetWeights()->GetNumLayers(); }
	virtual int  AddLayer();
	virtual void RemoveLayer(int index);

	virtual void SetLayerName(const int index, const TSTR& name) { GetWeights()->SetLayerName(index, name); }
	virtual TSTR GetLayerName(const int index) { return GetWeights()->GetLayerName(index); }

	void InsertLayer(int index);

	/**********************************************************************
	 * Loading and saving....
	 */
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	BOOL SavePreset(const TSTR& filename);
	BOOL LoadPreset(const TSTR& filename, const TSTR& name, BOOL bNewLayer);

	/************************************************************************/
	/*   PathNode Stuff                                                     */
	/************************************************************************/
	BOOL	bUpdatingSteps;
	BOOL	bUpdatingDistCovered;
	void	UpdateDistCovered();
	void	UpdateSteps();
	enum WalkMode {
		WALK_ON_SPOT,
		WALK_ON_LINE,
		WALK_ON_PATHNODE
	};
	WalkMode GetWalkMode() { return (WalkMode)pblock->GetInt(PB_WALKMODE); };
	void  SetWalkMode(WalkMode wm);//{  pblock->SetValue(PB_WALKMODE, 0, wm); };

	Control* GetStepEaseGraph() const;
	float	 GetStepEaseValue(TimeValue t) const;
	Interval GetStepEaseExtents();

	Control* GetDistCovered() { return pblock->GetControllerByID(PB_DISTCOVERED); };
	//		float	GetCATUnits(){ return GetCATParent()->GetCATUnits();	};

	void	SetGroundNode(INode* node) { nodeGround = node; };
	INode*	GetGroundNode() { return nodeGround; };

	void	GettmPath(TimeValue t, Matrix3 &val, Interval &valid, float pathOffset, BOOL isFoot);

	/**********************************************************************
	 * Published Functions....
	 */
	BaseInterface* GetInterface(Interface_ID id) {
		if (id == CATHIERARCHYROOT_INTERFACE) return (ICATHierarchyRoot*)this;
		else return FPMixinInterface::GetInterface(id);
	}

	virtual BOOL ILoadPreset(const TSTR& filename, BOOL bNewLayer);
	virtual BOOL ISavePreset(const TSTR& filename);

	virtual BOOL ISetPathNode(INode *pathnode);

	virtual void ICreateFootPrints();
	virtual void IResetFootPrints(BOOL bOnlySelected);
	virtual void IRemoveFootPrints(BOOL bOnlySelected);
	virtual BOOL ISnapToGround(INode *ground, BOOL bOnlySelected);

	virtual void UpdateUI();

	virtual void IUpdateDistCovered();
	virtual void IUpdateSteps();
	///////////////////////////////////////////////
	// CAT2 Stuff
	NLAInfo*	GetLayerInfo() { return (NLAInfo*)pblock->GetReferenceTarget(PB_NLAINFO); }
	int			GetLayerIndex();

	DWORD GetFileSaveVersion();

};

extern ClassDesc2* GetCATHierarchyRootDesc();
