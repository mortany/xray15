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

#include <CATAPI/INodeControl.h>

#pragma warning(push)

class INodeControlFP : public CatAPI::INodeControl
{
public:
	enum {
		fnAddArbBone,
		fnGetArbBone,
		fnCreateArbBoneController,
		fnCreateLayerFloat,

		// properties
		propGetNumArbBones,
//		propArbBones,
		propGetNode,
		propGetAddress,

		propGetData,

		propGetSetupTM,
		propSetSetupTM,
		propBoneDimensions,

		propGetSetupModeColour,
		propSetSetupModeColour,
		propGetSetupStretchy,
		propSetSetupStretchy,
		propGetAnimationStretchy,
		propSetAnimationStretchy,
		propGetEffectHierarchy,
		propSetEffectHierarchy,
		propGetUseSetupController,
		propSetUseSetupController,

		propNumLayerfloats,

		propArbBones,

		propGetSetupModeInherit,
		propSetSetupModeInherit,
		propGetAnimationModeInherit,
		propSetAnimationModeInherit,

		propGetRelativeToSetupMode,
		propSetRelativeToSetupMode,

		propGetSetupModeLocks,
		propSetSetupModeLocks,
		propGetAnimationLocks,
		propSetAnimationLocks,
		propGetMirrorBone,
		propSetMirrorBone
	};

	BEGIN_FUNCTION_MAP
		RO_PROP_FN(propGetNumArbBones, GetNumArbBones, TYPE_INT);
		FN_1(fnAddArbBone, TYPE_INTERFACE, AddArbBone, TYPE_BOOL);
		FN_1(fnGetArbBone, TYPE_INTERFACE, GetArbBone, TYPE_INDEX);
		FN_1(fnCreateArbBoneController, TYPE_CONTROL, CreateArbBoneController, TYPE_BOOL);

		RO_PROP_FN(propNumLayerfloats, NumLayerFloats, TYPE_INT);
		FN_0(fnCreateLayerFloat, TYPE_CONTROL, CreateLayerFloat);

		RO_PROP_FN(propGetNode, GetNode, TYPE_INODE);
		RO_PROP_FN(propGetData, GetManager, TYPE_INTERFACE);

		RO_PROP_FN(propBoneDimensions, GetBoneDimensions, TYPE_POINT3_BV);
		RO_PROP_FN(propGetAddress, GetBoneAddress, TYPE_TSTR_BV);
		PROP_FNS(propGetSetupTM, GetSetupMatrix, propSetSetupTM, SetSetupMatrix, TYPE_MATRIX3_BV);

		// all the flags we see in the hierarchy panel
		PROP_FNS(propGetSetupStretchy, GetSetupStretchy, propSetSetupStretchy, SetSetupStretchy, TYPE_BOOL);
		PROP_FNS(propGetAnimationStretchy, GetAnimationStretchy, propSetAnimationStretchy, SetAnimationStretchy, TYPE_BOOL);
		PROP_FNS(propGetEffectHierarchy, GetEffectHierarchy, propSetEffectHierarchy, SetEffectHierarchy, TYPE_BOOL);
		PROP_FNS(propGetUseSetupController, GetUseSetupController, propSetUseSetupController, SetUseSetupController, TYPE_BOOL);

		PROP_FNS(propGetSetupModeInherit, GetSetupModeInheritance, propSetSetupModeInherit, SetSetupModeInheritance, TYPE_BITARRAY);
		PROP_FNS(propGetAnimationModeInherit, GetAnimModeInheritance, propSetAnimationModeInherit, SetAnimModeInheritance, TYPE_BITARRAY);
		PROP_FNS(propGetRelativeToSetupMode, GetRelativeToSetupMode, propSetRelativeToSetupMode, SetRelativeToSetupMode, TYPE_BOOL);

		PROP_FNS(propGetSetupModeLocks, GetSetupModeLocks, propSetSetupModeLocks, SetSetupModeLocks, TYPE_BITARRAY);
		PROP_FNS(propGetAnimationLocks, GetAnimationLocks, propSetAnimationLocks, SetAnimationLocks, TYPE_BITARRAY);
		PROP_FNS(propGetMirrorBone, GetMirrorBone, propSetMirrorBone, SetMirrorBone, TYPE_INODE);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	static FPInterfaceDesc* GetFnPubDesc();
};

#pragma warning(pop)
