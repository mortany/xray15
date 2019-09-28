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

/**********************************************************************
	Point3 version of DataMusher
	Is used throughout the rig to retime the dataloops 
	to fit our current step rate
 **********************************************************************/

#include "CatPlugins.h"
#include "LimbData2.h"
#include "CATMotionLimb.h"
#include "CATHierarchyBranch2.h"
#include "MonoGraph.h"
#include "WeightShift.h"
#include "CATp3.h"

#define PBLOCK_REF	0

static CATp3ClassDesc CATp3Desc;
ClassDesc2* GetCATp3Desc() { return &CATp3Desc; }

static ParamBlockDesc2 datamusher_param_blk(CATp3::pb_params, _T("p3 params"), 0, &CATp3Desc,
	P_AUTO_CONSTRUCT, PBLOCK_REF,

	CATp3::PB_P3FLAGS, _T("flags"), TYPE_INT, 0, 0,
		p_end,
	CATp3::PB_LIMB_TAB, _T("Limbs"), TYPE_REFTARG_TAB, 0, P_VARIABLE_SIZE + P_NO_REF, IDS_CL_LIMBDATA2,	// (Requires) intial size of tab
		p_end,
	CATp3::PB_X_TAB, _T("DataX"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_TV_SHOW_ALL + P_VARIABLE_SIZE, IDS_DATAX,
		p_end,
	CATp3::PB_Y_TAB, _T("DataY"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_TV_SHOW_ALL + P_VARIABLE_SIZE, IDS_DATAY,
		p_end,
	CATp3::PB_Z_TAB, _T("DataZ"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_TV_SHOW_ALL + P_VARIABLE_SIZE, IDS_DATAZ,
		p_end,
	CATp3::PB_LIFTOFFSET_TAB, _T("liftOffset"), TYPE_FLOAT_TAB, 0, P_ANIMATABLE + P_TV_SHOW_ALL + P_VARIABLE_SIZE, IDS_HUBOFFSETLIFT,
		p_end,
	p_end
);

CATp3::CATp3() {
	bCtrlsInitialised = FALSE;
	pblock = NULL;
	CATp3Desc.MakeAutoParamBlocks(this);
}

CATp3::~CATp3() {
	DeleteAllRefsFromMe();
}

RefTargetHandle CATp3::Clone(RemapDir& remap)
{
	// make a new datamusher object to be the clone
	CATp3 *newCATp3 = new CATp3();
	remap.AddEntry(this, newCATp3);
	newCATp3->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));
	BaseClone(this, newCATp3, remap);
	return newCATp3;
}

void CATp3::Initialise(int p3flags)
{
	pblock->SetValue(PB_P3FLAGS, 0, p3flags);
}

void CATp3::RegisterLimb(CATMotionLimb* ctrlNewLimb, CATHierarchyBranch2 *branch)
{
	int nNumLimbs = pblock->Count(PB_LIMB_TAB);
	int p3flags = pblock->GetInt(PB_P3FLAGS);

	Control *ctrlX, *ctrlY, *ctrlZ, *liftOffset;
	ctrlX = ctrlY = ctrlZ = liftOffset = NULL;

	int nNumDefaults = 10;

	if (p3flags&P3FLAG_POS)
	{
		//////////////////////////////////////////////////////////////////////////
		// WeightShift
		float *dWeightShiftDefaultVals;
		float dRootHubWeightShiftDefaultVals[] = { 10.0f, 0.0f, 0.0f, 0.33f, 0.33f, /**/ 0.0f, 50.0f, 0.0f, 0.33f, 0.33f };
		float dHubWeightShiftDefaultVals[] = { 0.0f, 25.0f, 0.0f, 0.33f, 0.33f, /**/ 2.0f, 75.0f, 0.0f, 0.33f, 0.33f };

		if (p3flags&P3FLAG_ROOTHUB)
			dWeightShiftDefaultVals = dRootHubWeightShiftDefaultVals;
		else dWeightShiftDefaultVals = dHubWeightShiftDefaultVals;
		ctrlX = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
		branch->AddAttribute(ctrlX, GetString(IDS_WEIGHTSHIFT), ctrlNewLimb, nNumDefaults, &dWeightShiftDefaultVals[0]);

		//////////////////////////////////////////////////////////////////////////
		// Push
		float dPushDefaultVals[] = { 3.0f, 0.0f, 0.0f, 0.25f, 0.25f, /**/ -2.0f, 50.0f, 0.0f, 0.6f, 0.6f };
		ctrlY = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
		branch->AddAttribute(ctrlY, GetString(IDS_PUSH), ctrlNewLimb, nNumDefaults, &dPushDefaultVals[0]);

		//////////////////////////////////////////////////////////////////////////
		// Lift
		float dLiftDefaultVals[] = { 9.0f, 0.0f, 0.0f, 0.25f, 0.25f, /**/ -6.0f, 50.0f, 0.0f, 0.6f, 0.6f };
		ctrlZ = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
		branch->AddAttribute(ctrlZ, GetString(IDS_LIFT), ctrlNewLimb, nNumDefaults, &dLiftDefaultVals[0]);

		if (p3flags&P3FLAG_ROOTHUB&&ctrlNewLimb->GetisLeg())
		{
			// liftOffset
			liftOffset = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, LIFTOFFSET_CLASS_ID);
			branch->AddAttribute(liftOffset, GetString(IDS_LIFTOFFSET), ctrlNewLimb);
		}
	}
	else if (p3flags&P3FLAG_ROT)
	{
		if (p3flags&P3FLAG_TAIL)
		{
			// Lift
			float dPitchDefaultVals[] = { 3.0f, 0.0f, 0.0f, 0.25f, 0.25f, /**/ -2.0f, 50.0f, 0.0f, 0.6f, 0.6f };
			ctrlX = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			branch->AddAttribute(ctrlX, GetString(IDS_LIFT), ctrlNewLimb, nNumDefaults, &dPitchDefaultVals[0]);

			// Roll
			float dRollDefaultVals[] = { 0.0f, 0.0f, 0.0f, 0.33f, 0.33f, /**/ 10.0f, 50.0f, 0.0f, 0.33f, 0.33f };
			ctrlY = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			branch->AddAttribute(ctrlY, GetString(IDS_ROLL), ctrlNewLimb, nNumDefaults, &dRollDefaultVals[0]);

			// Swish
			float dTwistDefaultVals[] = { 0.0f, 25.0f, 0.0f, 0.33f, 0.33f, /**/ 10.0f, 75.0f, 0.0f, 0.33f, 0.33f };
			ctrlZ = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			branch->AddAttribute(ctrlZ, GetString(IDS_SWISH), ctrlNewLimb, nNumDefaults, &dTwistDefaultVals[0]);
		}
		else
		{
			// Pitch
			float dPitchDefaultVals[] = { 3.0f, 0.0f, 0.0f, 0.25f, 0.25f, /**/ -2.0f, 50.0f, 0.0f, 0.6f, 0.6f };
			ctrlX = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			branch->AddAttribute(ctrlX, GetString(IDS_PITCH), ctrlNewLimb, nNumDefaults, &dPitchDefaultVals[0]);

			// Roll
			float dRollDefaultVals[] = { 0.0f, 0.0f, 0.0f, 0.33f, 0.33f, /**/ 10.0f, 50.0f, 0.0f, 0.33f, 0.33f };
			ctrlY = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			branch->AddAttribute(ctrlY, GetString(IDS_ROLL), ctrlNewLimb, nNumDefaults, &dRollDefaultVals[0]);

			// Twist
			float dTwistDefaultVals[] = { 0.0f, 25.0f, 0.0f, 0.33f, 0.33f, /**/ 10.0f, 75.0f, 0.0f, 0.33f, 0.33f };
			ctrlZ = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			branch->AddAttribute(ctrlZ, GetString(IDS_TWIST), ctrlNewLimb, nNumDefaults, &dTwistDefaultVals[0]);
		}
	}

	pblock->EnableNotifications(FALSE);

	pblock->SetCount(PB_LIMB_TAB, nNumLimbs + 1);
	pblock->SetCount(PB_X_TAB, nNumLimbs + 1);
	pblock->SetCount(PB_Y_TAB, nNumLimbs + 1);
	pblock->SetCount(PB_Z_TAB, nNumLimbs + 1);

	pblock->SetValue(PB_LIMB_TAB, 0, ctrlNewLimb, nNumLimbs);
	pblock->SetControllerByID(PB_X_TAB, nNumLimbs, ctrlX, FALSE);
	pblock->SetControllerByID(PB_Y_TAB, nNumLimbs, ctrlY, FALSE);
	pblock->SetControllerByID(PB_Z_TAB, nNumLimbs, ctrlZ, FALSE);

	if (p3flags&P3FLAG_ROOTHUB)
	{
		pblock->SetCount(PB_LIFTOFFSET_TAB, nNumLimbs + 1);
		pblock->SetControllerByID(PB_LIFTOFFSET_TAB, nNumLimbs, liftOffset, FALSE);
	}

	InitControls();
	pblock->EnableNotifications(TRUE);
}

