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

 // Our Graph Controller base class
#include <vector>

#define CATHIERARCHYLEAF_CLASS_ID			Class_ID(0xec62904, 0x185a28dc)

// These are special notify messages for presets.  They
// are specifically for the function NotifyPreset().
#define CAT_LAYER_ADD			0x01
#define CAT_LAYER_INSERT		0x02
#define CAT_LAYER_REMOVE		0x03
#define CAT_LAYER_SELECT		0x04

class CATHierarchyBranch2;
class ECATParent;
class ICATParentTrans;

class CATHierarchyLeaf : public Control {

public:
	// Construction
	CATHierarchyLeaf(BOOL loading = FALSE);
	CATHierarchyLeaf(const CATHierarchyLeaf &ctrl);
	~CATHierarchyLeaf();

	// Copy / Clone
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	// Class stuff
	Class_ID ClassID() { return CATHIERARCHYLEAF_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s);
	void DeleteThis() { delete this; }

	// Bad bad!  No, we're not allowed to assign controllers =P
	BOOL AssignController(Animatable *control, int subAnim) { UNREFERENCED_PARAMETER(control); UNREFERENCED_PARAMETER(subAnim);  return FALSE; };

	// 02/06/03 ST - implemented function for
	// keyframe bracketing on sliders ect
	BOOL IsKeyAtTime(TimeValue t, DWORD flags);

	// GetValue and SetValue
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	// Control-specific stuff that we don't care too much about
	// but should keep in mind.
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }
	BOOL IsAnimated() { return TRUE; };
	virtual BOOL CanCopyAnim() { return TRUE; }

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
		CATLEAFPB
	};

	enum CATRootPB {
		PB_LAYERTAB,
		PB_LAYERNAMESTAB,
		PB_LEAFPARENT,
		PB_WEIGHTS,
		PB_LIMBDATA_DEPRECATED,
		PB_DEFAULTVAL,
		PB_ACTIVELAYER,
	};

	void Init();
	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	TSTR SubAnimName(int i);
	int NumSubs();
	int NumRefs() { return 1; }
	Animatable* SubAnim(int i);
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

	/////////////////////////////////////////////////////////////////////////////

	void MaybeDestructLeaf(LimbData2* pLimb);

	void	NotifyLeaf(UINT msg, int &data);
	int		AddLayer(int nNumNewLayers = 1);
	void	RemoveLayer(const int nLayerIndex);
	int		InsertLayer(const int nLayerIndex);

	void	SetDefaultVal(float val) { pblock->SetValue(PB_DEFAULTVAL, 0, val); }
	float 	GetDefaultVal() { return pblock->GetFloat(PB_DEFAULTVAL); }

	int		GetNumLayers() { return pblock->Count(PB_LAYERTAB); }
	Control* GetLayer(int index) { return pblock->GetControllerByID(PB_LAYERTAB, index); }
	void SetLayerName(const int index, const TSTR& name) { pblock->SetValue(PB_LAYERNAMESTAB, 0, name.data(), index); }
	TSTR GetLayerName(const int index) { return pblock->GetStr(PB_LAYERNAMESTAB, 0, index); }

	int GetActiveLayer() { return pblock->GetInt(PB_ACTIVELAYER); }
	Control *GetActiveLayerControl() { return ((GetActiveLayer() < GetNumLayers()) ? pblock->GetControllerByID(PB_LAYERTAB, GetActiveLayer()) : NULL); }
	void SetActiveLayer(int activeLayer) { pblock->SetValue(PB_ACTIVELAYER, 0, max(0, activeLayer)); }
	void SetActiveLayerControl(Control *ctrl) { if (GetActiveLayer() > 0) pblock->SetControllerByID(PB_LAYERTAB, GetActiveLayer(), ctrl, FALSE); };

	void				SetLeafParent(ReferenceTarget* leafParent) { pblock->SetValue(PB_LEAFPARENT, 0, leafParent); }
	CATHierarchyBranch2* GetLeafParent() { return (CATHierarchyBranch2*)pblock->GetReferenceTarget(PB_LEAFPARENT); }

	int FindPresetIndex(const CStr& name);
	int FindPresetIndex(int subNum);

	CATHierarchyLeaf* GetWeights() { return (CATHierarchyLeaf*)pblock->GetReferenceTarget(PB_WEIGHTS); }
	void SetWeights(CATHierarchyLeaf* ctrlWeights) { pblock->SetValue(PB_WEIGHTS, 0, (ReferenceTarget*)ctrlWeights); }
};

extern ClassDesc2* GetCATHierarchyLeafDesc();
