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
#include "FootTrans2.h"
#include "ICATParent.h"
#include "CATMotionLimb.h"
#include "CATMotionPlatform.h"
#include "CATHierarchyRoot.h"
#include "CATHierarchyBranch2.h"

class CATMotionPlatformClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATMotionPlatform(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONPLATFORM); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONPLATFORM_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionPlatform"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionPlatformClassDesc CATMotionPlatformDesc;
ClassDesc2* GetCATMotionPlatformDesc() { return &CATMotionPlatformDesc; }

static ParamBlockDesc2 weightshift_param_blk(CATMotionPlatform::PBLOCK_REF, _T("CATMotionPlatform params"), 0, &CATMotionPlatformDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, CATMotionPlatform::PBLOCK_REF,

	IDD_CAT_FOOT_MOT, IDS_CAT_FOOT_MOT, 0, 0, NULL,

	CATMotionPlatform::PB_FOOTTRANS, _T("FootTrans"), TYPE_REFTARG, P_NO_REF, IDS_CL_FOOTTRANS2,
		p_end,
	CATMotionPlatform::PB_CATMOTIONLIMB, _T("CATMotionLimb"), TYPE_REFTARG, P_NO_REF, IDS_CL_LIMBDATA,
		p_end,
	CATMotionPlatform::PB_P3CATOFFSETPOS, _T("OffsetPos"), TYPE_POINT3, P_ANIMATABLE, IDS_OFFSETPOS,
		p_end,
	CATMotionPlatform::PB_P3CATMOTIONPOS, _T("CATMotionPos"), TYPE_POINT3, P_ANIMATABLE, IDS_CATMOTIONPOS,
		p_end,
	CATMotionPlatform::PB_P3CATOFFSETROT, _T("OffsetRot"), TYPE_POINT3, P_ANIMATABLE, IDS_OFFSETROT,
		p_end,
	CATMotionPlatform::PB_FOLLOW_PATH, _T("PathFollow"), TYPE_FLOAT, P_ANIMATABLE, IDS_PATHFOLLOW_W,
		p_default, 1.0f,
		p_range, 0.0f, 1.0f,
		p_ui, TYPE_SLIDER, EDITTYPE_FLOAT, IDC_EDT_FOLLOWPATH, IDC_SLD_FOLLOWPATH, 4,
		p_end,
	CATMotionPlatform::PB_PIVOTPOS, _T("PivotPos"), TYPE_POINT3, P_ANIMATABLE, IDS_PIVOTPOS,
		p_end,
	CATMotionPlatform::PB_PIVOTROT, _T("PivotRot"), TYPE_POINT3, P_ANIMATABLE, IDS_PIVOTROT,
		p_end,
	CATMotionPlatform::PB_STEPSHAPE, _T("StepShape"), TYPE_FLOAT, P_ANIMATABLE, IDS_CL_STEPSHAPE,
		p_end,
	CATMotionPlatform::PB_STEPMASK, _T(""), TYPE_FLOAT, P_ANIMATABLE, IDS_MASK,
		p_end,
	CATMotionPlatform::PB_CAT_PRINT_TM, _T(""), TYPE_MATRIX3_TAB, 0, 0, IDS_FOOTPRINTS_TM,
		p_default, Matrix3(1),
		p_end,
	CATMotionPlatform::PB_CAT_PRINT_INV_TM, _T(""), TYPE_MATRIX3_TAB, 0, 0, IDS_FOOTPRINTS_INV_TM,
		p_default, Matrix3(1),
		p_end,
	CATMotionPlatform::PB_PRINT_NODES, _T("FootPrintNodes"), TYPE_INODE_TAB, 0, 0, IDS_FOOTPRINTS,
		p_end,
	p_end
);

CATMotionPlatform::CATMotionPlatform() {
	ip = NULL;
	flagsBegin = 0;
	calculating_footprints = FALSE;
	pblock = NULL;
	CATMotionPlatformDesc.MakeAutoParamBlocks(this);
}

CATMotionPlatform::~CATMotionPlatform() {
	DeleteAllRefs();
}

