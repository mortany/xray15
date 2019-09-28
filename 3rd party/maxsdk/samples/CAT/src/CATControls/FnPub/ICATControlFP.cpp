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

#include "ICATControlFP.h"

FPInterfaceDesc* ICATControlFP::GetDesc()
{
	return GetFnPubDesc();
}

FPInterfaceDesc* ICATControlFP::GetFnPubDesc()
{

	/**********************************************************************
	* Function publishing interface descriptor...
	*/
	static FPInterfaceDesc cat_control_FPinterface(
		CatAPI::CATCONTROL_INTERFACE_FP, _T("ICATControlFPInterface"), 0, NULL, FP_MIXIN,

		ICATControlFP::fnPasteLayer, _T("PasteLayer"), 0, TYPE_VOID, 0, 4,
			_T("source"), 0, TYPE_CONTROL,
			_T("fromindex"), 0, TYPE_INT,
			_T("toindex"), 0, TYPE_INT,
			_T("instance"), 0, TYPE_BOOL,

		ICATControlFP::fnPasteRig, _T("PasteRig"), 0, TYPE_BOOL, 0, 2,
			_T("source"), 0, TYPE_REFTARG,
			_T("mirrordata"), 0, TYPE_BOOL,

		ICATControlFP::fnSaveClip, _T("SaveClip"), 0, TYPE_BOOL, 0, 3,
			_T("filename"), 0, TYPE_TSTR,
			_T("StartTime"), 0, TYPE_TIMEVALUE,
			_T("EndTime"), 0, TYPE_TIMEVALUE,
		ICATControlFP::fnLoadClip, _T("LoadClip"), 0, TYPE_INODE, 0, 3,
			_T("filename"), 0, TYPE_TSTR,
			_T("time"), 0, TYPE_TIMEVALUE,	//	f_keyArgDefault, GetCurrentTime(),
			_T("mirrordata"), 0, TYPE_BOOL,		//	f_keyArgDefault, FALSE,

		ICATControlFP::fnSavePose, _T("SavePose"), 0, TYPE_BOOL, 0, 1,
			_T("filename"), 0, TYPE_TSTR,
		ICATControlFP::fnLoadPose, _T("LoadPose"), 0, TYPE_INODE, 0, 3,
			_T("filename"), 0, TYPE_TSTR,
			_T("time"), 0, TYPE_TIMEVALUE,	//	f_keyArgDefault, GetCurrentTime(),
			_T("mirrordata"), 0, TYPE_BOOL,		//	f_keyArgDefault, FALSE,

		ICATControlFP::fnCollapsePoseToCurrLayer, _T("CollapsePoseToCurLayer"), 0, TYPE_VOID, 0, 0,
		ICATControlFP::fnCollapseTimeRangeToCurrLayer, _T("CollapseTimeRangeToCurrLayer"), 0, TYPE_VOID, 0, 3,
			_T("StartTime"), 0, TYPE_TIMEVALUE,
			_T("EndTime"), 0, TYPE_TIMEVALUE,
			_T("Frequency"), 0, TYPE_TIMEVALUE,
		ICATControlFP::fnResetTransforms, _T("ResetTransforms"), 0, TYPE_VOID, 0, 0,

		properties,

		ICATControlFP::propGetCATParent, FP_NO_FUNCTION, _T("CATParent"), 0, TYPE_INODE,
		ICATControlFP::propGetName, ICATControlFP::propSetName, _T("Name"), 0, TYPE_TSTR_BV,

		p_end
	);

	return &cat_control_FPinterface;
}
