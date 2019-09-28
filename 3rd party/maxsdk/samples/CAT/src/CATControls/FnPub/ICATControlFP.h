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

#include <CATAPI/ICATControl.h>

#pragma warning(push)
#pragma warning(disable:4238) // necessary for _BV types

class ICATControlFP : public CatAPI::ICATControl
{
	enum {
		fnPasteLayer,
		fnPasteRig,

		fnSaveClip,
		fnLoadClip,

		fnSavePose,
		fnLoadPose,

		fnCollapsePoseToCurrLayer,
		fnCollapseTimeRangeToCurrLayer,
		fnResetTransforms,

		propGetCATParent,
		propGetName,
		propSetName
	};

	BEGIN_FUNCTION_MAP
		VFN_4(fnPasteLayer, PasteLayer, TYPE_CONTROL, TYPE_INT, TYPE_INT, TYPE_BOOL);
		FN_2(fnPasteRig, TYPE_BOOL, PasteFromCtrl, TYPE_REFTARG, TYPE_BOOL);

		FN_3(fnSaveClip, TYPE_BOOL, SaveClip, TYPE_TSTR, TYPE_TIMEVALUE, TYPE_TIMEVALUE);
		FN_3(fnLoadClip, TYPE_INODE, LoadClip, TYPE_TSTR, TYPE_TIMEVALUE, TYPE_BOOL);

		FN_1(fnSavePose, TYPE_BOOL, SavePose, TYPE_TSTR);
		FN_3(fnLoadPose, TYPE_INODE, LoadPose, TYPE_TSTR, TYPE_TIMEVALUE, TYPE_BOOL);

		VFNT_0(fnCollapsePoseToCurrLayer, CollapsePoseToCurrLayer);
		VFN_3(fnCollapseTimeRangeToCurrLayer, CollapseTimeRangeToCurrLayer, TYPE_TIMEVALUE, TYPE_TIMEVALUE, TYPE_TIMEVALUE);
		VFNT_0(fnResetTransforms, ResetTransforms);

		RO_PROP_FN(propGetCATParent, GetCATParent, TYPE_INODE);
		PROP_FNS(propGetName, GetName, propSetName, SetName, TYPE_TSTR_BV);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	static FPInterfaceDesc* GetFnPubDesc();
};

#pragma warning(pop)