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

extern ClassDesc2* GetMuscleBonesDesc();

class MuscleBones : public Muscle {

public:

	//From Animatable
	Class_ID ClassID() { return MUSCLEBONES_CLASS_ID; }
	//	SClass_ID SuperClassID() { return SYSTEM_CLASS_ID; }
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATMuscle"); }
	void InitNodeName(TSTR& s) { s = GetString(IDS_NODE_NAME_CATMUSCLE); }

	void Init() { deformer_type = IMuscle::DEFORMER_BONES; Muscle::Init(); };
	MuscleBones(BOOL loading = FALSE);
	~MuscleBones() { UnRegisterCallbacks(); muscleCopy = NULL; };
	void DeleteThis() { delete this; }
};
