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

// Leg's weight on Character movement

#include "CatPlugins.h"
#include "BezierInterp.h"
#include "LegWeight.h"
#include "CATMotionLimb.h"
#include "CATHierarchyBranch2.h"

static LegWeightClassDesc LegWeightDesc;
ClassDesc2* GetLegWeightDesc() { return &LegWeightDesc; }

enum { LegWeight_params };

static ParamBlockDesc2 LegWeight_param_blk(LegWeight_params, _T("params"), 0, &LegWeightDesc,
	P_AUTO_CONSTRUCT, LegWeight::PBLOCK_REF,
	// params
	LegWeight::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	LegWeight::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,

	LegWeight::PB_LIFTSTARTTIME, _T("LiftStartTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTSTART,
		p_default, 5.0f,
		p_end,
	LegWeight::PB_LIFTENDTIME, _T("LiftEndTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTEND,
		p_default, 45.0f,
		p_end,
	LegWeight::PB_PLANTSTARTTIME, _T("PlantStartTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTSTART,
		p_default, 55.0f,
		p_end,
	LegWeight::PB_PLANTENDTIME, _T("TroughTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTEND,
		p_default, 85.0f,
		p_end,

	p_end
);

LegWeight::LegWeight()
{
	LegWeightDesc.MakeAutoParamBlocks(this);
}

LegWeight::~LegWeight() {
	DeleteAllRefs();
}

RefTargetHandle LegWeight::Clone(RemapDir& remap)
{
	// make a new LegWeight object to be the clone
	LegWeight *newLegWeight = new LegWeight();
	CloneCATGraph(newLegWeight, remap);
	BaseClone(this, newLegWeight, remap);
	return newLegWeight;
}

void LegWeight::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		LegWeight *newctrl = (LegWeight*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void LegWeight::GetCATKey(const int			i,
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
			pblock->GetFloat(PB_LIFTSTARTTIME, t) * BASE,
			1.0f,
			0.0f,
			0.33f,
			0.33f);
		isInTan = FALSE;
		isOutTan = TRUE;
		break;
	}
	case 1:
	{
		key = CATKey(
			pblock->GetFloat(PB_LIFTENDTIME, t) * BASE,
			0.0f,
			0.0f,
			0.33f,
			0.33f);
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 2:
	{
		key = CATKey(
			pblock->GetFloat(PB_PLANTSTARTTIME, t) * BASE,
			0.0f,
			0.0f,
			0.33f,
			0.33f);
		isInTan = TRUE;
		isOutTan = FALSE;
		break;
	}
	case 3:
	{
		key = CATKey(
			pblock->GetFloat(PB_PLANTENDTIME, t) * BASE,
			1.0f,
			0.0f,
			0.33f,
			0.33f);
		isInTan = TRUE;
		isOutTan = FALSE;
		break;
	}
	default: {
		key = CATKey();
		isInTan = TRUE;
		isOutTan = TRUE;
	}
	}
}

/* ************************************************************************** **
** Description: Hooks up the controller pointers to the						  **
**        correct branches in the hierachy									  **
** ************************************************************************** */
void LegWeight::GetGraphKey(
	int iKeyNum, CATHierarchyBranch* ctrlBranch,
	Control**	ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
	Control**	ctrlValue, float &fValueVal, float &minVal, float &maxVal,
	Control**	ctrlTangent, float &fTangentVal,
	Control**	ctrlInTanLen, float &fInTanLenVal,
	Control**	ctrlOutTanLen, float &fOutTanLenVal,
	Control**	ctrlSlider)
{
	UNREFERENCED_PARAMETER(maxVal); UNREFERENCED_PARAMETER(minVal);
	TimeValue t = GetCOREInterface()->GetTime();

	switch (iKeyNum) {
	case 1:
		fPrevKeyTime = pblock->GetFloat(PB_PLANTENDTIME, t) - STEPRATIO100;
		fNextKeyTime = pblock->GetFloat(PB_LIFTENDTIME, t);
		fTimeVal = 15.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_LIFTSTART));
		fValueVal = 1.0f;		*ctrlValue = NULL;
		break;
	case 2:
		fPrevKeyTime = pblock->GetFloat(PB_LIFTSTARTTIME, t);
		fNextKeyTime = pblock->GetFloat(PB_PLANTSTARTTIME, t);
		fTimeVal = 35.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_LIFTEND));
		fValueVal = 0.0f;		*ctrlValue = NULL;
		break;
	case 3:
		fPrevKeyTime = pblock->GetFloat(PB_LIFTENDTIME, t);
		fNextKeyTime = pblock->GetFloat(PB_PLANTENDTIME, t);
		fTimeVal = 65.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_PLANTSTART));
		fValueVal = 0.0f;		*ctrlValue = NULL;
		break;
	case 4:
		fPrevKeyTime = pblock->GetFloat(PB_PLANTSTARTTIME, t);
		fNextKeyTime = pblock->GetFloat(PB_LIFTSTARTTIME, t) + STEPRATIO100;
		fTimeVal = 85.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_PLANTEND));
		fValueVal = 1.0f;		*ctrlValue = NULL;
		break;
	default:
		fTimeVal = 50.0f;		*ctrlTime = NULL;
		fValueVal = 1.0f;		*ctrlValue = NULL;
	}

	fTangentVal = 0.0f;		*ctrlTangent = NULL;
	fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
	fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
	*ctrlSlider = NULL;
}

