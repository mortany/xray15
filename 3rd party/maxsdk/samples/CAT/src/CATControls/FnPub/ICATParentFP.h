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

#include <CATAPI/ICATParent.h>

// This 'using' should apply to almost the entire CAT plugin
using namespace CatAPI;

#pragma warning(push)
#pragma warning(disable:4238) // necessary for _BV types

// This class defines the MxS exposure for ICATParent

class ICATParentFP : public CatAPI::ICATParent {
public:

	enum {
		fnAddHub,
		fnLoadRig,
		fnSaveRig,

		fnGetBoneByAddress,
		fnUpdateUserProps,
		fnAddRootNode,

		propGetCATMode,
		propSetCATMode,
		propGetCATName,
		propSetCATName,
		propGetCATUnits,
		propSetCATUnits,
		propGetColourMode,
		propSetColourMode,

		propGetLengthAxis,
		propSetLengthAxis,
		propGetNode,
		propGetRootHub,
		propGetVersion,
		propGetBuildNumber,
		propCATRigSpace,

		propCATRigNodes,
		propCATRigLayerCtrls,

		propRootNode
	};

	BEGIN_FUNCTION_MAP

		VFN_0(fnAddHub, AddHub);
		FN_1(fnLoadRig, TYPE_INODE, LoadRig, TYPE_TSTR);
		FN_1(fnSaveRig, TYPE_BOOL, SaveRig, TYPE_TSTR);
		FN_1(fnGetBoneByAddress, TYPE_INODE, GetBoneByAddress, TYPE_TSTR_BV);
		VFN_0(fnUpdateUserProps, UpdateUserProps);
		VFN_0(fnAddRootNode, AddRootNode);

		PROP_FNS(propGetCATMode, GetCATMode, propSetCATMode, SetCATMode, (CatAPI::CATMode)TYPE_INT);
		PROP_FNS(propGetCATName, GetCATName, propSetCATName, SetCATName, TYPE_TSTR_BV);
		PROP_FNS(propGetCATUnits, GetCATUnits, propSetCATUnits, SetCATUnits, TYPE_FLOAT);
		PROP_FNS(propGetColourMode, GetColourMode, propSetColourMode, SetColourMode, (CatAPI::CATColourMode)TYPE_INT);
		PROP_FNS(propGetLengthAxis, GetLengthAxisAsString, propSetLengthAxis, SetLengthAxisAsString, TYPE_TSTR_BV);
		RO_PROP_FN(propGetNode, GetNode, TYPE_INODE);
		RO_PROP_FN(propGetRootHub, GetRootIHub, TYPE_CONTROL);
		RO_PROP_FN(propGetVersion, GetVersion, TYPE_INT);
		RO_PROP_FN(propGetBuildNumber, GetBuildNumber, TYPE_INT);

		RO_PROP_TFN(propCATRigSpace, ApproxCharacterTransform, TYPE_MATRIX3_BV);
		RO_PROP_FN(propCATRigNodes, GetCATRigNodes, TYPE_INODE_TAB_BV);
		RO_PROP_FN(propCATRigLayerCtrls, GetCATRigLayerCtrls, TYPE_CONTROL_TAB_BV);
		RO_PROP_FN(propRootNode, GetRootNode, TYPE_INODE);

	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();
	static FPInterfaceDesc* GetFnPubDesc();

	// Two little helper functions allow us to convert incoming MxS string values to the defines we should be using.
	TSTR GetLengthAxisAsString();
	void SetLengthAxisAsString(const TSTR& val);
};

#pragma warning(pop)