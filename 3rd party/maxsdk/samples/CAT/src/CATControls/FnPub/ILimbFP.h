//**************************************************************************/
// Copyright (c) 1998-2011 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/

#pragma once

#include <CATAPI/ILimb.h>

class ILimbFP : public CatAPI::ILimb
{
public:
	enum {
/*		fnCollapsePoseToCurrLayer,
		fnCollapseTimeRangeToCurrLayer,
		fnResetTransforms,
*/
		fnCreateIKTarget,
		fnRemoveIKTarget,
		fnMoveIKTargetToEndOfLimb,

		fnCreateUpNode,
		fnRemoveUpNode,

		fnCreatePalmAnkle,
		fnRemovePalmAnkle,
		fnCreateCollarbone,
		fnRemoveCollarbone,

		propIKTarget,
		propUpNode,
		propIsLeg,
		propIsArm,
		propGetLMR,
		propSetLMR,

		propPalm,
		propCollarbone,

		propGetSymLimb
	};

	BEGIN_FUNCTION_MAP

//		VFNT_0(fnCollapsePoseToCurrLayer,		ICollapsePoseToCurrLayer );
//		VFN_3(fnCollapseTimeRangeToCurrLayer,	ICollapseTimeRangeToCurrLayer, TYPE_TIMEVALUE, TYPE_TIMEVALUE, TYPE_TIMEVALUE);
//		VFNT_0(fnResetTransforms,				IResetTransforms );

		FN_0(fnCreateIKTarget, TYPE_INODE, CreateIKTarget);
		FN_0(fnRemoveIKTarget, TYPE_BOOL, RemoveIKTarget);
		VFN_1(fnMoveIKTargetToEndOfLimb, MoveIKTargetToEndOfLimb, TYPE_TIMEVALUE);

		FN_0(fnCreateUpNode, TYPE_INODE, CreateUpNode);
		FN_0(fnRemoveUpNode, TYPE_BOOL, RemoveUpNode);

		FN_0(fnCreatePalmAnkle, TYPE_INTERFACE, CreatePalmAnkle);
		FN_0(fnRemovePalmAnkle, TYPE_BOOL, RemovePalmAnkle);
		FN_0(fnCreateCollarbone, TYPE_INTERFACE, CreateCollarbone);
		FN_0(fnRemoveCollarbone, TYPE_BOOL, RemoveCollarbone);

		RO_PROP_FN(propIKTarget, GetIKTarget, TYPE_INODE);
		RO_PROP_FN(propUpNode, GetUpNode, TYPE_INODE);
		RO_PROP_FN(propIsLeg, GetisLeg, TYPE_BOOL);
		RO_PROP_FN(propIsArm, GetisArm, TYPE_BOOL);
		RO_PROP_FN(propPalm, GetPalmAnkleINodeControl, TYPE_INTERFACE);
		RO_PROP_FN(propCollarbone, GetCollarboneINodeControl, TYPE_INTERFACE);
		PROP_FNS(propGetLMR, GetLMR, propSetLMR, SetLMR, TYPE_INT);

		RO_PROP_FN(propGetSymLimb, GetSymLimb, TYPE_INTERFACE);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	static FPInterfaceDesc* GetFnPubDesc();
};
