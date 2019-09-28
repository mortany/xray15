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

// Controls the knee angle through lift/plant

#include "CatPlugins.h"
#include "BezierInterp.h"
#include "KneeAngle.h"
#include "CATMotionLimb.h"
#include "CATHierarchy.h"

static KneeAngleClassDesc KneeAngleDesc;
ClassDesc2* GetKneeAngleDesc() { return &KneeAngleDesc; }

enum { kneeangle_params };

// TODO: We Must run angles in Radians to be consistent... change later

static ParamBlockDesc2 kneeangle_param_blk(kneeangle_params, _T("params"), 0, &KneeAngleDesc,
	P_AUTO_CONSTRUCT, KneeAngle::PBLOCK_REF,
	// params
	KneeAngle::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	KneeAngle::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,
	KneeAngle::PB_LIFTVALUE, _T("LiftValue"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTVALUE,
		p_default, 35.0f,  // 40 degrees
		p_end,
	KneeAngle::PB_LIFTTANGENT, _T("LiftTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTTANGENT,
		p_default, 0.15f,
		p_end,
	KneeAngle::PB_LIFTOUTTANLEN, _T("LiftOutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTOUTTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333,
		p_end,
	KneeAngle::PB_LIFTINTANLEN, _T("LiftInTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTINTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333,
		p_end,
	KneeAngle::PB_MIDLIFTTIME, _T("MidLiftTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDLIFTTIME,
		p_default, 50.0f,
		p_end,
	KneeAngle::PB_MIDLIFTVALUE, _T("MidLiftValue"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDLIFTVALUE,
		p_default, 60.0f, // 60 degrees
		p_end,
	KneeAngle::PB_MIDLIFTTANGENT, _T("MidLiftTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDLIFTTANGENT,
		p_default, 0.0f,
		p_end,
	KneeAngle::PB_MIDLIFTINTANLEN, _T("MidLiftInTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDLIFTINTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.4f,
		p_end,
	KneeAngle::PB_MIDLIFTOUTTANLEN, _T("MidLiftOutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDLIFTOUTTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.4f,
		p_end,
	/////////////////////////////////////
	KneeAngle::PB_PLANTVALUE, _T("PlantValue"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTVALUE,
		p_default, 35.0f,  // 10 degrees
		p_end,
	KneeAngle::PB_PLANTTANGENT, _T("PlantTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTTANGENT,
		p_default, -0.15f,
		p_end,

	KneeAngle::PB_PLANTINTANLEN, _T("PlantInTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTINTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333,
		p_end,
	KneeAngle::PB_PLANTOUTTANLEN, _T("PlantOutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTOUTTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333,
		p_end,

	KneeAngle::PB_MIDPLANTTIME, _T("MidPlantTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDPLANTTIME,
		p_default, 0.0f,
		p_end,
	KneeAngle::PB_MIDPLANTVALUE, _T("MidPlantValue"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDPLANTVALUE,
		p_default, 25.0f, // 30 degrees
		p_end,
	KneeAngle::PB_MIDPLANTTANGENT, _T("MidPlantTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDPLANTTANGENT,
		p_default, 0.0f,
		p_end,
	KneeAngle::PB_MIDPLANTINTANLEN, _T("MidPlantInTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDPLANTINTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	KneeAngle::PB_MIDPLANTOUTTANLEN, _T("MidPlantOutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_MIDPLANTOUTTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	p_end
);

KneeAngle::KneeAngle()
{
	KneeAngleDesc.MakeAutoParamBlocks(this);
}

KneeAngle::~KneeAngle() {
	DeleteAllRefs();
}

RefTargetHandle KneeAngle::Clone(RemapDir& remap)
{
	// make a new KneeAngle object to be the clone
	KneeAngle *newKneeAngle = new KneeAngle();
	CloneCATGraph(newKneeAngle, remap);
	BaseClone(this, newKneeAngle, remap);
	return newKneeAngle;
}

void KneeAngle::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		KneeAngle *newctrl = (KneeAngle*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

/* ************************************************************************** **
** Description: This could be used in the ui to hook up spinners to and in	  **
**        bezier interp to interpolate values								  **
** **************************************************************************
CATGraphKey* KneeAngle::GetCATGraphKey(int iKeyNum)
{
	switch(iKeyNum) {
	case 1:
		return &CATGraphKey(
			0.0f,											//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			pblock->GetController(PB_MIDPLANTTIME),			//ctrlTime
			pblock->GetController(PB_MIDPLANTVALUE),		//ctrlValue
			pblock->GetController(PB_MIDPLANTTANGENT),		//ctrlTangent
			pblock->GetController(PB_MIDPLANTINTANLEN),		//ctrlInTanLength
			pblock->GetController(PB_MIDPLANTOUTTANLEN));	//ctrlOutTanLength
		break;
	case 2:
		return &CATGraphKey(
			STEPRATIO25,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			NULL,			//ctrlTime
			pblock->GetController(PB_PLANTVALUE),				//ctrlValue
			pblock->GetController(PB_PLANTTANGENT),			//ctrlTangent
			pblock->GetController(PB_PLANTINTANLEN),		//ctrlInTanLength
			pblock->GetController(PB_PLANTOUTTANLEN));	//ctrlOutTanLength
		break;
	case 3:
		return &CATGraphKey(
			STEPRATIO50,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			pblock->GetController(PB_MIDLIFTTIME),			//ctrlTime
			pblock->GetController(PB_MIDLIFTVALUE),				//ctrlValue
			pblock->GetController(PB_MIDLIFTTANGENT),			//ctrlTangent
			pblock->GetController(PB_MIDLIFTINTANLEN),		//ctrlInTanLength
			pblock->GetController(PB_MIDLIFTOUTTANLEN));	//ctrlOutTanLength
		break;
	case 4:
		return &CATGraphKey(
			STEPRATIO50,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			NULL,			//ctrlTime
			pblock->GetController(PB_LIFTVALUE),			//ctrlValue
			pblock->GetController(PB_LIFTTANGENT),			//ctrlTangent
			pblock->GetController(PB_LIFTINTANLEN),		//ctrlInTanLength
			pblock->GetController(PB_LIFTOUTTANLEN));	//ctrlOutTanLength
		break;
	default:// incase something goes wrong
		return &CATGraphKey(
			STEPRATIO50,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			NULL,					//ctrlTime
			NULL,					//ctrlValue
			NULL,					//ctrlTangent
			NULL,					//ctrlInTanLength
			NULL);					//ctrlOutTanLength
		break;
	}
}

void KneeAngle::GetYRange(TimeValue t, float	&minY, float &maxY)
{
	float liftval = pblock->GetFloat(PB_LIFTVALUE, t);
	float midliftval = pblock->GetFloat(PB_MIDLIFTVALUE, t);
	float plantval = pblock->GetFloat(PB_PLANTVALUE, t);
	float midplantval = pblock->GetFloat(PB_MIDPLANTVALUE, t);

	float KAMax = max(max(liftval, midliftval), max(plantval, midplantval));
	float KAMin = min(min(liftval, midliftval), min(plantval, midplantval));

	if(KAMax > maxY) maxY = KAMax;
	if(KAMin < minY) minY = KAMin;
}
*/

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void KneeAngle::GetCATKey(const int			i,
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
			pblock->GetFloat(PB_MIDPLANTTIME, t) * BASE,
			pblock->GetFloat(PB_MIDPLANTVALUE, t),
			pblock->GetFloat(PB_MIDPLANTTANGENT, t),
			pblock->GetFloat(PB_MIDPLANTOUTTANLEN, t),
			pblock->GetFloat(PB_MIDPLANTINTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 1:
	{
		key = CATKey(
			STEPTIME25,
			pblock->GetFloat(PB_LIFTVALUE, t),
			pblock->GetFloat(PB_LIFTTANGENT, t),
			pblock->GetFloat(PB_LIFTOUTTANLEN, t),
			pblock->GetFloat(PB_LIFTINTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 2:
	{
		key = CATKey(
			pblock->GetFloat(PB_MIDLIFTTIME, t) * BASE,
			pblock->GetFloat(PB_MIDLIFTVALUE, t),
			pblock->GetFloat(PB_MIDLIFTTANGENT, t),
			pblock->GetFloat(PB_MIDLIFTOUTTANLEN, t),
			pblock->GetFloat(PB_MIDLIFTINTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 3:
	{
		key = CATKey(
			STEPTIME75,
			pblock->GetFloat(PB_PLANTVALUE, t),
			pblock->GetFloat(PB_PLANTTANGENT, t),
			pblock->GetFloat(PB_PLANTOUTTANLEN, t),
			pblock->GetFloat(PB_PLANTINTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
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
void KneeAngle::GetGraphKey(
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
		fPrevKeyTime = -25.0f;	fNextKeyTime = 25.0f;
		fTimeVal = 0.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_MIDPLANTTIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_MIDPLANTVALUE));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_MIDPLANTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_MIDPLANTINTANLENGTH));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_MIDPLANTOUTTANLENGTH));
		break;
	case 2:
		fPrevKeyTime = pblock->GetFloat(PB_MIDPLANTTIME, t);
		fNextKeyTime = pblock->GetFloat(PB_MIDLIFTTIME, t);
		fTimeVal = 25.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_LIFTVALUE));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_LIFTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_LIFTINTANLENGTH));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_LIFTOUTTANLENGTH)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
	case 3:
		fPrevKeyTime = 25.0f;		fNextKeyTime = 75.0f;
		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_MIDLIFTTIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_MIDLIFTVALUE));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_MIDLIFTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_MIDLIFTINTANLENGTH));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_MIDLIFTOUTTANLENGTH)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
	case 4:
		fPrevKeyTime = pblock->GetFloat(PB_MIDLIFTTIME, t);
		fNextKeyTime = (pblock->GetFloat(PB_MIDPLANTTIME, t) + STEPRATIO100);
		fTimeVal = 75.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_PLANTVALUE));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_PLANTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_PLANTINTANLENGTH));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_PLANTOUTTANLENGTH)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
		// Incase sometihng goes wrong
	default:
		fTimeVal = 50.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
	}
	*ctrlSlider = NULL;
}

/*
float KneeAngle::GetGraphYval(TimeValue t, int LoopT)
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData)
	{
		// Find out the step ratio, and offset LoopT by it
		// this will make the graphs scrub along when moving the time slider

	}
//	LoopT = t % STEPRATIO100;
	return GetYval(t, LoopT);
}

COLORREF KneeAngle::GetGraphColour()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return asRGB(ctrlLimbData->GetLimbColour());
	return RGB(200, 0, 0);
}

TSTR KneeAngle::GetGraphName()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return ctrlLimbData->GetLimbName();
	return _T("KneeAngle");
}
*/

float KneeAngle::GetYval(TimeValue t, int LoopT)
{
	// All Values +ve
	if (LoopT < 0)
		LoopT += STEPTIME100;

	if (LoopT <= STEPTIME25)
	{
		float midplanttime = pblock->GetFloat(PB_MIDPLANTTIME, t) * BASE;
		float midplantval = pblock->GetFloat(PB_MIDPLANTVALUE, t);
		float midplanttangent = pblock->GetFloat(PB_MIDPLANTTANGENT, t);

		if (midplanttime < -STEPTIME25) midplanttime = -STEPTIME25;
		if (midplanttime > STEPTIME25) midplanttime = STEPTIME25;

		if (LoopT < midplanttime)
		{
			/*	float plantval = pblock->GetFloat(PB_PLANTVALUE, t);*/
			float planttangent = pblock->GetFloat(PB_PLANTTANGENT, t);
			float plantoutlength = pblock->GetFloat(PB_PLANTOUTTANLEN, t);
			float midplantintanlength = pblock->GetFloat(PB_MIDPLANTINTANLEN, t);

			CATKey key1(-STEPTIME25, plantval, planttangent, plantoutlength, 0);
			CATKey key2(midplanttime, midplantval, -midplanttangent, 0, midplantintanlength);

			return InterpValue(key1, key2, LoopT);
		}
		else
		{
			float liftval = pblock->GetFloat(PB_LIFTVALUE, t);
			float lifttangent = pblock->GetFloat(PB_LIFTTANGENT, t);
			float liftinlength = pblock->GetFloat(PB_LIFTINTANLEN, t);
			float midplantoutlength = pblock->GetFloat(PB_MIDPLANTOUTTANLEN, t);

			CATKey key1(midplanttime, midplantval, midplanttangent, midplantoutlength, 0);
			CATKey key2(STEPTIME25, liftval, -lifttangent, 0, liftinlength);

			return InterpValue(key1, key2, LoopT);
		}
	}

	else if (LoopT > STEPTIME25 && LoopT <= STEPTIME75)
	{

		float midlifttime = pblock->GetFloat(PB_MIDLIFTTIME, t) * BASE;
		float midliftval = pblock->GetFloat(PB_MIDLIFTVALUE, t);
		float midlifttangent = pblock->GetFloat(PB_MIDLIFTTANGENT, t);

		if (midlifttime < STEPTIME25) midlifttime = STEPTIME25;
		if (midlifttime > STEPTIME75) midlifttime = STEPTIME75;

		if (LoopT < midlifttime)
		{
			float liftval = pblock->GetFloat(PB_LIFTVALUE, t);
			float lifttangent = pblock->GetFloat(PB_LIFTTANGENT, t);
			float liftoutlength = pblock->GetFloat(PB_LIFTOUTTANLEN, t);
			float midliftinlength = pblock->GetFloat(PB_MIDLIFTINTANLEN, t);

			CATKey key1(STEPTIME25, liftval, lifttangent, liftoutlength, 0);
			CATKey key2(midlifttime, midliftval, -midlifttangent, 0, midliftinlength);

			return InterpValue(key1, key2, LoopT);
		}
		else
		{
			float plantval = pblock->GetFloat(PB_PLANTVALUE, t);
			float planttangent = pblock->GetFloat(PB_PLANTTANGENT, t);
			float plantinlength = pblock->GetFloat(PB_PLANTINTANLEN, t);
			float midliftouttanlength = pblock->GetFloat(PB_MIDLIFTOUTTANLEN, t);

			CATKey key1(midlifttime, midliftval, midlifttangent, midliftouttanlength, 0);
			CATKey key2(STEPTIME75, plantval, -planttangent, 0, plantinlength);

			return InterpValue(key1, key2, LoopT);
		}
		// This is a bit hackey
		// we need an angle to return to when the mask is on
		// this must be done in the kneeangle controller but it makes the ui busy
//		return KneeRestAngle + ((bezierval - KneeRestAngle)*BodyMoveMask);

	}
	else if (LoopT > STEPTIME75)
	{
		float midplanttime = pblock->GetFloat(PB_MIDPLANTTIME, t) * BASE;
		float midplantval = pblock->GetFloat(PB_MIDPLANTVALUE, t);
		float midplanttangent = pblock->GetFloat(PB_MIDPLANTTANGENT, t);

		if (midplanttime < -STEPTIME25) midplanttime = -STEPTIME25;
		if (midplanttime > STEPTIME25) midplanttime = STEPTIME25;
		midplanttime += STEPTIME100;

		if (LoopT < midplanttime)
		{
			float plantval = pblock->GetFloat(PB_PLANTVALUE, t);
			float planttangent = pblock->GetFloat(PB_PLANTTANGENT, t);
			float plantoutlength = pblock->GetFloat(PB_PLANTOUTTANLEN, t);
			float midplantintanlength = pblock->GetFloat(PB_MIDPLANTINTANLEN, t);

			CATKey key1(STEPTIME75, plantval, planttangent, plantoutlength, 0);
			CATKey key2(midplanttime, midplantval, -midplanttangent, 0, midplantintanlength);

			return InterpValue(key1, key2, LoopT);
		}
		else
		{
			float liftval = pblock->GetFloat(PB_LIFTVALUE, t);
			float lifttangent = pblock->GetFloat(PB_LIFTTANGENT, t);
			float liftinlength = pblock->GetFloat(PB_LIFTINTANLEN, t);
			float midplantoutlength = pblock->GetFloat(PB_MIDPLANTOUTTANLEN, t);

			CATKey key1(midplanttime, midplantval, midplanttangent, midplantoutlength, 0);
			CATKey key2((STEPTIME100 + STEPTIME25), liftval, lifttangent, 0, liftinlength);

			return InterpValue(key1, key2, LoopT);
		}

		// This is a bit hackey
		// we need an angle to return to when the mask is on
		// this must be done in the kneeangle controller but it makes the ui busy
//		return KneeRestAngle + ((bezierval - KneeRestAngle)*BodyMoveMask);

	}
	//	}
		// This should really never happen
	return 0.0f;
}

void KneeAngle::GetValue(TimeValue t, void *val, Interval&valid, GetSetMethod method)
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
		{
			LoopT = (int)*(float*)val % STEPTIME100;
		}
		else
		{
			LoopT = t % STEPTIME100;
		}
	}

	// we will need this regardless of what keys we are interpolating
	plantval = DegToRad(pblock->GetFloat(PB_PLANTVALUE, t));

	// Pivot pos defaults to Plantval when the mask is on
	if (dFootStepMask > 0.0f)
	{
		//	float dYval = DegToRad(GetYval(t, LoopT));
		float dYval = GetYval(t, LoopT) * flipval;
		*(float*)val = plantval + ((dYval - plantval) * dFootStepMask);
	}
	else *(float*)val = plantval;
}
/*

RefResult KneeAngle::NotifyRefChanged( const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		CATHierarchyBranch* ctrlBranch = (CATHierarchyBranch*)pblock->GetReferenceTarget(PB_CATBRANCH);
		if(ctrlBranch)
			ctrlBranch->RootDrawGraph();
		break;
	}

	return REF_SUCCEED;
}
*/