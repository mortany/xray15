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

// Controls the foots angle

#include "CatPlugins.h"
#include "BezierInterp.h"
#include "PivotRot.h"
#include "CATMotionLimb.h"
#include "CATHierarchy.h"

class PivotRotClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new PivotRot(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_PIVOTROT); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return PIVOTROT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("PivotRot"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static PivotRotClassDesc PivotRotDesc;
ClassDesc2* GetPivotRotDesc() { return &PivotRotDesc; }

static ParamBlockDesc2 PivotRot_param_blk(PivotRot::PIVOTROT_PB, _T("params"), 0, &PivotRotDesc,
	P_AUTO_CONSTRUCT, PivotRot::PBLOCK_REF,
	// params
	PivotRot::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	PivotRot::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,
	PivotRot::PB_STARTIME, _T("StartTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_STARTTIME,
		p_default, 7.0f,
		p_end,
	PivotRot::PB_STARTTANGENT, _T("StartTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_STARTTANGENT,
		p_default, 0.0f,
		p_end,
	PivotRot::PB_STARTTANLEN, _T("StartTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_STARTTANLENGTH,
		p_default, 0.2f,
		p_end,
	PivotRot::PB_KEY1VAL, _T("KEY1Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1VAL,
		p_default, 0.0f,
		p_end,
	PivotRot::PB_KEY1TIME, _T("KEY1Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TIME,
		p_default, 30.0f,
		p_end,
	PivotRot::PB_KEY1TANGENT, _T("KEY1Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1TANGENT,
		p_default, 0.0f,
		p_end,
	PivotRot::PB_KEY1INTANLEN, _T("KEY1InTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1INTANLEN,
		p_default, 0.333f,
		p_end,
	PivotRot::PB_KEY1OUTTANLEN, _T("KEY1OutTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY1OUTTANLEN,
		p_default, 0.333f,
		p_end,

	PivotRot::PB_KEY2VAL, _T("KEY2Val"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2VAL,
		p_default, 0.0f,
		p_end,
	PivotRot::PB_KEY2TIME, _T("KEY2Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TIME,
		p_default, 70.0f,
		p_end,
	PivotRot::PB_KEY2TANGENT, _T("KEY2Tangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2TANGENT,
		p_default, 0.0f,
		p_end,
	PivotRot::PB_KEY2INTANLEN, _T("KEY2InTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2INTANLEN,
		p_default, 0.333f,
		p_end,
	PivotRot::PB_KEY2OUTTANLEN, _T("KEY2OutTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_KEY2OUTTANLEN,
		p_default, 0.333f,
		p_end,
	PivotRot::PB_ENDTIME, _T("endTime"), TYPE_FLOAT, P_ANIMATABLE, IDS_ENDTIME,
		p_default, 80.0f,
		p_end,
	PivotRot::PB_ENDTANGENT, _T("StartTangent"), TYPE_FLOAT, P_ANIMATABLE, IDS_ENDTANGENT,
		p_default, 0.0f,
		p_end,
	PivotRot::PB_ENDTANLEN, _T("endTanLen"), TYPE_FLOAT, P_ANIMATABLE, IDS_ENDTANLENGTH,
		p_default, 0.333f,
		p_end,

	p_end
);

PivotRot::PivotRot()
{
	PivotRotDesc.MakeAutoParamBlocks(this);
}

PivotRot::~PivotRot() {
	DeleteAllRefs();
}

RefTargetHandle PivotRot::Clone(RemapDir& remap)
{
	// make a new PivotRot object to be the clone
	PivotRot *newPivotRot = new PivotRot();
	CloneCATGraph(newPivotRot, remap);
	BaseClone(this, newPivotRot, remap);
	return newPivotRot;
}

void PivotRot::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		PivotRot *newctrl = (PivotRot*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

//----------------------------------------------------------------
/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void PivotRot::GetCATKey(const int			i,
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
			pblock->GetFloat(PB_STARTIME, t) * BASE,
			0,
			pblock->GetFloat(PB_STARTTANGENT, t),
			pblock->GetFloat(PB_STARTTANLEN, t),
			0);
		isInTan = FALSE;
		isOutTan = TRUE;
		break;
	}
	case 1:
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
	case 2:
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
	case 3:
	{
		key = CATKey(
			pblock->GetFloat(PB_ENDTIME, t) * BASE,
			0,
			pblock->GetFloat(PB_ENDTANGENT, t),
			0,
			pblock->GetFloat(PB_ENDTANLEN, t));
		isInTan = TRUE;
		isOutTan = FALSE;
		break;
	}
	}
}

/* ************************************************************************** **
** Description: Hooks up the controller pointers to the						  **
**        correct branches in the hierachy									  **
** ************************************************************************** */
void PivotRot::GetGraphKey(
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
		ctrlBranch->GetBranch(GetString(IDS_ENDTIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fPrevKeyTime -= STEPRATIO100;
		fTimeVal = 15.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_STARTTIME));
		fValueVal = 0.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_STARTTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = NULL;
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_STARTTANLENGTH));
		break;
	case 2:
		fPrevKeyTime = pblock->GetFloat(PB_STARTIME, t);
		fNextKeyTime = pblock->GetFloat(PB_KEY2TIME, t);
		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY1TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY1VAL));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY1TANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1INTANLEN));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1OUTTANLEN)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
	case 3:
		fPrevKeyTime = pblock->GetFloat(PB_KEY1TIME, t);
		fNextKeyTime = pblock->GetFloat(PB_ENDTIME, t);
		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY2TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY2VAL));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY2TANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2INTANLEN));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2OUTTANLEN)); //pblock->GetController(PB_PEAKOUTTANLEN);
		break;
	case 4:
		fPrevKeyTime = pblock->GetFloat(PB_KEY2TIME, t);
		fNextKeyTime = pblock->GetFloat(PB_STARTIME, t) + STEPRATIO100;
		fTimeVal = 85.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_ENDTIME));
		fValueVal = 0.0f;		*ctrlValue = NULL;
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_ENDTANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_ENDTANLENGTH));
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
/*
float PivotRot::GetGraphYval(TimeValue t, int LoopT)
{
	return GetYval(t, LoopT);
}

COLORREF PivotRot::GetGraphColour()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return asRGB(ctrlLimbData->GetLimbColour());
	return RGB(200, 0, 0);
}

TSTR PivotRot::GetGraphName()
{
	CATMotionLimb* ctrlLimbData = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(ctrlLimbData) return ctrlLimbData->GetLimbName();
	return _T("PivotRot");
}
*/
float PivotRot::GetYval(TimeValue t, int LoopT)
{
	// All Values +ve
	if (LoopT < 0)
		LoopT += STEPTIME100;

	float starttime = pblock->GetFloat(PB_STARTIME, t) * BASE;
	float KEY1time = pblock->GetFloat(PB_KEY1TIME, t) * BASE;
	float KEY2time = pblock->GetFloat(PB_KEY2TIME, t) * BASE;
	float endtime = pblock->GetFloat(PB_ENDTIME, t) * BASE;

	starttime = max(min(starttime, KEY1time), 0);
	KEY1time = min(max(starttime, KEY1time), KEY2time);
	endtime = min(endtime, STEPTIME100);
	KEY2time = max(min(KEY2time, endtime), KEY1time);

	float KEY1amount = pblock->GetFloat(PB_KEY1VAL, t);
	float KEY2amount = pblock->GetFloat(PB_KEY2VAL, t);

	if (LoopT > starttime && LoopT < KEY1time)
	{
		float startTangent = pblock->GetFloat(PB_STARTTANGENT, t);
		float starttanlength = pblock->GetFloat(PB_STARTTANLEN, t);
		float KEY1inlength = pblock->GetFloat(PB_KEY1INTANLEN, t);
		float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);

		CATKey key1(starttime, 0, startTangent, starttanlength, 0);
		CATKey key2(KEY1time, KEY1amount, KEY1tangent, 0, KEY1inlength);

		return InterpValue(key1, key2, LoopT);
	}
	else if (LoopT >= KEY1time && LoopT <= KEY2time)
	{
		float KEY1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);
		float KEY1tangent = -pblock->GetFloat(PB_KEY1TANGENT, t);
		float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
		float KEY2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);

		CATKey key1(KEY1time, KEY1amount, -KEY1tangent, KEY1outlength, 0);
		CATKey key2(KEY2time, KEY2amount, KEY2tangent, 0, KEY2inlength);

		return InterpValue(key1, key2, LoopT);

	}
	else if (LoopT > KEY2time && LoopT < endtime)
	{
		float KEY2outlength = pblock->GetFloat(PB_KEY2OUTTANLEN, t);
		float KEY2tangent = -pblock->GetFloat(PB_KEY2TANGENT, t);
		float endTangent = -pblock->GetFloat(PB_ENDTANGENT, t);
		float endTanlength = pblock->GetFloat(PB_ENDTANLEN, t);

		CATKey key1(KEY2time, KEY2amount, -KEY2tangent, KEY2outlength, 0);
		CATKey key2(endtime, 0, endTangent, 0, endTanlength);

		return InterpValue(key1, key2, LoopT);
	}

	else if (LoopT > (STEPTIME100 + starttime))	// StartTime is Negative
	{
		float starttanlength = pblock->GetFloat(PB_STARTTANLEN, t);
		float KEY1intanlength = pblock->GetFloat(PB_KEY1INTANLEN, t);
		float KEY1tangent = pblock->GetFloat(PB_KEY1TANGENT, t);

		CATKey key1(STEPTIME100 + starttime, 0, 0, starttanlength, 0);
		CATKey key2(STEPTIME100 + KEY1time, PB_KEY1VAL, KEY1tangent, 0, KEY1intanlength);

		return InterpValue(key1, key2, LoopT);
	}
	else
		return 0.0f;
}
//-----------------------------------------------------------------------------------------

void PivotRot::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
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

	if (dFootStepMask > 0.0f)
		*(float*)val = GetYval(t, LoopT) * dFootStepMask * flipval;
	else *(float*)val = 0.0f;
}

/*
RefResult PivotRot::NotifyRefChanged( const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		CATHierarchyBranch* ctrlBranch = (CATHierarchyBranch*)pblock->GetReferenceTarget(PB_CATBRANCH);
		if(ctrlBranch) ctrlBranch->RootDrawGraph();
		break;
	}

	return REF_SUCCEED;
}

  */