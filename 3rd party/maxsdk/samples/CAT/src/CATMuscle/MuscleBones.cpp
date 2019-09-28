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

// The MuscleBones object is the geometry that is used to build the muscle out of.

#include "CATMuscle.h"
#include "MuscleBones.h"
#include "MusclePatch.h"

#include "../CATObjects/CATDotIni.h"

 //keeps track of whether an FP interface desc has been added to the CATClipMatrix3 ClassDesc
static bool imuscleInterfaceAdded = false;

class MuscleBonesClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) {
		if (!imuscleInterfaceAdded) {
			// So we can have different muscle types sharing one Interface, we add the
			// Function publishing interface here. usulaly FPInterfaces are explicitly tied
			// to a ClassDesc. In our case we didn't want them to be tied.
			AddInterface(GetCATMuscleFPInterface());
			imuscleInterfaceAdded = true;
		}
		g_bIsResetting = FALSE;
		return new MuscleBones(loading);
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMUSCLE); }
	//	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return MUSCLEBONES_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMuscle"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};

static MuscleBonesClassDesc MuscleBonesDesc;
ClassDesc2* GetMuscleBonesDesc() { return &MuscleBonesDesc; }

MuscleBones::MuscleBones(BOOL loading) {
	Init();
	if (!loading) {
		CatDotIni *catini = new CatDotIni();
		TCHAR muscletype[6] = { 0 };
		_itot(IMuscle::DEFORMER_BONES, muscletype, 6);
		deformer_type = (DEFORMER_TYPE)catini->GetInt(KEY_MUSCLE_TYPE, muscletype);
		delete catini;

		dVersion = CAT_VERSION_CURRENT;
	}
};;
