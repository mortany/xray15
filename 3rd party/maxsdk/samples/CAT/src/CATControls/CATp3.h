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

#include <CATAPI/CATClassID.h>
#include "CATMotionController.h"

#define P3FLAG_POS		(1<<1)
#define P3FLAG_ROT		(1<<2)
#define P3FLAG_HUB		(1<<3)
#define P3FLAG_ROOTHUB	(1<<4)
#define P3FLAG_TAIL		(1<<5)

class CATp3 : public CATMotionController {
private:
	BOOL bCtrlsInitialised;

	Tab <Control*> tabX;
	Tab <Control*> tabY;
	Tab <Control*> tabZ;
	Tab <Control*> tabLiftOffset;

public:

	void SetFlag(USHORT f) { pblock->SetValue(PB_P3FLAGS, 0, pblock->GetInt(PB_P3FLAGS) | f); }
	void ClearFlag(USHORT f) { pblock->SetValue(PB_P3FLAGS, 0, pblock->GetInt(PB_P3FLAGS) & ~f); }
	BOOL TestFlag(USHORT f) const { return (pblock->GetInt(PB_P3FLAGS) & f) == f; }

	enum { PBLOCK_REF };

	// Parameter block bollocks
	enum {
		pb_params
	};

	enum {
		PB_P3FLAGS,
		PB_LIMB_TAB,
		PB_X_TAB,
		PB_Y_TAB,
		PB_Z_TAB,
		PB_LIFTOFFSET_TAB
	};

	IParamBlock2	*pblock;	//ref 0

	//From Animatable
	Class_ID ClassID() { return CATP3_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_POINT3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATP3); }

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	//		int NumSubs() { return pblock->Count(PB_LIMB_TAB) * (3 + ((pblock->GetInt(PB_P3FLAGS)&P3FLAG_ROOTHUB) ? 1 : 0)); }

		// We have now ended up with some inconsistancys in our tables so we cant garuantee that all rigs are equal
		// this *may* cause a *small* performance slowdown. But we will ignore that in favour of a clean general solution.
	int NumSubs() { return (pblock->Count(PB_LIMB_TAB) * 3) + pblock->Count(PB_LIFTOFFSET_TAB); };

	TSTR SubAnimName(int i); //  { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int i); // { return pblock; }

	// TODO: Maintain the number or references here
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	CATp3();
	~CATp3();

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit); UNREFERENCED_PARAMETER(method);
	}

	void Copy(Control *from) { UNREFERENCED_PARAMETER(from); }

	//
	// class CATp3
	//

	void Initialise(int p3flags);
	void InitControls();
	void ResetControls() { bCtrlsInitialised = FALSE; };

	void RegisterLimb(CATMotionLimb* ctrlNewLimb, CATHierarchyBranch2 *branch);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);
};

class CATp3ClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATp3(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATP3); }
	SClass_ID		SuperClassID() { return CTRL_POINT3_CLASS_ID; }
	Class_ID		ClassID() { return CATP3_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATp3"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

extern ClassDesc2* GetCATp3Desc();
