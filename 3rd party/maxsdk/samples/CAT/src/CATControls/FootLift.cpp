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

// Determines height the foot is at throughout a step

#include "CatPlugins.h"
#include "BezierInterp.h"
#include "FootLift.h"
#include "CATMotionLimb.h"
#include "CATHierarchyBranch2.h"

static FootLiftClassDesc FootLiftDesc;
ClassDesc2* GetFootLiftDesc() { return &FootLiftDesc; }

static ParamBlockDesc2 liftgraph_param_blk(FootLift::PBLOCK_REF, _T("params"), 0, &FootLiftDesc,
	P_AUTO_CONSTRUCT, FootLift::PBLOCK_REF,

	FootLift::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	FootLift::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,

	FootLift::PB_LIFTTANGENT, _T("LiftTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTTANGENT, //LiftTangent first tangent
		p_end,
	FootLift::PB_LIFTOUTTANLEN, _T("LiftTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTTANLENGTH, // third tangent
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootLift::PB_PEAKTIME, _T("PeakTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_PEAKTIME,
		p_default, 50.0f,
		p_end,
	FootLift::PB_PEAKTANGENT, _T("PeakTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_PEAKTANGENT,
		p_end,
	FootLift::PB_PEAKINTANLEN, _T("PeakInTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_PEAKINTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootLift::PB_PEAKOUTTANLEN, _T("PeakOutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_PEAKOUTTANLENGTH,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootLift::PB_PLANTTANGENT, _T("PlantTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTTANGENT,
		p_end,
	FootLift::PB_PLANTINTANLEN, _T("PlantTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTTANLENGTH,//PlantInTanLength
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootLift::PB_AMOUNT, _T("Amount"), TYPE_FLOAT, P_ANIMATABLE, IDS_AMOUNT,
		p_default, 0.0f,
		p_end,
	p_end
);

FootLift::FootLift()
{
	FootLiftDesc.MakeAutoParamBlocks(this);

	ctrlLIFTTANGENT = NULL;
	ctrlLIFTOUTTANLEN = NULL;

	ctrlPEAKTIME = NULL;
	ctrlPEAKTANGENT = NULL;
	ctrlPEAKINTANLEN = NULL;
	ctrlPEAKOUTTANLEN = NULL;

	ctrlPLANTTANGENT = NULL;
	ctrlPLANTINTANLEN = NULL;

	ctrlAMOUNT = NULL;
}

FootLift::~FootLift() {
	DeleteAllRefs();
}

RefTargetHandle FootLift::Clone(RemapDir& remap)
{
	// make a new FootLift object to be the clone
	FootLift *newFootLift = new FootLift();
	CloneCATGraph(newFootLift, remap);
	BaseClone(this, newFootLift, remap);
	return newFootLift;
}

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void FootLift::GetCATKey(const int			i,
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
			0.0f,
			pblock->GetFloat(PB_LIFTTANGENT, t),
			pblock->GetFloat(PB_LIFTOUTTANLEN, t),
			0.0f);
		isInTan = FALSE;
		isOutTan = TRUE;
		break;
	}
	case 1:
	{
		key = CATKey(
			pblock->GetFloat(PB_PEAKTIME, t) * BASE,
			pblock->GetFloat(PB_AMOUNT, t),
			pblock->GetFloat(PB_PEAKTANGENT, t),
			pblock->GetFloat(PB_PEAKOUTTANLEN, t),
			pblock->GetFloat(PB_PEAKINTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 2:
	{
		key = CATKey(
			STEPTIME75,
			0.0f,
			pblock->GetFloat(PB_PLANTTANGENT, t),
			0.0f,
			pblock->GetFloat(PB_PLANTINTANLEN, t));
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
void FootLift::GetGraphKey(
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
		fPrevKeyTime = 0;
		fNextKeyTime = pblock->GetFloat(PB_PEAKTIME, t);
		fTimeVal = 25.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_LIFTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_LIFTTANLENGTH));
		break;
	case 2:
		fPrevKeyTime = 25.0f;	fNextKeyTime = 75.0f;
		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_PEAKTIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_AMOUNT));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_PEAKTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_PEAKINTANLENGTH));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_PEAKOUTTANLENGTH));
		break;
	case 3:
		fPrevKeyTime = pblock->GetFloat(PB_PEAKTIME, t);
		fNextKeyTime = 100;
		fTimeVal = 75.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_PLANTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_PLANTTANLENGTH));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
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

/* ************************************************************************** **
** Description: This could be used in the ui to hook up spinners to and in	  **
**        bezier interp to interpolate values								  **
** **************************************************************************
CATGraphKey* FootLift::GetCATGraphKey(int iKeyNum)
{
	switch(iKeyNum) {
	case 1:
		return &CATGraphKey(
			STEPRATIO25,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			NULL,											//ctrlTime
			NULL,											//ctrlValue
			pblock->GetControllerByID(PB_LIFTTANGENT),			//ctrlTangent
			NULL,											//ctrlInTanLength
			pblock->GetControllerByID(PB_LIFTOUTTANLEN));		//ctrlOutTanLength
		break;
	case 2:
		return &CATGraphKey(
			STEPRATIO50,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			pblock->GetControllerByID(PB_PEAKTIME),			//ctrlTime
			pblock->GetControllerByID(PB_AMOUNT),				//ctrlValue
			pblock->GetControllerByID(PB_PEAKTANGENT),			//ctrlTangent
			pblock->GetControllerByID(PB_PEAKINTANLEN),		//ctrlInTanLength
			pblock->GetControllerByID(PB_PEAKOUTTANLEN));	//ctrlOutTanLength
		break;
	case 3:
		return &CATGraphKey(
			STEPRATIO50,									//Time
			0.0f,											//Value
			0.0f,											//Tangent
			0.0f,											//OutTanLength
			0.0f,											//InTanLength

			NULL,											//ctrlTime
			NULL,											//ctrlValue
			pblock->GetControllerByID(PB_PLANTTANGENT),		//ctrlTangent
			pblock->GetControllerByID(PB_PLANTINTANLEN),	//ctrlInTanLength
			NULL);											//ctrlOutTanLength
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

*/

/* ************************************************************************** **
** Description: Finds the max and min key value for this graph and sets the   **
**		new max and min if nececassry										  **
** ************************************************************************** */
void FootLift::GetYRange(TimeValue t, float &minY, float &maxY)
{
	float Amount = pblock->GetFloat(PB_AMOUNT, t);

	minY = min(minY, min(0.0f, Amount));
	maxY = max(maxY, max(0.0f, Amount));

}
/*
float FootLift::GetGraphYval(TimeValue t, int LoopT)
{

}

COLORREF FootLift::GetGraphColour()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return ctrlLimbData->GetLimbColour();
	return RGB(200, 0, 0);
}

TSTR FootLift::GetGraphName()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return ctrlLimbData->GetLimbName();
	return _T("FootLift");
}
*/

void FootLift::InitControls()
{

	ctrlLIFTTANGENT = pblock->GetControllerByID(PB_LIFTTANGENT);
	ctrlLIFTOUTTANLEN = pblock->GetControllerByID(PB_LIFTOUTTANLEN);

	ctrlPEAKTIME = pblock->GetControllerByID(PB_PEAKTIME);
	ctrlPEAKTANGENT = pblock->GetControllerByID(PB_PEAKTANGENT);
	ctrlPEAKINTANLEN = pblock->GetControllerByID(PB_PEAKINTANLEN);
	ctrlPEAKOUTTANLEN = pblock->GetControllerByID(PB_PEAKOUTTANLEN);

	ctrlPLANTTANGENT = pblock->GetControllerByID(PB_PLANTTANGENT);
	ctrlPLANTINTANLEN = pblock->GetControllerByID(PB_PLANTINTANLEN);

	ctrlAMOUNT = pblock->GetControllerByID(PB_AMOUNT);

	dLiftTangent = 0.0f;
	dLiftOutlength = 0.0f;

	dPlantTangent = 0.0f;
	dPlantInlength = 0.0f;

	dPeaktime = 0.0f;
	dPeakVal = 0.0f;
	dPeaktangent = 0.0f;
	dPeakinlength = 0.0f;
	dPeakoutlength = 0.0f;

}

float FootLift::GetYval(TimeValue t, int LoopT)
{
	if (LoopT < 0)
		LoopT += STEPTIME100;

	if (LoopT < STEPTIME25)
		return 0.0f;

	if (LoopT > STEPTIME75)
		return 0.0f;

	//---------------------to graph drw
//	float peaktime = pblock->GetFloat(PB_PEAKTIME, t) * BASE;
//	float peaktangent = pblock->GetFloat(PB_PEAKTANGENT, t);
//	float amount = pblock->GetFloat(PB_AMOUNT, t);

	Interval valid = FOREVER;
	ctrlAMOUNT->GetValue(t, (void*)&dPeakVal, valid);
	ctrlPEAKTIME->GetValue(t, (void*)&dPeaktime, valid);
	ctrlPEAKTANGENT->GetValue(t, (void*)&dPeaktangent, valid);
	dPeaktime *= BASE;

	/////////////
	if (LoopT < dPeaktime)			// Time Foot is highest point in arch
	{
		//		float peakintanlength = pblock->GetFloat(PB_PEAKINTANLEN, t);
		//		float lifttangent = pblock->GetFloat(PB_LIFTTANGENT, t);
		//		float lifttanlength = pblock->GetFloat(PB_LIFTOUTTANLEN, t);

		ctrlPEAKINTANLEN->GetValue(t, (void*)&dPeakinlength, valid);
		ctrlLIFTTANGENT->GetValue(t, (void*)&dLiftTangent, valid);
		ctrlLIFTOUTTANLEN->GetValue(t, (void*)&dLiftOutlength, valid);

		CATKey key1(STEPTIME25, 0, dLiftTangent, dLiftOutlength, 0);
		CATKey key2(dPeaktime, dPeakVal, -dPeaktangent, 0, dPeakinlength);

		return InterpValue(key1, key2, LoopT);
	}
	else
	{
		//	float peakouttanlength = pblock->GetFloat(PB_PEAKOUTTANLEN, t);
		//	float planttangent = pblock->GetFloat(PB_PLANTTANGENT, t);
		//	float planttanlength = pblock->GetFloat(PB_PLANTINTANLEN, t);

		ctrlPEAKOUTTANLEN->GetValue(t, (void*)&dPeakoutlength, valid);
		ctrlPLANTTANGENT->GetValue(t, (void*)&dPlantTangent, valid);
		ctrlPLANTINTANLEN->GetValue(t, (void*)&dPlantInlength, valid);

		CATKey key1(dPeaktime, dPeakVal, dPeaktangent, dPeakoutlength, 0);
		CATKey key2(STEPTIME75, 0, -dPlantTangent, 0, dPlantInlength);

		return InterpValue(key1, key2, LoopT);
	}

}

void FootLift::GetValue(TimeValue t, void* val, Interval &valid, GetSetMethod method)
{
	valid.SetInstant(t);
	int LoopT;
	float dFootStepMask = 0.0f;
	if (!ctrlAMOUNT) InitControls();
	if (GetCATMotionLimb())
	{
		dFootStepMask = GetCATMotionLimb()->GetStepTime(t, 1.0f, LoopT, TRUE, TRUE);
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

	if (dFootStepMask > 0.0f)
		*(float*)val = GetYval(t, LoopT) * dFootStepMask * flipval;
	else *(float*)val = 0.0f;
}
