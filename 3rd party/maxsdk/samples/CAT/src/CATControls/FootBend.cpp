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

// Bend in the feet segments, bezier curve 0->amount->0 as liftTime->PeakTime->plantTime

#include "CatPlugins.h"
#include "BezierInterp.h"

#include "CATMotionLimb.h"
#include "CATHierarchyBranch2.h"

#include "FootBend.h"

class FootBendClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new FootBend(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_FOOTBEND); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return FOOTBEND_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("FootBend"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

// Our global class description instance
static FootBendClassDesc FootBendDesc;
ClassDesc2* GetFootBendDesc() { return &FootBendDesc; }

static ParamBlockDesc2 FootBend_param_blk(FootBend::PBLOCK_REF, _T("params"), 0, &FootBendDesc,
	P_AUTO_CONSTRUCT, FootBend::PBLOCK_REF,
	FootBend::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	FootBend::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,

	FootBend::PB_KEY1VAL, _T("KEY1Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1VAL,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY1TIME, _T("KEY1Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TIME,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY1TANGENT, _T("KEY1Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TANGENT,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY1INTANLEN, _T("KEY1InTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1INTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootBend::PB_KEY1OUTTANLEN, _T("KEY1OutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1OUTTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,

	FootBend::PB_KEY2VAL, _T("KEY2Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2VAL,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY2TIME, _T("KEY2Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TIME,
		p_default, 25.0f,
		p_end,
	FootBend::PB_KEY2TANGENT, _T("KEY2Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TANGENT,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY2INTANLEN, _T("KEY2InTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2INTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootBend::PB_KEY2OUTTANLEN, _T("KEY2OutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2OUTTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,

	FootBend::PB_KEY3VAL, _T("KEY3Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY3VAL,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY3TIME, _T("KEY3Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY3TIME,
		p_default, 50.0f,
		p_end,
	FootBend::PB_KEY3TANGENT, _T("KEY3Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY3TANGENT,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY3INTANLEN, _T("KEY3InTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY3INTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootBend::PB_KEY3OUTTANLEN, _T("KEY3OutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY3OUTTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,

	FootBend::PB_KEY4VAL, _T("KEY4Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY4VAL,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY4TIME, _T("KEY4Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY4TIME,
		p_default, 75.0f,
		p_end,
	FootBend::PB_KEY4TANGENT, _T("KEY4Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY4TANGENT,
		p_default, 0.0f,
		p_end,
	FootBend::PB_KEY4INTANLEN, _T("KEY4InTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY4INTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,
	FootBend::PB_KEY4OUTTANLEN, _T("KEY4OutTanLength"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY4OUTTANLEN,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 0.333f,
		p_end,

	p_end
);

FootBend::FootBend()
{
	FootBendDesc.MakeAutoParamBlocks(this);
}

FootBend::~FootBend() {
	DeleteAllRefs();
}

RefTargetHandle FootBend::Clone(RemapDir& remap)
{
	// make a new FootBend object to be the clone
	FootBend *newFootBend = new FootBend();
	CloneCATGraph(newFootBend, remap);
	BaseClone(this, newFootBend, remap);
	return newFootBend;
}

void FootBend::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		FootBend *newctrl = (FootBend*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

//----------------------------------------------------------------
/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void FootBend::GetCATKey(const int			i,
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
	case 2:
	{
		key = CATKey(
			pblock->GetFloat(PB_KEY3TIME, t) * BASE,
			pblock->GetFloat(PB_KEY3VAL, t),
			pblock->GetFloat(PB_KEY3TANGENT, t),
			pblock->GetFloat(PB_KEY3OUTTANLEN, t),
			pblock->GetFloat(PB_KEY3INTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	case 3:
	{
		key = CATKey(
			pblock->GetFloat(PB_KEY4TIME, t) * BASE,
			pblock->GetFloat(PB_KEY4VAL, t),
			pblock->GetFloat(PB_KEY4TANGENT, t),
			pblock->GetFloat(PB_KEY4OUTTANLEN, t),
			pblock->GetFloat(PB_KEY4INTANLEN, t));
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
void FootBend::GetGraphKey(
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
		fPrevKeyTime = pblock->GetFloat(PB_KEY4TIME, t) - STEPRATIO100;
		fNextKeyTime = pblock->GetFloat(PB_KEY2TIME, t);
		if (GetCATMotionLimb()) {
			fTimeVal = 0.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY1TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY1VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY1TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1OUTTANLEN));
		}
		else {
			fTimeVal = 0.0f;		*ctrlTime = ctrlBranch->GetLeaf(GetString(IDS_KEY1TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetLeaf(GetString(IDS_KEY1VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetLeaf(GetString(IDS_KEY1TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY1INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY1OUTTANLEN));
		}
		break;
	case 2:
		fPrevKeyTime = pblock->GetFloat(PB_KEY1TIME, t);
		fNextKeyTime = pblock->GetFloat(PB_KEY3TIME, t);
		if (GetCATMotionLimb()) {
			fTimeVal = 25.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY2TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY2VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY2TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2OUTTANLEN));
		}
		else {
			fTimeVal = 25.0f;		*ctrlTime = ctrlBranch->GetLeaf(GetString(IDS_KEY2TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetLeaf(GetString(IDS_KEY2VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetLeaf(GetString(IDS_KEY2TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY2INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY2OUTTANLEN));
		}
		break;
	case 3:
		fPrevKeyTime = pblock->GetFloat(PB_KEY2TIME, t);
		fNextKeyTime = pblock->GetFloat(PB_KEY4TIME, t);
		if (GetCATMotionLimb()) {
			fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY3TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY3VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY3TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY3INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY3OUTTANLEN)); //pblock->GetController(PB_PEAKOUTTANLEN);
		}
		else {
			fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetLeaf(GetString(IDS_KEY3TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetLeaf(GetString(IDS_KEY3VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetLeaf(GetString(IDS_KEY3TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY3INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY3OUTTANLEN)); //pblock->GetController(PB_PEAKOUTTANLEN);
		}
		break;
	case 4:
		fPrevKeyTime = pblock->GetFloat(PB_KEY3TIME, t);
		fNextKeyTime = pblock->GetFloat(PB_KEY1TIME, t) + STEPRATIO100;
		if (GetCATMotionLimb()) {
			fTimeVal = 75.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY4TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY4VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY4TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY4INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY4OUTTANLEN));
		}
		else {
			fTimeVal = 75.0f;		*ctrlTime = ctrlBranch->GetLeaf(GetString(IDS_KEY4TIME));
			fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetLeaf(GetString(IDS_KEY4VAL));
			fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetLeaf(GetString(IDS_KEY4TANGENT));
			fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY4INTANLEN));
			fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetLeaf(GetString(IDS_KEY4OUTTANLEN));
		}
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

float FootBend::GetGraphYval(TimeValue t, int LoopT)
{
	/*	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
		if(ctrlLimbData)
		{
			// Find out the step ratio, and offset LoopT by it
			// this will make the graphs scrub along when moving the time slider

		}
	//	LoopT = t % STEPTIME100;
	*/
	return GetYval(t, LoopT);
}

COLORREF FootBend::GetGraphColour()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if (ctrlLimbData) return asRGB(ctrlLimbData->GetLimbColour());
	return RGB(200, 0, 0);
}

TSTR FootBend::GetGraphName()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if (ctrlLimbData) return ctrlLimbData->GetLimbName();
	return _T("FootBend");
}

float FootBend::GetYval(TimeValue t, int LoopT)
{
	// All Values +ve
	if (LoopT < 0)
		LoopT += STEPTIME100;

	float KEY1time = pblock->GetFloat(PB_KEY1TIME, t) * BASE;
	float KEY2time = pblock->GetFloat(PB_KEY2TIME, t) * BASE;
	float KEY3time = pblock->GetFloat(PB_KEY3TIME, t) * BASE;
	float KEY4time = pblock->GetFloat(PB_KEY4TIME, t) * BASE;

	/*	KEY1time = min(max(-STEPTIME25, KEY1time), KEY2time);
		KEY2time = max(min(KEY2time, KEY3time), KEY1time);
		KEY3time = min(max(KEY3time, KEY4time), KEY2time);
		KEY4time = max(min(KEY4time, STEPTIME100 + STEPTIME25), KEY3time);
	*/

	if (LoopT <= KEY1time)
	{
		float KEY4val = pblock->GetFloat(PB_KEY4VAL, t);
		float KEY4outlength = pblock->GetFloat(PB_KEY4OUTTANLEN, t);
		float KEY4tangent = -pblock->GetFloat(PB_KEY4TANGENT, t);
		float KEY1val = pblock->GetFloat(PB_KEY1VAL, t);
		float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);
		float KEY1inlength = pblock->GetFloat(PB_KEY1INTANLEN, t);

		CATKey key4(KEY4time - STEPTIME100, KEY4val, -KEY4tangent, KEY4outlength, 0);
		CATKey key1(KEY1time, KEY1val, KEY1tangent, 0, KEY1inlength);

		return InterpValue(key4, key1, LoopT);
	}
	else if (LoopT >= KEY1time && LoopT < KEY2time)
	{
		float KEY1val = pblock->GetFloat(PB_KEY1VAL, t);
		float KEY1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);
		float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);
		float KEY2val = pblock->GetFloat(PB_KEY2VAL, t);
		float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
		float KEY2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);

		CATKey key1(KEY1time, KEY1val, -KEY1tangent, KEY1outlength, 0);
		CATKey key2(KEY2time, KEY2val, KEY2tangent, 0, KEY2inlength);

		return InterpValue(key1, key2, LoopT);

	}
	else if (LoopT >= KEY2time && LoopT < KEY3time)
	{
		float KEY2val = pblock->GetFloat(PB_KEY2VAL, t);
		float KEY2outlength = pblock->GetFloat(PB_KEY2OUTTANLEN, t);
		float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
		float KEY3val = pblock->GetFloat(PB_KEY3VAL, t);
		float KEY3tangent = -pblock->GetFloat(PB_KEY3TANGENT, t);
		float KEY3inlength = pblock->GetFloat(PB_KEY3INTANLEN, t);

		CATKey key2(KEY2time, KEY2val, -KEY2tangent, KEY2outlength, 0);
		CATKey key3(KEY3time, KEY3val, KEY3tangent, 0, KEY3inlength);

		return InterpValue(key2, key3, LoopT);
	}

	else if (LoopT >= KEY3time && LoopT < KEY4time)
	{
		float KEY3val = pblock->GetFloat(PB_KEY3VAL, t);
		float KEY3tangent = -pblock->GetFloat(PB_KEY3TANGENT, t);
		float KEY3outlength = pblock->GetFloat(PB_KEY3OUTTANLEN, t);
		float KEY4val = pblock->GetFloat(PB_KEY4VAL, t);
		float KEY4inlength = pblock->GetFloat(PB_KEY4INTANLEN, t);
		float KEY4tangent = -pblock->GetFloat(PB_KEY4TANGENT, t);

		CATKey key3(KEY3time, KEY3val, -KEY3tangent, KEY3outlength, 0);
		CATKey key4(KEY4time, KEY4val, KEY4tangent, 0, KEY4inlength);

		return InterpValue(key3, key4, LoopT);
	}
	else if (LoopT >= KEY4time && LoopT <= (STEPTIME100 + KEY1time))	// StartTime is Negative
	{
		float KEY4val = pblock->GetFloat(PB_KEY4VAL, t);
		float KEY4outlength = pblock->GetFloat(PB_KEY4OUTTANLEN, t);
		float KEY4tangent = -pblock->GetFloat(PB_KEY4TANGENT, t);
		float KEY1val = pblock->GetFloat(PB_KEY1VAL, t);
		float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);
		float KEY1inlength = pblock->GetFloat(PB_KEY1INTANLEN, t);

		CATKey key4(KEY4time, KEY4val, -KEY4tangent, KEY4outlength, 0);
		CATKey key1((STEPTIME100 + KEY1time), KEY1val, KEY1tangent, 0, KEY1inlength);

		return InterpValue(key4, key1, LoopT);
	}
	else if (LoopT >= (STEPTIME100 + KEY1time) && LoopT <= (STEPTIME100 + KEY2time))
	{
		float KEY1val = pblock->GetFloat(PB_KEY1VAL, t);
		float KEY1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);
		float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);
		float KEY2val = pblock->GetFloat(PB_KEY2VAL, t);
		float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
		float KEY2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);

		CATKey key1(STEPTIME100 + KEY1time, KEY1val, -KEY1tangent, KEY1outlength, 0);
		CATKey key2(STEPTIME100 + KEY2time, KEY2val, KEY2tangent, 0, KEY2inlength);

		return InterpValue(key1, key2, LoopT);

	}
	else if (LoopT >= (STEPTIME100 + KEY2time) && LoopT <= (STEPTIME100 + KEY3time))
	{
		float KEY2val = pblock->GetFloat(PB_KEY2VAL, t);
		float KEY2outlength = pblock->GetFloat(PB_KEY2OUTTANLEN, t);
		float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
		float KEY3val = pblock->GetFloat(PB_KEY3VAL, t);
		float KEY3tangent = -pblock->GetFloat(PB_KEY3TANGENT, t);
		float KEY3inlength = pblock->GetFloat(PB_KEY3INTANLEN, t);

		CATKey key2(STEPTIME100 + KEY2time, KEY2val, -KEY2tangent, KEY2outlength, 0);
		CATKey key3(STEPTIME100 + KEY3time, KEY3val, KEY3tangent, 0, KEY3inlength);

		return InterpValue(key2, key3, LoopT);
	}
	else
		return 0.0f;
}
//-----------------------------------------------------------------------------------------

void FootBend::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod)
{
	valid.SetInstant(t);
	CATHierarchyBranch2* ctrlBranch = GetBranch();
	if (!ctrlBranch) return;

	int LoopT;
	float dFootStepMask;
	if (GetCATMotionLimb())
	{
		dFootStepMask = GetCATMotionLimb()->GetStepTime(t, 1.0f, LoopT);
		LoopT = LoopT % STEPTIME100;

		if (dFootStepMask > 0.0f)
			*(float*)val = GetYval(t, LoopT) * dFootStepMask * flipval;// * GetCATMotionLimb()->GetLMR();
		else *(float*)val = 0.0f;
		return;
	}
	else
	{
		LoopT = (int)ctrlBranch->GetCATRoot()->GetStepEaseValue(t) % STEPTIME100;
	}

	*(float*)val = GetYval(t, LoopT) * flipval;
}

