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

#include "CatPlugins.h"
#include "AllClasses.h"
#include "../CATMuscle/AllClasses.h"

 // Legacy ClassDesc's are duplicate class desc's used to migrate
 // old files to the new SuperClass ID
extern ClassDesc2* GetBoneDataDescLegacy();
extern ClassDesc2* GetLimbData2DescLegacy();
extern ClassDesc2* GetSpineData2DescLegacy();
extern ClassDesc2* GetDigitDataDescLegacy();

extern ClassDesc2* GetIKTargetObjDesc();
extern ClassDesc2* GetHDPivotTransDesc();
extern ClassDesc2* GetHIPivotTransDesc();

//
//	Our global instance object.
//
HINSTANCE hInstance;

__declspec(dllexport) int LibInitialize(void)
{
	InitCATIcons();
	// Do not register our callbacks on file load
	// We only want to register callbacks that are
	// used, so we wait till a CATParent is created
	return TRUE;
};
__declspec(dllexport) int LibShutdown(void)
{
	// Always release (we can't tell when the last
	// CATParent is deleted, so this is how we deal
	// with releasing our callbacks)
	ReleaseCATParentGlobalCallbacks();
	ReleaseCATIcons();
	return TRUE;
};

// This function is called by Windows when the DLL is loaded.  This
// function may also be called many times during time critical operations
// like rendering.  Therefore developers need to be careful what they
// do inside this function.  In the code below, note how after the DLL is
// loaded the first time only a few statements are executed.
BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		MaxSDK::Util::UseLanguagePackLocale();
		hInstance = hinstDLL;				// Hang on to this DLL's instance handle.
		DisableThreadLibraryCalls(hInstance);
	}

	return (TRUE);
}

//
// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
//
__declspec(dllexport) const TCHAR* LibDescription()
{
	return GetString(IDS_CATLIBDESCRIPTION);
}

//
// This function returns the number of plug-in classes this DLL
// TODO: Must change this number when adding a new class
//
__declspec(dllexport) int LibNumberClasses()
{
	return 130;
}

//
//  This function returns the Class Description for each
//	plugin class implemented in this DLL.
//
__declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
	switch (i) {
		// Rig Structure
	case 1:	return GetCATParentTransDesc();
	case 2:	return GetHubDesc();
	case 3:	return GetLimbData2Desc();
	case 4:	return GetBoneSegTransDesc();
	case 5:	return GetBoneDataDesc();
	case 6:	return GetFootTrans2Desc();
	case 7:	return GetCollarboneTransDesc();
	case 8:	return GetSpineData2Desc();
	case 9:	return GetSpineTrans2Desc();
	case 10:	return GetPalmTrans2Desc();
	case 11:	return GetDigitSegTransDesc();
	case 12:	return GetDigitDataDesc();

	case 13:	return GetIKTargTransDesc();
	case 14:	return GetArbBoneTransDesc();
	case 15:	return GetTailData2Desc();
	case 16:	return GetTailTransDesc();
	case 17:	return GetCATWeightDesc();

		// Objects
	case 20:	return GetCATParentDesc();
	case 21:	return GetCATHubDesc();
	case 22:	return GetCATBoneDesc();
	case 23:	return GetIKTargetObjDesc();

		// Layers
	case 30:	return GetCATClipRootDesc();
	case 32:	return GetNLAInfoDesc();
	case 33:	return GetCATClipWeightsDesc();
	case 34:	return GetHIPivotTransDesc();
	case 36:	return GetCATTransformOffsetDesc();
	case 37:	return GetLayerTransformDesc();
	case 38:	return GetGizmoTransformDesc();
	case 39:	return GetMocapLayerInfoDesc();
	case 40:	return GetCATUnitsPosDesc();
	case 41:	return GetCATUnitsSclDesc();
	case 85:	return GetCATClipFloatDesc();
	case 90:	return GetCATClipMatrix3Desc();

		// CATMotion
	case 50:	return GetCATHierarchyRootDesc();
	case 51:	return GetCATMotionLayerDesc();
	case 52:	return GetCATHierarchyLeafDesc();
	case 53:	return GetCATHierarchyBranch2Desc();
	case 54:	return GetHDPivotTransDesc();
	case 55:	return GetLiftPlantModDesc();
	case 56:	return GetStepShapeDesc();
	case 57:	return GetFootLiftDesc();
	case 58:	return GetMonoGraphDesc();
	case 59:	return GetKneeAngleDesc();
	case 60:	return GetPivotPosDesc();
	case 61:	return GetWeightShiftDesc();
	case 62:	return GetEaseDesc();
	case 63:	return GetLegWeightDesc();
	case 64:	return GetPivotRotDesc();
	case 65:	return GetFootBendDesc();
	case 66:	return GetLiftOffsetDesc();
	case 67:	return GetCATMotionLimbDesc();
	case 68:	return GetCATMotionHub2Desc();
	case 69:	return GetCATMotionTailRotDesc();
	case 70:	return GetCATMotionDigitRotDesc();
	case 71:	return GetCATMotionPlatformDesc();
	case 72:	return GetCATMotionRotDesc();
	case 73:	return GetCATMotionTailDesc();
	case 75:	return GetCATp3Desc();

		// CATMuscle is included in CAT3
	case 110: return GetSegTransDesc();
	case 111: return GetMusclePatchDesc();
	case 112: return GetMuscleBonesDesc();
	case 113: return GetMuscleStrandDesc();
	case 114: return GetHdlObjDesc();
	case 115: return GetHdlTransDesc();

		// Obsolete objects just waiting to be turfed
	case 125: return GetRootNodeCtrlDesc();

		// Don't turf the below - required to remap old ClassID to new ClassID.
	case 126: return GetBoneDataDescLegacy();
	case 127: return GetLimbData2DescLegacy();
	case 128: return GetSpineData2DescLegacy();
	case 129: return GetDigitDataDescLegacy();

	default: return 0;
	}
}

// This function returns a pre-defined constant indicating the version of
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec(dllexport) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}

