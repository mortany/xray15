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

#include "TailData2.h"
#include "TailTrans.h"
#include "Hub.h"
#include "SpineData2.h"

#include "CATHierarchyBranch2.h"
#include "Ease.h"

#include "CATMotionHub2.h"
#include "CATMotionTailRot.h"
#include "CATMotionTail.h"

#include "ICATParent.h"

class CATMotionTailRotClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATMotionTailRot(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONTAIL_ROT); }
	SClass_ID		SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONTAIL_ROT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionTailRot"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionTailRotClassDesc CATMotionTailRotDesc;
ClassDesc2* GetCATMotionTailRotDesc() { return &CATMotionTailRotDesc; }

static ParamBlockDesc2 weightshift_param_blk(CATMotionTailRot::PBLOCK_REF, _T("CATMotionTailRot params"), 0, &CATMotionTailRotDesc,
	P_AUTO_CONSTRUCT, CATMotionTailRot::PBLOCK_REF,
	CATMotionTailRot::PB_TAILTRANS, _T("TailTrans"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATMotionTailRot::PB_P3OFFSETROT_DEPRECATED, _T("Offset"), TYPE_POINT3, P_ANIMATABLE, IDS_CATMOTIONROT,
		p_end,
	CATMotionTailRot::PB_P3MOTIONROT, _T("CATMotionTail"), TYPE_POINT3, P_ANIMATABLE, IDS_CL_CATMOTIONTAIL,
		p_end,
	CATMotionTailRot::PB_TMOFFSET, _T("OffsetTM"), TYPE_MATRIX3, 0, 0,
		p_end,
	p_end
);

CATMotionTailRot::CATMotionTailRot() {
	pblock = NULL;
	CATMotionTailRotDesc.MakeAutoParamBlocks(this);
}

CATMotionTailRot::~CATMotionTailRot() {
	// If we are deleting we should remove the tail from the CATMotion Hierarchy
	if (pblock) {
		CATMotionTail *catmotion = (CATMotionTail*)pblock->GetControllerByID(PB_P3MOTIONROT);
		if (catmotion) {
			CATHierarchyBranch2 *branchTail = (CATHierarchyBranch2*)catmotion->GetCATHierarchyBranch();
			if (branchTail) branchTail->SelfDestruct();
		}
	}
	DeleteAllRefs();
}

RefTargetHandle CATMotionTailRot::Clone(RemapDir& remap)
{
	// make a new CATMotionTailRot object to be the clone
	CATMotionTailRot *newCATMotionTailRot = new CATMotionTailRot();
	remap.AddEntry(this, newCATMotionTailRot);

	newCATMotionTailRot->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATMotionTailRot, remap);

	// now return the new object.
	return newCATMotionTailRot;
}

void CATMotionTailRot::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		CATMotionTailRot *newctrl = (CATMotionTailRot*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void CATMotionTailRot::Initialise(TailTrans *tailbone, CATMotionTail* catmotiontail)
{
	pblock->SetValue(PB_TAILTRANS, 0, tailbone, FALSE);
	pblock->SetControllerByID(PB_P3MOTIONROT, 0, catmotiontail, FALSE);

	// Tails really nee to use the Dangle limbs, and tail stiffness graph.
	// In CAT1, CATMotion, and setup mode always had dangle limbs 'on'.
	// Now we need to try and generate an offset matrix that will keep the tail
	// as if Dangle limbs were on.

	// The 1st bone inherits directly off the hub. As Tail Stiffness goes up,
	// each bone inherrits more and more off the CATParent.

	TailData2 *tail = (TailData2*)tailbone->GetTail();
	Hub* hub = tail->GetHub();
	DbgAssert(hub);

	Matrix3 tmOffset = tailbone->GetSetupMatrix();
	for (int i = tailbone->GetBoneID() - 1; i >= 0; i--) {
		tmOffset = tmOffset * tail->GetBone(i)->GetSetupMatrix();
	}

	Matrix3 tmHub = tmOffset;
	tmOffset = tmOffset * hub->GetSetupMatrix();

	while (hub->GetInSpine() && !hub->GetLayerTrans()->TestFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT)) {
		tmHub = tmHub * hub->GetSetupMatrix();
		hub = hub->GetInSpine()->GetBaseHub();
	}
	// now mix up the 2 matricies
	BlendRot(tmOffset, tmHub, 1.0f - tail->GetTailStiffness(tailbone->GetBoneID()));

	Matrix3 tmSetup = tailbone->GetSetupMatrix();
	BlendRot(tmOffset, tmSetup, tail->GetHub()->GetDangleRatio(GetCurrentTime()));

	tmOffset.NoTrans();
	pblock->SetValue(PB_TMOFFSET, 0, tmOffset, FALSE);
}

//-------------------------------------------------------------
void CATMotionTailRot::SetValue(TimeValue t, void *val, int, GetSetMethod) {

	AngAxis ax = *(AngAxis*)val;
	Matrix3 tmOffset = pblock->GetMatrix3(PB_TMOFFSET);
	RotateMatrix(tmOffset, ax);
	tmOffset.NoTrans();
	pblock->SetValue(PB_TMOFFSET, t, tmOffset);
};

