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

#include <CATAPI/ILayerRoot.h>

#pragma warning(push)
#pragma warning(disable:4238) // necessary for _BV types

// This class implements the MxSExposure for ILayerRoot

using namespace CatAPI;

class ILayerRootFP : public CatAPI::ILayerRoot
{
public:
	enum ILayerRootFnPubOps
	{
		fnAppendLayer,
		fnInsertLayer,
		fnRemoveLayer,
		fnMoveLayerUp,
		fnMoveLayerDown,

		fnGetLayerColor,
		fnSetLayerColor,

		fnSaveClip,
		fnSavePose,

		fnLoadClip,
		fnLoadPose,
		fnCreatePasteLayerTransformNode,
		fnGetFileTagValue,

		fnLoadHTR,
		fnLoadBVH,
		fnLoadFBX,
		fnLoadBIP,

		fnCollapsePoseToCurLayer,
		fnCollapseTimeRangeToLayer,

		fnCopyLayer,
		fnPasteLayer,

		propNumLayers,
		propGetSelectedLayer,
		propSetSelectedLayer,
		propGetSoloLayer,
		propSetSoloLayer,
		propGetTrackDisplayMethod,
		propSetTrackDisplayMethod,
	};
	BEGIN_FUNCTION_MAP
		FN_2(fnAppendLayer, TYPE_INT, AppendLayer, TYPE_TSTR, TYPE_NAME);
		FN_3(fnInsertLayer, TYPE_BOOL, InsertLayer, TYPE_TSTR, TYPE_INDEX, TYPE_NAME);
		VFN_1(fnRemoveLayer, RemoveLayer, TYPE_INDEX);

		VFN_1(fnMoveLayerUp, MoveLayerUp, TYPE_INDEX);
		VFN_1(fnMoveLayerDown, MoveLayerDown, TYPE_INDEX);

		FN_1(fnGetLayerColor, TYPE_COLOR, GetLayerColor, TYPE_INDEX);
		FN_2(fnSetLayerColor, TYPE_BOOL, SetLayerColor, TYPE_INDEX, TYPE_COLOR);

		FN_5(fnSaveClip, TYPE_BOOL, SaveClip, TYPE_TSTR, TYPE_TIMEVALUE, TYPE_TIMEVALUE, TYPE_INDEX, TYPE_INDEX);
		FN_1(fnSavePose, TYPE_BOOL, SavePose, TYPE_TSTR);

		FN_7(fnLoadClip, TYPE_INODE, LoadClip, TYPE_TSTR, TYPE_TIMEVALUE, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL);
		FN_7(fnLoadPose, TYPE_INODE, LoadPose, TYPE_TSTR, TYPE_TIMEVALUE, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL);
		FN_0(fnCreatePasteLayerTransformNode, TYPE_INODE, CreatePasteLayerTransformNode);

		FN_2(fnGetFileTagValue, TYPE_TSTR_BV, GetFileTagValue, TYPE_TSTR, TYPE_TSTR);

		FN_2(fnLoadHTR, TYPE_BOOL, LoadHTR, TYPE_TSTR, TYPE_TSTR);
		FN_2(fnLoadBVH, TYPE_BOOL, LoadBVH, TYPE_TSTR, TYPE_TSTR);

		FN_2(fnLoadFBX, TYPE_BOOL, LoadFBX, TYPE_TSTR, TYPE_TSTR);
		FN_2(fnLoadBIP, TYPE_BOOL, LoadBIP, TYPE_TSTR, TYPE_TSTR);

		VFNT_0(fnCollapsePoseToCurLayer, CollapsePoseToCurLayer);
		FN_7(fnCollapseTimeRangeToLayer, TYPE_BOOL, CollapseTimeRangeToLayer, TYPE_TIMEVALUE, TYPE_TIMEVALUE, TYPE_TIMEVALUE, TYPE_BOOL, TYPE_INT, TYPE_FLOAT, TYPE_FLOAT);

		VFN_1(fnCopyLayer, CopyLayer, TYPE_INDEX);
		VFN_2(fnPasteLayer, PasteLayer, TYPE_BOOL, TYPE_BOOL);

		RO_PROP_FN(propNumLayers, NumLayers, TYPE_INT);
		PROP_FNS(propGetSelectedLayer, GetSelectedLayer, propSetSelectedLayer, SelectLayer, TYPE_INDEX);
		PROP_FNS(propGetSoloLayer, GetSoloLayer, propSetSoloLayer, SoloLayer, TYPE_INDEX);
		PROP_FNS(propGetTrackDisplayMethod, GetTrackDisplayMethod, propSetTrackDisplayMethod, SetTrackDisplayMethod, TYPE_INT);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc() { return GetDescByID(LAYERROOT_INTERFACE_FP); }

};

#pragma warning(pop)