void CATMotionPlatform::Initialise(int index, CATNodeControl* iktarget, CATHierarchyBranch2* CATHierarchyLimb, CATMotionLimb* catmotionlimb)
{
	TSTR name = GetString(IDS_FOOTPLATFORM);
	CATHierarchyBranch2 *PlatformCATHierarchy = (CATHierarchyBranch2 *)CATHierarchyLimb->AddBranch(name);
	int lengthaxis = catmotionlimb->GetCATParentTrans()->GetLengthAxis();

	// StepShape controls the rate of movement through a step
	Control *ctrlStepShape = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, STEPSHAPE_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(ctrlStepShape, GetString(IDS_STEPSHAPE), catmotionlimb);
	pblock->SetControllerByID(PB_STEPSHAPE, 0, (Control*)ctrlStepShape, FALSE);

	// Offsets //////////////////
	Control *offsetpos = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
	Control *offsetrot = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);

	// Offset Pos
	PlatformCATHierarchy->AddAttribute(offsetpos, GetString(IDS_OFFSETPOS), catmotionlimb);
	// Offset Rot
	PlatformCATHierarchy->AddAttribute(offsetrot, GetString(IDS_OFFSETROT), catmotionlimb);

	int nNumDefaults = 9;

	// FootSwerve gives iktarget a circular path, ie around other iktarget
	float dSwerveDefaultVals[] = { 0.0f, 0.2f, 50.0f, 0.0f, 0.25f, 0.25f, 0.0f, 0.25f, 2.0f };
	Control *xPos = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTLIFT_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(xPos, GetString(IDS_FOOTSWERVE), catmotionlimb, nNumDefaults, dSwerveDefaultVals);

	// FootPush gives the iktarget extra forward movement
	Control *yPos = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTLIFT_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(yPos, GetString(IDS_FOOTPUSH), catmotionlimb);

	// FootLift gives the iktarget height thru a step
	float dLiftDefaultVals[] = { 0.0f, 0.2f, 50.0f, 0.0f, 0.25f, 0.25f, 0.0f, 0.25f, 10.0f };
	Control *zPos = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, FOOTLIFT_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(zPos, GetString(IDS_FOOTLIFT), catmotionlimb, nNumDefaults, dLiftDefaultVals);

	// Create a P3 for it all to go on, and store it in pblock
	Control *catmotionpos = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
	if (lengthaxis == Z) {
		catmotionpos->AssignController(xPos, X);
		catmotionpos->AssignController(yPos, Y);
		catmotionpos->AssignController(zPos, Z);
	}
	else {
		catmotionpos->AssignController(zPos, X);
		catmotionpos->AssignController(yPos, Y);
		catmotionpos->AssignController(xPos, Z);
	}

	float dPitchDefaultVals[] = { 7.0f, 0.0f, 0.2f, 20.0f, 30.0f, 0.0f, 0.333f, 0.333f, -10.0f, 70.0f, 0.0f, 0.333f, 0.333f, 80.0f, 0.0f, 0.333f, };
	int nNumPitchDefaults = 16;
	CATGraph *pitch = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTROT_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(pitch, GetString(IDS_FOOTPITCH), catmotionlimb, nNumPitchDefaults, dPitchDefaultVals);

	// Pivot pos Y is tied to PivotRotX
	CATGraph *pivotpospitch = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTPOS_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(pivotpospitch, GetString(IDS_PIVOTPOSPITCH), catmotionlimb);

	// Foots Roll (Y axis)
	CATGraph *pivotrotY = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTROT_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(pivotrotY, GetString(IDS_FOOTROLL), catmotionlimb);

	// Pivot pos X is tied to PivotRotY

	CATGraph *pivotposroll = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTPOS_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(pivotposroll, GetString(IDS_PIVOTPOSROLL), catmotionlimb);

	// Foots Twist (Z axis)
	CATGraph *pivotrotZ = (CATGraph*)CreateInstance(CTRL_FLOAT_CLASS_ID, PIVOTROT_CLASS_ID);
	PlatformCATHierarchy->AddAttribute(pivotrotZ, GetString(IDS_TWIST), catmotionlimb);

	Control *pivotrot = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);

	if (lengthaxis == Z) {
		pivotrot->AssignController(pitch, X);
		pivotrot->AssignController(pivotrotY, Y);
		pivotrot->AssignController(pivotrotZ, Z);
	}
	else {
		pitch->FlipValues();
		pivotrot->AssignController(pivotrotZ, X);
		pivotrot->AssignController(pitch, Z);
		pivotrot->AssignController(pivotrotY, Y);
	}

	// Create a P3 for it all to go on, and store it in pblock
	Control *pivotpos = (Control*)CreateInstance(CTRL_POINT3_CLASS_ID, IPOINT3_CONTROL_CLASS_ID);
	if (lengthaxis == Z) {
		pivotpos->AssignController(pivotposroll, X);
		pivotpos->AssignController(pivotpospitch, Y);
	}
	else {
		pivotpos->AssignController(pivotposroll, Z);
		pivotpos->AssignController(pivotpospitch, Y);
	}

	pblock->SetValue(PB_FOOTTRANS, 0, iktarget);
	pblock->SetValue(PB_CATMOTIONLIMB, 0, catmotionlimb);
	pblock->SetControllerByID(PB_P3CATOFFSETPOS, 0, offsetpos, FALSE);
	pblock->SetControllerByID(PB_P3CATOFFSETROT, 0, offsetrot, FALSE);
	pblock->SetControllerByID(PB_P3CATMOTIONPOS, 0, catmotionpos, FALSE);

	pblock->SetControllerByID(PB_PIVOTPOS, 0, pivotpos, FALSE);
	pblock->SetControllerByID(PB_PIVOTROT, 0, pivotrot, FALSE);

	Control* stepmask = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(LININTERP_FLOAT_CLASS_ID, 0));
	pblock->SetControllerByID(PB_STEPMASK, 0, stepmask, FALSE);

}

RefTargetHandle CATMotionPlatform::Clone(RemapDir& remap)
{
	// make a new CATMotionPlatform object to be the clone
	CATMotionPlatform *newCATMotionPlatform = new CATMotionPlatform();
	remap.AddEntry(this, newCATMotionPlatform);

	newCATMotionPlatform->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	for (int i = 0; i < pblock->Count(PB_PRINT_NODES); i++) {
		INode* footprint = pblock->GetINode(PB_PRINT_NODES, 0, i);

		if (!footprint || remap.FindMapping(footprint)) continue;

		Object *tempobj = (Object*)CreateInstance(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));
		INode *node = GetCOREInterface()->CreateObjectNode(tempobj);
		remap.AddEntry(footprint, node);

		node->SetName(footprint->GetName());
		node->SetWireColor(footprint->GetWireColor());

		Control *ctrl = (Control*)remap.CloneRef(footprint->GetTMController());
		node->SetTMController(ctrl);

		remap.AddEntry(footprint->GetTMController(), ctrl);
		if (remap.FindMapping(footprint->GetObjectRef())) {
			node->SetObjectRef((Object*)remap.FindMapping(footprint->GetObjectRef()));
		}
		else {
			node->SetObjectRef(footprint->GetObjectRef());
		}

		// Reparent this object onto the remapped parent
		if (!footprint->GetParentNode()->IsRootNode()) {
			INode* parentnode = (INode*)remap.FindMapping(footprint->GetParentNode());
			if (parentnode) {
				parentnode->AttachChild(node, FALSE);
			}
			else {
				footprint->GetParentNode()->AttachChild(node, FALSE);
			}
		}
		newCATMotionPlatform->pblock->SetValue(PB_PRINT_NODES, 0, node, i);
		node->InvalidateTM();
	}

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATMotionPlatform, remap);

	// now return the new object.
	return newCATMotionPlatform;
}

