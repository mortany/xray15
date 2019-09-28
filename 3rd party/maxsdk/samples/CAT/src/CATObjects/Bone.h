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

#include "CATObject Superclass.h"

extern ClassDesc2* GetCATBoneDesc();

#define CATBONE_CLASS_ID	Class_ID(0x2e6a0c09, 0x43d5c9c0)

class CATBone : public CATObject {
public:

	enum PARAMLIST { BONE_PBLOCK_REF, NUMPARAMS };		// our subanim list.

	enum { CATBone_params };
	enum {
		PB_BONETRANS,
		PB_CATUNITS,
		PB_LENGTH,
		PB_WIDTH,
		PB_DEPTH
	};

	//Class vars
	static IObjParam *ip;			//Access to the interface
	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack();

	// From Object
	BOOL HasUVW();
	void SetGenUVW(BOOL sw);

	int CanConvertToType(Class_ID obtype);//{ return 0; };
	Object* ConvertToType(TimeValue t, Class_ID obtype);//{ return NULL; };
	void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist) { UNREFERENCED_PARAMETER(clist); UNREFERENCED_PARAMETER(nlist); };

	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	//TODO: Evaluate the object and return the ObjectState
	ObjectState Eval(TimeValue) { return ObjectState(this); };
	//TODO: Return the validity interval of the object as a whole
	Interval ObjectValidity(TimeValue) { return FOREVER; }

	// From SimpleObject
	void BuildMesh(TimeValue t);
	BOOL OKtoDisplay(TimeValue t);
	void InvalidateUI();

	//From Animatable
	Class_ID ClassID() { return CATBONE_CLASS_ID; }
	SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_BONE); }

	RefTargetHandle Clone(RemapDir &remap);

	int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock2; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

	RefTargetHandle GetReference(int i);

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	CATBone();
	~CATBone();

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	//////////////////////////////////////////////////////////////////////////
//		Control* GetTransformController(){ return (Control*)pblock2->GetReferenceTarget(PB_BONETRANS); }
	void	 SetTransformController(Control* ctrl) { pblock2->SetValue(PB_BONETRANS, 0, (ReferenceTarget*)ctrl); };

	float GetX() { return pblock2->GetFloat(PB_WIDTH); }
	float GetY() { return pblock2->GetFloat(PB_DEPTH); }
	float GetZ() { return pblock2->GetFloat(PB_LENGTH); }

	void SetX(float val) {
		pblock2->SetValue(PB_WIDTH, 0, val);
		Update();
	};
	void SetY(float val) {
		pblock2->SetValue(PB_DEPTH, 0, val);
		Update();
	};
	void SetZ(float val) {
		pblock2->SetValue(PB_LENGTH, 0, val);
		Update();
	};

	float GetCATUnits() { return pblock2->GetFloat(PB_CATUNITS); }
	void SetCATUnits(float val) { pblock2->SetValue(PB_CATUNITS, 0, val); };
};

extern ClassDesc2* GetCATBoneClassDesc();
