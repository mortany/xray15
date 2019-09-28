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

#include "ICATParentFP.h"

// Access resource strings
#include "../CatPlugins.h"

/**********************************************************************
 * Function publishing interface descriptor...
 */
class catparentFPValidator : public FPValidator
{
	bool Validate(FPInterface* fpi, FunctionID fid, int paramNum, FPValue& val, TSTR& msg);
};

bool catparentFPValidator::Validate(FPInterface* fpi, FunctionID fid, int paramNum, FPValue& val, TSTR& msg)
{
	UNREFERENCED_PARAMETER(paramNum); UNREFERENCED_PARAMETER(fpi);
	switch (fid) {
	case ICATParentFP::propSetCATMode:
		if ((val.i < 0) || (val.i > 2)) {
			msg = GetString(IDS_VLD_CATMODE);
			return false;
		}
		break;
	case ICATParentFP::propSetCATUnits:
		if (val.f < 0) {
			msg = GetString(IDS_VLD_CATUNITS);
			return false;
		}
		break;
	case ICATParentFP::propSetColourMode:
		if ((val.i < 0) || (val.i > 1)) {
			msg = GetString(IDS_VLD_COLORMODE);
			return false;
		}
		break;
	case ICATParentFP::propSetLengthAxis:
		if (((*val.tstr) != _M("X")) && ((*val.tstr) != _M("Z"))) {
			msg = GetString(IDS_VLD_LENAXIS);
			return false;
		}
		break;
	}
	return true;
}

static catparentFPValidator catparentFPValidator;

//////////////////////////////////////////////////////////////////////////

FPInterfaceDesc* ICATParentFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* ICATParentFP::GetFnPubDesc()
{
	static FPInterfaceDesc catparent_interface(
		CATPARENT_INTERFACE_FP, _T("CATParentFPInterface"), 0, NULL, FP_MIXIN,

		ICATParentFP::fnAddHub, _T("AddHub"), 0, TYPE_VOID, 0, 0,

		ICATParentFP::fnLoadRig, _T("LoadRig"), 0, TYPE_BOOL, 0, 1,
			_T("filename"), 0, TYPE_TSTR,
		ICATParentFP::fnSaveRig, _T("SaveRig"), 0, TYPE_BOOL, 0, 1,
			_T("filename"), 0, TYPE_TSTR,

		ICATParentFP::fnGetBoneByAddress, _T("GetBoneByAddress"), 0, TYPE_INODE, 0, 1,
			_T("Address"), 0, TYPE_TSTR_BV,

		ICATParentFP::fnUpdateUserProps, _T("UpdateUserProps"), 0, TYPE_VOID, 0, 0,
		ICATParentFP::fnAddRootNode, _T("AddRootNode"), 0, TYPE_VOID, 0, 0,

		properties,

		ICATParentFP::propGetCATMode, ICATParentFP::propSetCATMode, _T("CATMode"), 0, TYPE_INT,
			f_validator, &catparentFPValidator,
		ICATParentFP::propGetCATName, ICATParentFP::propSetCATName, _T("CATName"), 0, TYPE_TSTR_BV,
		ICATParentFP::propGetCATUnits, ICATParentFP::propSetCATUnits, _T("CATUnits"), 0, TYPE_FLOAT,
			f_validator, &catparentFPValidator,
		ICATParentFP::propGetColourMode, ICATParentFP::propSetColourMode, _T("ColourMode"), 0, TYPE_INT,

		ICATParentFP::propGetLengthAxis, ICATParentFP::propSetLengthAxis, _T("LengthAxis"), 0, TYPE_TSTR_BV,
			f_validator, &catparentFPValidator,
		ICATParentFP::propGetNode, FP_NO_FUNCTION, _T("Node"), 0, TYPE_INODE,
		ICATParentFP::propGetRootHub, FP_NO_FUNCTION, _T("RootHub"), 0, TYPE_CONTROL,
		ICATParentFP::propGetVersion, FP_NO_FUNCTION, _T("CATVersion"), 0, TYPE_INT,
		ICATParentFP::propCATRigSpace, FP_NO_FUNCTION, _T("CATRigSpace"), 0, TYPE_MATRIX3_BV,

		ICATParentFP::propCATRigNodes, FP_NO_FUNCTION, _T("CATRigNodes"), 0, TYPE_INODE_TAB_BV,
		ICATParentFP::propCATRigLayerCtrls, FP_NO_FUNCTION, _T("CATRigLayerCtrls"), 0, TYPE_CONTROL_TAB_BV,

		ICATParentFP::propRootNode, FP_NO_FUNCTION, _T("RootTransformNode"), 0, TYPE_INODE,

		p_end
	);

	return &catparent_interface;
}

TSTR ICATParentFP::GetLengthAxisAsString()
{
	return (GetLengthAxis() == X) ? _T("X") : _T("Z");
}

void ICATParentFP::SetLengthAxisAsString(const TSTR& val)
{
	if (val == _T("X")) SetLengthAxis(X); else SetLengthAxis(Z);
}