/* ************************************************************************** **
** Description: Finds the max and min key value for this graph and sets the   **
**		new max and min if nececassry										  **
** ************************************************************************** */
float LegWeight::GetGraphYval(TimeValue t, int LoopT)
{
	return GetYval(t, LoopT);
}

COLORREF LegWeight::GetGraphColour()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if (ctrlLimbData) return asRGB(ctrlLimbData->GetLimbColour());
	return RGB(200, 0, 0);
}

TSTR LegWeight::GetGraphName()  // SA 10/09 - none of the GetGraphName functions are ever called
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if (ctrlLimbData) return ctrlLimbData->GetLimbName();
	return GetString(IDS_LEGWEIGHT);
}

float LegWeight::GetYval(TimeValue t, int LoopT)
{
	if (LoopT < 0)
		LoopT += STEPTIME100;

	float liftS = pblock->GetFloat(PB_LIFTSTARTTIME, t) * BASE;
	float liftE = pblock->GetFloat(PB_LIFTENDTIME, t) * BASE;
	float plantS = pblock->GetFloat(PB_PLANTSTARTTIME, t) * BASE;
	float plantE = pblock->GetFloat(PB_PLANTENDTIME, t) * BASE;

	if (liftS < 0) liftS = 0.0f;
	if (plantS < liftE) plantS = liftE;
	if (plantE > STEPTIME100) plantE = STEPTIME100;

	// The foot has not yet Lifted (weight)
	if (LoopT < liftS)
		return 1.0f;

	// The foot has just lifted off the ground (falling to 0)
	if (LoopT < liftE)
	{
		CATKey key1(liftS, 1.0f, 0, 0.333f, 0);
		CATKey key2(liftE, 0.0f, 0, 0, 0.333f);

		return InterpValue(key1, key2, LoopT);
	}
	// the foot is in the air (steady 0)
	if (LoopT < plantS)
		return 0.0f;

	// the foot is hitting the ground (rise to 1)
	if (LoopT < plantE)
	{
		CATKey key1(plantS, 0.0f, 0, 0.333f, 0);
		CATKey key2(plantE, 1.0f, 0, 0, 0.333f);

		return InterpValue(key1, key2, LoopT); //, &bezierval);
	}
	return 1.0f;
}

/*
 * This function is bezier interp from 0 to 1 over the STEPTIME100 ticks. It represents the
 * legs weight on body-shift graphs over a step. As the foot hits the ground (BASE15) the
 * graph rises to 1 ect.
 */
void LegWeight::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	valid.SetInstant(t);
	CATHierarchyBranch2* branch = GetBranch();
	if (branch == NULL)
		return;
	CATHierarchyRoot* pRoot = branch->GetCATRoot();
	if (pRoot == NULL)
		return;

	int LoopT;
	float dFootStepMask = .0f;;
	if (GetCATMotionLimb())
	{
		dFootStepMask = GetCATMotionLimb()->GetStepTime(t, 1.0f, LoopT);
		LoopT = LoopT % STEPTIME100;
	}
	else
	{
		if (method == CTRL_RELATIVE)
		{
			LoopT = (int)*(float*)val % STEPTIME100;
		}
		else
		{
			LoopT = t % STEPTIME100;
		}
	}
	float kaliftamount = pRoot->GetKALifting(t);
	if (dFootStepMask > 0.0f)
		*(float*)val = GetYval(t, LoopT) * dFootStepMask * kaliftamount;
	else *(float*)val = 0.0f;
}