void CATMotionPlatform::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		CATMotionPlatform *newctrl = (CATMotionPlatform*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

//-------------------------------------------------------------
void CATMotionPlatform::GetValue(TimeValue t, void* val, Interval&valid, GetSetMethod)
{
	valid.SetInstant(t);
	CATMotionLimb* catmotionlimb = GetCATMotionLimb();
	DbgAssert(catmotionlimb);
	CATHierarchyRoot* root = catmotionlimb->GetCATMotionRoot();
	ICATParentTrans* catparenttrans = catmotionlimb->GetCATParentTrans();
	CATNodeControl* iktarget = GetIKTargetControl();
	if (!catmotionlimb || !iktarget) return;

	int walkmode = root->GetWalkMode();
	float dCATUnits = catparenttrans->GetCATUnits();
	int lengthaxis = catparenttrans->GetLengthAxis();
	float lmr = (float)catmotionlimb->GetLMR();
	float direction = DegToRad(root->GetDirection(t));

	Matrix3 tmSetup = iktarget->GetSetupMatrix();
	Point3 p3SetupPos = tmSetup.GetTrans() * dCATUnits; // TODO: Check scale here.
	tmSetup.NoTrans();

	p3SetupPos[lengthaxis] = 0.0f;

	// Now rotate the matrix so it sits flat on the ground.
	// The rig may have been built in a star shape or fetal position
	AngAxis ax;
	if (lengthaxis == Z) {
		ax.angle = acos(min(1.0f, DotProd(Point3(0.0f, 0.0f, 1.0f), tmSetup.GetRow(Z))));
		if (ax.angle > 0.0001f) {
			ax.axis = Normalize(CrossProd(tmSetup.GetRow(lengthaxis), Point3(0.0f, 0.0f, 1.0f)));
			RotateMatrix(tmSetup, ax);
		}
	}
	else {
		ax.angle = acos(min(1.0f, DotProd(Point3(1.0f, 0.0f, 0.0f), tmSetup.GetRow(X))));
		if (ax.angle > 0.0001f) {
			ax.axis = Normalize(CrossProd(tmSetup.GetRow(lengthaxis), Point3(1.0f, 0.0f, 0.0f)));
			RotateMatrix(tmSetup, ax);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// this is used mainly on footprints,
	// but I put is here so I can use it on WalkOnSpot
	int footTime;
	int footT = GetStepGraphTime(t);

	float dFollowPath = 1.0f;

	switch (walkmode) {
	case CATHierarchyRoot::WALK_ON_SPOT:
	{
		catmotionlimb->GetStepTime(t, 1.0f, footTime);
		float dist = ((float)(t - footT) / (float)root->GetMaxStepTime(t)) * root->GetMaxStepDist(t);
		root->GettmPath(footT, (*(Matrix3*)val), valid, p3SetupPos.y, TRUE);

		// Add the Stepping Motion
		Point3 p3StepOffset(dist * sin(direction), -dist * cos(direction), 0.0f);
		if (lengthaxis == Z)
			p3StepOffset.x = -p3StepOffset.x;
		ModVec(p3StepOffset, lengthaxis);
		(*(Matrix3*)val).PreTranslate(p3StepOffset);

		// Add the general offset
		p3SetupPos.y = 0;
		(*(Matrix3*)val).PreTranslate(p3SetupPos);
		break;
	}
	case CATHierarchyRoot::WALK_ON_LINE:
	{
		root->GettmPath(footT, *(Matrix3*)val, valid, p3SetupPos.y, TRUE);
		p3SetupPos.y = 0;
		(*(Matrix3*)val).PreTranslate(p3SetupPos);
		break;
	}
	case CATHierarchyRoot::WALK_ON_PATHNODE:
	{
		if (!calculating_footprints)
		{
			dFollowPath = pblock->GetFloat(PB_FOLLOW_PATH);

			// Here we use footprint Node TMs if at all possible
			// This means that we have animated footprints! Yay!
			if (dFollowPath < 1.0f)
			{
				int nStepNumber = catmotionlimb->GetStepNum(t);
				float ratio = GetStepGraphRatio(t);

				*(Matrix3*)val = GetPrintTM(nStepNumber, t);
				Matrix3 tmNextStep = GetPrintTM(nStepNumber + 1, t);
				BlendMat(*(Matrix3*)val, tmNextStep, ratio);
			}
		}

		if (dFollowPath > 0.0f)
		{
			Matrix3 tmFollow;
			root->GettmPath(footT, tmFollow, valid, p3SetupPos.y, TRUE);
			p3SetupPos[Y] = 0;
			tmFollow.PreTranslate(p3SetupPos);
			BlendMat(*(Matrix3*)val, tmFollow, dFollowPath);
		}
	}
	}
	//////////////////////////////////////////////////////////////////////////
	// Offset Pos
	// lift + swerve
	Point3 p3CATPos = pblock->GetPoint3(PB_P3CATMOTIONPOS, t);

	// This will be cleaned up one day.
	if (dFollowPath > 0.0f)
		p3CATPos += pblock->GetPoint3(PB_P3CATOFFSETPOS, t) * dFollowPath;

	p3CATPos *= dCATUnits; // TODO: Check scale here
	if (lengthaxis == Z)	p3CATPos[X] *= lmr;	// Feet move apart, ie opposite directions!
	else				p3CATPos[Z] *= -lmr;
	(*(Matrix3*)val).PreTranslate(p3CATPos);

	if (dFollowPath > 0.0f)
	{
		Point3 p3CATRot = pblock->GetPoint3(PB_P3CATOFFSETROT, t) * dFollowPath;
		if (lengthaxis == Z) {
			p3CATRot[Y] *= lmr;
			p3CATRot[Z] *= lmr;
		}
		else {
			p3CATRot[Y] *= lmr;
			p3CATRot[X] *= lmr;
		}

		Quat qtCattRot;
		float CATRotEuler[] = { DegToRad(p3CATRot.x), DegToRad(p3CATRot.y), DegToRad(p3CATRot.z) };
		EulerToQuat(&CATRotEuler[0], qtCattRot);
		PreRotateMatrix(*(Matrix3*)val, qtCattRot);

		// New additionzF
		// keep setup rotations even when not in setup mone
		// not a bad idea. But NOT in InterpByPrint mode
		BlendMat(tmSetup, Matrix3(1), 1.0f - dFollowPath);
		*(Matrix3*)val = tmSetup * *(Matrix3*)val;

		// A footprints motion is relative to the FINAL CATMotion transform
		// It must be evaluated last to take into account setup ect
		// However, it must NOT be included in the initial
		// calculation of a foots position, thats the base its calculated from.
		if (!calculating_footprints && walkmode == CATHierarchyRoot::WALK_ON_PATHNODE)
		{
			Matrix3 tmFollowFoot = *(Matrix3*)val;
			GetFootPrintMot(t, tmFollowFoot);
			BlendMat(*(Matrix3*)val, tmFollowFoot, dFollowPath);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Now do the pivot positions and  rotations

	Point3 pivotPos = pblock->GetPoint3(PB_PIVOTPOS, t);
	Point3 pivotRot = pblock->GetPoint3(PB_PIVOTROT, t);

	if (lengthaxis == Z) {
		pivotPos -= Point3(0.5f, 0.5f, 0.0f);
		pivotPos *= Point3(iktarget->GetObjX() * lmr, iktarget->GetObjY(), 0.0f) * dCATUnits;
		pivotRot[Y] *= lmr;
		pivotRot[Z] *= lmr;

		(*(Matrix3*)val).PreTranslate(pivotPos);
		(*(Matrix3*)val).PreRotateX(-DegToRad(pivotRot.x));
		(*(Matrix3*)val).PreRotateY(DegToRad(pivotRot.y));
		(*(Matrix3*)val).PreRotateZ(DegToRad(pivotRot.z));

		m_ptm = (*(Matrix3*)val);
		(*(Matrix3*)val).PreTranslate(-(pivotPos));
	}
	else {
		pivotPos -= Point3(0.0f, 0.5f, 0.5f);
		pivotPos *= Point3(0.0f, iktarget->GetObjY(), iktarget->GetObjX() * lmr) * dCATUnits;
		pivotRot[Y] *= lmr;
		pivotRot[X] *= lmr;

		(*(Matrix3*)val).PreTranslate(pivotPos);
		(*(Matrix3*)val).PreRotateZ(-DegToRad(pivotRot.z));
		(*(Matrix3*)val).PreRotateY(DegToRad(pivotRot.y));
		(*(Matrix3*)val).PreRotateX(DegToRad(pivotRot.x));

		m_ptm = (*(Matrix3*)val);
		(*(Matrix3*)val).PreTranslate(-(pivotPos));
	}

	//////////////////////////////////////////////////////////////////////////

}

void CATMotionPlatform::PutPrintTMtoPblock(int printid, TimeValue t) {
	INode* footprint = pblock->GetINode(PB_PRINT_NODES, 0, printid);
	if (footprint) {
		pblock->EnableNotifications(FALSE);
		theHold.Suspend();
		Matrix3 tm = footprint->GetNodeTM(t);
		pblock->SetValue(PB_CAT_PRINT_TM, t, tm, printid);
		theHold.Resume();
		UpdateStepMasks();
		pblock->EnableNotifications(TRUE);
	}
}

class FootPrintMoveRestore : public RestoreObj {
public:
	CATMotionPlatform	*catmotionplatform;
	int footprintid;
	TimeValue t;
	FootPrintMoveRestore(CATMotionPlatform *c, int id, TimeValue t) {
		catmotionplatform = c;
		footprintid = id;
		this->t = t;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			catmotionplatform->PutPrintTMtoPblock(footprintid, t);
		}
	}
	void Redo() {
		catmotionplatform->PutPrintTMtoPblock(footprintid, t);
	}
	void EndHold() {
		if (footprintid < catmotionplatform->pblock->Count(CATMotionPlatform::PB_PRINT_NODES)) {
			if (catmotionplatform->pblock->GetINode(CATMotionPlatform::PB_PRINT_NODES, 0, footprintid)) {
				catmotionplatform->pblock->GetINode(CATMotionPlatform::PB_PRINT_NODES, 0, footprintid)->ClearAFlag(A_PLUGIN3);
				catmotionplatform->PutPrintTMtoPblock(footprintid, t);
			}
		}
	}

	int Size() { return 1; }
};

RefResult CATMotionPlatform::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	UNREFERENCED_PARAMETER(partID); UNREFERENCED_PARAMETER(changeInt);
	switch (message)
	{
	case REFMSG_CHANGE:
		if (pblock == hTarget)
		{
			int tabIndex = 0;
			ParamID index = pblock->LastNotifyParamID(tabIndex);
			switch (index) {
			case PB_P3CATOFFSETPOS:
			case PB_P3CATOFFSETROT: {
				CalcAllPrintPos();
				break;
			}
			case PB_PRINT_NODES: {
				// messages get sent to us when the tables get resized.
				if (tabIndex < 0 || tabIndex >= pblock->Count(PB_PRINT_NODES)) break;

				INode* footprint = pblock->GetINode(PB_PRINT_NODES, 0, tabIndex);
				if (!footprint) break;

				int isUndoing = 0;
				if (!theHold.RestoreOrRedoing() || (theHold.Restoring(isUndoing) && isUndoing) || theHold.Redoing()) {
					if (!theHold.RestoreOrRedoing()) {
						//	PutPrintTMtoPblock(tabIndex, GetCOREInterface()->GetTime());
						if (theHold.Holding()) {
							INode* footprint = pblock->GetINode(PB_PRINT_NODES, 0, tabIndex);
							if (footprint && !footprint->TestAFlag(A_PLUGIN3)) {
								theHold.Put(new FootPrintMoveRestore(this, tabIndex, GetCOREInterface()->GetTime()));
								footprint->SetAFlag(A_PLUGIN3);
							}
						}
					}
					GetCATMotionLimb()->UpdateHub();
				}
				break;
			}
			}
		}
		break;
	}

	return REF_SUCCEED;
}

void CATMotionPlatform::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsBegin = flags;
	this->ip = ip;

	if (flagsBegin&BEGIN_EDIT_MOTION)
	{
		CATMotionPlatformDesc.BeginEditParams(ip, this, flags, prev);
	}
}
void CATMotionPlatform::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (flagsBegin&BEGIN_EDIT_MOTION)
	{
		CATMotionPlatformDesc.EndEditParams(ip, this, flags, next);
	}

	this->ip = NULL;
}

int CATMotionPlatform::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(inode); UNREFERENCED_PARAMETER(t);
	if (!this->ip) return 0;

	Interval iv;
	GraphicsWindow *gw = vpt->getGW();	// This line is here because I don't know how to initialize
										// a *gw. I will change it in the next line
	gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world

	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	DbgAssert(catmotionlimb);

	Box3 bbox;
	bbox.Init();

	////////
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

	// 1 CATUnit is approx 0.5 cm(orriginally it was supposed to be 1 cm ).
	// Here we want to draw a cross size of 5 cm.
	float length = catmotionlimb->GetCATMotionRoot()->GetCATParentTrans()->GetCATUnits() * 5.0f;
	Point3 p1, p2;

	Matrix3 tmX = m_ptm;
	tmX.PreTranslate(Point3(-length / 2.0f, 0.0f, 0.0f)); p1 = tmX.GetTrans();
	tmX.PreTranslate(Point3(length, 0.0f, 0.0f));		p2 = tmX.GetTrans();
	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

	Matrix3 tmY = m_ptm;
	tmY.PreTranslate(Point3(0.0f, -length / 2.0f, 0.0f)); p1 = tmY.GetTrans();
	tmY.PreTranslate(Point3(0.0f, length, 0.0f));		p2 = tmY.GetTrans();
	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

	Matrix3 tmZ = m_ptm;
	tmZ.PreTranslate(Point3(0.0f, 0.0f, -length / 2.0f)); p1 = tmZ.GetTrans();
	tmZ.PreTranslate(Point3(0.0f, 0.0f, length));		p2 = tmZ.GetTrans();
	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

	//	UpdateWindow(hWnd);
	/////////

	return 1;
}

