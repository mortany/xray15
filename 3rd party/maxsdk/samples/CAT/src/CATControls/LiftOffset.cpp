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
 //class LiftPlantDlg;
#include "CATHierarchy.h"
#include "CATMotionLimb.h"
#include "LiftOffset.h"

// our global instance of our classdesc class.
static LiftOffsetClassDesc LiftOffsetDesc;
ClassDesc2* GetLiftOffsetDesc() { return &LiftOffsetDesc; }

//
//	LiftOffset  Implementation.
//
//	Make it work
//
//	Steve T. 12 Nov 2002 <- (This is actaully almost the correct Date...)
static ParamBlockDesc2 liftoffset_param_blk(LiftOffset::PBLOCK_REF, _T("params"), 0, &LiftOffsetDesc,
	P_AUTO_CONSTRUCT, LiftOffset::PBLOCK_REF,
	LiftOffset::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	LiftOffset::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,
	LiftOffset::PB_LIFTVAL, _T("LiftValue"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTVALUE,
		p_default, 0.0f,
		p_end,
	LiftOffset::PB_PLANTVAL, _T("PlantValue"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTVALUE,
		p_default, 0.0f,
		p_end,

	p_end
);

RefTargetHandle LiftOffset::Clone(RemapDir& remap)
{
	// make a new LiftOffset object to be the clone
	LiftOffset *newliftoffset = new LiftOffset();
	CloneCATGraph(newliftoffset, remap);
	BaseClone(this, newliftoffset, remap);
	return newliftoffset;
}

LiftOffset::LiftOffset()
{
	LiftOffsetDesc.MakeAutoParamBlocks(this);
}

LiftOffset::~LiftOffset()
{
	DeleteAllRefsFromMe();
}

// Will be phased out later (apparently)
void LiftOffset::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		//LiftOffset *newctrl = (LiftOffset*)from;

//		if (newctrl->ctrlLiftOffset)
//			ReplaceReference(LiftOffset::LIFTPLANTRATIO, newctrl->ctrlLiftOffset);

	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}
/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void LiftOffset::GetCATKey(const int			i,
	const TimeValue	t,
	CATKey				&key,
	bool				&isInTan,
	bool				&isOutTan) const
{
	switch (i)
	{
	case 0:
	{

		key = CATKey(
			STEPTIME25,
			pblock->GetFloat(PB_LIFTVAL, t),
			0.0f,
			0.0f,
			0.0f);
		isInTan = FALSE;
		isOutTan = FALSE;
		break;
	}
	case 1:
	{
		key = CATKey(
			STEPTIME75,
			pblock->GetFloat(PB_PLANTVAL, t),
			0.0f,
			0.0f,
			0.0f);
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
void LiftOffset::GetGraphKey(
	int iKeyNum, CATHierarchyBranch* ctrlBranch,
	Control**	ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
	Control**	ctrlValue, float &fValueVal, float &minVal, float &maxVal,
	Control**	ctrlTangent, float &fTangentVal,
	Control**	ctrlInTanLen, float &fInTanLenVal,
	Control**	ctrlOutTanLen, float &fOutTanLenVal,
	Control**	ctrlSlider)
{
	UNREFERENCED_PARAMETER(maxVal); UNREFERENCED_PARAMETER(minVal);
	switch (iKeyNum) {
	case 1:
		fPrevKeyTime = -STEPRATIO25;
		fNextKeyTime = STEPRATIO75;
		fTimeVal = 50.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_LIFTVALUE));
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
		break;
	case 2:
		fPrevKeyTime = STEPRATIO25;
		fNextKeyTime = STEPRATIO100 + STEPRATIO25;

		fTimeVal = 50.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_PLANTVALUE));
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
		break;
	}
	*ctrlSlider = NULL;
}

float LiftOffset::GetYval(TimeValue t, int LoopT)
{
	float LiftVal = pblock->GetFloat(PB_LIFTVAL, t);
	float PlantVal = pblock->GetFloat(PB_PLANTVAL, t);

	if (LoopT < STEPTIME25)
	{   // (plant > Lift)
		LoopT += STEPTIME25;
		return PlantVal + ((LiftVal - PlantVal) * ((float)LoopT / (float)STEPTIME50));
	}
	if (LoopT > STEPTIME75)
	{	// (Plant > Lift)
		LoopT -= STEPTIME75;
		return PlantVal + ((LiftVal - PlantVal) * ((float)LoopT / (float)STEPTIME50));
	}

	// (Lift > Plant)
	LoopT -= STEPTIME25;
	return LiftVal + ((PlantVal - LiftVal) * ((float)LoopT / (float)STEPTIME50));

	/*	if((LoopT < STEPTIME25)||(LoopT > STEPTIME75))				// first 15 frames (ish)
		{
			if(LoopT > STEPTIME75)
				 LoopT -= STEPTIME75;
			else LoopT += STEPTIME25;
			(*((float*)val)) = PlantVal + ((LiftVal - PlantVal) * (LoopT/STEPTIME50));
			return;
		}

		LoopT -= STEPTIME25;
		(*((float*)val)) = LiftVal + ((PlantVal - LiftVal) * (LoopT/STEPTIME50));
		*/
}

// Brains behind the idea here??
void LiftOffset::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	valid.SetInstant(t);

	int LoopT;
	float dFootStepMask = 0.0f;
	if (GetCATMotionLimb())
	{
		dFootStepMask = GetCATMotionLimb()->GetStepTime(t, 1.0f, LoopT);
		LoopT = LoopT % STEPTIME100;
	}
	else
	{
		if (method == CTRL_RELATIVE)
			LoopT = (int)(*(float*)val);
		else
			LoopT = t;
	}

	LoopT = LoopT % STEPTIME100;

	if (dFootStepMask > 0.0f)
		*(float*)val = GetYval(t, LoopT) * dFootStepMask;
	else *(float*)val = 0.0f;
}