void CATp3::InitControls()
{
	int p3flags = pblock->GetInt(PB_P3FLAGS);
	int nNumLimbs = pblock->Count(PB_LIMB_TAB);
	CATMotionLimb*	limb;
	int			iLMR;

	Control *ctrlX, *ctrlY, *ctrlZ, *liftOffset;
	ctrlX = ctrlY = ctrlZ = liftOffset = NULL;

	tabX.SetCount(nNumLimbs);
	tabY.SetCount(nNumLimbs);
	tabZ.SetCount(nNumLimbs);
	if (p3flags&P3FLAG_ROOTHUB && p3flags&P3FLAG_POS)
		tabLiftOffset.SetCount(nNumLimbs);

	for (int i = 0; i < nNumLimbs; i++)
	{
		limb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMB_TAB, 0, i);
		if (!limb) return;

		iLMR = limb->GetLMR();

		ctrlX = pblock->GetControllerByID(PB_X_TAB, i);
		ctrlY = pblock->GetControllerByID(PB_Y_TAB, i);
		ctrlZ = pblock->GetControllerByID(PB_Z_TAB, i);

		if (!(ctrlX && ctrlY && ctrlZ)) return;

		if (p3flags&P3FLAG_POS)
		{
			if (ctrlX->ClassID() == MONOGRAPH_CLASS_ID)
				((MonoGraph*)ctrlX)->dFlipVal = (float)iLMR;
			else ((WeightShift*)ctrlX)->dFlipVal = (float)iLMR;
		}
		else if (p3flags&P3FLAG_ROT)
		{
			if (ctrlY->ClassID() == MONOGRAPH_CLASS_ID)
				((MonoGraph*)ctrlY)->dFlipVal = (float)iLMR;
			else ((WeightShift*)ctrlY)->dFlipVal = (float)iLMR;

			if (ctrlZ->ClassID() == MONOGRAPH_CLASS_ID)
				((MonoGraph*)ctrlZ)->dFlipVal = (float)iLMR;
			else ((WeightShift*)ctrlZ)->dFlipVal = (float)iLMR;
		}

		tabX[i] = ctrlX;
		tabY[i] = ctrlY;
		tabZ[i] = ctrlZ;

		if (p3flags&P3FLAG_ROOTHUB && p3flags&P3FLAG_POS)
			tabLiftOffset[i] = pblock->GetControllerByID(PB_LIFTOFFSET_TAB, i);
	}
	bCtrlsInitialised = TRUE;
}

