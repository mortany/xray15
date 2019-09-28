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
#include "CATHierarchyRoot.h"
#include "CATHierarchyLeaf.h"
#include "CATMotionPresets.h"

 // Our Graph Controller base class
#include "CATGraph.h"

//////////////////////////////////////////////////////////////////////
// CATHierarchyBranch2
//
// Represents a branch in our CAT hierarchy.  A branch can contain
// other branches and/or presets.  Branches can be created
//
class LimbData2;
class Hub;
class CATMotionLimb;

class CATHierarchyBranch2 : public CATHierarchyBranch {
private:
	// this is for the ui so I can use one controller
	// to display with one
	int nActiveLeaf;

public:

	// Class stuff
	Class_ID ClassID() { return CATHIERARCHYBRANCH_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATHIERARCHYBRANCH); }
	void DeleteThis() { delete this; }

	// Our own functions
	void Init();

	// Construction
	CATHierarchyBranch2(BOOL loading = FALSE);
	CATHierarchyBranch2(const CATHierarchyBranch2 &ctrl);
	~CATHierarchyBranch2();

	// Copy / Clone
	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); };
	RefTargetHandle Clone(RemapDir& remap);

	// Miscellaneous subanim stuff that we don't care about
	// This makes us Red
	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };
	int GetFCurveExtents(ParamDimensionBase *dim, float &min, float &max, DWORD flags);
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_XCOLOR; };
	int PaintFCurves(ParamDimensionBase *dim, HDC hdc, Rect& rcGraph, Rect& rcPaint,
		float tzoom, int tscroll, float vzoom, int vscroll, DWORD flags);

	ParamDimension* GetParamDimension(int) { return defaultDim; };

	int NumSubs();
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	// GetValue and SetValue
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	enum CATBranchPB {
		PB_BRANCHTAB,
		PB_BRANCHNAMESTAB,
		PB_EXPANDABLE,
		PB_BRANCHPARENT,
		PB_BRANCHINDEX,
		PB_BRANCHOWNER,

		PB_LEAFTAB,
		PB_LEAFNAMESTAB,

		PB_UITYPE,

		PB_ISWELDED,
		//	PB_DEFAULTVAL,
		//	PB_COMBINEVALS,
		PB_SUBNUM,

		PB_CONTROLLERTAB,
		PB_SUBANIMINDEX,
		PB_LIMBS_TAB,
		PB_ROOT
	};

	CATHierarchyLeaf* NewCATHierarchyLeafController();

	// Test if this branch is no longer needed, and if
	// not, triggers SelfDestruct.  It will return
	// true in the case where the branch is destructed
	bool MaybeDestructBranch();
	// A CHB2 needs to clean it's leaves when it destructs.  This is because
	// the leaves do not have references to this class, neither this
	// class to the leaves.
	virtual void	  SelfDestruct();

	CATHierarchyLeaf* GetLeaf(int index) { return (CATHierarchyLeaf*)pblock->GetReferenceTarget(PB_LEAFTAB, 0, index); }
	CATHierarchyLeaf* GetLeaf(const TSTR& name);
	CATHierarchyLeaf* AddLeaf(const TSTR& name);
	CATHierarchyLeaf* AddLimbPhasesLimb(LimbData2 *ctrlLMData);
	BOOL			  RemoveLeaf(CATHierarchyLeaf* leaf);
	BOOL			  RemoveLeaf(int index) { return RemoveLeaf(GetLeaf(index)); }

	void	 SetLeaf(int index, Control* leaf) { pblock->SetValue(PB_LEAFTAB, 0, leaf, index); };
	void	 SetLeafName(int index, const TSTR& name) { pblock->SetValue(PB_LEAFNAMESTAB, 0, name.data(), index); };
	TSTR	 GetLeafName(int index);//							  { return pblock->GetStr(PB_LEAFNAMESTAB, 0, index); };
	int		 GetLeafIndex(CATHierarchyLeaf* leaf);
	int		 GetNumLeaves() { return (pblock != NULL) ? pblock->Count(PB_LEAFTAB) : 0; };

	CATHierarchyBranch* AddAttribute(Control *ctrl, const TSTR& string, CATMotionLimb *catmotionlimb = NULL, int nNumDefaultVals = 0, float *dDefaultVals = NULL);

	void WeldLeaves();
	void UnWeldLeaves();

	int		 GetNumLimbs() { return pblock->Count(PB_LIMBS_TAB); };
	LimbData2* GetLimb(int index) { return (LimbData2*)pblock->GetReferenceTarget(PB_LIMBS_TAB, 0, index); };
	void	 SetLimb(int index, LimbData2* limb) { pblock->SetValue(PB_LIMBS_TAB, 0, (ReferenceTarget*)limb, index); };
	void	 AddLimb(LimbData2* limb);
	TSTR	 GetLimbName(int index);//
	bool	 RemoveLimb(LimbData2* limb);
	void	 ResetToDefaults(TimeValue t);

	CATHierarchyLeaf* GetWeights() { return GetCATRoot()->GetWeights(); };

	int		 GetNumControllerRefs() { return pblock->Count(PB_CONTROLLERTAB); };
	Control* GetControllerRef(int index) { return (Control*)pblock->GetReferenceTarget(PB_CONTROLLERTAB, 0, index); };
	BOOL	 SetControllerRef(int index, Control* ctrl) { return pblock->SetValue(PB_CONTROLLERTAB, 0, ctrl, index); };
	void	 AddControllerRef(Control* ctrl);

	int		 GetNumGraphKeys();
	void	 GetGraphKey(int iKeyNum,
		Control** ctrlTime, float &fTimeVal, float &minTime, float &maxTime,
		Control** ctrlValue, float &fValueVal, float &minVal, float &maxVal,
		Control** ctrlTangent, float &fTangentVal,
		Control** ctrlInTanLen, float &fInTanLenVal,
		Control** ctrlOutTanLen, float &fOutTanLenVal,
		Control** ctrlSlider);

	void	 PasteGraph();

	void	 GetP3Ctrls(Control** ctrlX, Control** ctrlY, Control** ctrlZ);

	// CATWindow Graph painting methods
	bool	 GetClickedKey(const Point2	&clickPos,
		const Point2	&clickRange,
		int &iKeynum,
		int &iLimb,
		BOOL &bKey,
		BOOL &bInTan,
		BOOL &bOutTan);

	void	 GetYRange(TimeValue t, float &minY, float &maxY);
	void	 DrawGraph(TimeValue t,
		bool	isActive,
		int		nSelectedKey,
		HDC		hGraphDC,
		int		iGraphWidth,
		int		iGraphHeight,
		float	alpha,
		float	beta);

	void	 DrawKeys(const TimeValue t,
		const int	dGraphWidth,
		const int	dGraphHeight,
		const float alpha,
		const float beta,
		HDC &hGraphDC,
		const int	nSelectedKey);

	void	 SetUIType(int uitype)							const { pblock->SetValue(PB_UITYPE, 0, uitype); };
	int		 GetUIType()									const { return pblock->GetInt(PB_UITYPE); };

	void	 SetisWelded(BOOL welded)						const { pblock->SetValue(PB_ISWELDED, 0, welded); };
	int		 GetisWelded()									const { return pblock->GetInt(PB_ISWELDED); };

	int GetActiveLeaf() { return nActiveLeaf; }
	void SetActiveLeaf(int index);

	void	 SetCATRoot(CATHierarchyRoot* newcatroot) { pblock->SetValue(PB_ROOT, 0, newcatroot); };
	CATHierarchyRoot*	 GetCATRoot() { return (CATHierarchyRoot*)pblock->GetReferenceTarget(PB_ROOT); };

	// i am stored on a controller somewhere an this is my subanim index
	void SetSubAnimIndex(int subanimindex) { pblock->SetValue(PB_SUBANIMINDEX, 0, subanimindex); }
	int	 GetSubAnimIndex() { return pblock->GetInt(PB_SUBANIMINDEX); }

	// 02/06/03 - ST Added in this function for
	// slider bracketing in our UI
	BOOL IsKeyAtTime(TimeValue t, DWORD flags);

	// These are not intended to be called by anyone else.
	// Use the functions ISavePreset() and ILoadPreset().
	BOOL SavePreset(CATPresetWriter *save, TimeValue t);
	BOOL LoadPreset(CATPresetReader *load, TimeValue t, int slot);

	// Some messages require to return a value.
	// AddLayer tells us what index was just created
	void NotifyLeaves(UINT msg, int &data);

	BOOL CleanupTree();
};

extern ClassDesc2* GetCATHierarchyBranch2Desc();