// Update the Foot step masks
Matrix3 CATMotionPlatform::GetPrintTM(int i, TimeValue t)//, BOOL *bIsAnimated/*=NULL*/)
{
	// If we have a footprint node in the scene, then use it.
	if (i >= 0 && i < pblock->Count(PB_PRINT_NODES)) {
		INode* footprintnode = pblock->GetINode(PB_PRINT_NODES, 0, i);
		if (footprintnode) {
			if (footprintnode->IsHidden()) {
				footprintnode->EvalWorldState(t, TRUE);
			}
			return footprintnode->GetObjTMAfterWSM(t);
		}
	}
	// or just use the cached position
	if (i >= 0 && i < pblock->Count(PB_CAT_PRINT_TM))
		return pblock->GetMatrix3(PB_CAT_PRINT_TM, 0, i);

	return Matrix3(1);
}

// Update the Foot step masks
void CATMotionPlatform::UpdateStepMasks()
{
	Control* ctrlStepMask = pblock->GetControllerByID(PB_STEPMASK);
	if (!ctrlStepMask) {
		ctrlStepMask = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(LININTERP_FLOAT_CLASS_ID, 0));
		pblock->SetControllerByID(PB_STEPMASK, 0, ctrlStepMask, FALSE);
	}

	ctrlStepMask->DeleteKeys(TRACK_DOALL + TRACK_RIGHTTOLEFT);

	int nNumPrints = pblock->Count(PB_CAT_PRINT_TM);
	if (nNumPrints == 0) return;

	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	DbgAssert(catmotionlimb);
	CATHierarchyRoot* root = catmotionlimb->GetCATMotionRoot();

	float dCATUnits = root->GetCATParentTrans()->GetCATUnits();
	float delta, on = 1.0f, off = 0.0f;
	TimeValue t;
	Interval ivStep = root->GetCATMotionRange();
	Matrix3 tmPrint1, tmPrint2;
	float dCurrStepMask = 1.0f;
	ivStep = catmotionlimb->GetStepTimeRange(ivStep.Start());

	BOOL b1stKey = TRUE;
	// make sure keys are created
	DisableRefMsgs();
	SuspendAnimate();
	AnimateOn();

	// what ever happened to print 0?
	int nPrintID = 1;
	tmPrint1 = GetPrintTM(nPrintID, ivStep.Start());
	for (nPrintID = 2; nPrintID < nNumPrints - 1; nPrintID++)
	{
		ivStep = catmotionlimb->GetStepTimeRange(ivStep.End() + 1);
		t = (nPrintID - 1) * STEPTIME100;
		tmPrint2 = GetPrintTM(nPrintID, ivStep.Start());
		delta = Length(tmPrint1.GetTrans() - tmPrint2.GetTrans());

		if (delta < dCATUnits)
		{
			if (b1stKey)
			{
				// just set the value
				ctrlStepMask->SetValue(0, (void*)&off);
				ctrlStepMask->DeleteKeyAtTime(0);
				b1stKey = FALSE;
			}
			else if (dCurrStepMask == 1.0f)
			{
				// we should have turned off last step
				ctrlStepMask->SetValue((t - STEPTIME100) - STEPTIME75, (void*)&on);
				ctrlStepMask->SetValue((t - STEPTIME100) - STEPTIME25, (void*)&off);
			}
			dCurrStepMask = 0.0f;
		}
		else
		{
			if (b1stKey)
			{
				// just set the value
				ctrlStepMask->SetValue(0, (void*)&on);
				ctrlStepMask->DeleteKeyAtTime(0);
				b1stKey = FALSE;
			}
			else if (dCurrStepMask == 0.0f)
			{
				// we are turning on
				ctrlStepMask->SetValue(t - STEPTIME75, (void*)&off);
				ctrlStepMask->SetValue(t - STEPTIME25, (void*)&on);
			}
			dCurrStepMask = 1.0f;
		}
		tmPrint1 = tmPrint2;
	}

	// make sure keys are created
	AnimateOff();
	ResumeAnimate();
	EnableRefMsgs();
}