//-------------------------------------------------------------
void CATp3::GetValue(TimeValue t, void *val, Interval&, GetSetMethod)
{
	if (!bCtrlsInitialised) {
		InitControls();
		if (!bCtrlsInitialised) return;
	}
	// always absolute
	(*(Point3*)val) = P3_IDENTITY;
	CATMotionLimb*	 limb;

	int nNumLimbs = pblock->Count(PB_LIMB_TAB);
	//	Point3 limbMove;
	//	float limbZRot;
	Point3 p3;

	for (int i = 0; i < nNumLimbs; i++) {
		limb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMB_TAB, t, i);
		if (!limb) return;

		p3.x = pblock->GetFloat(PB_X_TAB, t, i);
		p3.y = pblock->GetFloat(PB_Y_TAB, t, i);
		p3.z = pblock->GetFloat(PB_Z_TAB, t, i);

		// If there is an arm, there is no point doing this.
		// the space is there, but it is empty
	/*	if((p3flags&P3FLAG_ROOTHUB)&&limb->GetisLeg()){
			if(p3flags&P3FLAG_POS){
				if(i<nNumLiftOffsets) p3.z += pblock->GetFloat(PB_LIFTOFFSET_TAB, t, i);
				limb->GetFootPrintPos(t, limbMove);
				(*(Point3*)val) += (limbMove/(float)nNumLimbs);
			}
			else{
				limb->GetFootPrintRot(t, limbZRot);
				(*(Point3*)val).z += RadToDeg(limbZRot/(float)nNumLimbs);
			}
		}
	*/	(*(Point3*)val) += p3;
	}
}

