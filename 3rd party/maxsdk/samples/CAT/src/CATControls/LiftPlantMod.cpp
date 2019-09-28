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
#include "BezierInterp.h"
#include <CATAPI/CATClassID.h>
#include "CATHierarchy.h"
#include "CATMotionLimb.h"
#include "LiftPlantMod.h"

 //
 //	LiftPlantModClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class LiftPlantModClassDesc : public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }							// Show this in create branch?
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading);  return new LiftPlantMod(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_LIFTPLANTMOD); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }		// This determins the type of our controller
	Class_ID		ClassID() { return LIFTPLANTMOD_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CAT LiftPlantMod"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

// our global instance of our classdesc class.
static LiftPlantModClassDesc LiftPlantModDesc;
ClassDesc2* GetLiftPlantModDesc() { return &LiftPlantModDesc; }

//
//	LiftPlantMod  Implementation.
//
//	Make it work
//
//	Steve T. 12 Nov 2002 <- (This is actaully almost the correct Date...)
static ParamBlockDesc2 liftplantmod_param_blk(LiftPlantMod::PBLOCK_REF, _T("params"), 0, &LiftPlantModDesc,
	P_AUTO_CONSTRUCT, LiftPlantMod::PBLOCK_REF,
	LiftPlantMod::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	LiftPlantMod::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,
	LiftPlantMod::PB_LIFTPLANTRATIO, _T("LiftPlantRatio"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTPLANTRATIO,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.45f,
		p_end,
	LiftPlantMod::PB_CATMOTIONLIMB, _T("CATMotionLimb"), TYPE_REFTARG, P_NO_REF, IDS_CL_CATMOTIONLIMB,
		p_end,
	p_end
);

LiftPlantMod::LiftPlantMod()
{
	LIFTPLANTHEIGHT = 10.0f;

	LiftPlantModDesc.MakeAutoParamBlocks(this);
}

LiftPlantMod::~LiftPlantMod()
{
	DeleteAllRefs();
}

// Will be phased out later (apparently)
void LiftPlantMod::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		//LiftPlantMod *newctrl = (LiftPlantMod*)from;

//		if (newctrl->ctrlLiftPlantMod)
//			ReplaceReference(LiftPlantMod::LIFTPLANTRATIO, newctrl->ctrlLiftPlantMod);

	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

RefTargetHandle LiftPlantMod::Clone(RemapDir& remap)
{
	LiftPlantMod *newLiftPlantMod = new LiftPlantMod();
	CloneCATGraph(newLiftPlantMod, remap);
	BaseClone(this, newLiftPlantMod, remap);
	return newLiftPlantMod;
}

CATMotionLimb* LiftPlantMod::GetCATMotionLimb() {
	return (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	//	return catmotionlimb;
};

void LiftPlantMod::SetCATMotionLimb(Control* newLimb) {
	pblock->SetValue(REF_CATMOTIONLIMB, 0, (ReferenceTarget*)newLimb);
	//	catmotionlimb = (CATMotionLimb*)newLimb;
};

// DESCRIPTION: Returns the ith CAKey.
void LiftPlantMod::GetCATKey(const int			i,
	const TimeValue	t,
	CATKey				&key,
	bool				&isInTan,
	bool				&isOutTan) const
{
	float LiftPlantRatioVal = pblock->GetFloat(PB_LIFTPLANTRATIO, t);

	switch (i)
	{
	case 0:
	{
		key = CATKey(
			(STEPTIME50 - (LiftPlantRatioVal*STEPTIME50)),
			0,
			0.0f,
			0.33f,
			0.33f
		);
		isInTan = FALSE;
		isOutTan = FALSE;
		break;
	}
	case 1:
	{
		key = CATKey(
			STEPTIME50,
			LIFTPLANTHEIGHT,
			0.0f,
			0.33f,
			0.33f);
		isInTan = FALSE;
		isOutTan = FALSE;
		break;
	}
	case 2:
	{
		key = CATKey(
			(STEPTIME50 + (LiftPlantRatioVal*STEPTIME50)),
			0.0f,
			0.0f,
			0.33f,
			0.33f);
		isInTan = FALSE;
		isOutTan = FALSE;
		break;
	}
	default: {
		key = CATKey();
		isInTan = FALSE;
		isOutTan = FALSE;
	}
	}
}

/* ************************************************************************** **
** Description: Returns the controller and default values for each key		  **
** ************************************************************************** */
void LiftPlantMod::GetGraphKey(
	int iKeyNum, CATHierarchyBranch* ctrlBranch,
	Control**	ctrlTime, float &fTimeVal, float &minTime, float &maxTime,
	Control**	ctrlValue, float &fValueVal, float &minVal, float &maxVal,
	Control**	ctrlTangent, float &fTangentVal,
	Control**	ctrlInTanLen, float &fInTanLenVal,
	Control**	ctrlOutTanLen, float &fOutTanLenVal,
	Control**	ctrlSlider)
{
	UNREFERENCED_PARAMETER(maxVal); UNREFERENCED_PARAMETER(minVal); UNREFERENCED_PARAMETER(minTime); UNREFERENCED_PARAMETER(maxTime); UNREFERENCED_PARAMETER(iKeyNum);
	fTimeVal = 50.0f;		*ctrlTime = NULL;
	fValueVal = 0.0f;		*ctrlValue = NULL;
	fTangentVal = 0.0f;		*ctrlTangent = NULL;
	fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
	fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;

	*ctrlSlider = ctrlBranch->GetBranch(GetString(IDS_LIFTPLANTRATIO));
}

/*
void LiftPlantMod::GetYRange(TimeValue t, float	&minY, float &maxY)
{
	minY = min(minY, min(0.0f, 10.0f));
	maxY = max(maxY, max(0.0f, 10.0f));

}

float LiftPlantMod::GetGraphYval(TimeValue t, int LoopT)
{
	return GetYval(t, LoopT);
}

COLORREF LiftPlantMod::GetGraphColour()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return asRGB(ctrlLimbData->GetLimbColour());
	return RGB(200, 0, 0);
}
TSTR LiftPlantMod::GetGraphName()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return ctrlLimbData->GetLimbName();
	return _T("LiftPlantMod");
}

*/

float LiftPlantMod::GetYval(TimeValue t, int LoopT)
{
	float LiftPlantRatioVal = GetRatio(t);

	float LiftTime = STEPTIME50 - (LiftPlantRatioVal*STEPTIME50);
	float PlantTime = STEPTIME50 + (LiftPlantRatioVal*STEPTIME50);

	if (LoopT < LiftTime)
		return 0.0f;

	if (LoopT > PlantTime)
		return 0.0f;

	/////////////
	if (LoopT < STEPTIME50)			// Time Foot is highest point in arch
	{
		CATKey key1(LiftTime, 0.0f, 0.0f, 0.33f, 0.33f);
		CATKey key2(STEPTIME50, LIFTPLANTHEIGHT, 0.0f, 0.33f, 0.33f);

		return InterpValue(key1, key2, LoopT);
	}
	else
	{
		CATKey key1(STEPTIME50, LIFTPLANTHEIGHT, 0.0f, 0.33f, 0);
		CATKey key2(PlantTime, 0.0f, 0.0f, 0.0f, 0.33f);

		return InterpValue(key1, key2, LoopT);
	}
}

// Brains behind the idea here??
void LiftPlantMod::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	valid.SetInstant(t);

	float LiftPlantRatioVal = GetRatio(t);
	float LiftPlantModVal;

	//	if (this->ctrlLiftPlantMod)
	//		this->ctrlLiftPlantMod->GetValue(t, (void*)(&LiftPlantRatioVal), valid, CTRL_ABSOLUTE);

	if (LiftPlantRatioVal > 0.98f)
		LiftPlantRatioVal = 0.98f;
	else if (LiftPlantRatioVal < 0.02f)
		LiftPlantRatioVal = 0.02f;

	LiftPlantRatioVal = (((1.0f - LiftPlantRatioVal)*50.0f) - 25.0f);

	LiftPlantRatioVal *= BASE;							// Convert from FPS to Ticks per sec(ish...)
	int LoopT;
	if (method == CTRL_RELATIVE)
	{
		LoopT = (int)*(float*)val % STEPTIME100;
	}
	else
	{
		LoopT = t % STEPTIME100;
	}
	// Following code must have +ve numbers to work right,
	// ABS gives mirror image... Bad
	if (LoopT < 0) LoopT += STEPTIME100;
	//
	if (LoopT < (STEPTIME25 + LiftPlantRatioVal))				// first 15 frames (ish)
	{
		LiftPlantModVal = LoopT / (STEPTIME25 + LiftPlantRatioVal) *(-LiftPlantRatioVal);
	}
	else if (LoopT < (STEPTIME75 - LiftPlantRatioVal))		// next 30 (ish)
	{
		LiftPlantModVal = ((LoopT - (STEPTIME25 + LiftPlantRatioVal)) / (STEPTIME25 - LiftPlantRatioVal) - 1) * LiftPlantRatioVal;
	}
	else
	{													// last 15 frames of loop
		LiftPlantModVal = ((LoopT - (STEPTIME75 - LiftPlantRatioVal)) / (STEPTIME25 + LiftPlantRatioVal) * (-LiftPlantRatioVal)) + LiftPlantRatioVal;
	}

	(*((float*)val)) = LiftPlantModVal;
}

