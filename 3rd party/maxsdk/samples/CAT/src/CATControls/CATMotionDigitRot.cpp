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

#include "DigitData.h"
#include "DigitSegTrans.h"
#include "PalmTrans2.h"

#include "CATMotionLimb.h"
#include "CATMotionRot.h"
#include "CATMotionDigitRot.h"

#include "CATHierarchyLeaf.h"

class CATMotionDigitRotClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATMotionDigitRot(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONROT); }
	SClass_ID		SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONDIGITROT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionDigitRot"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionDigitRotClassDesc CATMotionDigitRotDesc;
ClassDesc2* GetCATMotionDigitRotDesc() { return &CATMotionDigitRotDesc; }

static ParamBlockDesc2 weightshift_param_blk(CATMotionDigitRot::PBLOCK_REF, _T("CATMotionDigitRot params"), 0, &CATMotionDigitRotDesc,
	P_AUTO_CONSTRUCT, CATMotionDigitRot::PBLOCK_REF,
	CATMotionDigitRot::PB_DIGITSEGTRANS, _T("DigitSegTrans"), TYPE_REFTARG, P_NO_REF, IDS_CL_DIGITSEGTRANS,
		p_end,
	CATMotionDigitRot::PB_P3CATMOTION, _T("CATMotion"), TYPE_POINT3, P_ANIMATABLE, IDS_CATMOTION,
		p_end,
	CATMotionDigitRot::PB_TMSETUP, _T(""), TYPE_MATRIX3, 0, 0,
		p_end,
	p_end
);

CATMotionDigitRot::CATMotionDigitRot() {
	pblock = NULL;
	CATMotionDigitRotDesc.MakeAutoParamBlocks(this);
}

CATMotionDigitRot::~CATMotionDigitRot() {
	DeleteAllRefs();
}

void CATMotionDigitRot::Initialise(CATNodeControl *digitsegtrans, Control* catmotionrot)
{
	Matrix3 tmSetup = digitsegtrans->GetSetupMatrix();
	tmSetup.NoTrans();
	pblock->SetValue(PB_TMSETUP, 0, tmSetup, FALSE);

	if (catmotionrot)pblock->SetControllerByID(PB_P3CATMOTION, 0, catmotionrot, FALSE);

	pblock->SetValue(PB_DIGITSEGTRANS, 0, digitsegtrans, FALSE);
}

RefTargetHandle CATMotionDigitRot::Clone(RemapDir& remap)
{
	// make a new CATMotionDigitRot object to be the clone
	CATMotionDigitRot *newCATMotionDigitRot = new CATMotionDigitRot();
	remap.AddEntry(this, newCATMotionDigitRot);

	newCATMotionDigitRot->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATMotionDigitRot, remap);

	// now return the new object.
	return newCATMotionDigitRot;
}

void CATMotionDigitRot::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{

		CATMotionDigitRot *newctrl = (CATMotionDigitRot*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void CATMotionDigitRot::SetValue(TimeValue t, void *val, int, GetSetMethod) {

	AngAxis ax = *(AngAxis*)val;
	Matrix3 tmSetup = pblock->GetMatrix3(PB_TMSETUP);
	RotateMatrix(tmSetup, ax);
	tmSetup.NoTrans();
	pblock->SetValue(PB_TMSETUP, t, tmSetup);
	return;
};

//-------------------------------------------------------------
void CATMotionDigitRot::GetValue(TimeValue t, void * val, Interval&, GetSetMethod)
{
	DigitSegTrans* digitsegtrans = (DigitSegTrans*)pblock->GetReferenceTarget(PB_DIGITSEGTRANS);
	if (!digitsegtrans) return;

	*(Matrix3*)val = pblock->GetMatrix3(PB_TMSETUP) * *(Matrix3*)val;

	Point3 segrot = pblock->GetPoint3(PB_P3CATMOTION, t) * digitsegtrans->GetBendWeight();
	segrot.x /= (float)digitsegtrans->GetDigit()->GetNumBones();

	if (digitsegtrans->GetBoneID() == 0) {
		Point3 rootpos = digitsegtrans->GetDigit()->GetRootPos();
		if (digitsegtrans->GetCATParentTrans()->GetLengthAxis() == Z)
			segrot *= Point3(1.0f, rootpos.x, rootpos.x);
		else segrot *= Point3(rootpos.z, rootpos.z, -1.0f);
	}
	else {
		if (digitsegtrans->GetCATParentTrans()->GetLengthAxis() == Z)
			segrot *= Point3(1.0f, 0.0f, 0.0f);
		else segrot *= Point3(0.0f, 0.0f, -1.0f);
	}

	float CATEuler[] = { DegToRad(segrot.x), DegToRad(segrot.y), DegToRad(segrot.z) };
	Quat qtCATRot;
	EulerToQuat(&CATEuler[0], qtCATRot);
	PreRotateMatrix(*(Matrix3*)val, qtCATRot);

}

TSTR CATMotionDigitRot::SubAnimName(int i)
{
	switch (i)
	{
	case 0:			return GetString(IDS_PARAMS);
	default:		return _T("");
	}
}
Animatable* CATMotionDigitRot::SubAnim(int i)
{
	switch (i)
	{
	case 0:			return pblock;
	default:		return NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATMotionDigitRotPLCB : public PostLoadCallback {
protected:
	CATMotionDigitRot *catmotiondigitrot;

public:
	CATMotionDigitRotPLCB(CATMotionDigitRot *pOwner) { catmotiondigitrot = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(catmotiondigitrot);
		DigitSegTrans* digitseg = (DigitSegTrans*)catmotiondigitrot->pblock->GetReferenceTarget(CATMotionDigitRot::PB_DIGITSEGTRANS);
		DbgAssert(digitseg);
		if (digitseg && digitseg->GetDigit() && digitseg->GetDigit()->GetParentCATControl())
			return digitseg->GetDigit()->GetParentCATControl()->GetFileSaveVersion();
		return 0;
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		if (GetFileSaveVersion() < CAT_VERSION_1700) {

		}

		if (GetFileSaveVersion() < CAT_VERSION_2430) {

		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

IOResult CATMotionDigitRot::Save(ISave *)
{
	return IO_OK;
}

IOResult CATMotionDigitRot::Load(ILoad *iload)
{
	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new CATMotionDigitRotPLCB(this));

	return IO_OK;
}

extern void CleanCATMotionHierarchy(ReferenceTarget* pCtrl, LimbData2* pOwningLimb, bool bHasCleanedCATMotion = false);

void CATMotionDigitRot::DestructCATMotionHierarchy(LimbData2* pLimb)
{
	// Only destruct if we really really should
	DigitSegTrans* pMySeg = static_cast<DigitSegTrans*>(pblock->GetReferenceTarget(CATMotionDigitRot::PB_DIGITSEGTRANS));
	if (pMySeg == NULL)
		return;

	// If we are the 0'th bone, then we need to destruct
	if (pMySeg->GetBoneID() == 0)
	{
		DigitData* pDigit = static_cast<DigitData*>(pMySeg->GetDigit());
		if (pDigit == NULL)
			return;

		PalmTrans2* pPalm = pDigit->GetPalm();
		if (pPalm == NULL)
			return;

		// If we are the last digit on the palm
		// destruct the CATHierarchyBranch
		if (pPalm->GetNumDigits() == 0)
		{
			Control* pCATMotionRot = pblock->GetControllerByID(PB_P3CATMOTION);
			CleanCATMotionHierarchy(pCATMotionRot, pLimb, true);
		}
	}
}