/////////////////////////////////////////////////////////////////////////
//
//	This function calculates all the points where the iktarget will land on the
//	ground, and saves out these tm's into an array for the CATMotionPlatform
//	to use later in footprint interpolation.
//	Pass in TRUE to cause footprint nodes to be created as well
//
BOOL CATMotionPlatform::CalcAllPrintPos(BOOL bCreateNodes/*=FALSE*/, BOOL bKeepCurrentOffsets/*=TRUE*/, BOOL bOnlySelected/*=FALSE*/)
{
	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	DbgAssert(catmotionlimb != NULL);
	if (catmotionlimb == NULL)
		return FALSE;

	CATHierarchyRoot* root = catmotionlimb->GetCATMotionRoot();
	DbgAssert(root != NULL);
	if (root == NULL)
		return FALSE;

	CATNodeControl *iktarget = GetIKTargetControl();
	DbgAssert(iktarget != NULL);
	if (iktarget == NULL)
		return FALSE;

	// Use the IKTarget object
	// as an object for every print node.
	// If this pointer is NULL, back out of this function.
	// It is likely the foot has been deleted.
	Object *helper = iktarget->GetObject();
	if (helper == NULL)
		return FALSE;

	Interface* ip = GetCOREInterface();
	Interval animRange = root->GetStepEaseExtents();

	TimeValue ticksInStepEase = (int)root->GetStepEaseValue(animRange.End());	// we can assume First key has time = val = 0;
	int numSteps = (int)floor((float)ticksInStepEase / STEPTIME100) + 2;					// We need print on time 0 too,

	// If we are WALK ON SPOT mode, then delete all footprints.
	if (root->GetWalkMode() != CATHierarchyRoot::WALK_ON_PATHNODE)
		numSteps = 0;
	else if (numSteps < 3)
		return FALSE;

	int oldNumPrints = pblock->Count(PB_CAT_PRINT_TM);
	INode *footprintnode = NULL;

	// Kill those annoying notifications!!!
	calculating_footprints = TRUE;

	// Lock the following to prevent being spammed with messages.
	{
		MaxReferenceMsgLock lockThis;
		// Ensure correct number of footprints. If Character slows
		// down, will take more steps etc...
		// Too many footprints
		if (numSteps < pblock->Count(PB_PRINT_NODES))
		{
			for (int i = numSteps; i < pblock->Count(PB_PRINT_NODES); i++)
			{
				INode *deadPrint = pblock->GetINode(PB_PRINT_NODES, 0, i);
				if (deadPrint)
					deadPrint->Delete(ip->GetTime(), TRUE);
			}
			pblock->SetCount(PB_PRINT_NODES, numSteps);
		}
		if (numSteps != oldNumPrints)
		{
			pblock->SetCount(PB_CAT_PRINT_INV_TM, numSteps);
			pblock->SetCount(PB_CAT_PRINT_TM, numSteps);
			pblock->SetCount(PB_PRINT_NODES, numSteps);
		}

		if (numSteps == 0)
		{
			calculating_footprints = false;
			return TRUE;
		}

		if (bCreateNodes)
		{
			pblock->SetCount(PB_PRINT_NODES, numSteps);

			Color footprintColour = catmotionlimb->GetLimbColour();
			TSTR footName = catmotionlimb->GetLimbName();
			footName = footName + GetString(IDS_PRINT);

			for (int i = 0; i < numSteps; i++) {
				// Create standard PointHelpers for needed footprints
				if (!pblock->GetINode(PB_PRINT_NODES, 0, i)) {	// If there is no node (just created or deleted)
					// Create node to control object.
					footprintnode = ip->CreateObjectNode(helper);
					DbgAssert(footprintnode);
					// Create reference to node.
					pblock->SetValue(PB_PRINT_NODES, 0, footprintnode, i);
					TSTR printName;
					if (numSteps <= 100)	printName.printf(_T("%s%2d"), footName.data(), i);	// We may have more than 100 footprints
					else				printName.printf(_T("%s%3d"), footName.data(), i); // I dont care if we have more than a thousand
					footprintnode->SetName((MCHAR *)printName.data());
					footprintnode->SetWireColor(asRGB(footprintColour));
				}
			}
		}
		// We have two arrays, each the correct size
		// We fill one with references to our point helpers
		// and the other with their initial WorldTM, before
		// the user creates offset.

		// GetStepTimeRange
		Interval step_time_range = catmotionlimb->GetStepTimeRange(animRange.Start());
		TimeValue step_time = step_time_range.Start();
		int StepNum = catmotionlimb->GetStepNum(step_time);

		// clean up the unused footprints.
		if (pblock->Count(PB_PRINT_NODES) > 0 && pblock->GetINode(PB_PRINT_NODES, 0, 0) && StepNum == 1)
			pblock->GetINode(PB_PRINT_NODES, 0, 0)->Delete(0, FALSE);

		// Decrement the stepnum so that when the following
		// for loop increments it again it will be back to the
		// starting position. 
		StepNum--;

		int tpf = GetTicksPerFrame();
		Matrix3 tmFootInWorld;
		Matrix3 footPos(1);
		Matrix3 tmOffset(1);

		// clean up our stepmasks
		// TODO : make sure these initialisationbs actually work
		TimeValue t = -STEPTIME100;

		// Prepare to create keys
		SuspendAnimate();
		AnimateOn();

		for (int i = 0; i < numSteps; i++)
		{

			// Problems arise, cause step number at time t is not constant,
			// and can be a possible 3 numbers. Get current StepNum as well
			if ((StepNum + 1) != catmotionlimb->GetStepNum(step_time))
			{
				step_time = catmotionlimb->GetStepTimeRange(step_time).Start() - step_time_range.End();
				step_time = step_time_range.End() + abs(step_time / 2);
			}
			DbgAssert(StepNum + 1 == catmotionlimb->GetStepNum(step_time));
			StepNum = catmotionlimb->GetStepNum(step_time);
			step_time_range = catmotionlimb->GetStepTimeRange(step_time);
			step_time = step_time_range.Start();

			if (StepNum >= numSteps)
			{
				//			DbgAssert(FALSE); // Well, what can we do?
				break;
			}

			Interval ivalid = NEVER;

			// Get initial footprint values, (without any external
			// offset) Save intial value and Invert and save (for click-and-drag
			// footprint offsets.
			calculating_footprints = true;	// turn offset off (CATMotion only)
			GetValue(step_time, (void*)&footPos, ivalid, CTRL_ABSOLUTE);
			calculating_footprints = false;	// turn offset on (RealWorldSpace)

			footPos.NoScale();

			// When positioning the footprints, we want to keep all the old offsets. If the print has been moved,
			// then the offset will be re-applied to the new position just calculated
			if (StepNum < oldNumPrints)
			{
				INode *currPrint = NULL;
				if (StepNum < pblock->Count(PB_PRINT_NODES))
					currPrint = pblock->GetINode(PB_PRINT_NODES, 0, StepNum);
				if (!bKeepCurrentOffsets && (!bOnlySelected || (currPrint && (!bOnlySelected || currPrint->Selected()))))
					tmOffset.IdentityMatrix();
				else
				{
					Matrix3 tmOldPos = pblock->GetMatrix3(PB_CAT_PRINT_TM, 0, StepNum);
					Matrix3 tmOldInvPos = pblock->GetMatrix3(PB_CAT_PRINT_INV_TM, 0, StepNum);
					tmOffset = tmOldPos * tmOldInvPos;
				}
			}

			// we need this inverted matrix stored
			// even if we don't have footprints
			Matrix3 tmInvFootPos = footPos;
			tmInvFootPos.Invert();
			pblock->SetValue(PB_CAT_PRINT_INV_TM, 0, tmInvFootPos, StepNum);

			t = (StepNum - 1) * STEPTIME100;

			// add the offset back onto the new foots position
			footPos = tmOffset * footPos;

			if (StepNum < pblock->Count(PB_PRINT_NODES)) {
				INode *currPrint = pblock->GetINode(PB_PRINT_NODES, 0, StepNum);
				if (currPrint) {
					currPrint->SetNodeTM(0, footPos);
					currPrint->InvalidateTreeTM();
				}
			}

			// put the new iktarget matrix back into the cache
			pblock->SetValue(PB_CAT_PRINT_TM, 0, footPos, StepNum);

			// This puts the interval outside of the current Interval
			// which means next GetStepTimeRange call will return the next SegInterval
			step_time = step_time_range.End() + tpf / 2;
		}
		calculating_footprints = FALSE;

		// This method just rips through the whole mask and rebuilds it from scratch.
		// save us trying to be clever and it doesn't seem to be a speed problem
		UpdateStepMasks();

		AnimateOff();
		ResumeAnimate();
	}

	return TRUE;
}

