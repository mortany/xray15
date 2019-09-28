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

#include "Muscle.h"

extern ClassDesc2* GetMusclePatchDesc();

class MusclePatch : public Muscle {
public:

	//From Animatable
	Class_ID ClassID() { return MUSCLEPATCH_CLASS_ID; }
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }//GEOMOBJECT_CLASS_ID; } //
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_MUSCLEPATCH); }
	void InitNodeName(TSTR& s) { s = GetString(IDS_NODE_NAME_CATMUSCLE); }

	void Init() { deformer_type = IMuscle::DEFORMER_MESH; Muscle::Init(); };;
	MusclePatch(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); Init(); };
	~MusclePatch() { UnRegisterCallbacks(); muscleCopy = NULL; };
	void DeleteThis() { delete this; }
};
