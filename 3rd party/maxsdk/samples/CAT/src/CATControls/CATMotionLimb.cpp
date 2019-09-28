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
#include "ICATParent.h"

#include "SpineData2.h"
#include "LimbData2.h"
#include "BoneData.h"
#include "CollarboneTrans.h"
#include "PalmTrans2.h"
#include "DigitData.h"
#include "DigitSegTrans.h"
#include "FootTrans2.h"
#include "Hub.h"

 // Layer System
#include "CATClipHierarchy.h"
#include "CATClipValue.h"

// CATMotion controllers
#include "CATHierarchyRoot.h"
#include "CATHierarchyBranch2.h"
#include "Ease.h"

#include "CATMotionHub2.h"

#include "CATMotionPlatform.h"
#include "CATMotionLimb.h"
#include "CATMotionRot.h"
#include "CATMotionDigitRot.h"

#include "LiftPlantMod.h"
#include "FootBend.h"
#include "PivotRot.h"
#include "Monograph.h"

class CATMotionLimbClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATMotionLimb(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONLIMB); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONLIMB_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionLimb"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionLimbClassDesc CATMotionLimbDesc;
ClassDesc2* GetCATMotionLimbDesc() { return &CATMotionLimbDesc; }

//weightshift_param_blk
static ParamBlockDesc2 CATMotionLimb_param_blk(CATMotionLimb::PBLOCK_REF, _T("CATMotionLimb params"), 0, &CATMotionLimbDesc,
	P_AUTO_CONSTRUCT, CATMotionLimb::PBLOCK_REF,
	CATMotionLimb::PB_CATPARENT, _T("CATParent"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATMotionLimb::PB_LIMB, _T("Limb"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATMotionLimb::PB_CATHIERARCHY, _T("CATMotionHierarchy"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATMotionLimb::PB_CATHIERARCHYROOT, _T("CATHierarchyRoot"), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	CATMotionLimb::PB_CATMOTIONPLATFORM_DEPRECATED, _T(""), TYPE_REFTARG, P_NO_REF | P_OBSOLETE, 0,
		p_end,
	CATMotionLimb::PB_PHASEOFFSET, _T("PhaseOffset"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHASEOFFSET,
		p_end,
	CATMotionLimb::PB_LIFTPLANTMOD, _T("LiftPlantMod"), TYPE_FLOAT, P_ANIMATABLE, IDS_LIFTPLANTMOD,
		p_end,
	CATMotionLimb::PB_CATMOTION_STEPPING_NODES, _T("SteppingNodes"), TYPE_REFTARG_TAB, 0, P_VARIABLE_SIZE + P_NO_REF, 0,
		p_end,
p_end
);

CATMotionLimb::CATMotionLimb() {

	tEvalTime = 0;
	tStepTimeEvalTime = 0;
	tStepTime = 0;
	dFootStepMask = 1.0f;
	pblock = NULL;
	CATMotionLimbDesc.MakeAutoParamBlocks(this);
}

void CATMotionLimb::RefDeleted()
{
	// If we are no longer referenced by any CATGraphs, then destruct
	DependentIterator itr(this);
	while (ReferenceMaker* maker = itr.Next())
	{
		if (dynamic_cast<CATGraph*>(maker) != NULL)
			return;
	}
	DestructCATMotionHierarchy(GetLimb());
}

CATMotionLimb::~CATMotionLimb() {

	if (pblock) {
		CATHierarchyRoot *root = GetCATMotionRoot();
		if (root && !root->TestAFlag(A_IS_DELETED))
			root->RemoveLimb(GetLimb());
	}

	DeleteAllRefs();
}

ICATParentTrans* CATMotionLimb::GetCATParentTrans()
{
	if (GetLimb())
		return GetLimb()->GetCATParentTrans();
	return NULL;
}

Control* CATMotionLimb::GetOwningCATMotionController()
{
	// first, find our idx
	int idx = -1;
	CATHierarchyRoot* pRoot = GetCATMotionRoot();
	if (pRoot != NULL)
		idx = pRoot->GetLayerIndex();

	// Do we have a valid index?
	if (idx < 0)
		return NULL;

	LimbData2* pLimb = GetLimb();
	if (pLimb != NULL)
	{
		Hub* pHub = pLimb->GetHub();
		if (pHub != NULL)
		{
			CATClipValue* pValue = pHub->GetLayerTrans();
			if (pValue != NULL)
				return dynamic_cast<CATMotionController*>(pValue->GetLayer(idx));
		}
	}
	return NULL;
}

void CATMotionLimb::DestructCATMotionHierarchy(LimbData2* pLimb)
{
	// Remove our linked limb from all the CATHierarchy branches
	CATHierarchyRoot* pRoot = GetCATMotionRoot();
	DbgAssert(pLimb == NULL || pLimb == GetLimb());
	if (pRoot != NULL)
		pRoot->RemoveLimb(GetLimb());

	// All done - proceed with regular destruction.
	CATMotionController::DestructCATMotionHierarchy(pLimb);
}

void CATMotionLimb::Initialise(int index, LimbData2* limb, CATHierarchyBranch2* branchHubGroup)
{
	CATHierarchyRoot* hierarchyRoot = branchHubGroup->GetCATRoot();

	pblock->SetValue(PB_LIMB, 0, limb);
	pblock->SetValue(PB_CATHIERARCHYROOT, 0, hierarchyRoot);

	// add our leaf to the limbphases branch
	CATHierarchyBranch2* limbphases = (CATHierarchyBranch2*)hierarchyRoot->AddBranch(GetString(IDS_LIMBPHASES), 2);
	Control* phaseoffset = limbphases->AddLimbPhasesLimb(limb);

	/////////////////////////////////
	// PhaseOffset initilaisation
	float numlimbs = (float)limb->GetHub()->GetNumLimbs();
	float limbid = (float)limb->GetLimbID();
	float fPhaseOffset = -0.5f + (float)1 / (float)(numlimbs * 2) + ((float)limbid / (float)numlimbs);
	//	float fPhaseOffset = (limbid / numlimbs) + (1.0f/numlimbs);

		// here we are reversing the direction of the pase distribution.
		// every second hub is clockwise
	if (limb->GetHub()->GetInSpine() && (!limb->GetHub()->GetInSpine()->GetBaseHub()->GetInSpine()))
		fPhaseOffset *= -1.0f;

	phaseoffset->SetValue(0, (void*)&fPhaseOffset, 1, CTRL_ABSOLUTE);
	pblock->SetControllerByID(PB_PHASEOFFSET, 0, phaseoffset, FALSE);

	/////////////////////////////////
	// First, create the CATMotion Hiearchy global variables
	// CATHierarchyBranch2* CATHierarchy = (CATHierarchyBranch2*)hierarchyRoot->GetBranch(GetString(IDS_GLOBALHIERARCHY));

	CATHierarchyBranch2 *branchLimb;
	if (GetisLeg())	branchLimb = (CATHierarchyBranch2*)branchHubGroup->AddBranch(GetString(IDS_LEGS));
	else			branchLimb = (CATHierarchyBranch2*)branchHubGroup->AddBranch(GetString(IDS_ARMS));

	pblock->SetValue(PB_CATHIERARCHY, 0, branchLimb);

	LiftPlantMod *ctrlLiftPlantMod = (LiftPlantMod*)CreateInstance(CTRL_FLOAT_CLASS_ID, LIFTPLANTMOD_CLASS_ID);
	pblock->SetControllerByID(PB_LIFTPLANTMOD, 0, ctrlLiftPlantMod, FALSE);
	//	pblock->SetValue(PB_LIFTPLANTMOD, 0, ctrlLiftPlantMod);
		// All CATGraphs hold references to CATMotion Limb. There references are set up during
		// CATHierarchyBranch2::AddAttribute. This reference will fail because CATMotionLimb references LiftPlantMod
	branchLimb->AddAttribute(ctrlLiftPlantMod, GetString(IDS_LIFTPLANTMOD), this);

	//////////////////////////////////////////////////////////////////////////////////////////
	// setup and apply the retargetting controller
	CATGraph *legweight = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, LEGWEIGHT_CLASS_ID);
	branchLimb->AddAttribute(legweight, GetString(IDS_LEGWEIGHT), this);

	limb->GetLayerController(2)->SetLayer(index, legweight);

	//////////////////////////////////////////////////////////////////////////////////////////
	// set some defaults
	float val;
	if (!limb->GetisLeg()) // arms default to FK
		val = 1.0f;
	else val = 0.0f;
	// laeyrikFKRatio
	((CATClipValue*)limb->GetReference(LimbData2::REF_LAYERIKFKRATIO))->GetLayer(index)->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);

	//////////////////////////////////////////////////////////////////////////////////////////
	// Now Keep working down thwe Hierarchy

	//
	if (limb->GetCollarbone()) {
		//		limb->GetCollarbone()->AddCATMotionLayer(index, branchLimb, this);

		CATHierarchyBranch2 *CollarboneCATHierarchy = (CATHierarchyBranch2*)branchLimb->AddBranch(GetString(IDS_COLLARBONE));

		Control *p3CATOffset = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
		CollarboneCATHierarchy->AddAttribute(p3CATOffset, GetString(IDS_OFFSET), this);

		Control *CollarboneX = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
		Control *CollarboneY = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
		Control *CollarboneZ = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);

		CollarboneCATHierarchy->AddAttribute(CollarboneX, GetString(IDS_MOTION_X), this);
		CollarboneCATHierarchy->AddAttribute(CollarboneY, GetString(IDS_MOTION_Y), this);
		CollarboneCATHierarchy->AddAttribute(CollarboneZ, GetString(IDS_MOTION_Z), this);

		Control *p3CATRotation = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
		if (limb->GetCATParentTrans()->GetLengthAxis() == Z) {
			p3CATRotation->AssignController(CollarboneX, X);
			p3CATRotation->AssignController(CollarboneY, Y);
			p3CATRotation->AssignController(CollarboneZ, Z);
		}
		else {
			p3CATRotation->AssignController(CollarboneZ, X);
			p3CATRotation->AssignController(CollarboneY, Y);
			p3CATRotation->AssignController(CollarboneX, Z);
		}

		CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONROT_CLASS_ID);
		ctrlCATMotionRot->Initialise(this, limb->GetCollarbone(), p3CATOffset, p3CATRotation, 0);
		limb->GetCollarbone()->GetLayerTrans()->GetLayer(index)->SetRotationController(ctrlCATMotionRot);;

		// forced an invalidation of the interval and caches
	//	p3CATRotation->GetXController()->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);

	}

	for (int i = 0; i < limb->GetNumBones(); i++) {

		CATNodeControl *limbbone = limb->GetBoneData(i);

		if (i == 0)// && !limb->GetisLeg())
		{
			Control* p3CATOffset = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
			branchLimb->AddAttribute(p3CATOffset, GetString(IDS_OFFSETROT), this);

			CATGraph* ArmSwing = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			CATGraph* ArmCrossSwing = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTBEND_CLASS_ID);
			CATGraph* ArmZ = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);

			float dSwingDefaultVals[] = { 10.0f, 0.0f, 0.0f, 0.333f, 0.333f,      -10.0f, 50.0f, 0.0f, 0.333f, 0.333f };
			int nNumSwingDefaults = 10;
			branchLimb->AddAttribute(ArmSwing, GetString(IDS_ARMSWING), this, nNumSwingDefaults, &dSwingDefaultVals[0]);
			branchLimb->AddAttribute(ArmCrossSwing, GetString(IDS_ARMCROSSSWING), this);
			float dTwistDefaultVals[] = { -10.0f, 0.0f, 0.0f, 0.333f, 0.333f,      10.0f, 50.0f, 0.0f, 0.333f, 0.333f };
			branchLimb->AddAttribute(ArmZ, GetString(IDS_ARMTWIST), this, nNumSwingDefaults, &dTwistDefaultVals[0]);

			Control* p3CATRotation = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);

			if (limb->GetCATParentTrans()->GetLengthAxis() == Z) {
				p3CATRotation->AssignController(ArmSwing, X);
				p3CATRotation->AssignController(ArmCrossSwing, Y);
				p3CATRotation->AssignController(ArmZ, Z);
			}
			else {
				ArmSwing->FlipValues();
				p3CATRotation->AssignController(ArmZ, X);
				p3CATRotation->AssignController(ArmCrossSwing, Y);
				p3CATRotation->AssignController(ArmSwing, Z);
			}

			CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONROT_CLASS_ID);
			//	CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)GetCATMotionRotDesc()->Create(FALSE);
			ctrlCATMotionRot->Initialise(this, limbbone, p3CATOffset, p3CATRotation, CATROT_INHERITORIENT);

			// Put the CATMotion controller into the layer
			limbbone->GetLayerTrans()->GetLayer(index)->SetRotationController(ctrlCATMotionRot);;

			/*		// Put the CATMotion controller into the layer
					Control* ctrlCATMotionRot = (Control*)NewDefaultRotationController();
					CATClipValue *layers = limbbone->GetLayerTrans();
					Control* ctrl = layers->GetLayer(index);

					TSTR name;
					//	GetClassName(name);
					ctrl->GetClassName(name);
					SClass_ID sclsid = ctrl->SuperClassID();
					Class_ID clsid = ctrl->ClassID();

					for(int j=0;j<ctrl->NumRefs();j++){
						if(ctrl->GetReference(j) && ctrl->GetReference(j)->SuperClassID()==CTRL_ROTATION_CLASS_ID){
							ctrl->ReplaceReference(j, ctrlCATMotionRot);
							break;
						}
					}
			*/
		}
		else if (i == (limb->GetNumBones() - 1))
		{
			CATGraph *bendangle = NULL;
			if (limb->GetisLeg()) {
				float dKADefaultVals[] = { 0.0f, 0.0f, 0.333f, 0.333f,    50.0f, 0.0f, 0.0f, 0.333f, 0.333f,     0.0f, 0.0f, 0.333f, 0.333f,     0.0f, 0.0f, 0.0f, 0.333f, 0.333f };
				int nNumKADefaults = 18;
				bendangle = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, KNEEANGLE_CLASS_ID);
				branchLimb->AddAttribute(bendangle, GetString(IDS_KAKNEEANGLE), this, nNumKADefaults, &dKADefaultVals[0]);
			}
			else {
				float dArmBendDefaultVals[] = { 20.0f, 0.0f, 0.0f, 0.333f, 0.333f,     -20.0f, 50.0f, 0.0f, 0.333f, 0.333f };
				int nNumArmBendDefaults = 10;
				bendangle = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
				branchLimb->AddAttribute(bendangle, GetString(IDS_ARMBEND), this, nNumArmBendDefaults, &dArmBendDefaultVals[0]);
				// we need to force this guy to return radians
			//	bendangle->SetUnits(ANGLE_DIM);
			}

			//			Control* rot = limb->GetBone(i)->GetLayerTrans()->GetLayer(index)->GetRotationController();
			//			if(rot->ClassID().PartA() != EULER_CONTROL_CLASS_ID){
			//				Quat qt;
			//				rot->GetValue(0, (void*)&qt, FOREVER);
			//				rot = (Control*)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(EULER_CONTROL_CLASS_ID, 0));
			//				rot->SetValue(0, (void*)&qt);
			//				limb->GetBone(i)->GetLayerTrans()->GetLayer(index)->SetRotationController(rot);
			//			}
			//			rot->ReplaceReference(X, bendangle);

			Control* p3CATRotation = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
			if (limb->GetCATParentTrans()->GetLengthAxis() == Z) {
				p3CATRotation->AssignController(bendangle, X);
			}
			else {
				bendangle->FlipValues();
				p3CATRotation->AssignController(bendangle, Z);
			}

			CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONROT_CLASS_ID);
			ctrlCATMotionRot->Initialise(this, limbbone, NULL, p3CATRotation, 0);

			// Put the CATMotion controller into the layer
			limbbone->GetLayerTrans()->GetLayer(index)->SetRotationController(ctrlCATMotionRot);;
		}
	}

	PalmTrans2* pPalm = limb->GetPalmTrans();
	if (pPalm) {
		// add our palm branch
		CATHierarchyBranch2 *CATHierarchyPalm;
		if (limb->GetisArm())
			CATHierarchyPalm = (CATHierarchyBranch2*)branchLimb->AddBranch(GetString(IDS_PALM));
		else CATHierarchyPalm = (CATHierarchyBranch2*)branchLimb->AddBranch(GetString(IDS_ANKLE));
		//	CATHierarchyBranch2 *CATHierarchyPalm = (CATHierarchyBranch2*)branchLimb->AddBranch(limb->GetPalm()->GetName());

		Control *p3CATOffset = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
		Control *p3CATRotation = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
		if (limb->GetisLeg())
		{
			CATGraph *footbend = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTBEND_CLASS_ID);
			CATHierarchyPalm->AddAttribute(footbend, GetString(IDS_FOOTBEND), this);

			CATGraph *targetAlign = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTPOS_CLASS_ID);
			float dtargetAlignDefaultVals[] = { 20.0f, 0.0f, 80.0f, 1.0f };
			int nNumDefaults = 4;
			CATHierarchyPalm->AddAttribute(targetAlign, GetString(IDS_TARGETALIGN), this, nNumDefaults, dtargetAlignDefaultVals);

			if (limb->GetCATParentTrans()->GetLengthAxis() == Z)
				p3CATRotation->AssignController(footbend, X);
			else {
				footbend->FlipValues();
				p3CATRotation->AssignController(footbend, Z);
			}

			// targetAlign is 1
			((CATClipValue*)pPalm->SubAnim(PalmTrans2::SUBTARGETALIGN))->SetLayer(index, targetAlign);
		}
		else
		{
			float dFlopXDefaultVals[] = { 20.0f, 25.0f, 0.0f, 0.333f, 0.333f,     -20.0f, 75.0f, 0.0f, 0.333f, 0.333f };
			int nNumFlopXDefaults = 10;
			CATGraph *ctrlHandFlopX = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			CATHierarchyPalm->AddAttribute(ctrlHandFlopX, GetString(IDS_HANDFLOP_X), this, nNumFlopXDefaults, &dFlopXDefaultVals[0]);

			CATGraph *ctrlHandFlopY = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			CATHierarchyPalm->AddAttribute(ctrlHandFlopY, GetString(IDS_HANDFLOP_Y), this);

			CATGraph *ctrlHandTwist = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
			CATHierarchyPalm->AddAttribute(ctrlHandTwist, GetString(IDS_HANDTWIST), this);

			if (limb->GetCATParentTrans()->GetLengthAxis() == Z) {
				p3CATRotation->AssignController(ctrlHandFlopX, X);
				p3CATRotation->AssignController(ctrlHandFlopY, Y);
				p3CATRotation->AssignController(ctrlHandTwist, Z);
			}
			else {
				p3CATRotation->AssignController(ctrlHandFlopX, Z);
				p3CATRotation->AssignController(ctrlHandFlopY, Y);
				p3CATRotation->AssignController(ctrlHandTwist, X);
			}

			float val = 1.0f;
			((CATClipValue*)pPalm->SubAnim(PalmTrans2::SUBTARGETALIGN))->GetLayer(index)->SetValue(0, (void*)&val, TRUE, CTRL_ABSOLUTE);
		}

		//////////////////////////////////////////////////////////////////////////
		// Add the catmotion controller to the first layer of the layerSystem
		CATHierarchyPalm->AddAttribute(p3CATOffset, GetString(IDS_OFFSETROT), this);

		CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONROT_CLASS_ID);
		int flags = 0;
		if (!limb->GetisLeg()) flags = CATROT_USEORIENT;
		ctrlCATMotionRot->Initialise(this, pPalm, p3CATOffset, p3CATRotation, flags);

		pPalm->GetLayerTrans()->GetLayer(index)->SetRotationController(ctrlCATMotionRot);

		//////////////////////////////////////////////////////////////////////////////////////////
		int nDigits = pPalm->NumChildCATNodeControls();
		if (nDigits > 0) {

			// Create a BendAngle controller for the digits. All digits
			// access this one controller, cause why would we need to have one each?
			CATGraph *ctrlDigitCurl, *ctrlDigitRoll, *ctrlDigitSpread;
			if (limb->GetisLeg())
			{
				ctrlDigitCurl = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTROT_CLASS_ID);
				CATHierarchyPalm->AddAttribute(ctrlDigitCurl, GetString(IDS_DIGITCURLANGLE), this);

				ctrlDigitRoll = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTROT_CLASS_ID);
				CATHierarchyPalm->AddAttribute(ctrlDigitRoll, GetString(IDS_DIGITROLLANGLE), this);

				ctrlDigitSpread = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTROT_CLASS_ID);
				CATHierarchyPalm->AddAttribute(ctrlDigitSpread, GetString(IDS_DIGITSPREADANGLE), this);
			}
			else
			{
				ctrlDigitCurl = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
				CATHierarchyPalm->AddAttribute(ctrlDigitCurl, GetString(IDS_DIGITCURLANGLE), this);

				ctrlDigitRoll = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
				CATHierarchyPalm->AddAttribute(ctrlDigitRoll, GetString(IDS_DIGITROLLANGLE), this);

				ctrlDigitSpread = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, MONOGRAPH_CLASS_ID);
				CATHierarchyPalm->AddAttribute(ctrlDigitSpread, GetString(IDS_DIGITSPREADANGLE), this);
			}

			// sprinkle these controllers across every digit segment
			for (int i = 0; i < nDigits; i++) {

				DigitData* pDigitData = pPalm->GetDigit(i);
				if (pDigitData == NULL)
					continue;

				for (int j = 0; j < pDigitData->GetNumBones(); j++) {

					Control *p3DigitBend = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);

					if (limb->GetCATParentTrans()->GetLengthAxis() == Z) {
						if (j == 0) {
							p3DigitBend->AssignController(ctrlDigitSpread, Y);
							p3DigitBend->AssignController(ctrlDigitRoll, Z);
						}
						p3DigitBend->AssignController(ctrlDigitCurl, X);
					}
					else {
						if (j == 0) {
							ctrlDigitSpread->FlipValues();
							p3DigitBend->AssignController(ctrlDigitSpread, Y);
							p3DigitBend->AssignController(ctrlDigitRoll, X);
						}
						p3DigitBend->AssignController(ctrlDigitCurl, Z);
					}

					CATMotionDigitRot* ctrlCATMotionRot = (CATMotionDigitRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONDIGITROT_CLASS_ID); //CATMOTIONROT_CLASS_ID);
					ctrlCATMotionRot->Initialise(pDigitData->GetBone(j), p3DigitBend);
					pDigitData->GetBone(j)->GetLayerTrans()->GetLayer(index)->SetRotationController(ctrlCATMotionRot);;
				}
			}
		}
	}

	INodeTab nodes;
	CATNodeControl* ctrl;
	limb->AddSystemNodes(nodes, kSNCFileSave);
	int numfeet = 0;
	for (int i = 0; i < nodes.Count(); i++) {
		if (nodes[i]) {
			ctrl = (CATNodeControl*)nodes[i]->GetTMController()->GetInterface(I_CATNODECONTROL);
			if (ctrl) {
				CATClipMatrix3* pLayer = ctrl->GetLayerTrans();
				if (NULL != pLayer && pLayer->TestFlag(CLIP_FLAG_HAS_TRANSFORM)) {
					CATMotionPlatform* ctrlCATMotionPlatform = (CATMotionPlatform*)CreateInstance(CTRL_MATRIX3_CLASS_ID, CATMOTIONPLATFORM_CLASS_ID);
					ctrlCATMotionPlatform->Initialise(index, ctrl, branchLimb, this);
					pLayer->SetLayer(index, ctrlCATMotionPlatform);

					// keep this pointer so later we can message
					// the foot to do things like create footprints
					numfeet++;
					pblock->SetCount(PB_CATMOTION_STEPPING_NODES, numfeet);
					pblock->SetValue(PB_CATMOTION_STEPPING_NODES, 0, ctrlCATMotionPlatform, (numfeet - 1));
				}
			}
		}
	}

}

