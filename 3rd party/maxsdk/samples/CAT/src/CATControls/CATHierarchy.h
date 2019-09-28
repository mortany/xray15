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

#include <vector>

class LimbData2;
class CATHierarchyLeaf;
class CATHierarchyRoot;
class CATHierarchyBranch2;
class Hub;

// used by the CATMotion UI to help
// navigate using our tree viewer
enum UIType {
	UI_BLANK,
	UI_GRAPH,
	UI_POINT3,
	UI_FLOAT,
	UI_GLOBALS,
	UI_FOOTSTEPS,
	UI_LIMBPHASES,
	UI_LIMBLESSGRAPH
};

#define CATHIERARCHYFLAG_ISEXPANDABLE	(1<<1)
#define CATHIERARCHYFLAG_UI_BLANK		(1<<2)
#define CATHIERARCHYFLAG_UI_GRAPH		(1<<3)
#define CATHIERARCHYFLAG_UI_POINT3		(1<<4)
#define CATHIERARCHYFLAG_UI_FLOAT		(1<<5)
#define CATHIERARCHYFLAG_UI_GLOBALS		(1<<6)
#define CATHIERARCHYFLAG_UI_FOOTSTEPS	(1<<7)
#define CATHIERARCHYFLAG_UI_LIMBPHASES	(1<<8)

class CATHierarchyBranch : public Control {
private:

public:

	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }
	virtual BOOL CanCopyAnim() { return TRUE; }
	//		BOOL CanCopyTrack(){ return FALSE; };
	//		BOOL CanPasteTrack(){ return FALSE; };
	//		BOOL CanMakeUnique(){ return FALSE; };
	//		BOOL CanCopyTrack(){ return FALSE; };

	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) {
		UNREFERENCED_PARAMETER(changeInt); UNREFERENCED_PARAMETER(hTarget); UNREFERENCED_PARAMETER(partID); UNREFERENCED_PARAMETER(propagate);
		if (message == REFMSG_FLAGDEPENDENTS)
			return REF_STOP;
		return REF_SUCCEED;
	};

	BOOL IsKeyAtTime(TimeValue t, DWORD flags);

	virtual void SetCATParent(ReferenceTarget *parent) { UNREFERENCED_PARAMETER(parent); };

	// this is only implemented on the Root
//	virtual HWND CreateCATWindow(){ return NULL; };
//		virtual CATHierarchyBranch* AddAttribute(Control *ctrl, int stringID, LimbData2 *ctrlLimbData = NULL, int subanimindex =-1){return NULL;};

		/*
		 *	parameter blocks ... they rock
		 */

	IParamBlock2	*pblock;	//ref 0
	// ref IDs
	enum refIDs {
		PBLOCK_REF,
	};
	// pblock ids
	enum pblockIDs {
		CATBRANCHPB
	};

	enum CATBranchPB {
		PB_BRANCHTAB,
		PB_BRANCHNAMESTAB,
		PB_EXPANDABLE,
		PB_BRANCHPARENT,
		PB_BRANCHINDEX,
		PB_BRANCHOWNER,
		PB_NUMPARAMS
	};

	BOOL bExpanded;
	BOOL loadedPreset;

	CATHierarchyBranch(BOOL loading = FALSE)
	{
		UNREFERENCED_PARAMETER(loading);
		bExpanded = FALSE;
		loadedPreset = FALSE;
		pblock = NULL;
	}

	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	RefTargetHandle GetReference(int) { return pblock; };
private:
	virtual void SetReference(int, RefTargetHandle rtarg);
public:
	int SubNumToRefNum(int subNum) { return subNum; }

	// Miscellaneous subanim stuff that we don't care about
	DWORD GetSubAnimCurveColor(int) { return PAINTCURVE_GENCOLOR; }
	ParamDimension* GetParamDimension(int) { return defaultDim; }

	// Bad bad!  No, we're not allowed to assign controllers =P
	BOOL AssignController(Animatable *control, int subAnim) { UNREFERENCED_PARAMETER(subAnim);  UNREFERENCED_PARAMETER(control); return FALSE; };
	// Cannot unique-ify us either...
	BOOL CanMakeUnique() { return FALSE; }

	//		CATHierarchyLeaf*		NewCATHierarchyLeafController();
	CATHierarchyBranch2*	NewCATHierarchyBranch2Controller();

	void	 SetExpandable(BOOL bExpandable)				const { pblock->SetValue(PB_EXPANDABLE, 0, bExpandable); };
	BOOL	 GetExpandable()								const { return pblock->GetInt(PB_EXPANDABLE); };

	void	NotifyLeaf(UINT msg, int &data);
	int		AddLayer();
	void	RemoveLayer(const int nLayerIndex);
	int		InsertLayer(const int nLayerIndex);

	CATHierarchyBranch* GetBranch(const TSTR& name);
	CATHierarchyBranch* GetBranch(int index) { return (CATHierarchyBranch*)pblock->GetControllerByID(PB_BRANCHTAB, index); };
	void	 SetBranch(int index, CATHierarchyBranch* ctrlBranch) { pblock->SetControllerByID(PB_BRANCHTAB, index, ctrlBranch, FALSE); };
	void	 SetBranchName(int index, const TSTR& name) { pblock->SetValue(PB_BRANCHNAMESTAB, 0, name.data(), index); };
	void	 SetBranchName(const TSTR& name) { GetBranchParent()->SetBranchName(GetBranchIndex(), name); };
	TSTR	 GetBranchName(int index);
	TSTR	 GetBranchName() { return GetBranchParent()->GetBranchName(GetBranchIndex()); };
	int		 GetBranchIndex(CATHierarchyBranch* branch);

	void				 SetBranchParent(ReferenceTarget* branchParent) { if (GetBranchParent() != branchParent) pblock->SetValue(PB_BRANCHPARENT, 0, branchParent); };
	CATHierarchyBranch*	 GetBranchParent() { return (CATHierarchyBranch*)pblock->GetReferenceTarget(PB_BRANCHPARENT); };

	void	 SetBranchIndex(int index) { if (GetBranchIndex() != index) pblock->SetValue(PB_BRANCHINDEX, 0, index); };
	int		 GetBranchIndex() { return pblock->GetInt(PB_BRANCHINDEX); };

	int		 GetNumBranches() { return (pblock != NULL) ? pblock->Count(PB_BRANCHTAB) : 0; };
	Control* AddBranch(const TSTR& name, int index = -1);
	Control* AddBranch(ReferenceTarget* owner, int index = -1);
	Control* AddBranch(ReferenceTarget* owner, const TSTR& groupName, int index = -1);
	virtual void SelfDestruct();
	virtual int	 GetNumLimbs() { return 0; };
	virtual bool RemoveLimb(LimbData2* limb);

	void			 SetBranchOwner(ReferenceTarget* branchOwner) { if (GetBranchOwner() != branchOwner) pblock->SetValue(PB_BRANCHOWNER, 0, branchOwner); };
	ReferenceTarget* GetBranchOwner() { return pblock->GetReferenceTarget(PB_BRANCHOWNER); };

	virtual CATHierarchyLeaf* GetLeaf(int index) { UNREFERENCED_PARAMETER(index); return NULL; };
	virtual CATHierarchyLeaf* GetLeaf(const TSTR& name) { UNREFERENCED_PARAMETER(name); return NULL; };;

	virtual CATHierarchyRoot* GetCATRoot() = 0;
	virtual CATHierarchyLeaf* GetWeights() = 0;
	int		GetNumLayers();

	int		SubAnimIndex(Control* sub);

	virtual BOOL CleanupTree();

};
