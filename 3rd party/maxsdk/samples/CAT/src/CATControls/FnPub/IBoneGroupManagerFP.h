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

#include <CATAPI/IBoneGroupManager.h>
#include "ioapi.h"

class IBoneGroupManagerFP : public CatAPI::IBoneGroupManager
{
public:

	enum FPDataProps {
		propGetNumBones,
		propSetNumBones,
		fnGetBone
	};

	BEGIN_FUNCTION_MAP
		PROP_FNS(propGetNumBones, GetNumBones, propSetNumBones, SetNumBones, TYPE_INT);
		FN_1(fnGetBone, TYPE_INTERFACE, GetBoneINodeControl, TYPE_INDEX);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	static FPInterfaceDesc* GetFnPubDesc();

	//////////////////////////////////////////////////////////////////////////
	// Internal functions (not published)

	// Internal only
	// Called to complete a clone operation
	virtual void PostCloneManager() = 0;

	// Internal only
	// Called by CATNodeControl when it is removed from the scene
	void NotifyBoneDelete(CatAPI::INodeControl* pBone);
};

// This class can be used to patch the various Data BoneID's from
// their local paramblock stored version to the CATControl BoneID
class CATControl;
class BoneGroupPostLoadIDPatcher : public PostLoadCallback
{
private:
	ParamID mOldIdParam;
	CATControl* mpOwner;

public:
	BoneGroupPostLoadIDPatcher(CATControl* pOwner, ParamID paramIndex);

	void proc(ILoad*);
};