class LimbAndRemap
{
public:
	CATMotionLimb* limb;
	CATMotionLimb* clonedlimb;
	RemapDir* remap;

	LimbAndRemap(CATMotionLimb* limb, CATMotionLimb* clonedlimb, RemapDir* remap) {
		this->limb = limb;
		this->clonedlimb = clonedlimb;
		this->remap = remap;
	}
};

void CATMotionLimbCloneNotify(void *param, NotifyInfo*)
{
	LimbAndRemap *limbAndremap = (LimbAndRemap*)param;

	for (int i = 0; i < limbAndremap->limb->pblock->Count(CATMotionLimb::PB_CATMOTION_STEPPING_NODES); i++) {
		Control* foot = (Control*)limbAndremap->limb->pblock->GetReferenceTarget(CATMotionLimb::PB_CATMOTION_STEPPING_NODES, 0, i);
		Control *clonedfoot = (Control*)limbAndremap->clonedlimb->pblock->GetReferenceTarget(CATMotionLimb::PB_CATMOTION_STEPPING_NODES, 0, i);
		if (foot) {
			Control *newfoot = (Control*)limbAndremap->remap->FindMapping(foot);
			if (newfoot && (newfoot != clonedfoot)) {
				limbAndremap->clonedlimb->pblock->SetValue(CATMotionLimb::PB_CATMOTION_STEPPING_NODES, i, (ReferenceTarget*)newfoot, i);
			}
		}
	}

	UnRegisterNotification(CATMotionLimbCloneNotify, limbAndremap, NOTIFY_NODE_CLONED);

	delete limbAndremap;
}