// Create FootPrints on existing positions
void CATMotionPlatform::CreateFootPrints()
{
	CalcAllPrintPos(TRUE);
}

BOOL CATMotionPlatform::SnapToGround(INode *targ, BOOL bOnlySelected)
{
	if (!(targ)) return FALSE;

	int cnt = pblock->Count(PB_PRINT_NODES);

	// Set up to snap
	int t = GetCOREInterface()->GetTime();
	Object *grndObject = targ->EvalWorldState(t).obj;
	BOOL hasBeenSet = FALSE;
	if (grndObject->CanConvertToType(triObjectClassID))
	{
		TriObject *triGrndObject = (TriObject*)grndObject->ConvertToType(t, triObjectClassID);
		Mesh meshGrndObject = triGrndObject->GetMesh();
		Matrix3 grndTM = targ->GetObjectTM(t);
		Matrix3 invGrndTM = Inverse(grndTM);
		INode *catPrint;

		for (int i = 0; i < cnt; i++)
		{
			catPrint = pblock->GetINode(PB_PRINT_NODES, 0, i);
			if (catPrint && (!bOnlySelected || catPrint->Selected()))
			{
				BOOL isSet = SnapPrintToGround(t, catPrint, &meshGrndObject, invGrndTM, grndTM);
				hasBeenSet = hasBeenSet || isSet;
			}
		}
	}
	return hasBeenSet;
}

