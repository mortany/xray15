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

 // Rig Structure
#include "ICATParent.h"
#include "TailData2.h"
#include "CATNodeControl.h"
#include "Hub.h"

//CATMotion
#include "CATHierarchyBranch2.h"

#include "CATp3.h"
#include "WeightShift.h"

#include "CATMotionTail.h"
#include "CATMotionTailRot.h"
#include "CATWeight.h"

class CATMotionTailClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATMotionTail(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONTAIL); }
	SClass_ID		SuperClassID() { return CTRL_POINT3_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONTAIL_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionTail"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionTailClassDesc CATMotionTailDesc;
ClassDesc2* GetCATMotionTailDesc() { return &CATMotionTailDesc; }

static ParamBlockDesc2 weightshift_param_blk(
	CATMotionTail::PBLOCK_REF, _T("CATMotionTail Animation"), 0, &CATMotionTailDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, CATMotionTail::PBLOCK_REF,
	IDD_CATMOTION_TAIL, IDS_CL_CATMOTIONTAIL, 0, 0, NULL,

	CATMotionTail::PB_P3OFFSETROT, _T("OffsetRot"), TYPE_POINT3, P_ANIMATABLE, IDS_OFFSETROT,
		p_end,
	CATMotionTail::PB_P3MOTIONROT, _T("MotionRot"), TYPE_POINT3, P_ANIMATABLE, IDS_CATMOTIONROT,
		p_end,
	CATMotionTail::PB_PHASEOFFSET, _T("PhaseOffset"), TYPE_FLOAT, 0, IDS_PHASEOFFSET,
		p_default, 0.0f,
		p_range, -1.0f, 1.0f,
		p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDIT_PHASEOFFSET, IDC_SLIDER_PHASEOFFSET, 5,
		p_end,
	CATMotionTail::PB_PHASE_BIAS, _T("PhaseBias"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHASEOFFSET,
		p_end,
	CATMotionTail::PB_FREQUENCY, _T("Frequency"), TYPE_FLOAT, 0, IDS_PHASEOFFSET,
		p_default, 0.5f,
		p_range, 0.1f, 5.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_TAILFREQUENCY, IDC_SPIN_TAILFREQUENCY, 0.01f,
		p_end,
	CATMotionTail::PB_CATHIERARCHYBRANCH, _T("CATHierarchyBranch"), TYPE_REFTARG, P_NO_REF, 0,//IDS_CATHIERARCHYROOT,
		p_end,
	p_end
);

CATMotionTail::CATMotionTail(BOOL loading) {
	pblock = NULL;
	CATMotionTailDesc.MakeAutoParamBlocks(this);

	if (!loading) {
		CATWeight *phasebias = (CATWeight*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATWEIGHT_CLASS_ID);
		phasebias->pblock->SetValue(CATWeight::PB_KEY1OUTTANLEN, 0, 0.0f);
		phasebias->pblock->SetValue(CATWeight::PB_KEY2INTANLEN, 0, 0.0f);
		pblock->SetControllerByID(PB_PHASE_BIAS, 0, phasebias, FALSE);
	}
}

CATMotionTail::~CATMotionTail() {
	DeleteAllRefs();
}

RefTargetHandle CATMotionTail::Clone(RemapDir& remap)
{
	// make a new CATMotionTail object to be the clone
	CATMotionTail *newCATMotionTail = new CATMotionTail();
	remap.AddEntry(this, newCATMotionTail);

	newCATMotionTail->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATMotionTail, remap);

	// now return the new object.
	return newCATMotionTail;
}

void CATMotionTail::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		CATMotionTail *newctrl = (CATMotionTail*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void CATMotionTail::Initialise(int index, TailData2* tail, CATHierarchyBranch2* branchHubGroup)
{
	CATHierarchyBranch2 *branchTail = (CATHierarchyBranch2*)branchHubGroup->AddBranch(tail);

	Control *offsetRot = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
	branchTail->AddAttribute(offsetRot, GetString(IDS_OFFSETROT));
	pblock->SetControllerByID(PB_P3OFFSETROT, 0, offsetRot, FALSE);

	Control *motionRot = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
	Control *motionRotX = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTBEND_CLASS_ID);
	Control *motionRotY = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTBEND_CLASS_ID);
	Control *motionRotZ = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTBEND_CLASS_ID);
	branchTail->AddAttribute(motionRotX, GetString(IDS_PITCH));
	branchTail->AddAttribute(motionRotY, GetString(IDS_ROLL));
	branchTail->AddAttribute(motionRotZ, GetString(IDS_TWIST));

	if (tail->GetCATParentTrans()->GetLengthAxis() == Z) {
		motionRot->AssignController(motionRotX, X);
		motionRot->AssignController(motionRotY, Y);
		motionRot->AssignController(motionRotZ, Z);
	}
	else {
		motionRot->AssignController(motionRotZ, X);
		motionRot->AssignController(motionRotY, Y);
		motionRot->AssignController(motionRotX, Z);
	}
	pblock->SetControllerByID(PB_P3MOTIONROT, 0, motionRot, FALSE);
	pblock->SetValue(PB_CATHIERARCHYBRANCH, 0, branchTail);

	for (int i = 0; i < tail->GetNumBones(); i++) {
		CATMotionTailRot *catmotiontailrot = (CATMotionTailRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONTAIL_ROT_CLASS_ID);
		catmotiontailrot->Initialise((TailTrans*)tail->GetBone(i), this);
		tail->GetBone(i)->GetLayerTrans()->GetLayer(index)->SetRotationController(catmotiontailrot);
	}
}

