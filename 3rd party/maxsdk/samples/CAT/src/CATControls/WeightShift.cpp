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
#include "WeightShift.h"
#include "CATMotionLimb.h"
#include "CATHierarchy.h"

class WeightShiftClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new WeightShift(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_WEIGHTSHIFT); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return WEIGHTSHIFT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("WeightShift"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static WeightShiftClassDesc WeightShiftDesc;
ClassDesc2* GetWeightShiftDesc() { return &WeightShiftDesc; }

static ParamBlockDesc2 weightshift_param_blk(WeightShift::PBLOCK_REF, _T("params"), 0, &WeightShiftDesc,
	P_AUTO_CONSTRUCT, WeightShift::PBLOCK_REF,
	WeightShift::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	WeightShift::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,

	WeightShift::PB_KEY1TIME, _T("KEY1Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TIME,
		p_default, 0.0f,
		p_end,
	WeightShift::PB_KEY1TANGENT, _T("KEY1Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TANGENT,
		p_default, 0.0f,
		p_end,
	WeightShift::PB_KEY1INTANLEN, _T("KEY1InTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1INTANLEN,
		p_default, 0.333f,
		p_end,
	WeightShift::PB_KEY1OUTTANLEN, _T("KEY1OutTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1OUTTANLEN,
		p_default, 0.333f,
		p_end,

	WeightShift::PB_KEY2VAL, _T("KEY2Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2VAL,
		p_default, 0.0f,
		p_end,
	WeightShift::PB_KEY2TIME, _T("KEY2Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TIME,
		p_default, 50.0f,
		p_end,
	WeightShift::PB_KEY2TANGENT, _T("KEY2Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TANGENT,
		p_default, 0.0f,
		p_end,
	WeightShift::PB_KEY2INTANLEN, _T("KEY2InTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2INTANLEN,
		p_default, 0.333f,
		p_end,
	WeightShift::PB_KEY2OUTTANLEN, _T("KEY2OutTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2OUTTANLEN,
		p_default, 0.333f,
		p_end,
	p_end
);

WeightShift::WeightShift()
{
	dFlipVal = 1.0f;
	WeightShiftDesc.MakeAutoParamBlocks(this);
}

WeightShift::~WeightShift() {
	DeleteAllRefs();
}

RefTargetHandle WeightShift::Clone(RemapDir& remap)
{
	WeightShift *newWeightShift = new WeightShift();
	CloneCATGraph(newWeightShift, remap);
	BaseClone(this, newWeightShift, remap);
	return newWeightShift;
}

void WeightShift::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		WeightShift *newctrl = (WeightShift*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void WeightShift::GetCATKey(const int			i,
	const TimeValue	t,
	CATKey				&key,
	bool				&isInTan,
	bool				&isOutTan) const
{
	/*	float KEY1time =
		KEY1time = fmod(KEY1time, STEPTIME100);
		if(KEY1time <= 0.0f) KEY1time += STEPTIME100;

		float KEY2time = pblock->GetFloat(PB_KEY2TIME, t) * BASE;
		KEY2time = fmod(KEY2time, STEPTIME100);
		if(KEY2time <= 0.0f) KEY2time += STEPTIME100;
	*/
	switch (i)
	{
	case 0:
	{

		key = CATKey(
			pblock->GetFloat(PB_KEY1TIME, t) * BASE,
			0,//pblock->GetFloat(PB_KEY1VAL, t),
			pblock->GetFloat(PB_KEY1TANGENT, t),
			pblock->GetFloat(PB_KEY1OUTTANLEN, t),
			pblock->GetFloat(PB_KEY1INTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 1:
	{
		key = CATKey(
			pblock->GetFloat(PB_KEY2TIME, t) * BASE,
			pblock->GetFloat(PB_KEY2VAL, t),
			pblock->GetFloat(PB_KEY2TANGENT, t),
			pblock->GetFloat(PB_KEY2OUTTANLEN, t),
			pblock->GetFloat(PB_KEY2INTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	/*	switch(i)
		{
		case 0:
			{
				key = CATKey(
					(pblock->GetFloat(PB_KEY1TIME, t) * BASE),
					0,//pblock->GetFloat(PB_KEY1VAL, t),
					pblock->GetFloat(PB_KEY1TANGENT, t),
					pblock->GetFloat(PB_KEY1OUTTANLEN, t),
					pblock->GetFloat(PB_KEY1INTANLEN, t));
				isInTan = TRUE;
				isOutTan = TRUE;
				break;
			}
		case 1:
			{
				key = CATKey(
					(pblock->GetFloat(PB_KEY2TIME, t) * BASE),
					pblock->GetFloat(PB_KEY2VAL, t),
					pblock->GetFloat(PB_KEY2TANGENT, t),
					pblock->GetFloat(PB_KEY2OUTTANLEN, t),
					pblock->GetFloat(PB_KEY2INTANLEN, t));
				isInTan = TRUE;
				isOutTan = TRUE;
				break;
			}
			*/
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
void WeightShift::GetGraphKey(
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
	Interval iv = FOREVER;
	switch (iKeyNum) {
	case 1:
		ctrlBranch->GetBranch(GetString(IDS_KEY2TIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		fPrevKeyTime -= STEPRATIO100;
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY2TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY1TIME));
		fValueVal = 0.0f;		*ctrlValue = NULL; //*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY1VAL));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY1TANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1INTANLEN));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1OUTTANLEN)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
	case 2:
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fNextKeyTime += STEPRATIO100;

		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY2TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY2VAL));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY2TANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2INTANLEN));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2OUTTANLEN)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
	}
	*ctrlSlider = NULL;
}

float WeightShift::GetYval(TimeValue t, int LoopT)
{
	// All Values +ve
	if (LoopT < 0)
		LoopT += STEPTIME100;
	else if (LoopT > STEPTIME100)
		LoopT -= STEPTIME100;

	float KEY1time = pblock->GetFloat(PB_KEY1TIME, t) * BASE;
	KEY1time = (float)fmod(KEY1time, (float)STEPTIME100);
	if (KEY1time <= 0.0f) KEY1time += STEPTIME100;

	float KEY2time = pblock->GetFloat(PB_KEY2TIME, t) * BASE;
	KEY2time = (float)fmod(KEY2time, (float)STEPTIME100);
	if (KEY2time <= 0.0f) KEY2time += STEPTIME100;

	float KEY1Val = 0.0f; //pblock->GetFloat(PB_KEY1VAL, t);
	float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);
	float KEY1inlength = pblock->GetFloat(PB_KEY1INTANLEN, t);
	float KEY1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);
	float KEY2Val = pblock->GetFloat(PB_KEY2VAL, t);
	float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
	float KEY2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);
	float KEY2outlength = pblock->GetFloat(PB_KEY2OUTTANLEN, t);

	if (KEY1time < KEY2time)
	{
		if (LoopT < KEY1time)
		{
			CATKey key2(KEY2time - STEPTIME100, KEY2Val, -KEY2tangent, KEY2outlength, KEY2inlength);
			CATKey key1(KEY1time, KEY1Val, KEY1tangent, KEY1outlength, KEY1inlength);

			return InterpValue(key2, key1, LoopT);
		}
		if (LoopT < KEY2time) {
			CATKey key1(KEY1time, KEY1Val, -KEY1tangent, KEY1outlength, KEY1inlength);
			CATKey key2(KEY2time, KEY2Val, KEY2tangent, KEY2outlength, KEY2inlength);

			return InterpValue(key1, key2, LoopT);
		}
		else {
			CATKey key2(KEY2time, KEY2Val, -KEY2tangent, KEY2outlength, KEY2inlength);
			CATKey key1(KEY1time + STEPTIME100, KEY1Val, KEY1tangent, KEY1outlength, KEY1inlength);

			return InterpValue(key2, key1, LoopT);
		}
	}
	else
	{
		if (LoopT < KEY2time)
		{
			CATKey key1(KEY1time - STEPTIME100, KEY1Val, -KEY1tangent, KEY1outlength, KEY1inlength);
			CATKey key2(KEY2time, KEY2Val, KEY2tangent, KEY2outlength, KEY2inlength);

			return InterpValue(key1, key2, LoopT);
		}
		if (LoopT < KEY1time) {
			CATKey key2(KEY2time, KEY2Val, -KEY2tangent, KEY2outlength, KEY2inlength);
			CATKey key1(KEY1time, KEY1Val, KEY1tangent, KEY1outlength, KEY1inlength);

			return InterpValue(key2, key1, LoopT);
		}
		else {
			CATKey key1(KEY1time, KEY1Val, -KEY1tangent, KEY1outlength, KEY1inlength);
			CATKey key2(KEY2time + STEPTIME100, KEY2Val, KEY2tangent, KEY2outlength, KEY2inlength);

			return InterpValue(key1, key2, LoopT);
		}
	}

	/*

	// All Values +ve
	if( LoopT < 0)
		LoopT += STEPTIME100;
	else if( LoopT > STEPTIME100)
		LoopT -= STEPTIME100;

	if(LoopT <= (KEY2time - STEPTIME100))
	{
		CATKey key1( KEY1time - STEPTIME100, KEY1Val, KEY1tangent, KEY1outlength, 0);
		CATKey key2( KEY2time - STEPTIME100, KEY2Val, -KEY2tangent, 0, KEY2inlength);

		return InterpValue(key1, key2, LoopT);
	}
	if((LoopT > (KEY2time - STEPTIME100))&&(LoopT <= KEY1time))
	{
		CATKey key1( KEY2time - STEPTIME100, KEY2Val, KEY2tangent, KEY2outlength, 0);
		CATKey key2( KEY1time, KEY1Val, -KEY1tangent, 0, KEY1inlength);

		return InterpValue(key1, key2, LoopT);
	}
	if((LoopT > KEY1time)&&(LoopT <= KEY2time))
	{
		CATKey key1( KEY1time, KEY1Val, KEY1tangent, KEY1outlength, 0);
		CATKey key2( KEY2time, KEY2Val, -KEY2tangent, 0, KEY2inlength);

		return InterpValue(key1, key2, LoopT);
	}
	else if((LoopT > KEY2time)&&(LoopT <= (KEY1time + STEPTIME100)))
	{
		CATKey key1( KEY2time, KEY2Val, KEY2tangent, KEY2outlength, 0);
		CATKey key2( KEY1time + STEPTIME100, KEY1Val, -KEY1tangent, 0, KEY1inlength);

		return InterpValue(key1, key2, LoopT);
	}
	else //if((LoopT > (KEY1time + STEPTIME100)))
	{
		CATKey key1( (KEY1time + STEPTIME100), KEY1Val, KEY1tangent, KEY1outlength, 0);
		CATKey key2( (KEY2time + STEPTIME100), KEY2Val, -KEY2tangent, 0, KEY2inlength);

		return InterpValue(key1, key2, LoopT);
	}
	*/
}

//-------------------------------------------------------------
void WeightShift::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	valid.SetInstant(t);

	int LoopT;
	float dFootStepMask = 0.0f;
	if (GetCATMotionLimb())
	{
		dFootStepMask = GetCATMotionLimb()->GetStepTime(t, 0.0f, LoopT);
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

	if ((dFootStepMask > 0.0f) && (dFlipVal != 0.0f))
		*(float*)val = GetYval(t, LoopT) * dFootStepMask * dFlipVal;
	else *(float*)val = 0.0f;
}