void CATMotionTailRot::GetValue(TimeValue t, void * val, Interval &valid, GetSetMethod)
{
	TailTrans *tailbone = (TailTrans*)pblock->GetReferenceTarget(PB_TAILTRANS);
	if (tailbone == NULL)
		return;

	TailData2 *tail = tailbone->GetTail();
	if (tail == NULL)
		return;

	ICATParentTrans *catparenttrans = tail->GetCATParentTrans();

	CATMotionTail *catmotion = (CATMotionTail*)pblock->GetControllerByID(PB_P3MOTIONROT);
	if (!(catparenttrans && catmotion)) return;

	int id = tailbone->GetBoneID();
	int lengthaxis = catparenttrans->GetLengthAxis();

	CATHierarchyBranch2 *branchTail = catmotion->GetCATHierarchyBranch();
	CATHierarchyRoot *root = branchTail->GetCATRoot();

	int index = root->GetLayerIndex();

	//////////////////////////////////////////////////////
	// This code came from the TailData2::GetCATMotion
	int newTime = t;
	Ease *ctrlStepEase = (Ease*)root->GetStepEaseGraph();
	if (ctrlStepEase)
	{
		// The phaseMult governs how many phases are present in the
		// tail, not how fast the phases move.
		float phaseMult = catmotion->pblock->GetFloat(CATMotionTail::PB_FREQUENCY);
		//		phaseMult *= GetPhaseBias(linkID);
		////////////////////////////////////////
		// Code From GetPhase Bias
		float LinkRatio = ((float)id + 0.5f) / ((float)tail->GetNumBones());

		// Magic 10000 number, Spinedata generates the weight graphs 1000 ticks long
		// these are only used in interpolating Quad spine Offset p3s
		int LinkIndex = (int)(LinkRatio * STEPTIME100);
		phaseMult *= catmotion->pblock->GetFloat(CATMotionTail::PB_PHASE_BIAS, LinkIndex);
		////////////////////////////////////////

		phaseMult += catmotion->pblock->GetFloat(CATMotionTail::PB_PHASEOFFSET);
		float currStepTime = ctrlStepEase->GetValue(t);
		newTime = ctrlStepEase->GetTime(currStepTime - (STEPTIME100 * phaseMult));
	}

	////////////////////////////////////////////////////
	// This code came from GetBaseTM on TailData2
	stepmask = 1.0f;
	if (index >= 0) {
		if (id == 0) {
			Control* catmotionhub = tail->GetHub()->GetLayerTrans()->GetLayer(index);
			if (catmotionhub->ClassID() == GetCATMotionHub2Desc()->ClassID()) {
				pathoffset = ((CATMotionHub2*)catmotionhub)->pathoffset;
				stepmask = ((CATMotionHub2*)catmotionhub)->GetFootStepMask(t);
			}
		}
		else {
			// Assuming that our parent has alreadybeen evaluated
			CATNodeControl *parentbone = tail->GetBone(id - 1);
			CATMotionTailRot* parentctrl = ((CATMotionTailRot*)parentbone->GetLayerTrans()->GetLayer(index)->GetRotationController());
			pathoffset = parentctrl->pathoffset;
			stepmask = parentctrl->stepmask;
		}
	}

	//////////////////////////////////////////////////////
	// Base Setup value
	Matrix3 tmOffset = pblock->GetMatrix3(PB_TMOFFSET);
	tmOffset.NoTrans();

	// Pos
	Matrix3* pResult = (Matrix3*)val;
	Point3 p3Pos = pResult->GetTrans();

	// Rot
	Point3 p3EulerRot;
	catmotion->GetValue(newTime, (void*)&p3EulerRot, valid, CTRL_ABSOLUTE);
	p3EulerRot *= DEG_TO_RAD * (tail->GetTailStiffness(id) * stepmask);

	Quat qtCATRot;
	EulerToQuat(&p3EulerRot.x, qtCATRot);
	PreRotateMatrix(tmOffset, qtCATRot);

	// this pathoffset stuff isnt' working
	// add on our own contribution to the path offset for the next bone in the tail
	Matrix3 tmOrient;
	// ensure we don't add in the effect of scale to the following eqn (dont use GetObjectLength)
	pathoffset += tmOffset.GetRow(lengthaxis)[Y] * tailbone->GetObjY() * tailbone->GetCATUnits();

	Hub* pHub = tail->GetHub();
	if (pHub != NULL)
		*pResult = pHub->GetNodeTM(t);

	root->GettmPath(t, tmOrient, valid, pathoffset, FALSE);
	BlendRot(*pResult, tmOrient, tail->GetTailStiffness(id));
	pResult->SetTrans(p3Pos);

	//////////////////////////////////////////////////////
	// This code came from the TailTrans::GetValue
	*pResult = tmOffset * *pResult;

}

void CATMotionTailRot::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	//	this->ip = ip;

	CATMotionTail *catmotiontail = (CATMotionTail*)pblock->GetControllerByID(PB_P3MOTIONROT);
	if (catmotiontail) catmotiontail->BeginEditParams(ip, flags, prev);
}

void CATMotionTailRot::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	CATMotionTail *catmotiontail = (CATMotionTail*)pblock->GetControllerByID(PB_P3MOTIONROT);
	if (catmotiontail) catmotiontail->EndEditParams(ip, flags, next);

	//	this->ip = NULL;
}

void CATMotionTailRot::DestructCATMotionHierarchy(LimbData2* pLimb)
{
	if (pblock == NULL)
		return;

	CATNodeControl *tailbone = (CATNodeControl*)pblock->GetReferenceTarget(PB_TAILTRANS);
	// If we are 0th tailbone, then we can assume that the entire tail
	// has been destructed.
	if (tailbone != NULL && tailbone->GetBoneID() == 0)
	{
		CATMotionTail* pTail = (CATMotionTail*)pblock->GetControllerByID(PB_P3MOTIONROT);
		if (pTail != NULL)
			pTail->DestructCATMotionHierarchy(pLimb);
	}

}
