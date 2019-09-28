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
#include "BezierInterp.h"

class CATWeight : public Control {
public:

	const TCHAR *strRolloutCaption;
	//TODO: Add enums for various parameters
	enum {
		PB_KEY1VAL, PB_KEY1OUTTANLEN,
		PB_KEY2VAL, PB_KEY2INTANLEN,
		PB_KEY1TAN, PB_KEY2TAN
	};

	//From Animatable
	Class_ID ClassID() { return CATWEIGHT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATWEIGHT); }

	RefTargetHandle Clone(RemapDir &remap);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	CATWeight();
	~CATWeight();
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }

	IParamBlock2	*pblock;	//ref 0
	enum EnumRefs {
		PBLOCK_REF
	};
	enum CATGraphparams {
		PB_LIMBDATA,
		PB_CATBRANCH
	};

	//		Tab<CATGraphKey*> tabCATGraphKeys;

		//
		// from class Control:
		//
	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int) { return pblock; }

	// TODO: Maintain the number or references here
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
	{
		UNREFERENCED_PARAMETER(t); UNREFERENCED_PARAMETER(val); UNREFERENCED_PARAMETER(commit); UNREFERENCED_PARAMETER(method);
	}
	int IsKeyable() { return FALSE; };
	BOOL IsLeaf() { return FALSE; }

	//		BOOL CanMakeUnique(){ return FALSE; };
	//		BOOL CanCopyTrack(){ return FALSE; };
	//		BOOL IsAnimated(){ return TRUE; };

	Interval GetTimeRange(DWORD flags) { UNREFERENCED_PARAMETER(flags); return GetCOREInterface()->GetAnimRange(); };

	//RefTargetHandle Clone( RemapDir &remap );
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; };

	//
	// from class Control:
	//
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);

	void Copy(Control *from);

	//	void GetCATKey(const int	i, const TimeValue t, CATKey &key,
	//							  bool &isInTan, bool &isOutTan);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	///////////////////////////////////////////////
	// CATWeight

	BOOL	LoadRig(CATRigReader *load);
	BOOL	SaveRig(CATRigWriter *save);

	BOOL	PasteRig(CATWeight* pastectrl);
	float	GetYval(TimeValue t, Interval& valid);
	void	SetRolloutCaption(const TCHAR *caption) { strRolloutCaption = caption; }

	void	DisplayRollout(IObjParam *ip, ULONG flags, Animatable *prev, const TCHAR *caption);
	void	DisplayWindow(const TCHAR *caption, HWND hWndOwner = NULL);
};

extern ClassDesc2* GetCATWeightDesc();