TSTR CATp3::SubAnimName(int i)
{
	int p3flags = pblock->GetInt(PB_P3FLAGS);
	int iNumLimbs = pblock->Count(PB_LIMB_TAB);
	int iParamIndex = (i - (i % iNumLimbs)) / iNumLimbs;
	CATMotionLimb *currLimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMB_TAB, 0, i%iNumLimbs);
	TSTR currLimbName = currLimb->GetLimbName();
	TSTR strSubName;;

	if (p3flags&P3FLAG_POS)
	{
		switch (iParamIndex)
		{
		case 0:
			strSubName = GetString(IDS_WEIGHTSHIFT);
			return strSubName + _T(" ") + currLimbName;
		case 1:
			strSubName = GetString(IDS_PUSH);
			return strSubName + _T(" ") + currLimbName;
		case 2:
			strSubName = GetString(IDS_LIFT);
			return strSubName + _T(" ") + currLimbName;
		case 3:
			strSubName = GetString(IDS_LIFTOFFSET);
			return strSubName + _T(" ") + currLimbName;
		default: return _T("");
		}
	}
	else
	{
		switch (iParamIndex)
		{
		case 0:
			strSubName = GetString(IDS_PITCH);
			return strSubName + _T(" ") + currLimbName;
		case 1:
			strSubName = GetString(IDS_ROLL);
			return strSubName + _T(" ") + currLimbName;
		case 2:
			strSubName = GetString(IDS_TWIST);
			return strSubName + _T(" ") + currLimbName;
		default: return _T("");
		}
	}
}

Animatable *CATp3::SubAnim(int i)
{
	int iNumLimbs = pblock->Count(PB_LIMB_TAB);

	// for hubs with arms and legs, there will be an
	// empty slot here which will return a NULL ptr.
	ParamID iParamIndex = (ParamID)(PB_X_TAB + (i - (i % iNumLimbs)) / iNumLimbs);
	int iLimbID = i % iNumLimbs;

	return pblock->GetControllerByID(iParamIndex, iLimbID);
}

//////////////////////////////////////////////////////////////////////////
// Loading and saving....
//

IOResult CATp3::Save(ISave *)
{
	return IO_OK;
}

IOResult CATp3::Load(ILoad *iload)
{
	return IO_OK;
}
