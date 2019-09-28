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

// The MusclePatch object is the geometry that is used to build the muscle out of.

#include "CATMuscle.h"
#include "MusclePatch.h"
#include "MuscleBones.h"
#include "SegTrans.h"
#include "../CATObjects/CATDotIni.h"

 //keeps track of whether an FP interface desc has been added to the CATClipMatrix3 ClassDesc
static bool imusclepatchInterfaceAdded = false;

class MusclePatchClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) {
		if (!imusclepatchInterfaceAdded) {
			// So we can have different muscle types sharing one Interface, we add the
			// Function publishing interface here. usually FPInterfaces are explicitly tied
			// to a ClassDesc. In our case we didn't want them to be tied.
			AddInterface(GetCATMuscleFPInterface());
			imusclepatchInterfaceAdded = true;
		}
		g_bIsResetting = FALSE;
		return new MusclePatch(loading);
	}
	const TCHAR *	ClassName() { return _T("MusclePatch"); }
	//	SClass_ID		SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return MUSCLEPATCH_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("MusclePatch"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }

};

static MusclePatchClassDesc MusclePatchDesc;
ClassDesc2* GetMusclePatchDesc() { return &MusclePatchDesc; }