Control* CATMotionTail::GetOwningCATMotionController()
{
	// We have no easy link back to the hub.  Cheat a little (making
	// assumptions here) look up the CATHierarchyBranch to find our owner.
	CATHierarchyBranch2* pOurBranch = GetCATHierarchyBranch();
	if (pOurBranch == NULL)
		return NULL;

	// This branch should be our hub...
	CATHierarchyBranch* pParentBranch = pOurBranch->GetBranchParent();
	if (pParentBranch == NULL)
		return NULL;

	ReferenceTarget *pOwner = pParentBranch->GetBranchOwner();

	// Verify the above assumption
	DbgAssert(pOwner->ClassID() == HUB_CLASS_ID);

	Hub* pHub = dynamic_cast<Hub*>(pOwner);
	if (pHub != NULL)
	{
		// We have our hub, now find our layer idx
		int idx = -1;
		if (pOurBranch != NULL)
		{
			CATHierarchyRoot* pRoot = pOurBranch->GetCATRoot();
			if (pRoot != NULL)
				idx = pRoot->GetLayerIndex();
		}

		// We should have all the info we need now to get our owning CATMotionHub
		if (idx >= 0)
		{
			CATClipValue* pValue = pHub->GetLayerTrans();
			if (pValue != NULL)
				return dynamic_cast<CATMotionController*>(pValue->GetLayer(idx));
		}
	}
	return NULL;
}

void CATMotionTail::DestructCATMotionHierarchy(LimbData2* pLimb)
{
	// We just need to make sure our offsetRot Branch
	// NULL's its pointer to the offsetRot Controller.
	CATHierarchyBranch2 *pBranch = GetCATHierarchyBranch();
	if (pBranch != NULL)
	{
		pblock->SetValue(PB_CATHIERARCHYBRANCH, 0, (ReferenceTarget*)NULL);
		pBranch->SelfDestruct();
	}

	CATMotionController::DestructCATMotionHierarchy(pLimb);
}

//-------------------------------------------------------------
void CATMotionTail::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod)
{
	valid.SetInstant(t);

	*(Point3*)val = pblock->GetPoint3(PB_P3OFFSETROT, t);
	*(Point3*)val += pblock->GetPoint3(PB_P3MOTIONROT, t);
}

void CATMotionTail::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	CATMotionTailDesc.BeginEditParams(ip, this, flags, prev);

	//	this->ip = ip;
	CATWeight* ctrlWeights = (CATWeight*)pblock->GetControllerByID(PB_PHASE_BIAS);
	if (ctrlWeights) {
		ctrlWeights->DisplayRollout(ip, flags, prev, GetString(IDS_TAILPHBIAS));
	}
}

void CATMotionTail::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	CATWeight* ctrlWeights = (CATWeight*)pblock->GetControllerByID(PB_PHASE_BIAS);
	if (ctrlWeights) ctrlWeights->EndEditParams(ip, flags, next);

	CATMotionTailDesc.EndEditParams(ip, this, flags, next);

	//	this->ip = NULL;
}

//RefTargetHandle Clone( RemapDir &remap );
RefResult CATMotionTail::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL) {
	switch (msg) {
	case REFMSG_CHANGE:
		ParamID changingParam;
		if (hTarg == pblock)
		{
			changingParam = pblock->LastNotifyParamID();
			switch (changingParam) {
			case PB_PHASE_BIAS:

				break;
			}
		}
	}

	return REF_SUCCEED;
};

