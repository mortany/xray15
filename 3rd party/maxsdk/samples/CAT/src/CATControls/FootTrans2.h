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

// Transform Controller for the feet and pelvis, creates a local axis where the foot is stepping

#pragma once

#include <CATAPI/CATClassID.h>
#include "CATNodeControl.h"

class LimbData2;

class FootTrans2 : public CATNodeControl {
private:
	friend class FootTransPLCB;

	// pointers to our controls.
	Control			*ctrlStepMask;

	// UI Stuff...

	IParamBlock2 *pblock;

public:
	enum PARAMLIST {
		PBLOCK_REF,
		FOOTPRINTPOS,
		FOOTPRINTROT,
		LAYERTRANS,
		// testing testing..
		STEPMASK,

		NUMPARAMS
	};		// our subanim list.

	enum {
		PB_CATPARENT_DEPRECATED = 0,
		PB_LIMBDATA,			// To get StepEase'd values, a direct handle is easiest
		PB_PALMTRANS,		// (For footprints and masks)
		PB_OBJPLATFORM,
		PB_NODEPLATFORM,

		//	pb_StepGraph,
		PB_TMSETUP,

		PB_LAYERPIVOTPOS,
		PB_LAYERPIVOTROT,

		//	PB_NODETABFOOTPRINTS,
		PB_STEPSHAPE,
		PB_STEPSLIDE,
		// FootPrints & MoveMask
		PB_CAT_PRINT_TM,
		PB_CAT_PRINT_INV_TM,
		PB_PRINT_NODES,
		PB_SCALE
	};
	enum SUBLIST {
		SUBTRANS,
		NUMSUBS,
	};

public:
	//
	// constructors.
	//
	FootTrans2(BOOL loading = FALSE);
	~FootTrans2();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return FOOTTRANS2_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_FOOTTRANS2); }

	int NumSubs() { return NUMSUBS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	void CATMessage(TimeValue t, UINT msg, int data = -1);
	void PurgeHierarchy();

	// Save/Load Reference Number
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	void Copy(Control *from);
	void PostCloneNode();
	RefTargetHandle Clone(RemapDir& remap);

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	void* GetInterface(ULONG id) {
		if (id == I_MASTER) 			return (void *)this;
		else						return CATNodeControl::GetInterface(id);
	}

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idPlatform; }

	TSTR GetName() { return GetString(IDS_PLATFORM); }

	CATGroup* GetGroup();

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	// Limb Creation/Maintenance ect.
	BOOL SaveRig(CATRigWriter *save);
	BOOL LoadRig(CATRigReader *load);

	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);
	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	// A Footplatform has no parent.
	CATNodeControl* FindParentCATNodeControl() { return NULL; };

	TSTR	GetRigName();
	void	UpdateUserProps();

	///////////////////////////////////////////////////////////////////////////
	// FootTrans
	LimbData2* GetLimbData();
	virtual void	SetLimbData(LimbData2* l);

	CATControl* GetParentCATControl();
	INode*	Initialise(Control *ctrlLMData, BOOL loading);
};

extern ClassDesc2* GetFootTrans2Desc();