// Create & Set footprints to CATMotion positions
void CATMotionPlatform::ResetFootPrints(BOOL bOnlySelected)
{
	Control* ctrlStepMask = pblock->GetControllerByID(PB_STEPMASK);

	Point3 p3Origin = Point3::Origin;
	Quat qOrigin(1);

	INode *currNode;
	SuspendAnimate();
	AnimateOff();
	for (int i = 0; i < pblock->Count(PB_PRINT_NODES); i++)
	{
		currNode = GetPrintNode(i);
		if (currNode && (!bOnlySelected || currNode->Selected()))
		{
			BOOL bReselect = FALSE;
			if (GetCOREInterface()->GetSelNodeCount() == 1 && currNode->Selected()) {
				GetCOREInterface()->ClearNodeSelection();
				bReselect = TRUE;
			}
			currNode->SetTMController(NewDefaultMatrix3Controller());
			if (bReselect) {
				DisableRefMsgs();
				GetCOREInterface()->SelectNode(currNode, FALSE);
				EnableRefMsgs();
			}
		}

		if (!bOnlySelected)
		{
			if (ctrlStepMask)
			{
				float on = 1.0f;
				ctrlStepMask->DeleteKeys(TRACK_DOALL + TRACK_RIGHTTOLEFT);
				ctrlStepMask->SetValue(0, (void*)&on, 0, CTRL_ABSOLUTE);
			}
		}
	}
	ResumeAnimate();

	// If there are footprints in the scene
	// move them back to original positions
	CalcAllPrintPos(FALSE, FALSE, bOnlySelected);
}

// Delete all footprints (CATMotion Only)
void CATMotionPlatform::RemoveFootPrints(BOOL bOnlySelected)
{
	// If we have no footprints anyway, dont run this method
	if (pblock->Count(PB_PRINT_NODES) == 0) return;
	calculating_footprints = TRUE;

	INode *currNode;
	Interface *ip = GetCOREInterface();

	for (int i = pblock->Count(PB_PRINT_NODES) - 1; i >= 0; i--)
	{
		currNode = pblock->GetINode(PB_PRINT_NODES, 0, i);
		if (currNode && (!bOnlySelected || currNode->Selected()))
		{
			//	if(!theHold.Holding()) theHold.Begin();
			ip->DeleteNode(currNode);
		}
	}

	if (!bOnlySelected)
	{
		pblock->SetCount(PB_PRINT_NODES, 0);
		pblock->Resize(PB_PRINT_NODES, 0);
	}
	calculating_footprints = FALSE;
}

BOOL CATMotionPlatform::SnapPrintToGround(TimeValue t, INode *footprint, Mesh *grndMesh, const Matrix3 &invGrndTM, const Matrix3 &grndTM)
{
	Matrix3 tmFootPrint = footprint->GetNodeTM(t);
	/*
	 *	We are intersecting a ray with an OBJECT, which has no
	 *	world TM, so for it to be possible, we have to transform
	 *	the footprint into the the grounds space...
	 */
	Matrix3 tmPrintFromGrnd = tmFootPrint * invGrndTM;
	/*
	 *	To find out if the object we are snapping
	 *	to is ABOVE or BELOW the footprint, find its
	 *	position relative to the footprint
	 */
	 //	Point3 p3GrndFromPrint = Inverse(tmFootPrint) * grndTM.GetTrans();
	 //	int aboveGroundSwitch = (p3GrndFromPrint.z > 0) ? 1 : -1;
	int lengthaxis = GetIKTargetControl()->GetCATParentTrans()->GetLengthAxis();

	Ray printUpRay;
	printUpRay.p = tmPrintFromGrnd.GetTrans(); //invGrndTM * tmFootPrint.GetTrans(); //groundToFootPrint.GetTrans();
	if (lengthaxis == Z)
		printUpRay.dir = /*float(aboveGroundSwitch) */ -tmPrintFromGrnd.GetRow(Z);
	else printUpRay.dir = /*float(aboveGroundSwitch) */ -tmPrintFromGrnd.GetRow(X);

	float distToTarg = 0;
	Point3 surfaceNormal;

	//
	// This is our best guess, based on the footprints position relative to the NodeTM.
	//
	BOOL connected = grndMesh->IntersectRay(printUpRay, distToTarg, surfaceNormal);
	if (connected)
	{
		tmFootPrint.PreTranslate(FloatToVec(-distToTarg, lengthaxis));
		Point3 printPos = tmFootPrint.GetTrans();

		surfaceNormal = VectorTransform(surfaceNormal, grndTM);

		float rotAngle = (float)acos(DotProd(Normalize(surfaceNormal), tmFootPrint.GetRow(lengthaxis)));
		Point3 rotAxis = Normalize(CrossProd(surfaceNormal, tmFootPrint.GetRow(lengthaxis)));
		RotateMatrix(tmFootPrint, AngAxis(rotAxis, rotAngle));	// Rotation is in world space
		tmFootPrint.SetTrans(printPos);

		footprint->SetNodeTM(t, tmFootPrint);
		footprint->InvalidateTreeTM();
	}
	else
	{
		int i;
		int nNumFaces = grndMesh->getNumFaces();
		// flip all the faces in the mesh
		for (i = 0; i < nNumFaces; i++)
			grndMesh->FlipNormal(i);

		printUpRay.dir *= -1;

		BOOL connected = grndMesh->IntersectRay(printUpRay, distToTarg, surfaceNormal);
		if (connected)
		{
			tmFootPrint.PreTranslate(FloatToVec(distToTarg, lengthaxis));
			Point3 printPos = tmFootPrint.GetTrans();

			surfaceNormal *= -1; // ST - dont forget that this normal was just flipped
			surfaceNormal = VectorTransform(surfaceNormal, grndTM);

			float rotAngle = (float)acos(DotProd(Normalize(surfaceNormal), tmFootPrint.GetRow(lengthaxis)));
			Point3 rotAxis = Normalize(CrossProd(surfaceNormal, tmFootPrint.GetRow(lengthaxis)));
			RotateMatrix(tmFootPrint, AngAxis(rotAxis, rotAngle));	// Rotation is in world space
			tmFootPrint.SetTrans(printPos);

			footprint->SetNodeTM(t, tmFootPrint);
			footprint->InvalidateTreeTM();
		}

		// put all the faces back the way we found them
		for (i = 0; i < nNumFaces; i++)
			grndMesh->FlipNormal(i);
	}

	return connected;
}

void CATMotionPlatform::SetFootPrintColour()
{
	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	if (catmotionlimb)
	{
		DWORD LimbColour = asRGB(catmotionlimb->GetLimbColour());

		INode *thisPrint;
		for (int i = 0; i < pblock->Count(PB_PRINT_NODES); i++)
		{
			thisPrint = pblock->GetINode(PB_PRINT_NODES, 0, i);
			if (thisPrint)
				thisPrint->SetWireColor(LimbColour);
		}
	}
}