RefTargetHandle CATMotionLimb::Clone(RemapDir& remap)
{
	// make a new CATMotionLimb object to be the clone
	CATMotionLimb *newCATMotionLimb = new CATMotionLimb();
	remap.AddEntry(this, newCATMotionLimb);

	newCATMotionLimb->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	RegisterNotification(CATMotionLimbCloneNotify, new LimbAndRemap(this, newCATMotionLimb, &remap), NOTIFY_NODE_CLONED);

	BaseClone(this, newCATMotionLimb, remap);
	return newCATMotionLimb;
}

void CATMotionLimb::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		CATMotionLimb *newctrl = (CATMotionLimb*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

/************************************************************************/
/* LimbData interface methods                                           */
/************************************************************************/

Color	CATMotionLimb::GetLimbColour()
{
	LimbData2* pLimb = GetLimb();
	if (pLimb != NULL)
		return pLimb->GetGroupColour();
	return Color(0xFF0000);
}

TSTR	CATMotionLimb::GetLimbName()
{
	LimbData2* pLimb = GetLimb();
	if (pLimb != NULL)
		return pLimb->GetName();
	return _M("ERROR: Missing Limb!");
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

// The problem with this is iStepNum can be -1. Which is not
// a wise array index. So, we always add one and maybe waste
// a Matrix space on FootTrans. 
int CATMotionLimb::GetStepNum(TimeValue t)
{
	GetStepTimeRange(t);		// Ensure current value
	return iStepNum + 1;	// This is to compensate for +ve phaseoffsets,
						// which will offset time 0 to be a -ve footstep
}

//
//	Calculates the time interval for this step in real time
//	TODO: Work over this function again, should be much faster.
//
Interval CATMotionLimb::GetStepTimeRange(TimeValue t)
{
	CATHierarchyRoot *root = GetCATMotionRoot();
	if (!root) return GetCOREInterface()->GetAnimRange();

	Ease *stepease = (Ease*)root->GetStepEaseGraph();
	if (!stepease) return GetCOREInterface()->GetAnimRange();

	int stepeaseval = (int)stepease->GetValue(t);

	iStepNum = (int)floor((float)stepeaseval / (float)STEPTIME100);

	float BaseSegstartv = (float)(iStepNum * STEPTIME100);
	float BaseSegendv = (float)((iStepNum + 1) * STEPTIME100);

	int BaseSegStartT = stepease->GetTime(BaseSegstartv);
	int BaseSegEndT = stepease->GetTime(BaseSegendv);

	phaseStart = pblock->GetFloat(PB_PHASEOFFSET, BaseSegStartT);
	phaseStart = max(min(0.5f, phaseStart), -0.5f);
	phaseStart = phaseStart * STEPTIME100;

	phaseEnd = pblock->GetFloat(PB_PHASEOFFSET, BaseSegEndT);
	phaseEnd = max(min(0.5f, phaseEnd), -0.5f);
	phaseEnd = phaseEnd * STEPTIME100;

	segstartv = BaseSegstartv + phaseStart;
	segendv = BaseSegendv + phaseEnd;

	if ((phaseEnd < 0.0f) && (stepeaseval >= segendv))
	{	// we are should be on the next segment
		// but with a negative phase offset
		iStepNum++;
		//			BaseSegStartv = BaseSegendv;
		BaseSegendv = (float)((iStepNum + 1) * STEPTIME100);
		phaseStart = phaseEnd;
		BaseSegEndT = stepease->GetTime(BaseSegendv);
		phaseEnd = pblock->GetFloat(PB_PHASEOFFSET, BaseSegEndT);

		phaseEnd = max(min(0.5f, phaseEnd), -0.5f);
		phaseEnd = phaseEnd * STEPTIME100;
		segstartv = segendv;
		segendv = BaseSegendv + phaseEnd;
	}
	//
	// We have a +ve phaseoffset and we lave leaked into the previous segment
	else if ((phaseStart > 0.0f) && (segstartv > stepeaseval))
	{
		iStepNum--;
		BaseSegstartv = (float)(iStepNum * STEPTIME100);
		phaseEnd = phaseStart;
		BaseSegStartT = stepease->GetTime(BaseSegstartv);
		phaseStart = pblock->GetFloat(PB_PHASEOFFSET, BaseSegStartT);
		phaseStart = max(min(0.5f, phaseStart), -0.5f);
		phaseStart = phaseStart * STEPTIME100;
		segendv = segstartv;
		segstartv = BaseSegstartv + phaseStart;
	}

	int SegStartT, SegEndT;

	SegStartT = stepease->GetTime(segstartv);
	SegEndT = stepease->GetTime(segendv);

	// Guess at the either side steps ignoring changes in phase
	LastStepRange.Set(stepease->GetTime(segstartv - (float)(STEPTIME100)), SegStartT);
	ThisStepRange.Set(SegStartT, SegEndT);
	NextStepRange.Set(SegEndT, stepease->GetTime(segendv + (float)(STEPTIME100)));

	return ThisStepRange;
}

float CATMotionLimb::GetFootStepMask(TimeValue t)
{
	return GetStepTime(t, 1.0f, tStepTime);
}

// This is going to replace the StepEase controller
float CATMotionLimb::GetStepTime(TimeValue t, float ModAmount, int &tOutStepTime, BOOL isStepMasked/* = TRUE*/, BOOL maskedbyFootPrints/* = FALSE*/)
{
	CATHierarchyRoot *root = GetCATMotionRoot();
	// we can be called during loading by PLCBs and we need to just bail
	if (root == NULL || root->GetCATParentTrans() == NULL) {
		tOutStepTime = 0;
		return 0.0;
	}

	// It would be really good if we could cache
/*	if(tStepTimeEvalTime == t)
	{
		tOutStepTime = this->tStepTime;
	}
	else
	{
*/
	Interval StepRange = GetStepTimeRange(t);

	if (StepRange.End() != StepRange.Start())
		// upto, but not including StepRange.End()
		// it is impor
		dStepRatio = ((float)(t - StepRange.Start())) / (float)((StepRange.End() + 1) - StepRange.Start());
	else // in case of devide by zero
		dStepRatio = 0.0;

	tOutStepTime = (int)(segstartv + (dStepRatio * (segendv - segstartv)) - (phaseStart + ((phaseEnd - phaseStart) * dStepRatio)));

	//		tStepTimeEvalTime = t;
	//		this->tStepTime = tOutStepTime;
	//	}

	if (ModAmount > 0.0f)
	{
		Control* ctrlLiftPlantMod = pblock->GetControllerByID(PB_LIFTPLANTMOD);
		//	ctrlLiftPlantMod = (Control*)pblock->GetReferenceTarget(PB_LIFTPLANTMOD);
		float liftPlantModVal = (float)tOutStepTime;
		if (ctrlLiftPlantMod) {
			Interval iv = FOREVER;
			ctrlLiftPlantMod->GetValue(t, (void*)&liftPlantModVal, iv, CTRL_RELATIVE);
			tOutStepTime += int(liftPlantModVal * ModAmount);
		}
	}

	// cache the time of eval so that subsequent calls wont get this far agian
	// we have a circular issue where GetStepMask calls GetStepTime so the
	// caches must be valid before we calculate the masks
	if (isStepMasked)
	{
		ICATParentTrans* pCATParent = root->GetCATParentTrans();
		if (pCATParent == NULL)
			return 0.0f;

		tStepTimeEvalTime = t;
		this->tStepTime = tOutStepTime;
		float dCATUnits = pCATParent->GetCATUnits();

		if (root->GetWalkMode() == CATHierarchyRoot::WALK_ON_PATHNODE && root->GetisStepMasks())
		{
			CATMotionPlatform *platform = GetCATMotionPlatform(0);
			if (maskedbyFootPrints && platform)
				this->dFootStepMask = platform->GetStepMask(tOutStepTime);
			else
			{
				Ease* distcovered = (Ease*)root->GetDistCovered();
				if (!distcovered) {
					this->dFootStepMask = 1.0f;
					return this->dFootStepMask;
				}

				BOOL bLastStepMoving;
				BOOL bThisStepMoving;
				BOOL bNextStepMoving;

				Interval ivMovementRange = distcovered->GetExtents();

				float dThisStepStart = distcovered->GetValue(ThisStepRange.Start());
				float dThisStepEnd = distcovered->GetValue(ThisStepRange.End());

				if (!ivMovementRange.InInterval(ThisStepRange.Start())) bLastStepMoving = FALSE;
				else
				{
					float dLastStepStart = distcovered->GetValue(LastStepRange.Start());
					bLastStepMoving = (dThisStepStart - dLastStepStart) > dCATUnits;
				}

				if (!ivMovementRange.InInterval(ThisStepRange.Start()) &&
					!ivMovementRange.InInterval(ThisStepRange.End()))  bThisStepMoving = FALSE;
				else bThisStepMoving = (dThisStepEnd - dThisStepStart) > dCATUnits;

				if (!ivMovementRange.InInterval(ThisStepRange.End())) bNextStepMoving = FALSE;
				else
				{
					float dNextStepEnd = distcovered->GetValue(NextStepRange.End());
					bNextStepMoving = (dNextStepEnd - dThisStepEnd) > dCATUnits;
				}

				/*				float dLastStepStart = distcovered->GetValue(LastStepRange.Start());
								float dThisStepStart = distcovered->GetValue(ThisStepRange.Start());
								float dThisStepEnd = distcovered->GetValue(ThisStepRange.End());
								float dNextStepEnd = distcovered->GetValue(NextStepRange.End());

								BOOL bLastStepMoving = (dThisStepStart - dLastStepStart) > dCATUnits;
								BOOL bThisStepMoving = (dThisStepEnd - dThisStepStart) > dCATUnits;
								BOOL bNextStepMoving = (dNextStepEnd - dThisStepEnd) > dCATUnits;
				*/
				if (bLastStepMoving && bThisStepMoving && bNextStepMoving)
					this->dFootStepMask = 1.0f;
				else if (!bLastStepMoving && !bThisStepMoving && !bNextStepMoving)
					this->dFootStepMask = 0.0f;
				// starting to move
				else if (!bLastStepMoving && bThisStepMoving)
					this->dFootStepMask = dStepRatio;
				// stopping
				else if (bThisStepMoving && !bNextStepMoving)
					this->dFootStepMask = 1.0f - dStepRatio;
				else
					this->dFootStepMask = 0.0f;
			}
		}
		else this->dFootStepMask = 1.0f;
	}
	return this->dFootStepMask;
}

int CATMotionLimb::GetLMR()
{
	if (GetLimb())	return GetLimb()->GetLMR();
	return 0;
}

void CATMotionLimb::SetLeftRight(Matrix3 &inVal)
{
	inVal.SetRow(0, (inVal.GetRow(0) * (float)GetLMR()));
}

void CATMotionLimb::SetLeftRightRot(Point3 &inVal)
{
	inVal[Y] *= GetLMR();
	inVal[Z] *= GetLMR();
}
void CATMotionLimb::SetLeftRightPos(Point3 &inVal)
{
	inVal[X] *= GetLMR();
};

void CATMotionLimb::SetLeftRight(Quat &inVal)
{
	if (GetLMR() == 0)return;

	AngAxis inValAA(inVal);
	SetLeftRightRot(inValAA.axis);
	inVal = Quat(inValAA);
};

void CATMotionLimb::SetLeftRight(float &inVal)
{
	inVal *= GetLMR();
};

BOOL CATMotionLimb::GetisLeg()
{
	return GetLimb() ? GetLimb()->GetisLeg() : FALSE;
}
BOOL CATMotionLimb::GetisArm()
{
	return GetLimb() ? GetLimb()->GetisArm() : FALSE;;
}

void CATMotionLimb::GetFootPrintPos(TimeValue t, Point3 &pos)
{
	pos = P3_IDENTITY;
	int numplatforms = GetNumCATMotionPlatforms();
	for (int i = 0; i < numplatforms; i++) {
		CATMotionPlatform *platform = GetCATMotionPlatform(i);
		Point3 footpos = P3_IDENTITY;
		if (platform) platform->GetFootPrintPos(t, footpos);
		pos += footpos;
	}
	pos /= (float)numplatforms;

}
void CATMotionLimb::GetFootPrintRot(TimeValue t, float &zRot)
{
	zRot = 0.0f;
	int numplatforms = GetNumCATMotionPlatforms();
	for (int i = 0; i < numplatforms; i++) {
		CATMotionPlatform *platform = GetCATMotionPlatform(i);
		float footzrot = 0.0f;
		if (platform) platform->GetFootPrintRot(t, footzrot);
		zRot += footzrot;
	}
	zRot /= numplatforms;
}

float CATMotionLimb::GetPathOffset()
{
	int index = GetCATMotionRoot()->GetLayerIndex();
	if (index < 0) return 0.0f;

	Control* catmotionhub = GetLimb()->GetHub()->GetLayerTrans()->GetLayer(index);
	DbgAssert(catmotionhub);
	// Get the path offset from CATMotionHub
	if (catmotionhub->ClassID() == GetCATMotionHub2Desc()->ClassID())
		return ((CATMotionHub2*)catmotionhub)->pathoffset;
	return 0.0f;
}

void CATMotionLimb::UpdateHub()
{
	LimbData2* pLimb = GetLimb();
	if (pLimb != NULL)
	{
		Hub* pHub = pLimb->GetHub();
		if (pHub != NULL)
		{
			pHub->Update();
		}
	}
}

//RefTargetHandle Clone( RemapDir &remap );
RefResult CATMotionLimb::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate) {
	UNREFERENCED_PARAMETER(partID);
	UNREFERENCED_PARAMETER(changeInt);

	switch (message)
	{
	case REFMSG_CHANGE:
	{
		// Check to see what has changed, and send appropriate message
		ParamID changingParam;
		if (hTarget == pblock)
		{
			changingParam = pblock->LastNotifyParamID();
			switch (changingParam) {
			case PB_PHASEOFFSET:
				CATMotionMessage(0, CATMOTION_FOOTSTEPS_CHANGED, -1);
				if (GetLimb()) GetLimb()->UpdateLimb();
				break;

			case PB_LIFTPLANTMOD:
				if (GetLimb()) GetLimb()->UpdateLimb();
				break;
			}
		}
		break;
	}
	case REFMSG_TARGET_DELETED:
		if (hTarget == pblock)
			pblock = NULL;
		break;
	}
	return REF_SUCCEED;

};

void CATMotionLimb::CATMotionMessage(TimeValue t, UINT msg, int data)
{
	for (int i = 0; i < GetNumCATMotionPlatforms(); i++) {
		CATMotionPlatform *platform = GetCATMotionPlatform(i);
		if (platform) platform->CATMotionMessage(t, msg, data);
	}
}

void CATMotionLimb::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	for (int i = 0; i < GetNumCATMotionPlatforms(); i++) {
		CATMotionPlatform *platform = GetCATMotionPlatform(i);
		if (platform) platform->AddSystemNodes(nodes, ctxt);
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATMotionLimbPLCB : public PostLoadCallback {
protected:
	CATMotionLimb *ctrl;

public:
	CATMotionLimbPLCB(CATMotionLimb *pOwner) { ctrl = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(ctrl);
		ICATParentTrans	*catparenttrans = ctrl->GetCATParentTrans();
		if (catparenttrans) return catparenttrans->GetFileSaveVersion();
		ECATParent	*catparent = (ECATParent*)ctrl->pblock->GetReferenceTarget(CATMotionLimb::PB_CATPARENT);;
		if (catparent) return catparent->GetFileSaveVersion();
		return 0;
	}

	int Priority() { return 5; }

	void proc(ILoad*) {

		if (GetFileSaveVersion() < CAT_VERSION_2430) {
			if (ctrl->GetLimb() && ctrl->GetLimb()->GetPalmTrans()) {
				CATHierarchyBranch2 *branchlimb = ctrl->GetCATHierarchyLimb();
				if (branchlimb) {
					CATHierarchyBranch2 *palmbranch = NULL;
					for (int i = 0; i < branchlimb->GetNumBranches(); i++) {
						if (branchlimb->GetBranch(i)->GetExpandable()) {
							palmbranch = (CATHierarchyBranch2*)branchlimb->GetBranch(i);
							if (ctrl->GetLimb()->GetCollarbone()) {
								palmbranch = (CATHierarchyBranch2*)branchlimb->GetBranch(palmbranch->GetBranchIndex() + 1);
							}
							break;
						}
					}
					if (palmbranch) {
						if (ctrl->GetLimb()->GetisArm())
							palmbranch->SetBranchName(GetString(IDS_PALM));
						else palmbranch->SetBranchName(GetString(IDS_ANKLE));
					}
				}
			}
		}

		if (GetFileSaveVersion() < CAT3_VERSION_2707) {
			DisableRefMsgs();
			// We now have an array of foot objects. This allows us to support things like walking sticks
			CATMotionPlatform *platform = (CATMotionPlatform*)ctrl->pblock->GetControllerByID(CATMotionLimb::PB_CATMOTIONPLATFORM_DEPRECATED);

			ctrl->pblock->SetCount(CATMotionLimb::PB_CATMOTION_STEPPING_NODES, 1);
			ctrl->pblock->SetValue(CATMotionLimb::PB_CATMOTION_STEPPING_NODES, 0, platform, 0);
			EnableRefMsgs();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

IOResult CATMotionLimb::Save(ISave*)
{
	return IO_OK;
}

IOResult CATMotionLimb::Load(ILoad *iload)
{

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new CATMotionLimbPLCB(this));

	return IO_OK;
}

