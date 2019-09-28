#pragma once
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

#include <CATAPI/IHub.h>

/**********************************************************************
 * IHub: Published functions for the Hub
 */

class IHubFP : public CatAPI::IHub {
public:
	enum {
//		fnCollapsePoseToCurrLayer,
//		fnCollapseTimeRangeToCurrLayer,
//		fnResetTransforms,

//		fnKeyFreeform,
		fnAddArm,
		fnAddLeg,
		fnAddSpine,
		fnAddTail,

		propGetPinHub,
		propSetPinHub,
		propTMOrient,
		propDangleCtrl
	};

	BEGIN_FUNCTION_MAP

		VFN_2(fnAddArm, AddArm, TYPE_BOOL, TYPE_BOOL);
		VFN_2(fnAddLeg, AddLeg, TYPE_BOOL, TYPE_BOOL);
		VFN_1(fnAddSpine, AddSpine, TYPE_INT);
		VFN_1(fnAddTail, AddTail, TYPE_INT);

		PROP_FNS(propGetPinHub, GetPinHub, propSetPinHub, SetPinHub, TYPE_BOOL);
//		RO_PROP_FN(propTMOrient,	GettmOrient,	TYPE_MATRIX3_BR);
//		RO_PROP_FN(propDangleCtrl,	GetDangleCtrl,	TYPE_CONTROL);

	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	static FPInterfaceDesc* GetFnPubDesc();
};