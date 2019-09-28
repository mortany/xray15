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
#include "MonoGraph.h"

#include <CATAPI/CATClassID.h>

#include "CATMotionLimb.h"
#include "CATHierarchy.h"

class MonoGraphClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new MonoGraph(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_MONOGRAPH); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return MONOGRAPH_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("MonoGraph"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static MonoGraphClassDesc MonoGraphDesc;
ClassDesc2* GetMonoGraphDesc() { return &MonoGraphDesc; }

/*
 * The Published function. Used to set dimensions based on enumerations in header

void MonoGraph::SetDim(int dimension)
{
	paramDim = dimension;
}
 */
static ParamBlockDesc2 monograph_param_blk(MonoGraph::PBLOCK_REF, _T("params"), 0, &MonoGraphDesc,
	P_AUTO_CONSTRUCT, MonoGraph::PBLOCK_REF,

	MonoGraph::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	MonoGraph::PB_CATBRANCH, _T("CATBranch"), TYPE_REFTARG, P_NO_REF, IDS_CATBRANCH,
		p_end,
	MonoGraph::PB_KEY1VAL, _T("KEY1Val"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY1VAL,
		p_default, 3.0f,
		p_end,
	MonoGraph::PB_KEY1TIME, _T("KEY1Time"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY1TIME,
		p_default, 0.0f,
		p_end,
	MonoGraph::PB_KEY1TANGENT, _T("KEY1Tangent"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY1TANGENT,
		p_default, 0.0f,
		p_end,
	MonoGraph::PB_KEY1INTANLEN, _T("KEY1InTanLen"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY1INTANLEN,
		p_default, 0.333f,
		p_end,
	MonoGraph::PB_KEY1OUTTANLEN, _T("KEY1OutTanLen"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY1OUTTANLEN,
		p_default, 0.333f,
		p_end,

	MonoGraph::PB_KEY2VAL, _T("KEY2Val"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY2VAL,
		p_default, -3.0f,
		p_end,
	MonoGraph::PB_KEY2TIME, _T("KEY2Time"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY2TIME,
		p_default, 50.0f,
		p_end,
	MonoGraph::PB_KEY2TANGENT, _T("KEY2Tangent"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY2TANGENT,
		p_default, 0.0f,
		p_end,
	MonoGraph::PB_KEY2INTANLEN, _T("KEY2InTanLen"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY2INTANLEN,
		p_default, 0.333f,
		p_end,
	MonoGraph::PB_KEY2OUTTANLEN, _T("KEY2OutTanLen"), TYPE_FLOAT, P_ANIMATABLE + P_RESET_DEFAULT, IDS_KEY2OUTTANLEN,
		p_default, 0.333f,
		p_end,
	p_end
);

MonoGraph::MonoGraph()
{
	MonoGraphDesc.MakeAutoParamBlocks(this);
	//	paramDim = defaultDim;

	ctrlKEY1VAL = NULL;
	ctrlKEY1TIME = NULL;
	ctrlKEY1TANGENT = NULL;
	ctrlKEY1INTANLEN = NULL;
	ctrlKEY1OUTTANLEN = NULL;

	ctrlKEY2VAL = NULL;
	ctrlKEY2TIME = NULL;
	ctrlKEY2TANGENT = NULL;
	ctrlKEY2INTANLEN = NULL;
	ctrlKEY2OUTTANLEN = NULL;

	dFlipVal = 1.0f;
}

MonoGraph::~MonoGraph() {
	DeleteAllRefs();
}

RefTargetHandle MonoGraph::Clone(RemapDir& remap)
{
	MonoGraph *newMonoGraph = new MonoGraph();
	CloneCATGraph(newMonoGraph, remap);
	BaseClone(this, newMonoGraph, remap);
	return newMonoGraph;
}

void MonoGraph::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		MonoGraph *newctrl = (MonoGraph*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

/* ************************************************************************** **
** DESCRIPTION: Returns the ith CAKey.										  **
** ************************************************************************** */
void MonoGraph::GetCATKey(const int			i,
	const TimeValue	t,
	CATKey				&key,
	bool				&isInTan,
	bool				&isOutTan) const
{
	//	float phaseOffset = pblock->GetFloat(PB_PHASEOFFSET, t) * STEPTIME100;
	switch (i)
	{
	case 0:
	{
		key = CATKey(
			//((pblock->GetFloat(PB_KEY1TIME, t) + phaseOffset) * BASE),
			(pblock->GetFloat(PB_KEY1TIME, t) * BASE),
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
			//((pblock->GetFloat(PB_KEY2TIME, t) + phaseOffset) * BASE),
			(pblock->GetFloat(PB_KEY2TIME, t) * BASE),
			pblock->GetFloat(PB_KEY2VAL, t),
			pblock->GetFloat(PB_KEY2TANGENT, t),
			pblock->GetFloat(PB_KEY2OUTTANLEN, t),
			pblock->GetFloat(PB_KEY2INTANLEN, t));
		isInTan = TRUE;
		isOutTan = TRUE;
		break;
	}
	/*	case 2:
			{
				key = CATKey(
					(STEPTIME100 + phaseOffset),
					-pblock->GetFloat(PB_AMOUNT, t)/2.0f,
					0.0f,
					0.33f,
					0.33f);
				isInTan = TRUE;
				isOutTan = TRUE;
				break;
			}*/
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
void MonoGraph::GetGraphKey(
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
	Interval iv;
	switch (iKeyNum) {
	case 1:
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY2TIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		fPrevKeyTime -= STEPRATIO100;
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY2TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY1TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY1VAL));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY1TANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1INTANLEN));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY1OUTTANLEN)); //pblock->GetControllerByID(PB_PEAKOUTTANLEN);
		break;
	case 2:
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fPrevKeyTime, iv);
		iv = FOREVER;
		ctrlBranch->GetBranch(GetString(IDS_KEY1TIME))->GetValue(t, (void*)&fNextKeyTime, iv);
		fNextKeyTime += STEPRATIO100;

		fTimeVal = 50.0f;		*ctrlTime = ctrlBranch->GetBranch(GetString(IDS_KEY2TIME));
		fValueVal = 0.0f;		*ctrlValue = ctrlBranch->GetBranch(GetString(IDS_KEY2VAL));
		fTangentVal = 0.0f;		*ctrlTangent = ctrlBranch->GetBranch(GetString(IDS_KEY2TANGENT));
		fInTanLenVal = 0.333f;	*ctrlInTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2INTANLEN));
		fOutTanLenVal = 0.333f;	*ctrlOutTanLen = ctrlBranch->GetBranch(GetString(IDS_KEY2OUTTANLEN)); //pblock->GetControllerByID(PB_PEAKOUTTANLEN);
		break;
	}
	*ctrlSlider = NULL;
}

/*
float MonoGraph::GetGraphYval(TimeValue t, int LoopT)
{
	return GetYval(t, LoopT);
}

COLORREF MonoGraph::GetGraphColour()
{
	CATMotionLimb* limb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(limb) return asRGB(limb->GetLimbColour());
	return RGB(200, 0, 0);
}
TSTR MonoGraph::GetGraphName()
{
	CATMotionLimb* limb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if(limb) return limb->GetLimbName();
	return _T("MonoGraph");
}*/

void MonoGraph::InitControls()
{
	ctrlKEY1VAL = pblock->GetControllerByID(PB_KEY1VAL);
	ctrlKEY1TIME = pblock->GetControllerByID(PB_KEY1TIME);
	ctrlKEY1TANGENT = pblock->GetControllerByID(PB_KEY1TANGENT);
	ctrlKEY1INTANLEN = pblock->GetControllerByID(PB_KEY1INTANLEN);
	ctrlKEY1OUTTANLEN = pblock->GetControllerByID(PB_KEY1OUTTANLEN);

	ctrlKEY2VAL = pblock->GetControllerByID(PB_KEY2VAL);
	ctrlKEY2TIME = pblock->GetControllerByID(PB_KEY2TIME);
	ctrlKEY2TANGENT = pblock->GetControllerByID(PB_KEY2TANGENT);
	ctrlKEY2INTANLEN = pblock->GetControllerByID(PB_KEY2INTANLEN);
	ctrlKEY2OUTTANLEN = pblock->GetControllerByID(PB_KEY2OUTTANLEN);

	dKey1time = 0.0f;
	dKey1Val = 0.0f;
	dKey1tangent = 0.0f;
	dKey1inlength = 0.0f;
	dKey1outlength = 0.0f;

	dKey2time = 0.0f;
	dKey2Val = 0.0f;
	dKey2tangent = 0.0f;
	dKey2inlength = 0.0f;
	dKey2outlength = 0.0f;
}

float MonoGraph::GetYval(TimeValue t, int LoopT)
{
	/*	float dKey1time = pblock->GetFloat(PB_KEY1TIME, t) * BASE;
		float dKey1Val = pblock->GetFloat(PB_KEY1VAL, t);
		float dKey1tangent = pblock->GetFloat(PB_KEY1TANGENT,t );
		float dKey1inlength = pblock->GetFloat(PB_KEY1INTANLEN, t);
		float dKey1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);

		float dKey2time = pblock->GetFloat(PB_KEY2TIME, t) * BASE;
		float dKey2Val = pblock->GetFloat(PB_KEY2VAL, t);
		float dKey2tangent = pblock->GetFloat(PB_KEY2TANGENT,t );
		float dKey2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);
		float dKey2outlength = pblock->GetFloat(PB_KEY2OUTTANLEN, t);

		// All Values +ve
		if( LoopT < 0)
			LoopT += STEPTIME100;
		else if( LoopT > STEPTIME100)
			LoopT -= STEPTIME100;

		if(LoopT <= (dKey2time - STEPTIME100))
		{
			CATKey key1( dKey1time - STEPTIME100, dKey1Val, dKey1tangent, dKey1outlength, 0);
			CATKey key2( dKey2time - STEPTIME100, dKey2Val, -dKey2tangent, 0, dKey2inlength);

			return InterpValue(key1, key2, LoopT);
		}
		if((LoopT > (dKey2time - STEPTIME100))&&(LoopT <= dKey1time))
		{
			CATKey key1( dKey2time - STEPTIME100, dKey2Val, dKey2tangent, dKey2outlength, 0);
			CATKey key2( dKey1time, dKey1Val, -dKey1tangent, 0, dKey1inlength);

			return InterpValue(key1, key2, LoopT);
		}
		if((LoopT > dKey1time)&&(LoopT <= dKey2time))
		{
			CATKey key1( dKey1time, dKey1Val, dKey1tangent, dKey1outlength, 0);
			CATKey key2( dKey2time, dKey2Val, -dKey2tangent, 0, dKey2inlength);

			return InterpValue(key1, key2, LoopT);
		}
		else if((LoopT > dKey2time)&&(LoopT <= (dKey1time + STEPTIME100)))
		{
			CATKey key1( dKey2time, dKey2Val, dKey2tangent, dKey2outlength, 0);
			CATKey key2( dKey1time + STEPTIME100, dKey1Val, -dKey1tangent, 0, dKey1inlength);

			return InterpValue(key1, key2, LoopT);
		}
		else //if((LoopT > (dKey1time + STEPTIME100)))
		{
			CATKey key1( (dKey1time + STEPTIME100), dKey1Val, dKey1tangent, dKey1outlength, 0);
			CATKey key2( (dKey2time + STEPTIME100), dKey2Val, -dKey2tangent, 0, dKey2inlength);

			return InterpValue(key1, key2, LoopT);
		}
		*/

		//	dKey1time = pblock->GetFloat(PB_KEY1TIME, t) * BASE;
		//	dKey1Val = pblock->GetFloat(PB_KEY1VAL, t);
		//	dKey1tangent = pblock->GetFloat(PB_KEY1TANGENT,t );
		//	dKey1inlength = pblock->GetFloat(PB_KEY1INTANLEN, t);
		//	dKey1outlength = pblock->GetFloat(PB_KEY1OUTTANLEN, t);
		//
		//	dKey2time = pblock->GetFloat(PB_KEY2TIME, t) * BASE;
		//	dKey2Val = pblock->GetFloat(PB_KEY2VAL, t);
		//	dKey2tangent = pblock->GetFloat(PB_KEY2TANGENT,t );
		//	dKey2inlength = pblock->GetFloat(PB_KEY2INTANLEN, t);
		//	dKey2outlength = pblock->GetFloat(PB_KEY2OUTTANLEN, t);

	if (!ctrlKEY1VAL) InitControls();
	// if still no controls present, in order to prevent a crash just return 0. This would normally
	// only be hit during testing where a stand-alone instance of this class is created.
	if (!ctrlKEY1VAL)
		return 0.f;

	Interval iv;
	iv = FOREVER;
	ctrlKEY1VAL->GetValue(t, (void*)&dKey1Val, iv);
	iv = FOREVER;
	ctrlKEY1TIME->GetValue(t, (void*)&dKey1time, iv);
	iv = FOREVER;
	dKey1time *= BASE;
	ctrlKEY1TANGENT->GetValue(t, (void*)&dKey1tangent, iv);
	iv = FOREVER;
	ctrlKEY1INTANLEN->GetValue(t, (void*)&dKey1inlength, iv);
	iv = FOREVER;
	ctrlKEY1OUTTANLEN->GetValue(t, (void*)&dKey1outlength, iv);

	iv = FOREVER;
	ctrlKEY2VAL->GetValue(t, (void*)&dKey2Val, iv);
	iv = FOREVER;
	ctrlKEY2TIME->GetValue(t, (void*)&dKey2time, iv);
	dKey2time *= BASE;
	ctrlKEY2TANGENT->GetValue(t, (void*)&dKey2tangent, iv);
	iv = FOREVER;
	ctrlKEY2INTANLEN->GetValue(t, (void*)&dKey2inlength, iv);
	iv = FOREVER;
	ctrlKEY2OUTTANLEN->GetValue(t, (void*)&dKey2outlength, iv);

	// All Values +ve
	if (LoopT < 0)
		LoopT += STEPTIME100;
	else if (LoopT > STEPTIME100)
		LoopT -= STEPTIME100;

	dKey1time = (float)((int)dKey1time % STEPTIME100);
	if (dKey1time < 0)
		dKey1time += STEPTIME100;
	else if (dKey1time > STEPTIME100)
		dKey1time -= STEPTIME100;

	dKey2time = (float)((int)dKey2time % STEPTIME100);
	if (dKey2time < 0)
		dKey2time += STEPTIME100;
	else if (dKey2time > STEPTIME100)
		dKey2time -= STEPTIME100;

	if (dKey1time < dKey2time)
	{
		if (LoopT < dKey1time)
		{
			CATKey key2(dKey2time - STEPTIME100, dKey2Val, dKey2tangent, dKey2outlength, dKey2inlength);
			CATKey key1(dKey1time, dKey1Val, -dKey1tangent, dKey1outlength, dKey1inlength);
			return InterpValue(key2, key1, LoopT);
		}
		if (LoopT < dKey2time)
		{
			CATKey key1(dKey1time, dKey1Val, dKey1tangent, dKey1outlength, dKey1inlength);
			CATKey key2(dKey2time, dKey2Val, -dKey2tangent, dKey2outlength, dKey2inlength);
			return InterpValue(key1, key2, LoopT);
		}
		if (LoopT < dKey1time + STEPTIME100)
		{
			CATKey key2(dKey2time, dKey2Val, dKey2tangent, dKey2outlength, dKey2inlength);
			CATKey key1(dKey1time + STEPTIME100, dKey1Val, -dKey1tangent, dKey1outlength, dKey1inlength);
			return InterpValue(key2, key1, LoopT);
		}
		if (LoopT < dKey2time + STEPTIME100)
		{
			CATKey key1(dKey1time + STEPTIME100, dKey1Val, dKey1tangent, dKey1outlength, dKey1inlength);
			CATKey key2(dKey2time + STEPTIME100, dKey2Val, -dKey2tangent, dKey2outlength, dKey2inlength);
			return InterpValue(key1, key2, LoopT);
		}
		else
		{
			CATKey key2(dKey2time, dKey2Val, dKey2tangent, dKey2outlength, dKey2inlength);
			CATKey key1(dKey1time + STEPTIME100, dKey1Val, -dKey1tangent, dKey1outlength, dKey1inlength);
			return InterpValue(key2, key1, LoopT);
		}
	}
	else
	{
		if (LoopT < dKey2time)
		{
			CATKey key1(dKey1time - STEPTIME100, dKey1Val, dKey1tangent, dKey1outlength, dKey1inlength);
			CATKey key2(dKey2time, dKey2Val, -dKey2tangent, dKey2outlength, dKey2inlength);

			return InterpValue(key1, key2, LoopT);
		}
		if (LoopT < dKey1time)
		{
			CATKey key2(dKey2time, dKey2Val, dKey2tangent, dKey2outlength, dKey2inlength);
			CATKey key1(dKey1time, dKey1Val, -dKey1tangent, dKey1outlength, dKey1inlength);

			return InterpValue(key2, key1, LoopT);
		}
		else
		{
			CATKey key1(dKey1time, dKey1Val, dKey1tangent, dKey1outlength, dKey1inlength);
			CATKey key2(dKey2time + STEPTIME100, dKey2Val, -dKey2tangent, dKey2outlength, dKey2inlength);

			return InterpValue(key1, key2, LoopT);
		}
	}

}

void MonoGraph::GetValue(TimeValue t, void* val, Interval&valid, GetSetMethod method)
{
	DbgAssert(this);
	valid.SetInstant(t);

	int LoopT = 0;
	float dFootStepMask = 0.0f;
	if (!ctrlKEY1VAL) InitControls();
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

	//	if((dFootStepMask > 0.0f)&&(dFlipVal != 0.0f))
	*(float*)val = GetYval(t, LoopT) * dFootStepMask * dFlipVal * flipval;
	//	else *(float*)val = 0.0f;

	if (dim == ANGLE_DIM) *(float*)val = DegToRad(*(float*)val);
}
/*
#define PARAM_CHUNK		0x00001

IOResult MonoGraph::Save(ISave *isave)
{

	ULONG nb;
	int PD = paramDim;
	isave->BeginChunk(PARAM_CHUNK);
	isave->Write(&PD,sizeof(int),&nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult MonoGraph::Load(ILoad *iload)
{
	ULONG nb;
	IOResult res = IO_OK;

	while (IO_OK==(res=iload->OpenChunk())) {
		if(iload->CurChunkID() == PARAM_CHUNK)
		{
			int PD;
			res = iload->Read(&PD,sizeof(int),&nb);
			paramDim = PD;
		}

		iload->CloseChunk();
		if (res!=IO_OK)  return res;
	}
	return IO_OK;
}

RefResult MonoGraph::NotifyRefChanged( const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		CATHierarchyBranch* ctrlBranch = (CATHierarchyBranch*)pblock->GetReferenceTarget(PB_CATBRANCH);
		if(ctrlBranch) ctrlBranch->RootDrawGraph();
		break;
	}

	return REF_SUCCEED;
}*/
