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
#include "StepShape.h"
#include "CATMotionLimb.h"
#include "CATHierarchy.h"

class StepShapeClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new StepShape(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_STEPSHAPE); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return STEPSHAPE_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("StepShape"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static StepShapeClassDesc StepShapeDesc;
ClassDesc2* GetStepShapeDesc() { return &StepShapeDesc; }

static ParamBlockDesc2 stepshape_param_blk(StepShape::PBLOCK_REF, _T("params"), 0, &StepShapeDesc,
	P_AUTO_CONSTRUCT, StepShape::PBLOCK_REF,
	// params
	StepShape::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	StepShape::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,
	StepShape::PB_LIFTTANGENT, _T("LiftTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTTANGENT,
		p_default, 0.0f,
		p_end,
	StepShape::PB_LIFTOUTTANLEN, _T("LiftOutTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTTANLENGTH,
		p_default, 0.15f,
		p_end,
	StepShape::PB_PLANTTANGENT, _T("PlantTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTTANGENT,
		p_default, 0.0f,
		p_end,
	StepShape::PB_PLANTINTANLEN, _T("PlantInTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_PLANTTANLENGTH,
		p_default, 0.15f,
		p_end,

	p_end
);

StepShape::StepShape()
{
	StepShapeDesc.MakeAutoParamBlocks(this);
}

StepShape::~StepShape() {
	DeleteAllRefs();
}

RefTargetHandle StepShape::Clone(RemapDir& remap)
{
	// make a new StepShape object to be the clone
	StepShape *newStepShape = new StepShape();
	CloneCATGraph(newStepShape, remap);
	BaseClone(this, newStepShape, remap);
	return newStepShape;
}

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void StepShape::GetCATKey(const int			i,
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
			0,
			pblock->GetFloat(PB_LIFTTANGENT, t),
			pblock->GetFloat(PB_LIFTOUTTANLEN, t),
			0.33f
		);
		isInTan = FALSE;
		isOutTan = TRUE;
		break;
	}
	case 1:
	{
		key = CATKey(
			STEPTIME75,
			STEPTIME100,
			pblock->GetFloat(PB_PLANTTANGENT, t),
			0.33f,
			pblock->GetFloat(PB_PLANTINTANLEN, t));
		isInTan = TRUE;
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
** Description: Hooks up the controller pointers to the						  **
**        correct branches in the hierachy									  **
** ************************************************************************** */
void StepShape::GetGraphKey(
	int iKeyNum, CATHierarchyBranch* ctrlBranch,
	Control**	ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
	Control**	ctrlValue, float &fValueVal, float &minVal, float &maxVal,
	Control**	ctrlTangent, float &fTangentVal,
	Control**	ctrlInTanLen, float &fInTanLenVal,
	Control**	ctrlOutTanLen, float &fOutTanLenVal,
	Control**	ctrlSlider)
{
	UNREFERENCED_PARAMETER(maxVal); UNREFERENCED_PARAMETER(minVal); UNREFERENCED_PARAMETER(ctrlSlider);
	/*
	 *	I have disabled the tangent values in this controller because they refused to cooperate
	 *  Dragging them in the CATWindow didn't quite work. They never moved the correct distance
	 *  Tangent values probably arn't needed here anyway. there is at least 3 ways of getting the same effect.
	 */

	switch (iKeyNum) {
	case 1:
		fPrevKeyTime = 24.0f;	fNextKeyTime = 75.0f;
		fTimeVal = 25.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = NULL; //ctrlBranch->GetBranch(GetString(IDS_LIFTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_LIFTTANLENGTH));
		break;
	case 2:
		fPrevKeyTime = 25.0f;	fNextKeyTime = 76.0f;
		fTimeVal = 75.0f;		*ctrlTime = NULL;
		fValueVal = 100.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = NULL;// ctrlBranch->GetBranch(GetString(IDS_PLANTTANGENT));
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
		break;
	}
}

float StepShape::GetYval(TimeValue t, int LoopT)
{
	if (LoopT < 0)
	{
		LoopT += STEPTIME100;
	}

	if (LoopT < STEPTIME25)
		return 0;

	if (LoopT > STEPTIME75)
		return STEPTIME100;

	float lifttan, lifttanlength, planttan, planttanlength;
	lifttan = pblock->GetFloat(PB_LIFTTANGENT, t);
	lifttanlength = pblock->GetFloat(PB_LIFTOUTTANLEN, t);
	planttan = pblock->GetFloat(PB_PLANTTANGENT, t);
	planttanlength = pblock->GetFloat(PB_PLANTINTANLEN, t);

	// Bezier interpolation takes two keys, start and end. First key
	// defines the first node, time, position, tangent of line and
	// weights in and out. Second key is target destination.

	CATKey key1(STEPTIME25, 0, lifttan, lifttanlength, 0);
	CATKey key2(STEPTIME75, STEPTIME100, -planttan, 0, planttanlength);

	return InterpValue(key1, key2, LoopT);

}

void StepShape::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	valid.SetInstant(t);

	int LoopT;
	if (GetCATMotionLimb()) {
		// StepGraph is used in the calculatino of the mask
		// so passing false as the 4th param stops a circular loop
		GetCATMotionLimb()->GetStepTime(t, 1.0f, LoopT, FALSE);
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

	*(float*)val = GetYval(t, LoopT);
}