void CATMotionPlatform::GetFootPrintPos(TimeValue t, Point3 &pos)
{
	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	DbgAssert(catmotionlimb);
	CATHierarchyRoot* root = catmotionlimb->GetCATMotionRoot();
	if (root->GetWalkMode() != CATHierarchyRoot::WALK_ON_PATHNODE || root->GetCATParentTrans()->GetCATMode() == SETUPMODE)
	{
		pos = P3_IDENTITY;
		return;
	}
	//	Animated footprints
	//	Here we use the footprint NodeTm's and multiply
	//	by the inverse of the original foots position to find the difference
	//	we interpolate 2 print node tms

	int LoopT;
	catmotionlimb->GetStepTime(t, 0.0f, LoopT, FALSE);

	CalcFootPrintMot(t, catmotionlimb->GetStepRatio());

	// this position offset will soon be re-multiplied by CATUnits
	pos = tmPrint1.GetTrans();// / root->GetCATParent()->GetCATUnits();
}

void CATMotionPlatform::GetFootPrintRot(TimeValue, float &zRot)
{
	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);
	DbgAssert(catmotionlimb);
	CATHierarchyRoot* root = catmotionlimb->GetCATMotionRoot();
	if (root->GetWalkMode() != CATHierarchyRoot::WALK_ON_PATHNODE)
	{
		zRot = 0.0f;
		return;
	}
	/*
	 *	Animated footprints
	 *	Here we use the footprint NodeTm's and multiply
	 *	by the inverse of the origin:GetValal foots position to find the difference
	 *	we interpolate 2 print node tms
	 */
	AngAxis axPrint;
	axPrint.Set(tmPrint1);

	zRot = -axPrint.angle * axPrint.axis[root->GetCATParentTrans()->GetLengthAxis()];//.z;
}

// ST - New functions to make UNDO easy, Gets the footprint offset info
// without having a footprint node
void CATMotionPlatform::GetFootPrintMot(TimeValue t, Matrix3 &printMot)
{
	CalcFootPrintMot(t, GetStepGraphRatio(t));
	printMot = tmPrint1 * printMot;
}

void CATMotionPlatform::CalcFootPrintMot(TimeValue t, float ratio)
{
	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);

	//	if(t == tLastFPEval) return;

		// PT - now we cache the ratio that now gets passed in
		// the pelvis will call this function with a linear ratio
		// and the feet will call it with a stepshape based ratio
	//	if(ratio == tLastFPEval) return;

	//	float ratio = limb->GetStepRatio();
	int nStepNumber = catmotionlimb->GetStepNum(t);
	int nNumSteps = pblock->Count(FootTrans2::PB_CAT_PRINT_TM);

	// Crop the index to within range
//	nStepNumber = min(max(nStepNumber, 0), nNumSteps-2);

	if (nStepNumber >= 0 && nStepNumber < nNumSteps - 1)
	{
		Matrix3 tmPrint2;
		// Here we use footprint Node TMs if at all possible
		// This means that we have animated footprints! Yay!
		tmPrint1 = GetPrintTM(nStepNumber, t) * pblock->GetMatrix3(PB_CAT_PRINT_INV_TM, t, nStepNumber);
		tmPrint2 = GetPrintTM(nStepNumber + 1, t) * pblock->GetMatrix3(PB_CAT_PRINT_INV_TM, t, nStepNumber + 1);

		BlendMat(tmPrint1, tmPrint2, ratio);
	}
	// we are outside of the range of the footprints.
	else tmPrint1 = Matrix3(1);
}

//////////////////////////////////////////////////////////////////////////

int CATMotionPlatform::GetStepGraphTime(TimeValue t)
{
	CATMotionLimb* catmotionlimb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_CATMOTIONLIMB);

	TimeValue stepeaseval;
	catmotionlimb->GetStepTime(t, 0.0f, stepeaseval, FALSE);

	Interval stepseg = catmotionlimb->GetStepTimeRange(t);

	float footstepspeedmult = (float)(abs(stepseg.End() - stepseg.Start())) / STEPTIME100;

	// StepShape handles the 'modding' of its self.
	// No need to do anything special like the old stepgraph
	float stepshapeval = pblock->GetFloat(PB_STEPSHAPE, t);

	stepeaseval = (int)stepeaseval % STEPTIME100;
	if (stepeaseval < 0)
		stepeaseval += STEPTIME100;

	int steps = (int)((stepshapeval - stepeaseval) * footstepspeedmult);
	return steps + t;
}

float CATMotionPlatform::GetStepGraphRatio(TimeValue t)//, float &ratio, int &stepnum)
{
	// StepShape handles the 'modding' of its self.
	// No need to do anything special like the old stepgraph
	return pblock->GetFloat(PB_STEPSHAPE, t) / (float)STEPTIME100;
	//	stepnum = catmotionlimb->GetStepNum(t);
}

void CATMotionPlatform::CATMotionMessage(TimeValue, UINT msg, int data)
{
	CATMotionLimb* catmotionlimb = GetCATMotionLimb();
	DbgAssert(catmotionlimb);

	switch (msg)
	{
	case CATMOTION_FOOTSTEPS_CHANGED:
	{
		// update our cache
		if (!calculating_footprints)
		{
			CalcAllPrintPos(TRUE);
		}
		break;
	}
	case CATMOTION_FOOTSTEPS_CREATE:
		CreateFootPrints();
		break;
	case CATMOTION_FOOTSTEPS_REMOVE:
		RemoveFootPrints((BOOL)data);
		break;
	case CATMOTION_FOOTSTEPS_RESET:
		ResetFootPrints((BOOL)data);
		break;
	case CATMOTION_FOOTSTEPS_SNAP_TO_GROUND:
	{
		INode *groundNode = catmotionlimb->GetCATMotionRoot()->GetGroundNode();
		SnapToGround(groundNode, (BOOL)data);
	}
	break;
	}
}

void CATMotionPlatform::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	UNREFERENCED_PARAMETER(ctxt);
	for (int i = pblock->Count(PB_PRINT_NODES) - 1; i >= 0; i--) {
		INode *footprint = pblock->GetINode(PB_PRINT_NODES, 0, i);
		if (footprint) nodes.AppendNode(footprint);
	}
}

void CATMotionPlatform::SaveClip(CATRigWriter *save, int flags, Interval timerange)
{
	flags |= CLIPFLAG_SKIP_NODE_TABLES;
	save->WriteController(this, flags, timerange);
	flags &= ~CLIPFLAG_SKIP_NODE_TABLES;

	save->BeginGroup(idFootprints);
	for (int i = pblock->Count(PB_PRINT_NODES) - 1; i >= 0; i--) {
		INode *node = pblock->GetINode(PB_PRINT_NODES, 0, i);
		if (node) {
			//	save->WriteNode(node);
		}
	}
	save->EndGroup();
}

BOOL CATMotionPlatform::LoadClip(CATRigReader *load, Interval range, int flags)
{
	UNREFERENCED_PARAMETER(load); UNREFERENCED_PARAMETER(range); UNREFERENCED_PARAMETER(flags);
	return TRUE;
};

Control* CATMotionPlatform::GetOwningCATMotionController()
{
	if (pblock != NULL)
		return GetCATMotionLimb();

	return NULL;
}
