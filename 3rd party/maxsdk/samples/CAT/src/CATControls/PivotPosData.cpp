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

#include "PivotPosData.h"

class PivotPosClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new PivotPos(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_PIVOTPOS); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return PIVOTPOS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("PivotPos"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static PivotPosClassDesc PivotPosDesc;
ClassDesc2* GetPivotPosDesc() { return &PivotPosDesc; }

class PivotPosAccessor : public PBAccessor
{
	void Set(PB2Value& v, ReferenceMaker*, ParamID, int, TimeValue)
	{
		v.f = min(max(v.f, 0.0f), 1.0f);
	}
	void Get(PB2Value& v, ReferenceMaker*, ParamID, int, TimeValue)
	{
		v.f = min(max(v.f, 0.0f), 1.0f);
	}
};
static PivotPosAccessor PivotPosKeyAccessor;

static ParamBlockDesc2 pivotpos_param_blk(PivotPos::PBLOCK_REF, _T("params"), 0, &PivotPosDesc,
	P_AUTO_CONSTRUCT, PivotPos::PBLOCK_REF,

	PivotPos::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	PivotPos::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,

	PivotPos::PB_KEY1TIME, _T("KEY1Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TIME,
		p_default, 15.0f,
		p_end,
	PivotPos::PB_KEY1VAL, _T("KEY1Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1VAL,
		p_default, 1.0f,
		p_accessor, &ZeroToOneParamAccessor,
		p_end,
	PivotPos::PB_KEY2TIME, _T("KEY2Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TIME,
		p_default, 85.0f,
		p_end,
	PivotPos::PB_KEY2VAL, _T("KEY2Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2VAL,
		p_default, 0.0f,
		p_accessor, &ZeroToOneParamAccessor,
		p_end,
	p_end
);

PivotPos::PivotPos()
{
	PivotPosDesc.MakeAutoParamBlocks(this);
}

PivotPos::~PivotPos()
{
	DeleteAllRefs();
}

RefTargetHandle PivotPos::Clone(RemapDir& remap)
{
	// make a new PivotPos object to be the clone
	PivotPos *newPivotPos = new PivotPos();
	CloneCATGraph(newPivotPos, remap);
	BaseClone(this, newPivotPos, remap);
	return newPivotPos;
}

//----------------------------------------------------------------
/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void PivotPos::GetCATKey(const int			i,
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
			pblock->GetFloat(PB_KEY1TIME, t) * BASE,
			pblock->GetFloat(PB_KEY1VAL, t),
			0.0f,
			0.2f,
			0.2f);
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 1:
	{
		key = CATKey(
			STEPTIME25,
			pblock->GetFloat(PB_KEY1VAL, t),
			0.0f,
			0.2f,
			0.2f);
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 2:
	{
		key = CATKey(
			STEPTIME75,
			pblock->GetFloat(PB_KEY2VAL, t),
			0.0f,
			0.2f,
			0.2f);
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 3:
	{
		key = CATKey(
			pblock->GetFloat(PB_KEY2TIME, t) * BASE,
			pblock->GetFloat(PB_KEY2VAL, t),
			0.0f,
			0.2f,
			0.2f);
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	}
}

/* ************************************************************************** **
** Description: Hooks up the controller pointers to the						  **
**        correct branches in the hierachy									  **
** ************************************************************************** */
void PivotPos::GetGraphKey(
	int iKeyNum, CATHierarchyBranch* ctrlBranch,
	Control**	ctrlTime, float &fTimeVal, float &fPrevKeyTime, float &fNextKeyTime,
	Control**	ctrlValue, float &fValueVal, float &minVal, float &maxVal,
	Control**	ctrlTangent, float &fTangentVal,
	Control**	ctrlInTanLen, float &fInTanLenVal,
	Control**	ctrlOutTanLen, float &fOutTanLenVal,
	Control**	ctrlSlider)
{
	TimeValue t = GetCOREInterface()->GetTime();
	Interval iv = FOREVER;
	switch (iKeyNum) {
	case 1:
		ctrlBranch->GetBranch(GetString(IDS_KEY2TIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		fPrevKeyTime -= STEPRATIO100;
		fNextKeyTime = STEPRATIO25;

		fTimeVal = 15.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY1TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY1VAL));
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
		break;
	case 2:
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		fNextKeyTime = STEPRATIO75;
		fTimeVal = 25.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY1VAL));
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
		break;
	case 3:
		fPrevKeyTime = STEPRATIO25;
		ctrlBranch->GetBranch(GetString(IDS_KEY2TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fTimeVal = 75.0f;		*ctrlTime = NULL;
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY2VAL));
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = NULL;
		break;
	case 4:
		fPrevKeyTime = STEPRATIO75;
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fNextKeyTime += STEPRATIO100;
		fTimeVal = 85.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY2TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY2VAL));
		fTangentVal = 0.0f;		*ctrlTangent = NULL;
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
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

	minVal = 0.0;
	maxVal = 1.0f;

	*ctrlSlider = NULL;
}

float PivotPos::GetYval(TimeValue t, int LoopT)
{
	Liftval = pblock->GetFloat(PB_KEY1VAL, t);
	Plantval = pblock->GetFloat(PB_KEY2VAL, t);

	Lifttime = pblock->GetFloat(PB_KEY1TIME, t) * BASE;;
	Planttime = pblock->GetFloat(PB_KEY2TIME, t) * BASE;;
	// All Values +ve
	if (LoopT < 0)
		LoopT += STEPTIME100;

	if (LoopT < (Planttime - STEPTIME100))//		 LoopT -= Planttime;
	{
		return Plantval;
	}

	if (LoopT < Lifttime)//		 LoopT -= Planttime;
	{
		CATKey key1(Planttime - STEPTIME100, Plantval, 0, 0.1f, 0);
		CATKey key2(Lifttime, Liftval, 0, 0, 0.1f);

		return InterpValue(key1, key2, LoopT);
	}

	if (LoopT > Planttime)
	{
		CATKey key1(Planttime, Plantval, 0, 0.1f, 0);
		CATKey key2(Lifttime + STEPTIME100, Liftval, 0, 0, 0.1f);

		return InterpValue(key1, key2, LoopT);
	}

	if (LoopT < STEPTIME25 && LoopT >= Lifttime)
		return Liftval;

	if (LoopT > STEPTIME75 && LoopT <= Planttime)
		return Plantval;

	CATKey key1(STEPTIME25, Liftval, 0, 0.1f, 0);
	CATKey key2(STEPTIME75, Plantval, 0, 0, 0.1f);

	return InterpValue(key1, key2, LoopT);
}
//-----------------------------------------------------------------------------------------

void PivotPos::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
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
			LoopT = (int)*(float*)val;
		else
			LoopT = t;
	}

	// this forces the mask to switch on and off faster meaning the
	dFootStepMask = 1.0f - ((1.0f - dFootStepMask) * (1.0f - dFootStepMask));

	// we will be needing this
	Plantval = pblock->GetFloat(PB_KEY2VAL, t);

	//	*(float*)val = GetYval(t, LoopT);// * dFootStepMask;

		// Pivot pos defaults to Plantval when the mask is on
		// we need this for the ankles to sit properly when relaxexed
	if (dFootStepMask > 0.0f)
	{
		float dYval = GetYval(t, LoopT) * flipval;
		*(float*)val = Plantval + ((dYval - Plantval) * dFootStepMask);
	}
	else *(float*)val = Plantval;
}
