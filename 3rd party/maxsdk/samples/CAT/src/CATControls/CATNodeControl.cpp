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

#include "CATNodeControl.h"
#include "ArbBoneTrans.h"
#include "CATClipWeights.h"
#include "CATClipRoot.h"
#include "CATCharacterRemap.h"
#include "Hub.h"

#include "../DataRestoreObj.h"
#include "FnPub/IBoneGroupManagerFP.h"
#include <decomp.h>

#ifdef _DEBUG // We need these 3 classes for some DbgAssert checks
#include "SpineData2.h"
#include "SpineTrans2.h"
#include "BoneSegTrans.h"
#endif

using namespace CatAPI;
/************************************************************************/
/* CATNodeControl                                                       */
/************************************************************************/

CATNodeControl::CATNodeControl()
	: mpNode(NULL)
	, mpParentCATCtrl(NULL)
	, mLayerTrans(NULL)
	, preRigSaveParent(NULL)
	, preRigSaveLayerTransFlags(0)
	, tmBoneWorld(1)
	, mWorldValid(NEVER)
	, mLocalValid(NEVER)
	, mpMirrorBone(NULL)
	, obj_dim(Point3::Origin)
	, pos_limits_pos(1, 1, 1)
	, pos_limits_neg(1, 1, 1)
	, rot_limits_pos(1, 1, 1)
	, rot_limits_neg(1, 1, 1)
	, scl_limits_min(1, 1, 1)
	, scl_limits_max(1, 1, 1)
	, mp3ScaleWorld(1, 1, 1)
{
	// We inherit POS/ROT/SCL by default
	ccflags |= CNCFLAG_INHERIT_ANIM_ROT | CNCFLAG_INHERIT_ANIM_POS | CNCFLAG_INHERIT_ANIM_SCL;
}

CATNodeControl::~CATNodeControl()
{
	DbgAssert(mLayerTrans == NULL); // Our derivation is responsible for managing this pointer

	for (int i = 0; i < tabArbControllers.Count(); i++)
	{
		if (tabArbControllers[i] != NULL && !tabArbControllers[i]->TestAFlag(A_IS_DELETED))
		{
			// We probably should be deleting these things?
			tabArbControllers[i]->SetCATParentTrans(NULL);
			//	tabArbControllers[i]->DeleteAllRefs();
			tabArbControllers[i] = NULL;
		}
	}
}

// Find and link to our parent node
void CATNodeControl::LinkParentChildNodes()
{
	INode* pNode = GetNode();
	if (pNode == NULL)
		return;
	if (pNode->TestAFlag(A_IS_DELETED))
		return;

	// Find a parent for our INode
	CATNodeControl* pMyParent = GetParentCATNodeControl(true);
	if (pMyParent != NULL)
	{
		INode* pParentNode = pMyParent->GetNode();
		// It is not legal to have a parent with a NULL node
		DbgAssert(pParentNode != NULL);
		if (pParentNode != NULL)
		{
			DbgAssert(!pParentNode->TestAFlag(A_IS_DELETED));
			pParentNode->AttachChild(pNode, FALSE);
		}
	}

	int iNumChildren = NumChildCATNodeControls();
	for (int i = 0; i < iNumChildren; i++)
	{
		CATNodeControl* pChild = GetChildCATNodeControl(i);
		if (pChild == NULL)
			continue;

		// Don't do the link if we are already the parent
		if (pChild->mpParentCATCtrl == this)
			continue;

		pChild->SetParentCATNodeControl(this);
		INode* pChildNode = pChild->GetNode();
		if (pChildNode != NULL && !pChildNode->TestAFlag(A_IS_DELETED) && pNode != pChildNode)
			pNode->AttachChild(pChildNode, FALSE);
	}
}

INode* CATNodeControl::CreateNode(Object* pObject)
{
	ICATObject *pCATObj = (ICATObject*)pObject->GetInterface(I_CATOBJECT);
	DbgAssert(pCATObj != NULL);
	// Give the object a pointer back to the controller.
	// This is used so the object can display controller rollouts
	pCATObj->SetTransformController(this);
	pCATObj->SetLengthAxis(GetLengthAxis());

	INode* pNode = GetCOREInterface()->CreateObjectNode(pObject);

	/////////////////////////////////////////////////
	// we are flagging all nodes as bones now
	// so that game exporters see them
	pNode->SetBoneNodeOnOff(TRUE, 0);
	pNode->SetBoneAutoAlign(FALSE);
	pNode->SetBoneFreezeLen(TRUE);
	pNode->SetBoneScaleType(BONE_SCALETYPE_NONE);

	// we dont want to be scaled but somehow it happens, make it Z
	if (GetLengthAxis() == Z)
		pNode->SetBoneAxis(BONE_AXIS_Z);
	else pNode->SetBoneAxis(BONE_AXIS_X);
	/////////////////////////////////////////////////\

	// Assign ourselves as the transform
	pNode->SetTMController(this);

	// Find and link to our parent.
	LinkParentChildNodes();

	// Initialize our HAS_TRANSFORMS flag
	CalculateHasTransformFlag();

	return pNode;
}

INode* CATNodeControl::GetNode() const
{
	return mpNode;
}

Matrix3 CATNodeControl::GetNodeTM(TimeValue t)
{
	INode* pNode = GetNode();
	DbgAssert(pNode != NULL);
	if (pNode != NULL)
		return pNode->GetNodeTM(t);

	return tmBoneWorld;
}

void CATNodeControl::SetNodeTM(TimeValue t, Matrix3& tmCurrent)
{
	INode* pNode = GetNode();
	DbgAssert(pNode != NULL);
	if (pNode != NULL)
		pNode->SetNodeTM(t, tmCurrent);
}

Matrix3 CATNodeControl::GetParentTM(TimeValue t) const
{
	// If possible, return our parents transform
	CATNodeControl* pParentCtrl = const_cast<CATNodeControl*>(this)->GetParentCATNodeControl();
	if (pParentCtrl != NULL)
		return pParentCtrl->GetNodeTM(t);

	// else, the nodes parents transform
	INode* pNode = GetNode();
	DbgAssert(pNode != NULL);
	if (pNode != NULL)
		return pNode->GetParentTM(t);

	// else the identity.
	return Matrix3(1);
}

Point3 CATNodeControl::GetLocalScale(TimeValue t)
{
	Point3 p3Scale = P3_IDENTITY_SCALE;
	if (GetCATMode() != SETUPMODE)
	{
		CATClipMatrix3* pController = GetLayerTrans();
		if (pController != NULL)
		{
			Interval iv;
			pController->GetScale(t, p3Scale, iv);
		}
	}
	return p3Scale;
}

bool CATNodeControl::CanStretch(CATMode iCATMode)
{
	bool bIsStretchy = (iCATMode == SETUPMODE && TestCCFlag(CCFLAG_SETUP_STRETCHY)) ||
		(iCATMode == NORMAL    && TestCCFlag(CCFLAG_ANIM_STRETCHY));
	if (!bIsStretchy)
	{
		// A stretchy data overrides an individual bone
		// Currently (Rampage) this is only applicable to BoneSegTrans/BoneData
		// but in the future it should encompass digit/spine/tail behaviour
		CATNodeControl* pDataCtrl = dynamic_cast<CATNodeControl*>((BaseInterface*)GetManager());
		if (pDataCtrl != NULL)
		{
			bIsStretchy = pDataCtrl->IsStretchy(iCATMode);
		}
	}
	return bIsStretchy;
}

bool CATNodeControl::IsStretchy(CATMode iCATMode)
{
	// First test if it is possible for us to stretch
	bool bIsStretchy = CanStretch(iCATMode);
	if (!bIsStretchy)
		return false;

	// If I stretch, I only stretch if (a) I have an ancestor (b) I am selected, and
	// my immediate ancestor is not, of (c) I am not selected,
	// but my immediate child is.
	if (IsThisBoneSelected())
	{
		CATNodeControl* pChild = GetChildCATNodeControl(0);
		bIsStretchy = (pChild != NULL) &&
			!pChild->IsThisBoneSelected();
	}
	return bIsStretchy;
};

// From CATControl
int CATNodeControl::NumChildCATControls()
{
	return tabArbBones.Count();
}

CATControl* CATNodeControl::GetChildCATControl(int i)
{
	if (i < tabArbBones.Count())
		return tabArbBones[i];
	return NULL;
}

void CATNodeControl::ClearChildCATControl(CATControl* pDestructingClass)
{
	for (int i = 0; i < tabArbBones.Count(); i++)
	{
		if (pDestructingClass == tabArbBones[i])
		{
			tabArbBones.Delete(i, 1);
			return;
		}
	}
	// If we are here, we need to clean this some other way
	CATControl::ClearChildCATControl(pDestructingClass);
}

int CATNodeControl::NumLayerControllers()
{
	return ((GetLayerTrans() ? 1 : 0) + tabArbControllers.Count());
}

CATClipValue* CATNodeControl::GetLayerController(int i)
{
	if (GetLayerTrans())
	{
		if (i == 0)
			return GetLayerTrans();
		else i--;
	}
	return (i < tabArbControllers.Count()) ? tabArbControllers[i] : NULL;
}

TSTR	CATNodeControl::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	if (GetBoneID() >= 0) {
		TSTR boneid;
		boneid.printf(_T("[%i]"), GetBoneID());
		bonerigname = bonerigname + boneid;
	}
	CATControl *parentcatcontrol = GetParentCATControl();
	if (parentcatcontrol)
		return (parentcatcontrol->GetBoneAddress() + _T(".") + bonerigname);
	else return (TSTR(IdentName(idSceneRootNode)) + _T(".") + bonerigname);
};

INode*	CATNodeControl::GetBoneByAddress(TSTR address) {
	if (address.Length() == 0) return GetNode();
	return CATControl::GetBoneByAddress(address);
};

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATNodeControlPLCB : public PostLoadCallback {
protected:
	CATNodeControl *ctrl;

public:
	CATNodeControlPLCB(CATNodeControl *pOwner) { ctrl = pOwner; }

	DWORD GetFileSaveVersion() {
		if (ctrl->GetFileSaveVersion() > 0)	return ctrl->GetFileSaveVersion();
		if (ctrl->GetCATParentTrans())
			return ctrl->GetCATParentTrans()->GetFileSaveVersion();
		return 0;
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		// This only accured when a file is getting upgraded to pre-CAT2 scene files.
		// The complex upgrade path meant that sometimes a controller would be deleted
		// by a different controllers PLCB function.

		if (!ctrl || ctrl->TestAFlag(A_IS_DELETED) || !DependentIterator(ctrl).Next())
		{
			delete this;
			return;
		}

		if (GetFileSaveVersion() < CAT_VERSION_2435) {
			MaxReferenceMsgLock lockThis;
			ctrl->UpdateCATUnits();
		}

		if (GetFileSaveVersion() < CAT3_VERSION_2707) {
			ctrl->SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT, TRUE);
			ctrl->SetCCFlag(CNCFLAG_INHERIT_ANIM_POS, TRUE);
			ctrl->SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL, TRUE);
			ctrl->ClearCCFlag(CCFLAG_ANIM_STRETCHY);

		}

		// ST Nov-18-2009.  Previous versions of the CAT
		// had the flags backwards.  This should have no
		// effect, as 99% of the files will have both flags
		// enabled.
		if (GetFileSaveVersion() < CAT3_VERSION_3500)
		{
			bool bTemp = ctrl->TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT);
			if (!ctrl->TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT))
				ctrl->ClearCCFlag(CNCFLAG_INHERIT_ANIM_POS);
			if (!bTemp)
				ctrl->ClearAFlag(CNCFLAG_INHERIT_ANIM_ROT);

			// We need to patch up any old ID's lying around
			IBoneGroupManager* pManager = ctrl->GetManager();
			if (pManager)
			{
				int nBones = pManager->GetNumBones();
				for (int i = 0; i < nBones; i++)
				{
					if (pManager->GetBoneINodeControl(i) == ctrl)
					{
						ctrl->SetBoneID(i);
						break;
					}
				}
			}

			// I'm finding old scenes have some of the bone scale
			// flags set.  This is causing crazy slowdowns, but
			// is completely useless for CAT, so remove it.
			INode* pNode = ctrl->GetNode();
			if (pNode != NULL)
			{
				pNode->SetBoneFreezeLen(TRUE);
				pNode->SetBoneScaleType(BONE_SCALETYPE_NONE);
			}
		}

		ctrl->UpdateObjDim();

		// after loading is complete we do a quick
		// check of the ARB bone tables for errors
		ctrl->CleanUpArbBones();

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define		CATNODECTRLCHUNK_CATCTRLCHUNK		1

#define		CATNODECTRLCHUNK_NODE				2
//#define		CATNODECTRLCHUNK_NAME			3
#define		CATNODECTRLCHUNK_GROUP				4

#define		CATNODECTRLCHUNK_NUMARBBONES		5
#define		CATNODECTRLCHUNK_ARBBONES			6

#define		CATNODECTRLCHUNK_NUMARBLAYERCTRLS	7
#define		CATNODECTRLCHUNK_ARBLAYERCTRLS		8

#define		CATNODECTRLCHUNK_BONEID				9

#define		CATNODECTRLCHUNK_OBJ_DIM			10
#define		CATNODECTRLCHUNK_MIRRORBONE			12

#define		CATNODECTRLCHUNK_BONELIM_POS_POS	14
#define		CATNODECTRLCHUNK_BONELIM_POS_NEG	15
#define		CATNODECTRLCHUNK_BONELIM_ROT_POS	16
#define		CATNODECTRLCHUNK_BONELIM_ROT_NEG	17

#define		CATNODECTRLCHUNK_ERN				20

IOResult CATNodeControl::Save(ISave *isave)
{
	DWORD nb;//, refID;
	ULONG id;
	IOResult res = IO_OK;
	int i;

	isave->BeginChunk(CATNODECTRLCHUNK_CATCTRLCHUNK);
	res = CATControl::Save(isave);
	isave->EndChunk();

	isave->BeginChunk(CATNODECTRLCHUNK_NUMARBBONES);
	i = tabArbBones.Count();
	isave->Write(&i, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(CATNODECTRLCHUNK_ARBBONES);
	for (i = 0; i < tabArbBones.Count(); i++) {
		id = isave->GetRefID(tabArbBones[i]);
		isave->Write(&id, sizeof(ULONG), &nb);
	}
	isave->EndChunk();

	if (tabArbControllers.Count() > 0) {
		int numarbctrls = 0;
		for (int i = 0; i < tabArbControllers.Count(); i++) {
			// Arb Controllers are controllers that can be assigned to other non-CAT parameters in the scene.
			// They are not referenced by CAT, and so it is difficult to know when the ceace to exist.
			// Thier destructor tells us when they are finally deleted, but there are cases where they are on
			// the undo stack and Max scene file is saved.
			if (tabArbControllers[i] && !tabArbControllers[i]->TestAFlag(A_IS_DELETED) && DependentIterator(tabArbControllers[i]).Next())
			{
				numarbctrls++;
			}
		}

		isave->BeginChunk(CATNODECTRLCHUNK_NUMARBLAYERCTRLS);
		isave->Write(&numarbctrls, sizeof(int), &nb);
		isave->EndChunk();

		isave->BeginChunk(CATNODECTRLCHUNK_ARBLAYERCTRLS);
		for (int i = 0; i < tabArbControllers.Count(); i++) {
			if (tabArbControllers[i] && !tabArbControllers[i]->TestAFlag(A_IS_DELETED) && DependentIterator(tabArbControllers[i]).Next())
			{
				id = isave->GetRefID(tabArbControllers[i]);
				isave->Write(&id, sizeof(ULONG), &nb);
			}
		}
		isave->EndChunk();
	}

	/////////////////////////////////////////////////////
	// 2.5
	isave->BeginChunk(CATNODECTRLCHUNK_OBJ_DIM);
	isave->Write(&obj_dim, sizeof(Point3), &nb);
	isave->EndChunk();

	isave->BeginChunk(CATNODECTRLCHUNK_MIRRORBONE);
	id = (ULONG)-1;
	if (mpMirrorBone) id = isave->GetRefID(mpMirrorBone);
	isave->Write(&id, sizeof(ULONG), &nb);
	isave->EndChunk();

	/////////////////////////////////////////////////////
	// CAT3 Bone Limits
	isave->BeginChunk(CATNODECTRLCHUNK_BONELIM_POS_POS);	isave->Write(&pos_limits_pos, sizeof(Point3), &nb);	isave->EndChunk();
	isave->BeginChunk(CATNODECTRLCHUNK_BONELIM_POS_NEG);	isave->Write(&pos_limits_neg, sizeof(Point3), &nb);	isave->EndChunk();
	isave->BeginChunk(CATNODECTRLCHUNK_BONELIM_ROT_POS);	isave->Write(&rot_limits_pos, sizeof(Point3), &nb);	isave->EndChunk();
	isave->BeginChunk(CATNODECTRLCHUNK_BONELIM_ROT_NEG);	isave->Write(&rot_limits_neg, sizeof(Point3), &nb);	isave->EndChunk();

	isave->BeginChunk(CATNODECTRLCHUNK_ERN);
	ExtraRigNodes::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult CATNodeControl::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;//, refID;
	int nNumArbBones = 0;
	ULONG id = 0L;
	int i = 0;

	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case CATNODECTRLCHUNK_CATCTRLCHUNK:
			res = CATControl::Load(iload);
			break;
		case CATNODECTRLCHUNK_NODE:
			res = iload->Read(&id, sizeof(ULONG), &nb);
			if (res == IO_OK && id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&mpNode);
			break;
		case CATNODECTRLCHUNK_BONEID:
		{
			// SimCity: Moved the bone id back to CATControl
			// Every CATControl also has an ID, and it's silly
			// to have 2 different ID's
			int anID = -1;
			res = iload->Read(&anID, sizeof(anID), &nb);
			DbgAssert(GetBoneID() == -1 || GetBoneID() == anID);
			SetBoneID(anID);
			break;
		}
		case CATNODECTRLCHUNK_NUMARBBONES:
		{
			res = iload->Read(&nNumArbBones, sizeof(int), &nb);
			tabArbBones.SetCount(nNumArbBones);
			for (i = 0; i < nNumArbBones; i++)
				tabArbBones[i] = NULL;
			break;
		}
		case CATNODECTRLCHUNK_ARBBONES:
			for (i = 0; i < nNumArbBones; i++)
			{
				res = iload->Read(&id, sizeof(ULONG), &nb);
				if (res == IO_OK && id != 0xffffffff)
					iload->RecordBackpatch(id, (void**)&tabArbBones[i]);
			}
			break;
		case CATNODECTRLCHUNK_NUMARBLAYERCTRLS:
			res = iload->Read(&i, sizeof(int), &nb);
			tabArbControllers.SetCount(i);
			for (int i = 0; i < tabArbControllers.Count(); i++)
				tabArbControllers[i] = NULL;
			break;
		case CATNODECTRLCHUNK_ARBLAYERCTRLS:
			for (int i = 0; i < tabArbControllers.Count(); i++)
			{
				res = iload->Read(&id, sizeof(ULONG), &nb);
				if (res == IO_OK && id != 0xffffffff && id != 0)
					iload->RecordBackpatch(id, (void**)&tabArbControllers[i]);
			}
			break;
			///////////////////////////////////////////////
			// CAT 2.5
		case CATNODECTRLCHUNK_OBJ_DIM:
			res = iload->Read(&obj_dim, sizeof(Point3), &nb);
			break;
		case CATNODECTRLCHUNK_MIRRORBONE:
			res = iload->Read(&id, sizeof(ULONG), &nb);
			if (res == IO_OK && id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&mpMirrorBone);
			break;
			///////////////////////////////////////////////
			// CAT 3
		case CATNODECTRLCHUNK_BONELIM_POS_POS:	res = iload->Read(&pos_limits_pos, sizeof(Point3), &nb);	break;
		case CATNODECTRLCHUNK_BONELIM_POS_NEG:	res = iload->Read(&pos_limits_neg, sizeof(Point3), &nb);	break;
		case CATNODECTRLCHUNK_BONELIM_ROT_POS:	res = iload->Read(&rot_limits_pos, sizeof(Point3), &nb);	break;
		case CATNODECTRLCHUNK_BONELIM_ROT_NEG:	res = iload->Read(&rot_limits_neg, sizeof(Point3), &nb);	break;
		case CATNODECTRLCHUNK_ERN:				ExtraRigNodes::Load(iload);									break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new CATNodeControlPLCB(this));
	return IO_OK;
}

void CATNodeControl::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	ExtraRigNodes::AddSystemNodes(nodes, ctxt);

	if (ctxt == kSNCDelete) {
		if (mpMirrorBone) {
			mpMirrorBone->SetMirrorBone(NULL);
		}

		// Here we set an unbalanced evaluation stop (there is
		// no matching call to allow evalutations again).  This
		// is because this class is now deleted, so we don't need
		// to match it.  Set the flag, and if the deletion is undone,
		// then the flag setting is undone as well, and we go back to normal.
		SetCCFlag(CNCFLAG_LOCK_STOP_EVALUATING, TRUE);

		for (int i = 0; i < tabArbControllers.Count(); i++) {
			// Note: This is only necessary if the arb controller is assigned to something that is not being deleted.
			// and in that case, we should just disable the controller and keep going.
			// This leaves an orphaned controller in the scene, but at least it shouldn't evaluate from now on.
			if (tabArbControllers[i]) {
				tabArbControllers[i]->SetFlag(CLIP_FLAG_DISABLE_LAYERS);
			}
		}
	}
	INode* node = GetNode();
	nodes.AppendNode(node);

	// add all our children to the list
	CATControl::AddSystemNodes(nodes, ctxt);
}

Point3	CATNodeControl::GetBoneDimensions(TimeValue t)
{
	Point3 p3Scale = GetLocalScale(t);
	return obj_dim * p3Scale;
}

float CATNodeControl::GetBoneLength() {
	int lengthaxis = GetLengthAxis();
	return obj_dim[lengthaxis];
}

void CATNodeControl::SetBoneLength(float val) {
	ICATObject* iobj = GetICATObject();
	if (!iobj) return;
	iobj->SetZ(val / GetCATUnits());
}

// this method is used by stretch limbs to force bones to stretch
//This function isn't used
void	CATNodeControl::SetBoneDimensions(TimeValue t, Point3 dim)
{
	if (GetCATMode() == SETUPMODE)
	{
		if (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL) || IsXReffed()) return;

		ICATObject* iobj = GetICATObject();
		if (!iobj) return;
		dim /= GetCATUnits();
		ModVec(dim, GetLengthAxis());
		iobj->SetBoneDim(dim);
	}
	else
	{
		if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) return;

		// what is the scale factor
		dim = dim / GetBoneDimensions();
		SetXFormPacket scl(dim, 1);
		scl.command = XFORM_SCALE;
		SetValue(t, (void*)&scl, 1, CTRL_RELATIVE);
	}
}

// Access to the Setup Matrix
Matrix3 CATNodeControl::GetSetupMatrix() {
	Matrix3 tmSetup(1);
	if (GetLayerTrans()) GetLayerTrans()->GetSetupVal((void*)&tmSetup);
	return tmSetup;
}

void CATNodeControl::SetSetupMatrix(Matrix3 tmSetup) {
	if (GetLayerTrans())
		GetLayerTrans()->SetSetupVal((void*)&tmSetup);
}

BOOL CATNodeControl::IsUsingSetupController() const
{
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	return pLayerTrans && pLayerTrans->IsUsingSetupController();
}

void CATNodeControl::CreateSetupController(BOOL tf)
{
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	INode* pNode = GetNode();
	if (pNode == NULL || pLayerTrans == NULL)
		return;

	TimeValue t = GetCOREInterface()->GetTime();
	Matrix3 tmParent = pNode->GetParentTM(t);
	Point3 p3Scale;
	CalcParentTransform(t, tmParent, p3Scale);
	pLayerTrans->CreateSetupController(tf, tmParent);
}

BOOL CATNodeControl::SaveRigArbBones(CATRigWriter *save)
{
	if (tabArbBones.Count() == 0) return TRUE;
	save->BeginGroup(idArbBones);

	for (int i = 0; i < tabArbBones.Count(); i++)
		if (tabArbBones[i]) tabArbBones[i]->SaveRig(save);

	save->EndGroup();
	return TRUE;
}

BOOL CATNodeControl::LoadRigArbBones(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
			if (load->CurGroupID() != idArbBones) return FALSE;

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idArbBone:
			{
				CATNodeControl* pNewCtrl = static_cast<CATNodeControl*>(AddArbBone(TRUE, FALSE));
				pNewCtrl->LoadRig(load);
				break;
			}
			case idHub: {
				Hub* hub = (Hub*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HUB_CLASS_ID);
				hub->Initialise(GetCATParentTrans(), true, NULL);
				InsertArbBone(GetNumArbBones(), hub);
				hub->LoadRig(load);
				break;
			}
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	return ok && load->ok();
}

BOOL CATNodeControl::SaveRigCAs(CATRigWriter *save)
{
	UNREFERENCED_PARAMETER(save);
	return TRUE;
	/*
	//	ICustAttribContainer * objCAs = GetObject()->GetCustAttribContainer();
	ICustAttribContainer * ctrlCAs = GetObject()->GetCustAttribContainer();

	//	FPValue result;
	//	ca_def = (custAttributes.getDefs $.baseobject)[1]

	if(!ctrlCAs) return TRUE;
	save->BeginGroup(idCAs);

	for(int i = 0; i < ctrlCAs->GetNumCustAttribs(); i++){
	CustAttrib *ca = ctrlCAs->GetCustAttrib(i);

	//	TSTR name = ca->GetName();
	//	save->Write(idName, (void*)&name);

	//	ca_def_string = custAttributes.getDefSource ((custAttributes.getDefs this)[i])
	}

	save->EndGroup();
	return TRUE;
	*/
}

BOOL CATNodeControl::LoadRigCAs(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	void AllocCustAttribContainer();
	ICustAttribContainer * ctrlCAs = GetCustAttribContainer();

	if (!ctrlCAs) {
		load->SkipGroup();
		return TRUE;
	}

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
			if (load->CurGroupID() != idArbBones) return FALSE;

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idArbBone:
			{
				CATNodeControl* pNewCtrl = static_cast<CATNodeControl*>(AddArbBone());
				pNewCtrl->LoadRig(load);
				break;
			}
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idName:
				//	ctrlCAs->AppendCustAttrib(ca);
				break;
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	return ok && load->ok();
}

BOOL CATNodeControl::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(GetRigID());
	//	if(GetCATMode()==SETUPMODE){
	//		Color nodecolour = Color(GetNode()->GetWireColor());
	//		if(!(nodecolour==GetBonesRigColour())){
	//			SetBonesRigColour(nodecolour);
	//		}
	//	}
	save->Write(idBoneName, GetName());
	save->Write(idFlags, ccflags);

	// TODO: Make this work in release build
	INode* parentnode = GetParentNode();
	save->Write(idParentNode, parentnode);
	DWORD dwCLR = asRGB(GetBonesRigColour());
	save->Write(idBoneColour, dwCLR);

	ICATObject* iobj = GetICATObject();
	if (iobj) iobj->SaveRig(save);

	if (GetLayerTrans()) GetLayerTrans()->SaveRig(save);

	SaveRigArbBones(save);
	//	SaveRigCAs(save);

	save->EndGroup();
	return TRUE;
}

BOOL CATNodeControl::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	float val;
	Matrix3 tmSetup;
	ICATObject* iobj = GetICATObject();
	DWORD clr;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != GetRigID()) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idObjectParams:	iobj->LoadRig(load);											break;
			case idController:		DbgAssert(GetLayerTrans()); GetLayerTrans()->LoadRig(load);		break;
			case idArbBones:		LoadRigArbBones(load);											break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idBoneName:	load->GetValue(name);	SetName(name);					break;
			case idParentNode: {
				TSTR parent_address;
				load->GetValue(parent_address);
				load->AddParent(GetNode(), parent_address);
				/*		//	load->GetValue(&parentnode);
				// really this call *should* be a GetValue call just like
				// everything else. But I had trouble getting the return parameter
				// thing to work.
				parentnode = load->GetINode();
				if(parentnode&&GetNode())
				parentnode->AttachChild(GetNode(), FALSE);
				*/			break;
			}
			case idFlags:		load->GetValue(ccflags);											break;
			case idSetupTM:		load->GetValue(tmSetup);		SetSetupMatrix(tmSetup);			break;
			case idWidth:		load->GetValue(val);			SetObjX(val);						break;
			case idHeight:		load->GetValue(val);			SetObjY(val);						break;
			case idLength:		load->GetValue(val);			SetObjZ(val);						break;
			case idBoneColour:	load->GetValue(clr);			SetBonesRigColour(Color(clr));		break;
			default:			load->AssertOutOfPlace();
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}
	if (ok) {
		if (load->GetVersion() < CAT3_VERSION_2707) {
			HoldSuspend hs;
			SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_POS);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL);
			ClearCCFlag(CCFLAG_ANIM_STRETCHY);
		}
	}
	return ok && load->ok();
}

BOOL CATNodeControl::SaveClipArbBones(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	if (tabArbBones.Count() == 0) return TRUE;

	save->BeginGroup(idArbBones);

	for (int i = 0; i < tabArbBones.Count(); i++)
		if (tabArbBones[i]) {
			tabArbBones[i]->SaveClip(save, flags, timerange, layerindex);
		}

	save->EndGroup();
	return TRUE;
}

BOOL CATNodeControl::LoadClipArbBones(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	int nArbBoneIndex = 0;

	while (load->ok() && !done && ok) {

		// An error occurred in the clause, and we've
		// ended up with the next valid one.  If we're
		// not still in the correct group, return.
		if (!load->NextClause())
			if (load->CurGroupID() != idArbBones) return FALSE;

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup: {
			// In some cases, we can now end up with ahub in our list of Arb Bones when a spine is set to have 0 bones
			if ((load->CurIdentifier() == idArbBone || load->CurIdentifier() == idHub) && nArbBoneIndex < tabArbBones.Count() && tabArbBones[nArbBoneIndex])
			{
				tabArbBones[nArbBoneIndex]->LoadClip(load, timerange, layerindex, dScale, flags);
				nArbBoneIndex++;
			}
			else load->SkipGroup();
			break;
		}
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}

	return ok && load->ok();
}

BOOL CATNodeControl::SaveClipExtraControllers(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	if (tabArbControllers.Count() == 0) return TRUE;

	save->BeginGroup(idExtraControllers);

	for (int i = 0; i < tabArbControllers.Count(); i++)
		if (tabArbControllers[i]) {
			save->BeginGroup(idExtraController);
			tabArbControllers[i]->SaveClip(save, flags, timerange, layerindex);
			save->EndGroup();
		}

	save->EndGroup();
	return TRUE;
}

BOOL CATNodeControl::LoadClipExtraControllers(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	int nExtraControllerIndex = 0;

	while (load->ok() && !done && ok) {

		// An error occurred in the clause, and we've
		// ended up with the next valid one.  If we're
		// not still in the correct group, return.
		if (!load->NextClause())
			if (load->CurGroupID() != idArbBones) return FALSE;

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup: {
			if (load->CurIdentifier() == idExtraController && nExtraControllerIndex < tabArbControllers.Count() && tabArbControllers[nExtraControllerIndex])
			{
				tabArbControllers[nExtraControllerIndex]->LoadClip(load, timerange, layerindex, dScale, flags);
				nExtraControllerIndex++;
			}
			else load->SkipGroup();
			break;
		}
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}

	return ok && load->ok();
}

BOOL CATNodeControl::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(GetRigID());

	// Add a comment for legibility
	if (GetNode()) save->Comment(GetNode()->GetName());

	if (GetLayerTrans()) {
		save->BeginGroup(idController);
		GetLayerTrans()->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}

	// Allow saving of any other information this bone requires.
	SaveClipOthers(save, flags, timerange, layerindex);

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);

	// Save all our extra controllers as well.
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

BOOL CATNodeControl::LoadClip(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	// A counter passed to LoadClipOthers to allow derived classes to count loaded items
	int data = 0;

	// Is this a worldspace clip?
	INode* node = GetNode();
	INode *parentNode = node->GetParentNode();
	if (parentNode->IsRootNode())
		flags |= CLIPFLAG_WORLDSPACE;
	else flags &= ~CLIPFLAG_WORLDSPACE;

	// If we are loading a mirrored clip and we need to swap data,
	// like in the case of a facial rig taht we are loading a mirrored pose onto
	// we load onot the mirror bone instead and set a flag so it doesn't re-swap deeper in the tree
	if (flags&CLIPFLAG_MIRROR && mpMirrorBone && !(flags&CLIPFLAG_MIRROR_DATA_SWAPPED))
	{
		// Set this flag so that we don't try to re-swap on a sub part of this bone
		flags |= CLIPFLAG_MIRROR_DATA_SWAPPED;
		return mpMirrorBone->LoadClip(load, timerange, layerindex, dScale, flags);
	}

	while (load->ok() && !done && ok) {

		// An error occurred in the clause, and we've
		// ended up with the next valid one.  If we're
		// not still in the correct group, return.
		if (!load->NextClause())
			if (load->CurGroupID() != GetRigID()) return FALSE;

		// Allow overridden loading behaviour.
		if (LoadClipOthers(load, timerange, layerindex, dScale, flags, data))
			continue;

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idController:
			{
				int newflags = flags;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)) newflags |= CLIPFLAG_SKIPPOS;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)) newflags |= CLIPFLAG_SKIPROT;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) newflags |= CLIPFLAG_SKIPSCL;

				if (GetLayerTrans())
					GetLayerTrans()->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			}
			case idArbBones:
				LoadClipArbBones(load, timerange, layerindex, dScale, flags);
				break;

			case idExtraControllers:
				LoadClipExtraControllers(load, timerange, layerindex, dScale, flags);
				break;

			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idValMatrix3: {
				// this method will do all the processing of the pose for us(Mirroring, transforming)
				Matrix3 tm;
				int newflags = flags;//|CLIPFLAG_APPLYTRANSFORMS;
				if (load->GetValuePose(newflags, SuperClassID(), (void*)&tm)) {
					tm = tm * GetCATParentTrans()->ApproxCharacterTransform(timerange.Start());
					if (mpNode) mpNode->SetNodeTM(timerange.Start(), tm);
				}
				break;
			}
			case idValPoint:
			{
				Point3 p3LocalScale;
				// Don't use the GetValuePose because that will assume you are loading a position and then transfomr it
				if (load->GetValue(p3LocalScale)) {
					if (GetLayerTrans())
						GetLayerTrans()->SetScale(timerange.Start(), p3LocalScale, P3_IDENTITY_SCALE, CTRL_ABSOLUTE);
				}
				break;
			}
			default:
				load->AssertOutOfPlace();
				break;
			}
			break;
		case rigAbort:
		case rigEnd:
			ok = FALSE;
		case rigEndGroup:
			done = TRUE;
			break;
		}
	}

	return ok && load->ok();
}

int CATNodeControl::GetNumArbBones()
{
	return tabArbBones.Count();
}

CatAPI::INodeControl* CATNodeControl::AddArbBone(BOOL bAsNewGroup)
{
	return AddArbBone(FALSE, bAsNewGroup);
}
CATNodeControl* CATNodeControl::AddArbBone(BOOL loading, BOOL bAsNewGroup)
{
	ArbBoneTrans* newArbBone = (ArbBoneTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, ARBBONETRANS_CLASS_ID);
	DbgAssert(newArbBone);

	// If we create the bone with a NULL group, it will set itself
	// as the head of a new group
	CATGroup* pBoneGroup = bAsNewGroup ? NULL : GetGroup();
	int index = tabArbBones.Count();
	newArbBone->Initialise(pBoneGroup, this, index, loading);

	InsertArbBone(index, newArbBone);

	return newArbBone;
};

void CATNodeControl::RemoveArbBone(CATNodeControl* pArbBone, BOOL deletenode/*=TRUE*/)
{
	if (pArbBone == NULL)
		return;

	// A bones ID should be its position in the tab.
	int id = pArbBone->GetBoneID();
	DbgAssert(id >= 0 && id < tabArbBones.Count());
	if (id < 0 || id >= tabArbBones.Count())
		return; // nothing to remove

	CATNodeControl* arbbone = tabArbBones[id];
	DbgAssert(arbbone == pArbBone);

	// Last-resort sanity check - try to find the ID if we have the wrong one
	if (arbbone != pArbBone)
	{
		id = -1;
		for (int i = 0; i < tabArbBones.Count(); i++)
		{
			if (tabArbBones[i] == pArbBone)
			{
				id = i;
				arbbone = tabArbBones[i];
			}
		}
	}
	DbgAssert(id >= 0);
	if (id < 0)
		return;

	HoldData(tabArbBones);

	// shuffle the remaining bones down the list
	int iLastPtr = tabArbBones.Count() - 2;
	for (int i = id; i <= iLastPtr; i++) {
		tabArbBones[i] = tabArbBones[i + 1];
		if (tabArbBones[i])
			((CATNodeControl*)tabArbBones[i])->SetBoneID(i);
	}

	// Shrink the array, removing the last pointer and any NULL pointers under it
	while (iLastPtr > 0 && tabArbBones[iLastPtr] == NULL)
		iLastPtr--;

	tabArbBones.SetCount(iLastPtr + 1);

	// With our link to the ArbBone removed, make
	// sure we break it's link back to us (in case
	// we are deleted before it, it's pointer can no
	// longer be NULL'ed)
	if (deletenode)
	{
		INode* pArbNode = pArbBone->GetNode();
		if (pArbNode != NULL && pArbNode->TestAFlag(A_IS_DELETED))
			pArbNode->Delete(0, FALSE);
	}
	else
	{
		DbgAssert(arbbone->GetParentCATNodeControl(true) == this);
		arbbone->SetParentCATNodeControl(NULL);
		static_cast<ArbBoneTrans*>(arbbone)->SetGroup(NULL);
	}
};

void CATNodeControl::InsertArbBone(int id, CATNodeControl *arbbone)
{
	DbgAssert(arbbone != NULL);
	if (arbbone == NULL)
		return;

	if (id < 0)
		id = tabArbBones.Count();

	int oldCount = tabArbBones.Count();
	int newCount = (id <= oldCount) ? oldCount + 1 : id + 1;

	// Cache the current state.
	HoldData(tabArbBones);

	// Resize the array
	tabArbBones.SetCount(newCount);
	for (int i = oldCount; i < newCount; i++)
		tabArbBones[i] = NULL;

	// shuffle the remaining up a notch to make room for the new bone
	if (id < oldCount)
	{
		for (int i = oldCount; i > id; --i)
		{
			tabArbBones[i] = tabArbBones[i - 1];
			if (tabArbBones[i])
				((CATNodeControl*)tabArbBones[i])->SetBoneID(i);
		}
	}

	if (arbbone)
	{
		arbbone->SetParentCATNodeControl(this);
		arbbone->SetBoneID(id);
	}

	tabArbBones[id] = arbbone;
};

CatAPI::INodeControl* CATNodeControl::GetArbBoneINodeControl(int i) {
	return GetArbBone(i);
}

CATNodeControl* CATNodeControl::GetArbBone(int i) {
	return (i >= 0 && i < tabArbBones.Count()) ? tabArbBones[i] : NULL;
}

Control* CATNodeControl::CreateArbBoneController(BOOL bAsNewGroup)
{
	ArbBoneTrans* pNewController = static_cast<ArbBoneTrans*>(CreateInstance(CTRL_MATRIX3_CLASS_ID, ARBBONETRANS_CLASS_ID));

	// If we create the bone with a NULL group, it will set itself
	// as the head of a new group
	CATGroup* pBoneGroup = bAsNewGroup ? NULL : GetGroup();
	pNewController->Initialise(pBoneGroup, this, NULL, false, false);

	InsertArbBone(-1, pNewController);
	CATClipValue* pNewBoneLayerTrans = pNewController->GetLayerTrans();

	return pNewController;
}

CATClipValue* CATNodeControl::CreateLayerFloat()
{
	tabArbControllers.SetCount(tabArbControllers.Count() + 1);
	tabArbControllers[tabArbControllers.Count() - 1] = CreateClipValueController(GetCATClipFloatDesc(), GetClipWeights(), GetCATParentTrans(), FALSE);
	tabArbControllers[tabArbControllers.Count() - 1]->SetFlag(CLIP_FLAG_ARB_CONTROLLER);
	return tabArbControllers[tabArbControllers.Count() - 1];
}

int CATNodeControl::FindLayerFloat(Control *ctrl) {
	int i, id = -1;
	// find the controller in the list
	for (i = 0; i < tabArbControllers.Count(); i++) {
		if (tabArbControllers[i] == ctrl) { id = i; break; }
	}
	return id;
}
void CATNodeControl::RemoveLayerFloat(int id) {
	if (id >= 0 && id < tabArbControllers.Count()) {
		//	if(tabArbControllers[id]->TestAFlag(A_IS_DELETED))
		//		tabArbControllers[id]->DeleteThis();
		for (int i = id; i < (tabArbControllers.Count() - 1); i++) {
			tabArbControllers[i] = tabArbControllers[i + 1];
		}
		tabArbControllers.SetCount(tabArbControllers.Count() - 1);
	}
}

void CATNodeControl::InsertLayerFloat(int id, CATClipValue *arb_controller) {

	tabArbControllers.SetCount(tabArbControllers.Count() + 1);
	tabArbControllers[tabArbControllers.Count() - 1] = NULL;

	// shuffle the remaining up a knotch to make room for the new bone
	if ((id < tabArbControllers.Count() - 1)) {
		for (int i = (tabArbControllers.Count() - 1); i > (id - 1); --i) {
			tabArbControllers[i] = tabArbControllers[i - 1];
		}
	}

	tabArbControllers[id] = arb_controller;
};

CATClipValue* CATNodeControl::GetLayerFloat(int i) {
	return (i >= 0 && i < tabArbControllers.Count()) ? tabArbControllers[i] : NULL;
}

// this method is used by PLCBs to try and remove corruption in files.
// of course we are trying to find the src of the corruption
void CATNodeControl::CleanUpArbBones() {
	TSTR message = GetString(IDS_ERR_CORRUPTION);
	TSTR summary = GetString(IDS_ERR_CORRUPT1);
	TSTR errordetails;

	for (int i = 0; i < tabArbBones.Count(); i++) {
		// look for missing bones
		CATNodeControl* pArbBone = tabArbBones[i];
		if (pArbBone == NULL)
		{
			tabArbBones.Delete(i, 1);
			i--;
			continue;
		}

		DbgAssert(pArbBone->GetNode() != NULL);
		TSTR bonename(pArbBone->GetNode()->GetName());
		bonename += _M("\n\n");

		// look for wrongly numbered bones
		if (pArbBone->GetBoneID() != i)
		{
			if (!GetCOREInterface()->GetQuietMode())
			{
				errordetails = GetString(IDS_ERR_WRONGINDEX);

				::MessageBox(GetCOREInterface()->GetMAXHWnd(),
					message + errordetails/* + boneaddress*/ + bonename + summary,
					GetString(IDS_ERR_FILELOAD), MB_OK);
			}
			pArbBone->SetBoneID(i);
		}

		// make sure that we own our arb bones
		DbgAssert(pArbBone->mpParentCATCtrl == NULL);
		if (pArbBone->mpParentCATCtrl != NULL && pArbBone->mpParentCATCtrl != this)
		{
			// This is extremely problematic.  In some cases (MAXX-10395)
			// the same ArbBone is appearing in both lists.  Quite how it got
			// to this state I have no idea, but its a real problem.  The issue
			// is, we don't know which link is the correct one! Sooo... we guess,
			// and just remove the existing parents, keep the current one.
			CATNodeControl* pOldParent = pArbBone->mpParentCATCtrl;
			for (int i = 0; i < pOldParent->GetNumArbBones(); i++)
			{
				if (pOldParent->GetArbBone(i) == pArbBone)
				{
					pOldParent->RemoveArbBone(pArbBone, FALSE);
					if (!GetCOREInterface()->GetQuietMode())
					{
						errordetails = GetString(IDS_ERR_LISTEDTWICE);
						::MessageBox(GetCOREInterface()->GetMAXHWnd(),
							message + errordetails/* + boneaddress*/ + bonename + summary,
							GetString(IDS_ERR_FILELOAD), MB_OK);
					}
				}
			}
		}
		pArbBone->SetParentCATNodeControl(this);

		// look for duplicate bones
		for (int j = i + 1; j < tabArbBones.Count(); j++)
		{
			if (tabArbBones[j] == pArbBone)
			{
				errordetails = GetString(IDS_ERR_LISTEDTWICE);
				if (!GetCOREInterface()->GetQuietMode())
				{
					::MessageBox(GetCOREInterface()->GetMAXHWnd(),
						message + errordetails/* + boneaddress*/ + bonename + summary,
						GetString(IDS_ERR_FILELOAD), MB_OK);
				}
				tabArbBones[j] = NULL;
				tabArbBones.Delete(j, 1);
				j--;
			}
		}
	}

	if (GetRigID() == idArbBone)
	{
		CATNodeControl* pOwner = GetParentCATNodeControl(true);
		if (pOwner)
		{
			// Our owner bone may have lost its pointer to us some how.
			if (pOwner->GetArbBone(GetBoneID()) != this)
			{
				BOOL bonefound = FALSE;
				for (int i = 0; i < pOwner->GetNumArbBones() && !bonefound; i++)
				{
					if (pOwner->GetArbBone(i) == this)
						bonefound = TRUE;
				}
				if (!bonefound)
				{
					TSTR bonename(GetNode()->GetName());
					errordetails = GetString(IDS_ERR_LOSTCON);
					if (!GetCOREInterface()->GetQuietMode())
					{
						::MessageBox(GetCOREInterface()->GetMAXHWnd(),
							message + errordetails/* + boneaddress*/ + bonename + summary,
							GetString(IDS_ERR_FILELOAD), MB_OK);
					}
					pOwner->InsertArbBone(pOwner->GetNumArbBones(), this);
				}
			}
		}

	}
}

void CATNodeControl::UpdateVisibility()
{
	if (GetNode())
	{
		//		float vis = catparenttrans->GetVisibility();

		//		GetNode()->SetRenderable(catparenttrans->GetRenderability());
		//		GetNode()->SetVisibility(GetCOREInterface()->GetTime(), vis);

		//		if(vis <= 0.0f)
		//			GetNode()->Hide(true);
		//		else
		//			GetNode()->Hide(false);
		//
		//		GetNode()->Freeze(catparenttrans->GetFrozen());
	}
}

void CATNodeControl::UpdateCATUnits()
{
	ICATObject* iobj = GetICATObject();
	if (iobj) {
		iobj->SetCATUnits(GetCATUnits());
		iobj->Update();

		UpdateObjDim();
	}
}

void CATNodeControl::UpdateObjDim()
{
	if (GetLengthAxis() == Z)
	{
		obj_dim.x = GetObjX() / 2.0f;
		obj_dim.y = GetObjY() / 2.0f;
		obj_dim.z = GetObjZ();
	}
	else
	{
		obj_dim.x = GetObjZ();
		obj_dim.y = GetObjY() / 2.0f;
		obj_dim.z = GetObjX() / 2.0f;
	}

	obj_dim *= GetCATUnits();
}

// The following 2 methods are for displaying ghosts.
// We simply disable all but the target layer and then call dispaly on all the objects in the rig.
void CATControl::DisplayLayer(TimeValue t, ViewExp *vpt, int flags, Box3 &bbox)
{
	for (int i = 0; i < NumChildCATControls(); i++) {
		CATControl *child = GetChildCATControl(i);
		if (child) child->DisplayLayer(t, vpt, flags, bbox);
	}
}

void CATNodeControl::DisplayLayer(TimeValue t, ViewExp *vpt, int flags, Box3 &bbox)
{
	ICATObject*	iobj = GetICATObject();
	CATClipWeights* pWeights = GetClipWeights();

	if (iobj && mpNode && pWeights != NULL)
	{
		iobj->DisplayObject(t, mpNode, vpt, flags, pWeights->GetBlendedLayerColour(t), bbox);
	}
	CATControl::DisplayLayer(t, vpt, flags, bbox);
}

void CATNodeControl::DisplayPosLimits(TimeValue t, GraphicsWindow *gw)
{
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_GIZMOS));
	DrawBox(gw, GetNodeTM(t), pos_limits_neg, pos_limits_pos, bbox);
}

void CATNodeControl::DisplayRotLimits(TimeValue, GraphicsWindow *gw)
{
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_GIZMOS));
	Matrix3 tm = GetNodeTM(1);
	tm.SetTrans(tmBoneWorld.GetTrans());
	int lengthaxis = GetLengthAxis();
	float arclengths = GetBoneLength() / 3.0f; //catparenttrans->GetCATUnits() * 20.0f;

	// Draw -X to +X arc
	if (lengthaxis == X)
		DrawArc10Segs(gw, tm, tm.GetAddr()[X], rot_limits_neg.x, rot_limits_pos.x, Point3(0.0f, 0.0f, arclengths), bbox, true, SM_HOLLOW_BOX_MRKR);
	else DrawArc10Segs(gw, tm, tm.GetAddr()[X], rot_limits_neg.x, rot_limits_pos.x, Point3(0.0f, arclengths, 0.0f), bbox, true, SM_HOLLOW_BOX_MRKR);

	// Draw -Y to +Y arc
	DrawArc10Segs(gw, tm, tm.GetAddr()[Y], rot_limits_neg.y, rot_limits_pos.y, Point3(0.0f, 0.0f, arclengths), bbox, true, SM_HOLLOW_BOX_MRKR);

	// Draw -Z to +Z arc
	if (lengthaxis == X)
		DrawArc10Segs(gw, tm, tm.GetAddr()[Z], rot_limits_neg.z, rot_limits_pos.z, Point3(arclengths, 0.0f, 0.0f), bbox, true, SM_HOLLOW_BOX_MRKR);
	else DrawArc10Segs(gw, tm, tm.GetAddr()[Z], rot_limits_neg.z, rot_limits_pos.z, Point3(0.0f, 0.0f, arclengths), bbox, true, SM_HOLLOW_BOX_MRKR);
}

void CATNodeControl::DisplaySclLimits(TimeValue, GraphicsWindow *)
{

}

// TODO: override this method for BoneData
int CATNodeControl::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	bbox.Init();

	if (GetCATMode() != SETUPMODE && TestCCFlag(CNCFLAG_DISPLAY_ONION_SKINS) && mpNode) {
		ICATObject*	iobj = GetICATObject();
		if (!iobj) return 0;

		int numonionskins = 20;
		int tpf = GetTicksPerFrame();
		for (int i = -(numonionskins / 2); i < (numonionskins / 2); i++) {
			mpNode->InvalidateTM();
			iobj->DisplayObject(t + (i*tpf), mpNode, vpt, flags, GetCurrentColour(t), bbox);
		}
		//	return 1;
	}
	// Some subanims seem to be able to display themselves without thier parent
	// controllers needing to pass this method on. Some do not. HI Pivot trans
	// can display itsself without any help.
	//	if(GetLayerTrans())	return GetLayerTrans()->Display(t, inode, vpt, flags);

	if (inode->Selected()) {
		GraphicsWindow *gw = vpt->getGW();
		gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world
		if (TestCCFlag(CNCFLAG_IMPOSE_POS_LIMITS)) DisplayPosLimits(t, gw);
		if (TestCCFlag(CNCFLAG_IMPOSE_ROT_LIMITS)) DisplayRotLimits(t, gw);
		if (TestCCFlag(CNCFLAG_IMPOSE_SCL_LIMITS)) DisplaySclLimits(t, gw);
	}

	return 0;
}

void CATNodeControl::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box)
{
	box += bbox;
	if (GetLayerTrans()) GetLayerTrans()->GetWorldBoundBox(t, inode, vpt, box);
}

void CATNodeControl::CATMessage(TimeValue t, UINT msg, int data) {
	switch (msg)
	{
	case CAT_COLOUR_CHANGED:
		UpdateColour(t);
		break;
	case CAT_NAMECHANGE:
		UpdateName();
		break;
	case CAT_VISIBILITY:
		UpdateVisibility();
		break;
	case CAT_SHOWHIDE:
		if (GetNode()) GetNode()->Hide(data);
		break;
	case CAT_CATUNITS_CHANGED:
		UpdateCATUnits();
	case CLIP_LAYER_MOVEUP:
	case CLIP_LAYER_MOVEDOWN:
	case CAT_UPDATE:
		if (!IsEvaluationBlocked()) {
			InvalidateTransform();
			//BlockAllEvaluation(TRUE, FALSE);
			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			//BlockAllEvaluation(FALSE, FALSE);
		}
		break;
	case CAT_CATMODE_CHANGED:
		InvalidateTransform();
		// TODO: Consider limiting this notify so only the
		// the first node in the hierarchy sends it.
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
		UpdateUI();
		break;
	case CAT_INVALIDATE_TM:
		InvalidateTransform();
		break;
	case CAT_SET_LENGTH_AXIS:
		SetLengthAxis(data);
		break;
	case CAT_UPDATE_USER_PROPS:
		UpdateUserProps();
		break;
	case CAT_REFRESH_OBJECTS_LENGTHAXIS: {
		ICATObject *iobj = GetICATObject();
		if (iobj)iobj->SetLengthAxis(GetLengthAxis());
		break;
	}
	case CAT_PRE_RIG_SAVE: {
		INode* parentnode = GetParentNode();
		// If this node is linked to a node htat is not part of this rig
		if (!parentnode->IsRootNode()) {
			BOOL unlinknode = TRUE;

			// now we need to check to see if this bone is linked to a node that is not part of this rig.
			INodeTab allrignodes;
			GetCATParentTrans()->AddSystemNodes(allrignodes, kSNCFileSave);
			for (int i = 0; i < allrignodes.Count(); i++) {
				if (allrignodes[i] == parentnode)
					unlinknode = FALSE;
			}
			if (unlinknode) {
				// Unlink this node temporarily
				preRigSaveParent = parentnode;
				if (GetLayerTrans())	preRigSaveLayerTransFlags = GetLayerTrans()->GetFlags();

				MaxReferenceMsgLock lockThis;
				GetCOREInterface()->GetRootNode()->AttachChild(GetNode(), FALSE);
			}
		}
		else {
			preRigSaveParent = NULL;
		}
		break;
	}
	case CAT_POST_RIG_SAVE:
		if (preRigSaveParent) {
			MaxReferenceMsgLock lockThis;
			if (GetLayerTrans()) GetLayerTrans()->SetFlags(preRigSaveLayerTransFlags);
			preRigSaveParent->AttachChild(GetNode(), FALSE);
		}
		break;
	case CAT_EVALUATE_BONES:
		// We want all CATBones to have valid node caches
		if (mpNode) {
			// Force complete evaluation, do not return cached values here.
			mWorldValid.SetEmpty();
			mpNode->InvalidateTM();

			// Calling GetNodeTM triggers update of all values.
			mpNode->GetNodeTM(t);
		}
		break;
	case CLIP_ARB_LAYER_DELETED:
	{
		Animatable* pTargetFloat = GetAnimByHandle(data);
		DbgAssert(pTargetFloat != NULL && dynamic_cast<CATClipValue*>(pTargetFloat) != NULL);
		int id = FindLayerFloat((Control*)pTargetFloat);
		if (id != -1)
			RemoveLayerFloat(id);
	}
	}

	// use CATControl to propagate the message
	CATControl::CATMessage(t, msg, data);

	// Run this after the CATMessage, as it relies on the
	// value calculated by our CATClipValue to be invalidated first
	if (msg == CLIP_WEIGHTS_CHANGED)
	{
		UpdateColour(t);
		InvalidateTransform();
	}
}

void CATNodeControl::KeyFreeform(TimeValue t, ULONG flags)
{
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (pLayerTrans != NULL)
	{
		Matrix3 tmCurrent;
		INode* pNode = GetNode();
		if (pNode != NULL)
		{

			SetXFormPacket xform;
			Matrix3 tmWorld = pNode->GetNodeTM(t);

			if ((flags&(KEY_POSITION | KEY_ROTATION)) == (KEY_POSITION | KEY_ROTATION)) {
				xform.command = XFORM_SET;
				xform.tmAxis = tmWorld;
			}
			else if ((flags&(KEY_POSITION)) == KEY_POSITION) {
				xform.command = XFORM_MOVE;
				xform.p = tmWorld.GetTrans();
				xform.tmAxis = Matrix3(1);
			}
			else if ((flags&(KEY_ROTATION)) == KEY_ROTATION) {
				xform.command = XFORM_ROTATE;
				// this is not strictly correct. We should be passing in an AngleAxis or a quat,
				// but I can't get the SetValue to Set an absolute rotation. I know Keying a layer has always worked
				// So here I pass in a full transform and CATClipMatrix3::SetValue will generate the correct rotation
				xform.tmAxis = tmWorld;
			}

			// Get our parent matrix while in KeyFreeform, this ensures
			// that we evaluate in the context of our final evaluation
			xform.tmParent = pNode->GetParentTM(t);

			KeyFreeformMode SetMode(pLayerTrans);
			SetValue(t, &xform, 1, CTRL_ABSOLUTE);
		}
	}
}

// this function is called to recolur the bones accurding to he current colour mode
Color CATNodeControl::GetCurrentColour(TimeValue t)
{
	if (GetCATMode() == SETUPMODE)
		return GetBonesRigColour();

	ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent->GetEffectiveColourMode() == COLOURMODE_BLEND)
	{
		CATClipWeights* pWeights = GetClipWeights();
		if (pWeights != NULL)
			return pWeights->GetBlendedLayerColour(t);
	}

	return GetBonesRigColour();
}

void CATNodeControl::UpdateColour(TimeValue t/*=0*/)
{
	if (GetNode())
		GetNode()->SetWireColor(asRGB(GetCurrentColour(t)));
}

Color CATNodeControl::GetBonesRigColour()
{
	CATGroup* pGroup = GetGroup();
	if (pGroup != NULL)
		return pGroup->GetGroupColour();
	return Color(0,0,0);
}

void CATNodeControl::SetBonesRigColour(Color clr)
{
	CATGroup* pGroup = GetGroup();
	if (pGroup != NULL)
		pGroup->SetGroupColour(clr);
}

TSTR CATNodeControl::GetRigName() {
	return GetCATParentTrans()->GetCATName() + GetName();
};

void CATNodeControl::UpdateName()
{
	INode * node = GetNode();
	TSTR newname = GetRigName();

	if (node != NULL && _tcscmp(newname, node->GetName()) != 0) {
		node->SetName(newname);
	}
}

class SetLimitRestore : public RestoreObj {
public:
	CATNodeControl	*c;
	Point3	pos_limits_pos_undo, pos_limits_neg_undo, rot_limits_pos_undo, rot_limits_neg_undo, scl_limits_min_undo, scl_limits_max_undo;
	Point3	pos_limits_pos_redo, pos_limits_neg_redo, rot_limits_pos_redo, rot_limits_neg_redo, scl_limits_min_redo, scl_limits_max_redo;

	SetLimitRestore(CATNodeControl *c) {
		this->c = c;
		pos_limits_pos_undo = c->pos_limits_pos;	pos_limits_neg_undo = c->pos_limits_neg;
		rot_limits_pos_undo = c->rot_limits_pos;	rot_limits_neg_undo = c->rot_limits_neg;
		scl_limits_min_undo = c->scl_limits_min;	scl_limits_max_undo = c->scl_limits_max;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			pos_limits_pos_redo = c->pos_limits_pos;	pos_limits_neg_redo = c->pos_limits_neg;
			rot_limits_pos_redo = c->rot_limits_pos;	rot_limits_neg_redo = c->rot_limits_neg;
			scl_limits_min_redo = c->scl_limits_min;	scl_limits_max_redo = c->scl_limits_max;
		}
		c->pos_limits_pos = pos_limits_pos_undo;	c->pos_limits_neg = pos_limits_neg_undo;
		c->rot_limits_pos = rot_limits_pos_undo;	c->rot_limits_neg = rot_limits_neg_undo;
		c->scl_limits_min = scl_limits_min_undo;	c->scl_limits_max = scl_limits_max_undo;
	}
	void Redo() {
		c->pos_limits_pos = pos_limits_pos_undo;	c->pos_limits_neg = pos_limits_neg_undo;
		c->rot_limits_pos = rot_limits_pos_undo;	c->rot_limits_neg = rot_limits_neg_undo;
		c->scl_limits_min = scl_limits_min_undo;	c->scl_limits_max = scl_limits_max_undo;
	}
	int Size() { return ((sizeof(Point3) * 12) + 1); }
	void EndHold() {}
};

BOOL CATNodeControl::PasteLayer(CATControl* pPasteCtrl, int fromindex, int toindex, DWORD flags, RemapDir &remap)
{
	// Call super class, does most of the work.
	BOOL res = CATControl::PasteLayer(pPasteCtrl, fromindex, toindex, flags, remap);
	if (res)
	{
		// Paste Arb Bones
		// We know this cast is safe, because PasteLayer passed
		CATNodeControl* pPasteNodeCtrl = static_cast<CATNodeControl*>(pPasteCtrl);
		int nArbBones = min(GetNumArbBones(), pPasteNodeCtrl->GetNumArbBones());
		for (int i = 0; i < nArbBones; i++) {
			CATControl *pMyArbBone = tabArbBones[i];
			CATControl *pPastedArbBone = pPasteNodeCtrl->tabArbBones[i];
			if (pMyArbBone && pPastedArbBone)
				pMyArbBone->PasteLayer(pPastedArbBone, fromindex, toindex, flags, remap);
		}
	}
	return res;
}

BOOL CATNodeControl::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	HoldActions hold(IDS_HLD_CATNODEPASTE);

	CATNodeControl* pastecatnodectrl = (CATNodeControl*)pastectrl;

#ifdef PASTE_EXTRA_RIG_PARTS
	BOOL firstpastedbone = FALSE;
	if (!(flags&PASTERIGFLAG_FIRST_PASTED_BONE)) {
		flags |= PASTERIGFLAG_FIRST_PASTED_BONE;
		firstpastedbone = TRUE;
	}
#endif

	// if we are pasting a
	if (flags&PASTERIGFLAG_MIRROR) {
		SetMirrorBone(pastecatnodectrl->GetNode());
	}

	if (!(flags&PASTERIGFLAG_DONT_PASTE_CHILDREN)) {
		// just before we keep going down the hierarchy
		int numarbbones = GetNumArbBones();
		int pastenumarbbones = pastecatnodectrl->GetNumArbBones();
		if (numarbbones < pastenumarbbones)
		{
			for (int i = numarbbones; i < pastenumarbbones; i++)
			{
				// Maintain grouping
				CATNodeControl* pBoneToClone = pastecatnodectrl->GetArbBone(i);
				if (pBoneToClone != NULL)
				{
					CATGroup* pGroup = pBoneToClone->GetGroup();
					if (pGroup != NULL)
						AddArbBone(FALSE, pGroup->AsCATControl() == pBoneToClone);
				}
			}
		}
		else for (int i = pastenumarbbones; i < numarbbones; i++)
			RemoveArbBone(GetArbBone(i));
	}

	ICATObject* iobj = GetICATObject();
	ICATObject* pasteiobj = pastecatnodectrl->GetICATObject();
	if (iobj && pasteiobj)	iobj->PasteRig(pasteiobj, flags, scalefactor);

	if (!(flags&PASTERIGFLAG_DONT_PASTE_CONTROLLER)) {
		if (GetLayerTrans() && pastecatnodectrl->GetLayerTrans()) {
			GetLayerTrans()->PasteRig(pastecatnodectrl->GetLayerTrans(), flags, scalefactor);
		}
		else {
			Matrix3 tmSetup = pastecatnodectrl->GetSetupMatrix();
			if (flags&PASTERIGFLAG_MIRROR) {
				if (GetLengthAxis() == Z)	MirrorMatrix(tmSetup, kXAxis);
				else					MirrorMatrix(tmSetup, kZAxis);
			}
			tmSetup.SetTrans(tmSetup.GetTrans() * scalefactor);
			SetSetupMatrix(tmSetup);
		}
	}

	// Paste all the flags
	SetCCFlag(CCFLAG_EFFECT_HIERARCHY, pastecatnodectrl->TestCCFlag(CCFLAG_EFFECT_HIERARCHY));

	SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS, pastecatnodectrl->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS));
	SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT, pastecatnodectrl->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT));
	SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL, pastecatnodectrl->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL));

	SetCCFlag(CCFLAG_SETUP_STRETCHY, pastecatnodectrl->TestCCFlag(CCFLAG_SETUP_STRETCHY));

	SetCCFlag(CNCFLAG_LOCK_LOCAL_POS, pastecatnodectrl->TestCCFlag(CNCFLAG_LOCK_LOCAL_POS));
	SetCCFlag(CNCFLAG_LOCK_LOCAL_ROT, pastecatnodectrl->TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT));
	SetCCFlag(CNCFLAG_LOCK_LOCAL_SCL, pastecatnodectrl->TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL));

	SetCCFlag(CCFLAG_ANIM_STRETCHY, pastecatnodectrl->TestCCFlag(CCFLAG_ANIM_STRETCHY));

	SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT, pastecatnodectrl->TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT));
	SetCCFlag(CNCFLAG_INHERIT_ANIM_POS, pastecatnodectrl->TestCCFlag(CNCFLAG_INHERIT_ANIM_POS));
	SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL, pastecatnodectrl->TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL));

	SetCCFlag(CNCFLAG_IMPOSE_POS_LIMITS, pastecatnodectrl->TestCCFlag(CNCFLAG_IMPOSE_POS_LIMITS));
	SetCCFlag(CNCFLAG_IMPOSE_ROT_LIMITS, pastecatnodectrl->TestCCFlag(CNCFLAG_IMPOSE_ROT_LIMITS));
	SetCCFlag(CNCFLAG_IMPOSE_SCL_LIMITS, pastecatnodectrl->TestCCFlag(CNCFLAG_IMPOSE_SCL_LIMITS));

	// Paste all the limit values
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));

	pos_limits_pos = pastecatnodectrl->pos_limits_pos * scalefactor;
	pos_limits_neg = pastecatnodectrl->pos_limits_neg * scalefactor;
	rot_limits_pos = pastecatnodectrl->rot_limits_pos;
	rot_limits_neg = pastecatnodectrl->rot_limits_neg;
	scl_limits_max = pastecatnodectrl->scl_limits_max;
	scl_limits_min = pastecatnodectrl->scl_limits_min;

	CATControl::PasteRig(pastectrl, flags, scalefactor);

	UpdateCATUnits();

#ifdef PASTE_EXTRA_RIG_PARTS
	if (firstpastedbone) {
		CATCharacterRemap remap;

		// Add Required Mapping
		remap.AddEntry(pastectrl->GetCATParentTrans(), mpCATParentTrans);
		remap.AddEntry(pastectrl->GetCATParentTrans()->GetNode(), mpCATParentTrans->GetNode());
		remap.AddEntry(pastectrl->GetCATParent(), mpCATParent);

		BuildMapping(pastectrl, remap, FALSE);
		PasteERNNodes(pastecatnodectrl, *remap);
	}
#endif

	return TRUE;
}

BOOL CATControl::PasteERNNodes(CATControl* pastectrl, CATCharacterRemap &remap)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	int numchildcontrols = min(NumChildCATControls(), pastectrl->NumChildCATControls());
	for (int i = 0; i < numchildcontrols; i++) {
		CATControl* child = GetChildCATControl(i);
		CATControl* pastechild = pastectrl->GetChildCATControl(i);
		if (child && pastechild) child->PasteERNNodes(pastechild, remap);
	}
	return TRUE;
}

BOOL CATNodeControl::PasteERNNodes(CATControl* pastectrl, CATCharacterRemap &remap)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;
	CATNodeControl* pastecatnodectrl = (CATNodeControl*)pastectrl;

	//ExtraRigNodes::PasteERNNodes(remap, pastecatnodectrl);
	return CATControl::PasteERNNodes(pastecatnodectrl, remap);
}

void AddControllerHierarchy(ReferenceTarget* from, ReferenceTarget* to, RemapDir &remap) {

	remap.AddEntry(from, to);
	for (int i = 0; i < min(from->NumRefs(), to->NumRefs()); i++) {
		if (from->GetReference(i) && to->GetReference(i) && (from->GetReference(i)->ClassID() == to->GetReference(i)->ClassID())) {
			AddControllerHierarchy(from->GetReference(i), to->GetReference(i), remap);
		}
	}
}

BOOL CATControl::BuildMapping(CATControl* pastectrl, CATCharacterRemap &remap, BOOL includeERNnodes)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	remap.AddEntry(pastectrl, this);
	int i;
	// Build a mapping o
	int numlayercontrollers = min(NumLayerControllers(), pastectrl->NumLayerControllers());
	for (i = 0; i < numlayercontrollers; i++) {
		if (pastectrl->GetLayerController(i) && GetLayerController(i))
		{
			remap.AddEntry(pastectrl->GetLayerController(i), GetLayerController(i));

			if (pastectrl->GetLayerController(i)->GetSetupController() && GetLayerController(i)->GetSetupController()) {
				//	AddControllerHierarchy( pastectrl->GetLayerController(i)->GetSetupController(), GetLayerController(i)->GetSetupController(), remap );
				//	AddControllerHierarchy( pastectrl->GetLayerController(i), GetLayerController(i), remap );
			}
		}
	}
	Tab<CATControl*> dAllMyCtrls;
	GetAllChildCATControls(dAllMyCtrls);
	Tab<CATControl*> dAllPasteCtrls;
	pastectrl->GetAllChildCATControls(dAllPasteCtrls);

	// Try and find a mapping for all of our controls.
	for (int i = 0; i < dAllMyCtrls.Count(); i++)
	{
		CATControl* pChild = dAllMyCtrls[i];
		if (pChild == NULL)
			continue;

		// Search through all pastectrls for a matching class
		for (int j = 0; j < dAllPasteCtrls.Count(); j++)
		{
			CATControl* pPasteChild = dAllPasteCtrls[j];
			if (pPasteChild == NULL)
				continue;

			// Our only criteria is that the target child
			// has the same class as our child
			if (pPasteChild->ClassID() != pChild->ClassID())
				continue;

			// Success, build mapping and waltz on!
			pChild->BuildMapping(pPasteChild, remap, includeERNnodes);
			dAllMyCtrls[i] = NULL;
			dAllPasteCtrls[j] = NULL;
			// No reason to continue this loop!
			break;
		}
	}
	return TRUE;
}

BOOL CATNodeControl::BuildMapping(CATControl* pastectrl, CATCharacterRemap &remap, BOOL includeERNnodes)
{
	if (pastectrl == NULL || ClassID() != pastectrl->ClassID()) return FALSE;

	CATNodeControl* pastecatnodectrl = (CATNodeControl*)pastectrl;

	INode* pSrcNode = pastecatnodectrl->GetNode();
	INode* pDstNode = GetNode();
	// It is now legal to have CATNodeControl
	// instances that are not tied to a node
	// This can happen when creating ArbBones
	// via the function CreateArbBoneController
	if (pSrcNode != NULL && pDstNode != NULL)
	{
		// Clone the settings off the source
		remap.AddEntry(pSrcNode, pDstNode);

		// Ensure that any pointers to the objects are included as well.
		// TODO: Do we need to worry about modifiers?  For example, if a modifier
		// is instanced over several objects, this may break that instancing.
		remap.AddEntry(pSrcNode->GetObjectRef(), pDstNode->GetObjectRef());
	}

	// remap between ArbBones.  Remapping standard nodes (eg; ArbBones)
	// is handled by the CATCharacterRemap class, so simply create
	// a list of standard nodes and send it off to the Remap class.
	Tab<INode*> dAllSrcArbBones;
	dAllSrcArbBones.SetCount(pastecatnodectrl->GetNumArbBones());
	for (int i = 0; i < dAllSrcArbBones.Count(); i++)
	{
		CATNodeControl* pArbBone = static_cast<CATNodeControl*>(pastecatnodectrl->tabArbBones[i]);
		if (pArbBone != NULL)
			dAllSrcArbBones[i] = pArbBone->GetNode();
	}
	Tab<INode*> dAllDstArbBones;
	dAllDstArbBones.SetCount(GetNumArbBones());
	for (int i = 0; i < dAllDstArbBones.Count(); i++)
	{
		CATNodeControl* pArbBone = static_cast<CATNodeControl*>(tabArbBones[i]);
		if (pArbBone != NULL)
			dAllDstArbBones[i] = pArbBone->GetNode();
	}
	remap.BuildMapping(dAllSrcArbBones, dAllDstArbBones);

	// Do standard remapping for CATControl
	BOOL res = CATControl::BuildMapping(pastectrl, remap, includeERNnodes);

	// Do remapping for ERN nodes (this uses CATCharacterRemap algo too)
	if (includeERNnodes) {
		ExtraRigNodes::BuildMapping((ExtraRigNodes*)pastecatnodectrl, remap);
	}

	return res;
}

void CATNodeControl::CloneCATNodeControl(CATNodeControl* clonedctrl, RemapDir& remap)
{
	if (ClassID() != clonedctrl->ClassID()) return;

	clonedctrl->obj_dim = obj_dim;

	// clone our superclass parameters
	CloneCATControl(clonedctrl, remap);

	// just before we keep going down the hierarchy

	// Do NOT clone ArbBones.  When they are cloned, they will
	// set their pointers back on this bone.  It needs to figure
	// out if just the Arb bone or the whole segment has been cloned.

	//int numarbbones = GetNumArbBones();
	//int i;
	//clonedctrl->tabArbBones.SetCount(numarbbones);

	//for(i = 0; i<numarbbones; i++)
	//	clonedctrl->tabArbBones[i] = NULL;

	//// all these INodes have already ben added to the List of cloned INodes
	//// so this patch pointer will patch the pinters to these nodes once they are cloned
	//for(i = 0; i<numarbbones; i++)
	//{
	//	if(tabArbBones[i])
	//	{
	//		ArbBoneTrans* pClonedArbBone = static_cast<ArbBoneTrans*>(remap.CloneRef(tabArbBones[i]));
	//	}
	//}

	// Make sure all the extra rig node pointers get patched
	CloneERNNodes(clonedctrl, remap);

	Object* obj = GetObject();
	ICATObject* iobj = (ICATObject*)obj->GetInterface(I_CATOBJECT);
	if (iobj) {
		Object* newobj = (Object*)remap.CloneRef(obj);
		DbgAssert(newobj);
		ICATObject* inewobj = (ICATObject*)newobj->GetInterface(I_CATOBJECT);
		DbgAssert(inewobj);
		if (inewobj)
			inewobj->SetTransformController(clonedctrl);
		else
		{
			HoldSuspend hs;
			newobj->DeleteThis();
		}
	}
}

void CATNodeControl::PostCloneNode()
{
	// For each clone operation, we need to ensure
	// that our pointers are patched back from our data to
	// whatever owns it.
	if (GetBoneID() == 0)
	{
		IBoneGroupManagerFP* pManager = static_cast<IBoneGroupManagerFP*>(GetManager());
		if (pManager == NULL)
			return;

		// Pass this call onto this class.
		pManager->PostCloneManager();
	}
}

//////////////////////////////////////////////////////////////////////////
// from class Control:
//

void CATNodeControl::SetParentCATNodeControl(CATNodeControl* pNewParent)
{
	if (pNewParent != mpParentCATCtrl)
	{
		HoldData(mpParentCATCtrl);
		mpParentCATCtrl = pNewParent;
	}
}

CATNodeControl*	CATNodeControl::GetParentCATNodeControl(bool bGetCATParent/*=false*/) {

	// If we don't have our CATNodeControl yet, lets find it!
	if (mpParentCATCtrl == NULL)
	{
		CATNodeControl* pMyParent = this;
		while (NULL != (pMyParent = pMyParent->FindParentCATNodeControl()))
		{
			// Our parent must have a valid node...
			INode* pMyParentNode = pMyParent->GetNode();
			if (pMyParentNode != NULL && !pMyParentNode->TestAFlag(A_IS_DELETED))
			{
				mpParentCATCtrl = pMyParent;
				break;
			}
		}
	}

	// This is our CAT defined parent, sometimes we need this link
	if (bGetCATParent)
		return mpParentCATCtrl;

	// But in normal operation, we query our Max-defined parent
	INode* pParentNode = GetParentNode();
	if (pParentNode != NULL)
	{
		Control* pCtrl = pParentNode->GetTMController();
		if (pCtrl != NULL)
			return static_cast<CATNodeControl*>(pCtrl->GetInterface(I_CATNODECONTROL));
		else
			return NULL;
	}
	return mpParentCATCtrl;
}

#ifdef FBIK

virtual CATNodeControl*	GetChildCATNodeControl(int i) {
	if (i < tabArbBones.Count()) {
		if (!parent->TestCCFlag(CCFLAG_LOCK_ROT)) return tabArbBones[i];
		tabArbBones[i]
			return tabArbBones[i];
	}
	return NULL;
};

virtual Point3	MoveChildCATNodeControls(Point3 vec) {
	Point3 movement(0.0f, 0.0f, 0.0f);
	if (TestFlag()) return movement;
	for (int i = 0; i < NumChildCATNodeControls(); i++) {
		CATNodeControl *child = GetChildCATNodeControl(i);
		if (child) movement += child->MoveChildCATNodeControls();
	}
	movement /= NumChildCATNodeControls();
	return movement;
};
#endif

void CATNodeControl::ApplyChildOffset(TimeValue /*t*/, Matrix3& tmChild) const
{
	int lengthAxis = GetLengthAxis();
	Point3 offset = Point3::Origin;
	offset[lengthAxis] = obj_dim[lengthAxis];

	tmChild.PreTranslate(offset);
}

void CATNodeControl::ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid)
{
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (pLayerTrans != NULL)
	{
		if (GetCATMode() == SETUPMODE || pLayerTrans->TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE))
		{
			pLayerTrans->ApplySetupVal(t, tmOrigParent, tmWorldTransform, p3BoneLocalScale, ivValid);

			// Ensure that there's no local scale in setup mode. CAT makes sure that there's no scale
			// in the static setup matrix but there could be scale in the setup animation controller.
			// The scale in the controller only works in animation mode and relative to setup pose.
			if (GetCATMode() == SETUPMODE)
				p3BoneLocalScale = P3_IDENTITY_SCALE;
		}
	}
}

void CATNodeControl::CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	// ArbBoneTrans has a very icky override of this function.
	// This assert is more to remind me of this in the future,
	// in case I modify this function than to assert that
	//.ArbBoneTrans doesn't call this function.
	DbgAssert(ClassID() != ARBBONETRANS_CLASS_ID);
	CATNodeControl* pParentCtrl = GetParentCATNodeControl(); // Is it possible to calculate this without accessing the parent?

	if (pParentCtrl != NULL)
		pParentCtrl->ApplyChildOffset(t, tmParent);

	CalcInheritance(t, tmParent, p3ParentScale);
}

void CATNodeControl::CalcInheritance(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	// Was our parent scaled?  If so, we need to remove that first.
	if ((tmParent.GetIdentFlags()&SCL_IDENT) == 0)
	{
		// First, we calculate the scale
		p3ParentScale.x = tmParent.GetRow(X).Length();		// Calculate the X Scale
		p3ParentScale.y = tmParent.GetRow(Y).Length();		// Our scale in the Y axis
		p3ParentScale.z = tmParent.GetRow(Z).Length();		// our scale in the Z axis

		// Now, we remove the scale.
		// NOTE:  This will not
		// orthonormalize the matrix, so inherited rotated scale will
		// still cause shearing!  This never happens internal to the CAT
		// hierarchy though, so we dont care (if this happens externally
		// to CAT, there is nothing we can do about it, the animators screwed us up!)
		// The parents scale is cached, and re-applied later
		// PS - No thanks at all to whoever made Matrix3::operator [] private!!!
		Point3* pMtxInternals = (Point3*)tmParent.GetAddr();
		pMtxInternals[X] /= p3ParentScale.x;				// Remove X scale
		pMtxInternals[Y] /= p3ParentScale.y;				// Remove Y scale
		pMtxInternals[Z] /= p3ParentScale.z;				// Remove Z scale
		tmParent.SetIdentFlags(SCL_IDENT);
	}
	else
		p3ParentScale = P3_IDENTITY_SCALE;

	int catmode = GetCATMode();
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (pLayerTrans == NULL)
		return;

	// Now, apply position/rotational inheritance for setupmode
	if ((catmode == SETUPMODE && !pLayerTrans->TestFlag(CLIP_FLAG_KEYFREEFORM)) ||
		pLayerTrans->TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE))
	{
		// No scale in setupmode, nope, not ever.
		if (catmode == SETUPMODE)
			p3ParentScale = P3_IDENTITY_SCALE;
		CATClipMatrix3* pLayerTrans = GetLayerTrans();
		if (pLayerTrans == NULL)
			return;

		if (pLayerTrans->TestFlag(CLIP_FLAG_HAS_TRANSFORM))
		{
			Matrix3 tmCATParent;
			if (GetCATParentTM(t, tmCATParent))
				tmParent = tmCATParent;
		}
		else
		{
			// Is our position applied relative to our parent?
			if (pLayerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT))
			{
				// These 2 flags are mutually exclusive
				DbgAssert(!pLayerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT));
				Matrix3 tmCATParent;
				if (GetCATParentTM(t, tmCATParent))
					tmParent.SetTrans(tmCATParent.GetTrans());
			}

			if (pLayerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT))
			{
				Matrix3 tmCATParent;
				if (GetCATParentTM(t, tmCATParent))
				{
					tmCATParent.SetTrans(tmParent.GetTrans());
					if (pLayerTrans->TestFlag(CLIP_FLAG_SETUP_FLIPROT_FROM_CATPARENT))
						tmCATParent.PreRotateY(PI);
					tmParent = tmCATParent;
				}
			}
		}
	}
	else
	{
		// Start removing bits of the matrix

		// Should we keep position?
		if (!TestCCFlag(CNCFLAG_INHERIT_ANIM_POS))
			tmParent.NoTrans();

		// Should we keep rotation?
		if (!TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT))
			tmParent.NoRot();

		// Should we keep scale? (Its already removed from the tmParent
		if (!TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL))
			p3ParentScale = P3_IDENTITY_SCALE;
	}
};

bool CATNodeControl::GetCATParentTM(TimeValue t, Matrix3& tmCATParent)
{
	ICATParent* pCATParent = GetCATParentTrans();
	if (pCATParent != NULL)
	{
		INode* pCATParentNode = pCATParent->GetNode();
		if (pCATParentNode != NULL)
		{
			tmCATParent = pCATParentNode->GetNodeTM(t);

			// CATParent provides scale by CATUnit so there
			// shouldn't be scale in the transform.
			if ((tmCATParent.GetIdentFlags() & SCL_IDENT) == 0)
				tmCATParent.NoScale();

			return true;
		}
	}
	return false;
}

void CATNodeControl::ScaleObjectSize(Point3 p3LocalScale)
{
	if (p3LocalScale == P3_IDENTITY_SCALE)
		return;

	// Disable Macrorecorder for this bit
	MacroRecorder::MacroRecorderDisable disableGuard;

	// When we are scaled, we want to scale our children position offset as well
	int nChildren = NumChildCATNodeControls();
	for (int i = 0; i < nChildren; i++)
	{
		CATNodeControl* pChild = GetChildCATNodeControl(i);
		if (pChild == NULL)
			continue;

		Matrix3 tmChildSetup = pChild->GetSetupMatrix();
		Point3 p3ChildOffset = tmChildSetup.GetTrans();
		p3ChildOffset *= p3LocalScale;
		tmChildSetup.SetTrans(p3ChildOffset);
		//pChild->SetSetupMatrix(tmChildSetup);
	}

	// And any arb bones
	int nArbBones = GetNumArbBones();
	for (int i = 0; i < nArbBones; i++)
	{
		CATNodeControl* pChild = tabArbBones[i];
		if (pChild == NULL)
			continue;

		Matrix3 tmChildSetup = pChild->GetSetupMatrix();
		Point3 p3ChildOffset = tmChildSetup.GetTrans();
		p3ChildOffset *= p3LocalScale;
		tmChildSetup.SetTrans(p3ChildOffset);
		pChild->SetSetupMatrix(tmChildSetup);
	}

	if (GetLengthAxis() != Z)
	{
		float tmp = p3LocalScale.x;
		p3LocalScale.x = p3LocalScale.z;
		p3LocalScale.z = tmp;
	}

	SetObjX(GetObjX() * p3LocalScale.x);
	SetObjY(GetObjY() * p3LocalScale.y);
	SetObjZ(GetObjZ() * p3LocalScale.z);
}

void CATNodeControl::GetAllChildCATNodeControls(Tab<CATNodeControl*>& dAllChilds)
{
	int nChildren = NumChildCATNodeControls();
	dAllChilds.SetCount(nChildren);
	for (int i = 0; i < nChildren; i++)
		dAllChilds[i] = GetChildCATNodeControl(i);
}

CatAPI::IBoneGroupManager* CATNodeControl::GetManager()
{
	CATControl* pParentCtrl = GetParentCATControl();
	if (pParentCtrl != NULL)
		return static_cast<CatAPI::IBoneGroupManager*>(pParentCtrl->GetInterface(BONEGROUPMANAGER_INTERFACE_FP));
	return NULL;
}

BOOL CATNodeControl::IsXReffed()
{
	INode* node = GetNode();
	Object *obj = node->GetObjectRef();
	if (obj->ClassID() == XREFOBJ_CLASS_ID)
		return TRUE;
	return FALSE;

}
ICATObject*		CATNodeControl::GetICATObject()
{
	INode* node = GetNode();
	if (node != NULL)
	{
		Object *obj = node->GetObjectRef();
		if (obj)
			return (ICATObject*)obj->FindBaseObject()->GetInterface(I_CATOBJECT);
	}
	return NULL;
}

void CATNodeControl::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	CATMode catmode = GetCATMode();

	// Invalidate local caches (probably unnecessary)
	InvalidateTransform();

	SetXFormPacket *ptr = (SetXFormPacket*)val;

	// Cache the incoming parents nodeTM
	Matrix3 tmOrigParent = ptr->tmParent;
	// First, get the appropriate tmParent
	Point3 p3ParentScale(P3_IDENTITY_SCALE);
	CalcParentTransform(t, ptr->tmParent, p3ParentScale);

	// of course, we only want to add in our SetupVal if we are NOT in SetupMode
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (catmode != SETUPMODE || (pLayerTrans != NULL && pLayerTrans->TestFlag(CLIP_FLAG_KEYFREEFORM | CLIP_FLAG_RELATIVE_TO_SETUPPOSE)))
	{
		// Apply our setup value.
		Interval ivDontCare = FOREVER;
		ApplySetupOffset(t, tmOrigParent, ptr->tmParent, p3ParentScale, ivDontCare);
	}
	else if (ptr->command != XFORM_SET) // in SetupMode
	{
		// When in setup mode, we always inherit from the CATParent,
		// so skip any setvalues that would affect it too.
		ICATParentTrans* pCATParentTrans = GetCATParentTrans();
		if (pCATParentTrans != NULL)
		{
			INode* pCATParentNode = pCATParentTrans->GetNode();
			if (pCATParentNode != NULL && pCATParentNode->Selected())
				return;
		}
	}

	if (!ApplySetValueLocks(t, ptr, catmode))
		return;

	switch (ptr->command)
	{
	case XFORM_MOVE:
	{
		if (TestCCFlag(CNCFLAG_IMPOSE_POS_LIMITS)) {
			DbgAssert(0); // Fix this...

			// Calculate our current local offset from our parent
			tmBoneLocal = tmBoneWorld * Inverse(ptr->tmParent);

			// Convert the incoming rotation to local space
			ptr->p = VectorTransform(ptr->tmAxis*Inverse(ptr->tmParent), ptr->p);
			ptr->tmAxis.IdentityMatrix();
			ptr->tmParent.IdentityMatrix();

			Point3 pos_total;
			if (method == CTRL_RELATIVE) {
				pos_total = tmBoneLocal.GetTrans() + ptr->p;
			}
			else {
				pos_total = (ptr->p * Inverse(ptr->tmParent));
			}
			float new_x = min(max(pos_total.x, pos_limits_neg.x), pos_limits_pos.x);
			float new_y = min(max(pos_total.y, pos_limits_neg.y), pos_limits_pos.y);
			float new_z = min(max(pos_total.z, pos_limits_neg.z), pos_limits_pos.z);

			float threshold = 0.01f; // degrees
			if ((fabs((float)(new_x - pos_total.x)) > threshold) ||
				(fabs((float)(new_y - pos_total.y)) > threshold) ||
				(fabs((float)(new_z - pos_total.z)) > threshold)) {

				ptr->p.x -= (pos_total.x - new_x);
				ptr->p.y -= (pos_total.y - new_y);
				ptr->p.z -= (pos_total.z - new_z);
			}
		}

		// First, we handle the case of the stretchy bones....
		CATNodeControl* pParent = GetParentCATNodeControl();
		bool bIStretch = IsStretchy(catmode);
		bool bLocalPosLocked = (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS) && catmode == SETUPMODE) ||
			(TestCCFlag(CNCFLAG_LOCK_LOCAL_POS) && catmode != SETUPMODE);

		// We do not want ArbBones affecting the regular hierarchy
		// Prevent Arb stretching from propagating to regular bones
		bool bParStretch = false;
		bool bArbStretch = false;
		if (pParent != NULL)
		{
			if (ClassID() != ARBBONETRANS_CLASS_ID ||
				pParent->ClassID() == ARBBONETRANS_CLASS_ID)
			{
				bArbStretch = true;
				bParStretch = pParent->IsStretchy(catmode);
			}
		}

		// If stretchy behavior is desired
		if (bIStretch || bParStretch)
		{
			SetValueMoveStretch(t, ptr, bIStretch, bParStretch, tmOrigParent, pParent);

			// We've may have stretched to reach the position indicated.
			// This will happen if either the parent stretches, or
			// our local position can be modified.
			// In these cases, we can simply call this done.
			if (bParStretch || !bLocalPosLocked)
				return;
		}

		// If our position is locked, try to rotate the
		// parent to meet it (this causes no actual stretching)
		if (bLocalPosLocked && pParent != NULL && ClassID() != PALMTRANS2_CLASS_ID)
		{
			// Do not let ArbBones stretching affect regular hierarchy bones
			if (bArbStretch)
				SetValueMoveStretch(t, ptr, bIStretch, true, tmOrigParent, pParent);
			return;
		}
		break;
	}
	// based on the incoming rotation our children will be forced to move a certain amount;
	// here we figure out how much or childre will be translated, and if any of themare pinned,
	// we calculate the translation that we need tomake to keep our locked children in thier
	// current world space locations.
	case XFORM_ROTATE:
	{
		if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT))
			return;
		if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS))
			ptr->localOrigin = TRUE;

		if (ccflags&(CNCFLAG_IMPOSE_ROT_LIMITS)) {
			// Calculate our current local offset rotation from our parent
			tmBoneLocal = tmBoneWorld * Inverse(ptr->tmParent);
			float ang_initial[3];
			MatrixToEuler(tmBoneLocal, ang_initial, EULERTYPE_XYZ);

			// Convert the incoming rotation to local space
			ptr->aa.axis = Normalize(VectorTransform(ptr->tmAxis*Inverse(ptr->tmParent), ptr->aa.axis));
			ptr->tmAxis.IdentityMatrix();
			ptr->tmParent.IdentityMatrix();

			// Because when we convert between angle axis and Euler angles we will lose
			// important info like number of revolutions, and even if a rotation is over
			// 180 degrees, the Euler will not recored this and actually invert the rotation
			// to the shortest arc on the rotational sphere. Here we convert a fraction of the
			// Rotation to Euler, and then multiply that Euler by the inverse of the fraction.
			float ang_total[3];
			ptr->aa.angle = min(max(ptr->aa.angle, -TWOPI), TWOPI);
			Quat quat;
			AngAxis ax1 = ptr->aa;
			ax1.angle *= 1.0 / 4.0;
			quat.Set(ax1);
			float ang_temp[3];
			QuatToEuler(quat, ang_temp, EULERTYPE_XYZ);
			ang_total[X] = ang_initial[X] + (ang_temp[X] * 4.0f);
			ang_total[Y] = ang_initial[Y] + (ang_temp[Y] * 4.0f);
			ang_total[Z] = ang_initial[Z] + (ang_temp[Z] * 4.0f);

			float restricted_x_angle = DegToRad(min(max(RadToDeg(ang_total[X]), rot_limits_neg.x), rot_limits_pos.x));
			float restricted_y_angle = DegToRad(min(max(RadToDeg(ang_total[Y]), rot_limits_neg.y), rot_limits_pos.y));
			float restricted_z_angle = DegToRad(min(max(RadToDeg(ang_total[Z]), rot_limits_neg.z), rot_limits_pos.z));

			float threshold = 0.001f; // radians
			if ((fabs(restricted_x_angle - ang_total[X]) > threshold) ||
				(fabs(restricted_y_angle - ang_total[Y]) > threshold) ||
				(fabs(restricted_z_angle - ang_total[Z]) > threshold)) {

				quat.Identity();
				float ang[3];
				ang[X] = (ang_initial[X] - restricted_x_angle);
				ang[Y] = (ang_initial[Y] - restricted_y_angle);
				ang[Z] = (ang_initial[Z] - restricted_z_angle);
				EulerToQuat(ang, quat, EULERTYPE_XYZ);

				ptr->q = quat;
				ptr->aa.Set(quat);
			}
		}

		break;
	}
	case XFORM_SET:
	{
		if (catmode == SETUPMODE)
		{
			// If we have scale, remove it and resize the object instead
			if ((ptr->tmAxis.GetIdentFlags()&SCL_IDENT) == 0)
			{
				// Remove the scale from the setvalue, and apply separately
				Point3 p3Scale;
				Point3* p3TmAxisRows = (Point3*)ptr->tmAxis.GetAddr();
				p3Scale[0] = p3TmAxisRows[0].LengthUnify();
				p3Scale[1] = p3TmAxisRows[1].LengthUnify();
				p3Scale[2] = p3TmAxisRows[2].LengthUnify();

				if (!TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL))
					ScaleObjectSize(p3Scale);

				ptr->tmAxis.SetIdentFlags(SCL_IDENT);
			}
		}
		else
		{

			// Calculate the scale in the tmAxis, and re-apply
			// it after the Orthogonalize call.
			Point3 p3Scale;
			Point3* p3TmAxisRows = (Point3*)ptr->tmAxis.GetAddr();
			p3Scale[0] = p3TmAxisRows[0].LengthUnify();
			p3Scale[1] = p3TmAxisRows[1].LengthUnify();
			p3Scale[2] = p3TmAxisRows[2].LengthUnify();

			// This should be called Ortho-normalize
			ptr->tmAxis.Orthogonalize();

			// Remove the parents scale.  This is necessary
			// because the tmParent has the scale removed from it
			p3Scale = p3Scale / p3ParentScale;
			// Re-apply the scale.
			ptr->tmAxis.PreScale(p3Scale);
		}
		break;
	}
	case XFORM_SCALE:
	{
		if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL))
			return;
		if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS))
			ptr->localOrigin = TRUE;

		// In SETUP mode, we resize the object, not scale
		if (catmode == SETUPMODE && !TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL))
		{
			Matrix3 tmScaledAxis = ptr->tmAxis;
			//tmScaledAxis.Scale(ptr->p);

			// Put the scale relative to ourselves
			Matrix3 tmScaleOffset = tmScaledAxis * Inverse(GetNodeTM(t));
			tmScaleOffset = ScaleMatrix(ptr->p) * tmScaleOffset;
			// Find the scale to take us from local to scale axis
			Point3 p3LocalScale;
			tmScaleOffset.SetTrans(Point3::Origin);
			p3LocalScale[0] = tmScaleOffset.GetColumn(0).Length();
			p3LocalScale[1] = tmScaleOffset.GetColumn(1).Length();
			p3LocalScale[2] = tmScaleOffset.GetColumn(2).Length();

			ScaleObjectSize(p3LocalScale);
			return;
		}
		break;
	}
	}

	// If we are Scaling in SetupMode, don't call SetWorldTransform
	if (ptr->command != XFORM_SCALE || catmode != SETUPMODE)
		SetWorldTransform(t, tmOrigParent, p3ParentScale, ptr, commit, method);
}

bool CATNodeControl::ApplySetValueLocks(TimeValue t, SetXFormPacket * ptr, int iCATMode)
{
	switch (ptr->command)
	{
	case XFORM_SCALE:
	{
		// Don't process this
		if (iCATMode == SETUPMODE)
		{
			if (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL))
				return false;
		}
		else
		{
			if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL))
				return false;
		}
		break;
	}
	case XFORM_ROTATE:
	{
		if (iCATMode == SETUPMODE)
		{
			if (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT))
				return false;
		}
		else
		{
			if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT))
				return false;
		}
		break;
	}
	case XFORM_MOVE:
	{
		// A move can usually be handled by rotating the parent if we are locked
		if ((iCATMode == SETUPMODE && TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS)) ||
			(iCATMode != SETUPMODE && TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)))
		{
			return GetParentCATNodeControl() != NULL;
		}
		break;
	}
	case XFORM_SET:
	{
		// Ignore all locks when keying for freeform.
		CATClipMatrix3* pLayerTrans = GetLayerTrans();
		if (pLayerTrans == NULL || pLayerTrans->TestFlag(CLIP_FLAG_KEYFREEFORM))
			return true;

		// TODO (Later).
		// The below code to remove positional offset _Should_
		// not need to call GetNodeTM.  Investigate using direct
		// calls to GetLayerTrans or something similar to investigate.
		Matrix3 tmCurrent = GetNodeTM(t);
		Matrix3 tmInvParent = Inverse(ptr->tmParent);
		Matrix3 tmLocal = tmCurrent * tmInvParent;
		Matrix3 tmNewLocal = ptr->tmAxis * tmInvParent;
		if ((iCATMode == SETUPMODE && TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS)) ||
			(iCATMode != SETUPMODE && TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)))
		{
			tmNewLocal.SetTrans(tmLocal.GetTrans());
			ptr->tmAxis = tmNewLocal * ptr->tmParent;
		}
		if ((iCATMode == SETUPMODE && TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT)) ||
			(iCATMode != SETUPMODE && TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)))
		{
			tmLocal.SetTrans(tmNewLocal.GetTrans());
			ptr->tmAxis = tmLocal * ptr->tmParent;
		}
	}
	}
	return true;
}

void CATNodeControl::SetValueMoveStretch(TimeValue t, SetXFormPacket * ptr, bool bIStretch, bool bParStretch, const Matrix3 &tmOrigParent, CATNodeControl* pParent)
{
	// Get our current transform
	Matrix3 tmCurrent = GetNodeTM(t);

	// First, we want to calculate the position offset we want.
	Point3 p3Initial = tmCurrent.GetTrans();
	Point3 p3WorldOffset = ptr->tmAxis.VectorTransform(ptr->p);
	Point3 p3Final = p3Initial + p3WorldOffset;

	// Now, offset tmCurrent to the pivot point.
	Matrix3 tmChild = tmCurrent;
	ApplyChildOffset(t, tmChild);

	// If we are going to change our orientation we cache our childrens TM:
	Tab<Matrix3> dtmChildWorld;
	int nChildren = 0;
	if (bIStretch)
	{
		nChildren = NumChildCATNodeControls();
		dtmChildWorld.SetCount(nChildren);
		for (int i = 0; i < nChildren; i++)
		{
			CATNodeControl* pChild = GetChildCATNodeControl(i);
			if (pChild != NULL)
				dtmChildWorld[i] = pChild->GetNodeTM(t);
		}
	}

	// Now we can modify our immediate parent
	if (bParStretch)
	{
		// If the parent is responsible for setting our position (it offsets us),
		// we'll need to modify its transform to allow us to reach the new pos
		const Point3& p3ParentPos = tmOrigParent.GetTrans();
		bParStretch = (p3ParentPos != ptr->tmParent.GetTrans()) == TRUE;
		if (bParStretch)
		{
			Matrix3 tmParent = pParent->GetNodeTM(t);
			pParent->CalcPositionStretch(t, tmParent, p3ParentPos, p3Initial, p3Final);
		}
	}

	if (bIStretch)
	{
		// Now, we apply the same sort of rotation to ourselves
		// This should counter-rotate the affect our parent had,
		// so we end up pointing back at the original tip position.
		CalcPositionStretch(t, tmCurrent, tmChild.GetTrans(), p3Initial, p3Final);

		// Finally, restore our children to their original transforms
		for (int i = 0; i < nChildren; i++)
		{
			CATNodeControl* pChild = GetChildCATNodeControl(i);
			if (pChild != NULL)
				pChild->SetNodeTM(t, dtmChildWorld[i]);
		}
	}
	// If we have modified our parent, we need to counter-rotate ourselves
	else if (bParStretch)
	{
		tmCurrent.SetTrans(p3Final);
		SetNodeTM(t, tmCurrent);
	}
	return;
}

void CATNodeControl::CalcPositionStretch(TimeValue t, Matrix3& tmCurrent, Point3 p3Pivot, const Point3& p3Initial, const Point3& p3Final)
{
	// Rotate us so that initial becomes parallel to final
	Point3 p3ToInitial = p3Pivot - p3Initial;

	// If our parent has no offset, this vector has 0 length, and
	// we cannot compute the rotation.  This happens with the hub & palm
	DbgAssert(p3ToInitial != Point3::Origin);
	if (p3ToInitial == Point3::Origin)
		return;

	Point3 p3ToFinal = p3Pivot - p3Final;
	float dInitLength = p3ToInitial.LengthUnify();
	float dFinalLength = p3ToFinal.LengthUnify();

	// Calculate the WORLD rotation to apply
	Point3 p3Axis = CrossProd(p3ToInitial, p3ToFinal);
	p3Axis.Unify();
	float dAngle = acos(min(1.0f, DotProd(p3ToInitial, p3ToFinal)));
	Matrix3 tmRot;
	tmRot.SetAngleAxis(p3Axis, dAngle);

	// Our scale to reach the appropriate spot.
	float dScale = dFinalLength / dInitLength;

	// Apply the transform
	tmCurrent.SetTrans((tmCurrent.GetTrans() - p3Pivot) * dScale);
	tmCurrent = tmCurrent * tmRot;
	tmCurrent.SetTrans(tmCurrent.GetTrans() + p3Pivot);

	// Set in our transform
	SetNodeTM(t, tmCurrent);

	// Set the scale, but only if
	// "Manip Causes Stretching" is enabled.
	CATMode iCATMode = GetCATMode();
	if (IsStretchy(iCATMode))
	{
		Point3 p3Scale(1, 1, 1);
		p3Scale[GetLengthAxis()] = dScale;

		if (iCATMode == SETUPMODE)
			ScaleObjectSize(p3Scale);
		else
		{
			CATClipMatrix3* pLayerTrans = GetLayerTrans();
			if (pLayerTrans != NULL)
			{
				// We attempt to directly set the scale, this
				// avoids float errors (which are problematic
				// at lower scales).
				Control* pScaleCtrl = pLayerTrans->GetScaleController();
				if (pScaleCtrl != NULL)
				{
					ScaleValue svScale(p3Scale);
					pScaleCtrl->SetValue(t, &svScale, 1, CTRL_RELATIVE);
				}
				else
				{
					// No dice, set scale via matrix3
					SetXFormPacket scalePacket(p3Scale, TRUE, tmCurrent);
					pLayerTrans->SetValue(t, &scalePacket, 1, CTRL_RELATIVE);
				}
			}
		}
	}
}

#ifdef FBIK

// FBIK in CAT3
// With regular evaluation
void CATNodeControl::FBIKInit() {
	Matrix3 tmLocal = tmBoneWorld * Inverse(tmBoneParent);

	// If our positoin is locked
	//	Point3 idealpos = (tmSetupModeLocal.GetTrans() * tmBoneParent) + tmBoneParent.GetTrans();
}

void CATNodeControl::FBIKPullBones(TimeValue t, CATNodeControl *src, Ray force)
{
	// Given the force that has been applied to us, so we rotate, or do we simply pass it on to our parent.
	Point3	this_bone_to_child = force.p - tmBoneWorld.GetTrans();
	Point3	this_bone_to_child_target = (force.p + force.dir) - tmBoneWorld.GetTrans();
	float	this_bone_to_child_target_length = Length(this_bone_to_child_target);
	this_bone_to_child_target = Normalize(this_bone_to_child_target);

	AngAxis ax(Normalize(CrossProd(this_bone_to_child, this_bone_to_child_target)), -acos(DotProd(Normalize(this_bone_to_child), this_bone_to_child_target)));
	Point3 vec = this_bone_to_child_target * (this_bone_to_child_target_length - Length(this_bone_to_child));

	/*	for(i = 0; i < NumChildCATControls(); i++){
	CATControl *child = GetChildCATControl(i);
	if(child && ){

	}
	}
	*/
}

// This function could get called right at the very end, and we could use CATMessage
void CATNodeControl::FBIKUpdateGlobalTM(TimeValue t)
{
	GetParentBoneScaleAndChildParent(t);

	tmBoneWorld = tmBoneLocal * tmBoneParent;

	tmChildParent = tmBoneWorld;
	// During a clone, we are asked to evaluate before the pointers are patched. We must always check for the catparenttrans
	if (mpCATParentTrans)	tmChildParent.PreTranslate(GetBoneLengthVec());

	/*	///////////////////////////////////////////////////
	// Now make sure our children are up to date
	int numchildren = node->NumberOfChildren();
	for(int i=0; i<numchildren; i++){
	INode *child = node->GetChildNode(i);

	CATNodeControl	*ichild = (CATNodeControl*)child->GetTMController()->GetInterface(I_CATNODECONTROL);
	if(ichild){
	ichild->FBIKUpdateGlobalTM(t);
	}
	}
	*/
}

#endif

void CATNodeControl::CalcWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid)
{
	if (GetCATMode() != SETUPMODE)
	{
		CATClipMatrix3* pLayerTrans = GetLayerTrans();
		if (pLayerTrans != NULL)
			pLayerTrans->GetTransformation(t, tmOrigParent, tmWorld, ivLocalValid, p3ParentScale, p3LocalScale);
	}
}

void CATNodeControl::SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method)
{
	CATClipMatrix3* pLayerTransform = GetLayerTrans();
	if (pLayerTransform)
	{
		Matrix3 tmOrigParentFlipped = tmOrigParent;
		if (GetCATMode() == SETUPMODE && pLayerTransform->TestFlag(CLIP_FLAG_SETUP_FLIPROT_FROM_CATPARENT))
			tmOrigParentFlipped.PreRotateY(PI);
		pLayerTransform->SetValue(t, tmOrigParentFlipped, p3ParentScale, *packet, commit, method);
	}
}

void CATNodeControl::ImposeLimits(TimeValue /*t*/, const Matrix3 &tmBoneParent, Matrix3 &tmBoneWorld, Point3& p3ScaleLocal, Interval& /*ivValid*/)
{
	if (!(ccflags&(CNCFLAG_IMPOSE_POS_LIMITS | CNCFLAG_IMPOSE_ROT_LIMITS | CNCFLAG_IMPOSE_SCL_LIMITS)))
		return;

	Matrix3 tmBoneLocal = tmBoneWorld * Inverse(tmBoneParent);

	AffineParts parts;
	decomp_affine(tmBoneLocal, &parts);

	if (ccflags&CNCFLAG_IMPOSE_POS_LIMITS)
	{
		parts.t.x = min(max(pos_limits_neg.x, parts.t.x), pos_limits_pos.x);
		parts.t.y = min(max(pos_limits_neg.y, parts.t.y), pos_limits_pos.y);
		parts.t.z = min(max(pos_limits_neg.z, parts.t.z), pos_limits_pos.z);
	}

	if (ccflags&CNCFLAG_IMPOSE_ROT_LIMITS)
	{
		float ang[3];
		float temp[3];
		QuatToEuler(parts.q, ang, EULERTYPE_XYZ, TRUE);
		memcpy(temp, ang, sizeof(float) * 3);

		ang[0] = min(DegToRad(rot_limits_pos.x), max(ang[0], DegToRad(rot_limits_neg.x)));
		ang[1] = min(DegToRad(rot_limits_pos.y), max(ang[1], DegToRad(rot_limits_neg.y)));
		ang[2] = min(DegToRad(rot_limits_pos.z), max(ang[2], DegToRad(rot_limits_neg.z)));
		if ((temp[X] != ang[X]) || (temp[Y] != ang[Y]) || (temp[Z] != ang[Z])) {
			EulerToQuat(ang, parts.q, EULERTYPE_XYZ);
		}
	}

	if (ccflags&CNCFLAG_IMPOSE_SCL_LIMITS)
	{
		p3ScaleLocal.x = min(max(scl_limits_min.x, p3ScaleLocal.x), scl_limits_max.x);
		p3ScaleLocal.y = min(max(scl_limits_min.y, p3ScaleLocal.y), scl_limits_max.y);
		p3ScaleLocal.z = min(max(scl_limits_min.z, p3ScaleLocal.z), scl_limits_max.z);
	}

	// Reset our transfomr and re-apply the new local offsets
	tmBoneWorld = tmBoneParent;
	tmBoneWorld.PreTranslate(parts.t);
	PreRotateMatrix(tmBoneWorld, parts.q);
}

void CATNodeControl::ISetMaxPositionLimits(Point3 p) {
	BOOL newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; };
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));
	pos_limits_pos = p;
	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
	UpdateUI();
}
void CATNodeControl::ISetMinPositionLimits(Point3 p) {
	BOOL newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; };
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));
	pos_limits_neg = p;
	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
	UpdateUI();
}
void CATNodeControl::ISetMaxRotationLimits(Point3 p) {
	BOOL newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; };
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));
	rot_limits_pos = p;
	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
	UpdateUI();
}
void CATNodeControl::ISetMinRotationLimits(Point3 p) {
	BOOL newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; };
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));
	rot_limits_neg = p;
	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
	UpdateUI();
}
void CATNodeControl::ISetMinScaleLimits(Point3 p) {
	BOOL newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; };
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));
	scl_limits_min = p;
	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
	UpdateUI();
}
void CATNodeControl::ISetMaxScaleLimits(Point3 p) {
	BOOL newundo = false;
	if (!theHold.Holding()) { theHold.Begin();	newundo = TRUE; };
	if (theHold.Holding()) theHold.Put(new SetLimitRestore(this));
	scl_limits_max = p;
	if (theHold.Holding() && newundo) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
	UpdateUI();
}

//////////////////////////////////////////////////////////////////////////

void CATNodeControl::SetNode(INode* pNode)
{
	HoldData(mpNode);
	mpNode = pNode;
}

void CATNodeControl::RefAdded(RefMakerHandle rm)
{
	INode* pNode = dynamic_cast<INode*>(rm);
	if (pNode != NULL && mpNode != pNode)
	{
		DbgAssert(mpNode == NULL);
		SetNode(pNode);
	}
}

// This class searches a dependent tree to find multiple
// instances of a CATClipMatrix3 reference the same source controller.
class NodeFinder : public DependentEnumProc
{
	bool mNodeFound;		// Set to true if an instance is found.

public:
	NodeFinder()
		: mNodeFound(false)
	{
	}

	int proc(ReferenceMaker *rmaker)
	{
		// If we find a node, job done.
		if (rmaker != NULL && rmaker->SuperClassID() == BASENODE_CLASS_ID)
		{
			mNodeFound = true;
			return DEP_ENUM_HALT;
		}
		return DEP_ENUM_CONTINUE;
	}

	// After enumeration, call this function to see if another
	// instance of CATClipValue referenced this class
	bool NodeWasFound() { return mNodeFound; }
};

void CATNodeControl::RefDeleted()
{
	// If we don't have a node, we don't care...
	if (GetNode() == NULL)
		return;

	// If we no longer have an INode referencing us directly
	// Remove and re-parent our children (if necessary).
	if (FindReferencingClass<INode>(this, 1) == NULL)
	{
		// Unlinking -may- delete this class.
		MaxAutoDeleteLock lock(this);
		UnlinkCATHierarchy();
	}
}

RefResult CATNodeControl::AutoDelete()
{
	if (mpNode != NULL)
	{
		// We should only be here if we are NOT holding
		DbgAssert(!theHold.Holding() || theHold.IsSuspended());
		DbgAssert(TestAFlag(A_LOCK_TARGET) == 0);
		MaxAutoDeleteLock lock(this, false);
		UnlinkCATHierarchy();
	}

	return CATControl::AutoDelete();
}

void CATNodeControl::UnlinkCATHierarchy()
{
	// We've been deleted.  Remove ourselves
	// from the equations
	SetNode(NULL);

	// Remove all pointers to ArbBones, and vice-versa
	int iNumArbBones = GetNumArbBones();
	for (int i = iNumArbBones - 1; i >= 0; --i)
	{
		CATNodeControl* pArbBone = GetArbBone(i);
		if (pArbBone != NULL)
			RemoveArbBone(pArbBone, FALSE);
	}
	DbgAssert(GetNumArbBones() == 0);

	// Disable any arb controllers.  After we are deleted,
	// there is no way for the CATParentTrans to access it
	// cleanly.
	for (int i = 0; i < tabArbControllers.Count(); i++)
	{
		if (tabArbControllers[i] != NULL)
			tabArbControllers[i]->SetCATParentTrans(NULL);
	}
	tabArbControllers.SetCount(0);

	// Re-link all our children...
	int iNumChildren = NumChildCATNodeControls();
	for (int i = iNumChildren - 1; i >= 0; --i)
	{
		// NULL child is legal
		CATNodeControl* pChild = GetChildCATNodeControl(i);
		if (pChild == NULL)
			continue;

		// Force relink to their new parent
		pChild->SetParentCATNodeControl(NULL);

		if (!GetCOREInterface10()->SceneResetting())
		{
			// Re-link INode pointers
			pChild->LinkParentChildNodes();
		}
	}

	// If we are an ArbBone, ensure we are removed from our parent
	if (ClassID() == ARBBONETRANS_CLASS_ID)
	{
		CATNodeControl* pParent = GetParentCATNodeControl();
		if (pParent != NULL)
			pParent->RemoveArbBone(this, false);
	}
	// If we are NOT an arb bone, unlink any manager (Tail/Limb/Spine|Data) we might have
	else
		UnlinkBoneManager();

	// Now remove our own pointers.
	SetParentCATNodeControl(NULL);

	DestructCATControlHierarchy();
}

//* TO DELETE ALL OF THE BELOW
#include "LimbData2.h"
void NotifyBoneDelete(CATNodeControl* pDeletedBone, CATControl* manager)
{
	DbgAssert(_T("FIX THIS CODE HERE"));
	// test if there are any valid bones left.
	// If there are, leave.
	int nBones = manager->NumChildCATControls();
	for (int i = 0; i < nBones; i++)
	{
		CATControl* pBone = manager->GetChildCATControl(i);
		if (pBone != NULL && pBone != pDeletedBone)
			return;
	}
	// There were no valid bones - unlink the manager
	if (manager != NULL)
	{
		MaxAutoDeleteLock lock(manager);
		// Gosh I'm glad this code isn't necessary in SimCity...
		CATNodeControl* pManagerNode = dynamic_cast<CATNodeControl*>(manager);
		if (pManagerNode != NULL)
			pManagerNode->UnlinkCATHierarchy();
		else
			manager->DestructCATControlHierarchy();
	}
}
void CATNodeControl::UnlinkBoneManager()
{
	IBoneGroupManagerFP* pManager = static_cast<IBoneGroupManagerFP*>(GetManager());
	if (pManager != NULL)
	{
		pManager->NotifyBoneDelete(this);
	}
}

//* Stop deleting things here

void CATNodeControl::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	Matrix3 tmOrigParent = *(Matrix3*)val;
	Matrix3& tmWorldTransform = *(Matrix3*)val;

	// Evaluation is temporarily locked during rig load.
	int isUndoing = 0;
	if (CATEvaluationLock::IsEvaluationLocked() ||
		(IsEvaluationBlocked() && (!theHold.Restoring(isUndoing) && isUndoing)))
	{
		// don't say the value is valid
		*(Matrix3*)val = tmBoneWorld;
		return;
	}

	// Our world transform starts with the parent matrix
	// No matter what happens, we have to work from a consistent matrix
	Point3 p3ParentScale(1, 1, 1);
	if (method == CTRL_RELATIVE)
		CalcParentTransform(t, tmWorldTransform, p3ParentScale);
	else	// Does this ever happen?
		tmWorldTransform.IdentityMatrix();

	// Cache the parent matrix before any transforms are applied
	mLocalValid = FOREVER;
	Matrix3 tmBoneParent = tmWorldTransform;

	// Apply our setup value.
	Point3 p3LocalScale = P3_IDENTITY_SCALE;
	mLocalValid = FOREVER;		// TODO: Fix local caching
	ApplySetupOffset(t, tmOrigParent, tmWorldTransform, p3LocalScale, mLocalValid);

	//if (!mLocalValid.InInterval(t))
	{
		// Next, we add our local transformation to it.
		CalcWorldTransform(t, tmOrigParent, p3ParentScale, tmWorldTransform, p3LocalScale, mLocalValid);

		// Impose any limits on the transform
		ImposeLimits(t, tmBoneParent, tmWorldTransform, p3LocalScale, mLocalValid);

		// Cache our local matrix.
		tmBoneLocal = tmWorldTransform * Inverse(tmBoneParent);
	}
	//else
	//	tmWorldTransform = tmBoneLocal * tmWorldTransform;

	// Cache our final matrix
	tmBoneWorld = tmWorldTransform;

	// Cache our scale
	mp3ScaleWorld = p3LocalScale * p3ParentScale;

	// Now add back in our scale.  We can't add this to the cache,
	// because it includes the parent scale we extracted in CalcInheriteValues
	tmWorldTransform.PreScale(p3LocalScale * p3ParentScale);

	// cache our initial parent matrix
	tmLastEvalParent = tmOrigParent;

	// Set the validity
	valid &= mLocalValid;
	// Cache the world validity of this bone.
	mWorldValid = valid;
}

BOOL CATNodeControl::GetRelativeToSetupMode()
{
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (pLayerTrans != NULL)
		return pLayerTrans->TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE);

	return FALSE;
}

void CATNodeControl::SetRelativeToSetupMode(BOOL isRelative)
{
	// Enable undo for the following actions.
	HoldActions hold(IDS_HLD_LYRFLAG);

	// This switch controls whether SetupTM is included or
	// excluded from this bones transformation stack
	Matrix3 tmSetup = GetSetupMatrix();

	// If relative is enabled, we are adding tmSetup
	// to the stack.  To keep the end result equivalent,
	// we compensate for adding tmSetup to the stack by
	// removing it from the animation ctrl (by inverting)
	Point3 p3SetupPos = tmSetup.GetTrans() * GetCATUnits();
	Quat qSetupRot(tmSetup);
	if (isRelative)
	{
		// Invert our values
		p3SetupPos *= -1;
		qSetupRot = qSetupRot.Inverse();
	}

	// Prevent creating keyframes
	SuspendAnimate();

	// Apply offset to every animation layer
	CATClipValue* pLayerTrans = GetLayerTrans();
	if (pLayerTrans != NULL)
	{
		for (int i = 0; i < pLayerTrans->GetNumLayers(); i++)
		{
			if (pLayerTrans->GetLayerMethod(i) == LAYER_ABSOLUTE)
			{
				SetXFormPacket pos(p3SetupPos);
				SetXFormPacket rot(qSetupRot, TRUE);
				Control* pCtrl = pLayerTrans->GetLayer(i);
				pCtrl->SetValue(0, &pos);
				pCtrl->SetValue(0, &rot);
			}
		}
	}

	ResumeAnimate();

	// Set the flag to control whether tmSetup is being applied
	if (pLayerTrans)
		pLayerTrans->SetFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE, isRelative);
}

BitArray*	CATNodeControl::GetSetupModeLocks() {
	BitArray *ba = new BitArray(3);
	if (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS)) ba->Set(0);
	if (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT)) ba->Set(1);
	if (TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL)) ba->Set(2);
	return ba;
};

void	CATNodeControl::SetSetupModeLocks(BitArray *locks) {
	SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS, (locks->GetSize() > 0) && (*locks)[0]);
	SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT, (locks->GetSize() > 1) && (*locks)[1]);
	SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL, (locks->GetSize() > 2) && (*locks)[2]);
	UpdateUI();
};

BitArray*	CATNodeControl::GetAnimationLocks() {
	BitArray *ba = new BitArray(3);
	if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)) ba->Set(0);
	if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)) ba->Set(1);
	if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) ba->Set(2);
	return ba;
};

void	CATNodeControl::SetAnimationLocks(BitArray *locks) {
	SetCCFlag(CNCFLAG_LOCK_LOCAL_POS, (locks->GetSize() > 0) && (*locks)[0]);
	SetCCFlag(CNCFLAG_LOCK_LOCAL_ROT, (locks->GetSize() > 1) && (*locks)[1]);
	SetCCFlag(CNCFLAG_LOCK_LOCAL_SCL, (locks->GetSize() > 2) && (*locks)[2]);

	UpdateUI();
};

void	CATNodeControl::SetSetupModeInheritance(BitArray *locks)
{
	// These flags are stored on the LayerTrans, for no particular reason...
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (pLayerTrans != NULL)
	{
		Matrix3 tmCurrent = GetNodeTM(0);

		BOOL Inh_Pos = FALSE, Inh_Rot = FALSE;
		if (locks->GetSize() > 0) Inh_Pos = (*locks)[0];
		if (locks->GetSize() > 1) Inh_Rot = (*locks)[1];

		pLayerTrans->SetFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT, Inh_Pos);
		pLayerTrans->SetFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT, !Inh_Pos);
		pLayerTrans->SetFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT, !Inh_Rot);

		Update();
		// If we are not in SetupMode, we shouldn't modify the current transform
		if (GetCATMode() == SETUPMODE)
		{
			// disables locks.
			BitArray* setupModeLocks = GetSetupModeLocks();
			BitArray removeLocks(3);
			removeLocks.ClearAll();
			SetSetupModeLocks(&removeLocks);

			// Set the node transform
			SetNodeTM(0, tmCurrent);

			// restore original locks
			SetSetupModeLocks(setupModeLocks);

			// Note, the memory for this array is not managed by anyone, so we need to free it
			delete setupModeLocks;
		}

		UpdateUI();
	}
}

BitArray*	CATNodeControl::GetSetupModeInheritance()
{
	BitArray *ba = new BitArray(3);	// TODO: Who releases this, huh?
	CATClipMatrix3* pLayerTrans = GetLayerTrans();
	if (pLayerTrans != NULL)
	{
		if (!pLayerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT))
			ba->Set(0);
		if (!pLayerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT))
			ba->Set(1);
	}
	return ba;
}

void	CATNodeControl::SetAnimModeInheritance(BitArray *locks) {
	INode* pMyNode = GetNode();
	if (pMyNode != NULL)
	{
		// To prevent the pop, we get the transform now, and set it back afterwards
		TimeValue t = GetCOREInterface()->GetTime();
		Matrix3 tmCurrentWorld = pMyNode->GetNodeTM(t);

		// We don't specifically need to hold our ccflags.
		// However, this is the easiest way to get a callback
		// to update our UI on undo.
		HoldData(ccflags, this);

		SetCCFlag(CNCFLAG_INHERIT_ANIM_POS, (locks->GetSize() > 0) && (*locks)[0]);
		SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT, (locks->GetSize() > 1) && (*locks)[1]);
		SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL, (locks->GetSize() > 2) && (*locks)[2]);

		// Mirror the flags onto the clip, this allows them to inherit
		// from the layer transform gizmo if not inheriting from the parent
		CATClipMatrix3* pLayerTrans = GetLayerTrans();
		// Set Keying mode.  This disables the transform locks.
		KeyFreeformMode keyMode(pLayerTrans);
		if (pLayerTrans != NULL)
		{
			pLayerTrans->SetFlag(CLIP_FLAG_INHERIT_POS, TestCCFlag(CNCFLAG_INHERIT_ANIM_POS));
			pLayerTrans->SetFlag(CLIP_FLAG_INHERIT_ROT, TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT));
			pLayerTrans->SetFlag(CLIP_FLAG_INHERIT_SCL, TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL));
		}

		Update();

		// Dont create keys
		SuspendAnimate();
		pMyNode->SetNodeTM(t, tmCurrentWorld);
		ResumeAnimate();

		ICATParentTrans* pCATParent = GetCATParentTrans();
		if (pCATParent != NULL)
			pCATParent->UpdateCharacter();
	}
}

BitArray*	CATNodeControl::GetAnimModeInheritance() {
	BitArray *ba = new BitArray(3);	// TODO: Who releases this, huh?
	if (TestCCFlag(CNCFLAG_INHERIT_ANIM_POS)) ba->Set(0);
	if (TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT)) ba->Set(1);
	if (TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL)) ba->Set(2);
	return ba;
};

INode*	CATNodeControl::GetMirrorBone()
{
	return (mpMirrorBone ? mpMirrorBone->GetNode() : NULL);
};

void	CATNodeControl::SetMirrorBone(INode* boneNode)
{
	CATNodeControl* pNewMirrorCtrl = NULL;
	if (boneNode != NULL)
		pNewMirrorCtrl = dynamic_cast<CATNodeControl*>(boneNode->GetTMController());

	// Cannot mirror yourself
	if (pNewMirrorCtrl == this)
		return;

	// If we already mirror this bone, no change
	if (pNewMirrorCtrl == mpMirrorBone)
		return;

	// We are going to change the value - init undo
	HoldActions hold(IDS_HLD_CATNODEPASTE);
	HoldData(mpMirrorBone);

	// cache old mirrorbone
	CATNodeControl* pOldMirror = mpMirrorBone;

	// Break the current link to the old mirror bone
	mpMirrorBone = NULL;
	if (pOldMirror != NULL && pOldMirror->GetMirrorBone() == GetNode())
		pOldMirror->SetMirrorBone(NULL);

	// Set to new mirror bone
	mpMirrorBone = pNewMirrorCtrl;
	if (pNewMirrorCtrl != NULL)
		pNewMirrorCtrl->SetMirrorBone(GetNode());

	UpdateUI();
};

void CATNodeControl::ApplyForce(Point3 &force, Point3 &force_origin, AngAxis &rotation, Point3 &rotation_origin, CATNodeControl *source)
{
	if (TestCCFlag(CCFLAG_FB_IK_LOCKED)) {
		// if we are locked, then basicly we need to send the force
		// back where it came from because we cannot absorb it.
		force = -force;
		rotation.Set(0.0f, 0.0f, 1.0f, 0.0f);
		force_origin = tmBoneWorld.GetTrans();
		// there is no point in propogating to our children
		return;
	}
	if (TestCCFlag(CCFLAG_FB_IK_BYPASS)) {
		// we are not taking part in the IK solution so just pretend we wern't here
		for (int i = 0; i < NumChildCATControls(); i++) {
			CATControl* child = GetChildCATControl(i);
			if (child && child != source)
				child->ApplyForce(force, force_origin, rotation, rotation_origin, this);
		}
		return;
	}
	///	INode* GetParentNode();

	// due to the incomming rotation, how much translation will we incur?
	Matrix3 tm = tmBoneWorld;
	tm.Translate(-rotation_origin);
	RotateMatrix(tm, rotation);
	tm.Translate(rotation_origin);
	Point3 pos_offset_due_to_rotation = tm.GetTrans() - tmBoneWorld.GetTrans();

	Point3 thisbone_to_force_origin = force_origin - tmBoneWorld.GetTrans();
	Point3 thisbone_to_force_target = (force_origin + force) - tmBoneWorld.GetTrans();

	AngAxis ax;
	if (force_origin != tmBoneWorld.GetTrans()) {
		// calculate the rotation that the force will rotate us.
		ax.angle = (float)acos(DotProd(thisbone_to_force_origin, thisbone_to_force_target));
		ax.axis = Normalize(CrossProd(thisbone_to_force_origin, thisbone_to_force_target));
	}
	else ax.Set(0.0f, 0.0f, 1.0f, 0.0f);

	float old_child_dist = Length(thisbone_to_force_origin);
	float new_child_dist = Length(thisbone_to_force_target);
	thisbone_to_force_target.Normalize();

	Point3 our_position_force = thisbone_to_force_target * (old_child_dist - new_child_dist);

	int numcatnodecontrols = 0;
	for (int i = 0; i < NumChildCATControls(); i++) {
		CATControl* child = GetChildCATControl(i);
		if (source != child && child->GetInterface(I_CATNODECONTROL)) {
			numcatnodecontrols++;

			//Matrix3 tmChild = ((CATNodeControl*)child)->GetNodeTM(t);
			//
			//	tmChild.Translate(-tmBoneWorld.GetTrans());
			//	RotateMatrix(tmChild, ax);
			//	tmChild.Translate(tmBoneWorld.GetTrans());
			Point3 childforce = our_position_force;

			//			((CATNodeControl*)child)->ApplyForce(childforce, ax, tmBoneWorld.GetTrans(), this);

			// deal with child force
			//			totalforce += childforce;

		}
	}
}

///////////////////////////////////////////////////////////////////////////
// UI Code
// This is a generic UI handler for CATBone rollouts
// Some bones may derrive thier own UI handlers form this
// one and add the extra functionality they require

// this class is the UI handler for CATNodeControl
class CATNodeControlSetupDlgCallBack
{
	HWND			hDlg;
	CATNodeControl	*bone;
	ICATObject		*iobj;

	ICustEdit		*edtName;
	IColorSwatch	*swchColour;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;

	float val;
	ISpinnerControl *spnX, *spnY, *spnZ;

	ICustButton	*btnAddArbBone;
	ICustButton *btnAddERN;

public:

	CATNodeControlSetupDlgCallBack();
	BOOL IsDlgOpen() { return (bone != NULL); }
	HWND GetHWnd() { return hDlg; }

	virtual void InitControls(HWND hWnd, CATNodeControl *owner);
	virtual void ReleaseControls();
	virtual void Update();

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void DeleteThis() { }
};

CATNodeControlSetupDlgCallBack::CATNodeControlSetupDlgCallBack() : btnPaste(NULL), val(0.0f), btnPasteMirrored(NULL), btnCopy(NULL)
{
	edtName = NULL;
	swchColour = NULL;

	spnZ = NULL;	spnX = NULL;	spnY = NULL;

	bone = NULL;
	iobj = NULL;
	btnAddArbBone = NULL;
	btnAddERN = NULL;
	hDlg = NULL;
}

void CATNodeControlSetupDlgCallBack::InitControls(HWND hWnd, CATNodeControl *owner)
{
	// Do not double create this
	DbgAssert(hDlg == NULL);

	hDlg = hWnd;
	bone = owner;
	DbgAssert(bone);

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	// Initialise custom Max controls
	edtName = GetICustEdit(GetDlgItem(hDlg, IDC_EDIT_NAME));
	edtName->SetText(bone->GetName().data());

	Color clr = bone->GetBonesRigColour();
	swchColour = GetIColorSwatch(GetDlgItem(hDlg, IDC_SWATCH_COLOR), clr, GetString(IDS_BONE_COLOR_SETUP));

	iobj = (ICATObject*)bone->GetObject()->GetInterface(I_CATOBJECT);
	// if the INode modifier stack has been collapsed, the this rollout won't be
	// displayed, because it is the ICATObject that will pop up this dlg in the first place
	DbgAssert(iobj);

	spnX = SetupFloatSpinner(hDlg, IDC_SPIN_X, IDC_EDIT_X, 0.0f, 1000.0f, owner->GetObjX());
	spnX->SetAutoScale();

	spnY = SetupFloatSpinner(hDlg, IDC_SPIN_Y, IDC_EDIT_Y, 0.0f, 1000.0f, owner->GetObjY());
	spnY->SetAutoScale();

	spnZ = SetupFloatSpinner(hDlg, IDC_SPIN_Z, IDC_EDIT_Z, 0.0f, 1000.0f, owner->GetObjZ());
	spnZ->SetAutoScale();

	// Copy button
	btnCopy = GetICustButton(GetDlgItem(hDlg, IDC_BTN_COPY));
	btnCopy->SetType(CBT_PUSH);
	btnCopy->SetButtonDownNotify(TRUE);
	btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyBone"), GetString(IDS_TT_COPYBONE)));
	btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

	// Paste button
	btnPaste = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE));
	btnPaste->SetType(CBT_PUSH);
	btnPaste->SetButtonDownNotify(TRUE);
	btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteBone"), GetString(IDS_TT_PASTERIG)));
	btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

	// btnPasteMirrored button
	btnPasteMirrored = GetICustButton(GetDlgItem(hDlg, IDC_BTN_PASTE_MIRRORED));
	btnPasteMirrored->SetType(CBT_PUSH);
	btnPasteMirrored->SetButtonDownNotify(TRUE);
	btnPasteMirrored->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteBoneMirrored"), GetString(IDS_TT_PASTERIGMIRROR)));
	btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

	if (iobj->CustomMeshAvailable()) {
		SET_CHECKED(hDlg, IDC_CHK_USECUSTOMMESH, iobj->UsingCustomMesh());
	}
	else EnableWindow(GetDlgItem(hDlg, IDC_CHK_USECUSTOMMESH), FALSE);

	btnAddArbBone = GetICustButton(GetDlgItem(hDlg, IDC_BTN_ADDARBBONE));
	btnAddArbBone->SetType(CBT_PUSH);
	btnAddArbBone->SetButtonDownNotify(TRUE);

	btnAddERN = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADDRIGGING));		btnAddERN->SetType(CBT_CHECK);		btnAddERN->SetButtonDownNotify(TRUE);
	btnAddERN->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddERN"), GetString(IDS_TT_ADDRIGOBJS)));

	if (bone->GetCATMode() != SETUPMODE) {
		btnCopy->Disable();
		btnPaste->Disable();
		btnPasteMirrored->Disable();
		btnAddArbBone->Disable();
	}
	else if (!bone->CanPasteControl())
	{
		btnPaste->Disable();
		btnPasteMirrored->Disable();
	}

	SET_TEXT(hWnd, IDC_STATIC_NAME, catCfg.Get(_T("ButtonTexts"), _T("lblName"), GetString(IDS_BTN_NAME1)));
	SET_TEXT(hWnd, IDC_CHK_USECUSTOMMESH, catCfg.Get(_T("ButtonTexts"), _T("chkUseCustomMesh"), GetString(IDS_BTN_CUSTMESH)));
	SET_TEXT(hWnd, IDC_STATIC_X, catCfg.Get(_T("ButtonTexts"), _T("spnX"), GetString(IDS_BTN_X)));
	SET_TEXT(hWnd, IDC_STATIC_Y, catCfg.Get(_T("ButtonTexts"), _T("spnY"), GetString(IDS_BTN_Y)));
	SET_TEXT(hWnd, IDC_STATIC_Z, catCfg.Get(_T("ButtonTexts"), _T("spnZ"), GetString(IDS_BTN_Z)));
	SET_TEXT(hWnd, IDC_GROUP_ARBBONES, catCfg.Get(_T("ButtonTexts"), _T("grpAddBodyPart"), GetString(IDS_BTN_ADDBODYPART)));

}

void CATNodeControlSetupDlgCallBack::ReleaseControls() {

	SAFE_RELEASE_EDIT(edtName);
	SAFE_RELEASE_COLORSWATCH(swchColour);

	SAFE_RELEASE_BTN(btnCopy);
	SAFE_RELEASE_BTN(btnPaste);
	SAFE_RELEASE_BTN(btnPasteMirrored);

	SAFE_RELEASE_SPIN(spnZ);
	SAFE_RELEASE_SPIN(spnX);
	SAFE_RELEASE_SPIN(spnY);

	SAFE_RELEASE_BTN(btnAddArbBone);
	SAFE_RELEASE_BTN(btnAddERN);
	ExtraRigNodes *ern = (ExtraRigNodes*)bone->GetInterface(I_EXTRARIGNODES_FP);
	DbgAssert(ern);
	ern->IDestroyERNWindow();

	bone = NULL;
	iobj = NULL;
	hDlg = NULL;
}

void CATNodeControlSetupDlgCallBack::Update()
{
	spnZ->SetValue(iobj->GetZ(), FALSE);
	spnX->SetValue(iobj->GetX(), FALSE);
	spnY->SetValue(iobj->GetY(), FALSE);
}

class EditRestore : public RestoreObj {
public:
	CATNodeControl	*bone;
	EditRestore(CATNodeControl *b) {
		bone = b;
		bone->SetAFlag(A_HELD);
	}
	void Restore(int isUndo) {
		bone->UpdateObjDim();
		if (isUndo) {
			bone->Update();
			bone->UpdateUI();
		}
	}
	void Redo() {
		bone->UpdateObjDim();
		bone->Update();
		bone->UpdateUI();
	}
	int Size() { return 1; }
	void EndHold() { bone->ClearAFlag(A_HELD); }
};

INT_PTR CALLBACK CATNodeControlSetupDlgCallBack::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:		InitControls(hWnd, (CATNodeControl*)lParam);		break;
	case WM_DESTROY:		ReleaseControls();									return FALSE;
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_CHK_USECUSTOMMESH:		iobj->SetUseCustomMesh(IS_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH));		break;
			}
			break;
		case BN_BUTTONUP: {
			switch (LOWORD(wParam)) {
			case IDC_BTN_ADDARBBONE:
			{
				HoldActions actions(IDS_HLD_ADDBONE);
				bone->AddArbBone();
			}
			break;
			case IDC_BTN_COPY:				CATControl::SetPasteControl(bone);													break;
			case IDC_BTN_PASTE: {	bone->PasteFromCtrl(CATControl::GetPasteControl(), false);	}	break;
			case IDC_BTN_PASTE_MIRRORED: {	bone->PasteFromCtrl(CATControl::GetPasteControl(), true);		}	break;
			case IDC_BTN_ADDRIGGING: {
				ExtraRigNodes* ern = (ExtraRigNodes*)bone->GetInterface(I_EXTRARIGNODES_FP);
				DbgAssert(ern);
				if (btnAddERN->IsChecked())
					ern->ICreateERNWindow(hWnd);
				else ern->IDestroyERNWindow();
				break;
			}
			}
		}
		break;
		}
		break;
	case WM_CUSTEDIT_ENTER:
	{
		switch (LOWORD(wParam))
		{
		case IDC_EDIT_NAME: {	TCHAR strbuf[128];	edtName->GetText(strbuf, 64);// max 64 characters
			TSTR name(strbuf);	bone->SetName(name);				break;
		}
		}
		break;
	}
	case CC_SPINNER_BUTTONDOWN: 	theHold.Begin();	break;
	case CC_SPINNER_CHANGE: {
		if (theHold.Holding()) {
			MaxReferenceMsgLock lockThis;
			theHold.Restore();
		}
		if (theHold.Holding() && !bone->TestAFlag(A_HELD)) theHold.Put(new EditRestore(bone));
		switch (LOWORD(wParam)) {
		case IDC_SPIN_X:	if (bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL)) { break; }	bone->SetObjX(spnX->GetFVal());	break;
		case IDC_SPIN_Y:	if (bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL)) { break; }	bone->SetObjY(spnY->GetFVal());	break;
		case IDC_SPIN_Z:	if (bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL)) { break; }	bone->SetObjZ(spnZ->GetFVal());	break;
		}
		//	iobj->Update();
		//	bone->UpdateObjDim();// This is now done by the object when it gets a change message
		bone->Update();
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
		break;
	}
	case CC_SPINNER_BUTTONUP: {
		if (theHold.Holding()) {
			if (HIWORD(wParam)) {
				switch (LOWORD(wParam)) {
				case IDC_SPIN_X:	theHold.Accept(GetString(IDS_HLD_OBJWID));	break;
				case IDC_SPIN_Y:	theHold.Accept(GetString(IDS_HLD_OBJDEPTH));	break;
				case IDC_SPIN_Z:	theHold.Accept(GetString(IDS_HLD_OBJLEN));	break;
				}
			}
			else theHold.Cancel();
		}
		break;
	}
	case CC_COLOR_CLOSE:
	case CC_COLOR_CHANGE:			bone->SetBonesRigColour(Color(swchColour->GetColor()));		break;
	case WM_NOTIFY:					break;
	default:						return FALSE;
	}
	return TRUE;
};

///////////////////////////////////////////////////////////////////////////
// UI Code
// This is a generic UI handler for CATBone rollouts
// Some bones may derrive thier own UI handlers form this
// one and add the extra functionality they require

// this class is the UI handler for CATNodeControl
class CATNodeControlHierarchyDlgCallBack
{
private:
	enum SPINNER_LIMITS {
		SPIN_X_MIN_POS,
		SPIN_Y_MIN_POS,
		SPIN_Z_MIN_POS,
		SPIN_X_MAX_POS,
		SPIN_Y_MAX_POS,
		SPIN_Z_MAX_POS,
		SPIN_X_MIN_ROT,
		SPIN_Y_MIN_ROT,
		SPIN_Z_MIN_ROT,
		SPIN_X_MAX_ROT,
		SPIN_Y_MAX_ROT,
		SPIN_Z_MAX_ROT,
		// For now, no scale limits are exposed.
		NUM_LIMITS
	};
	HWND			hDlg;
	CATNodeControl	*bone;
	CATClipMatrix3	*layerTrans;
	INode			*nextnode;

	float val;
	ISpinnerControl* spnLimits[NUM_LIMITS];

public:

	CATNodeControlHierarchyDlgCallBack();
	BOOL IsDlgOpen() { return (bone != NULL); }
	HWND GetHWnd() { return hDlg; }

	virtual void InitControls(HWND hWnd, CATNodeControl *owner);
	virtual void ReleaseControls();
	virtual void Update();

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void SetLimits(UINT wParam);

	virtual void DeleteThis() { }
};

CATNodeControlHierarchyDlgCallBack::CATNodeControlHierarchyDlgCallBack() : val(0.0f), nextnode(NULL)
{
	hDlg = NULL;
	bone = NULL;
	layerTrans = NULL;
	for (int i = 0; i < NUM_LIMITS; i++)	spnLimits[i] = NULL;
}

void CATNodeControlHierarchyDlgCallBack::InitControls(HWND hWnd, CATNodeControl *owner)
{
	// Do not double create this
	DbgAssert(hDlg == NULL);

	hDlg = hWnd;
	bone = owner;
	DbgAssert(bone);
	layerTrans = bone->GetLayerTrans();

	// Initialise the INI files so we can read button text and tooltips
	CatDotIni catCfg;

	ICustButton *btnBakeLayerSettings;
	btnBakeLayerSettings = GetICustButton(GetDlgItem(hWnd, IDC_BTN_BAKE_LAYER_SETTINGS));
	btnBakeLayerSettings->SetType(CBT_PUSH);
	btnBakeLayerSettings->SetButtonDownNotify(TRUE);
	SAFE_RELEASE_BTN(btnBakeLayerSettings);

	spnLimits[SPIN_X_MIN_POS] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_POS_X_MIN, IDC_EDIT_LIMIT_POS_X_MIN, -9999.0, 9999.0f, bone->IGetMinPositionLimits().x);
	spnLimits[SPIN_Y_MIN_POS] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_POS_Y_MIN, IDC_EDIT_LIMIT_POS_Y_MIN, -9999.0, 9999.0f, bone->IGetMinPositionLimits().y);
	spnLimits[SPIN_Z_MIN_POS] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_POS_Z_MIN, IDC_EDIT_LIMIT_POS_Z_MIN, -9999.0, 9999.0f, bone->IGetMinPositionLimits().z);

	spnLimits[SPIN_X_MAX_POS] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_POS_X_MAX, IDC_EDIT_LIMIT_POS_X_MAX, -9999.0, 9999.0f, bone->IGetMaxPositionLimits().x);
	spnLimits[SPIN_Y_MAX_POS] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_POS_Y_MAX, IDC_EDIT_LIMIT_POS_Y_MAX, -9999.0, 9999.0f, bone->IGetMaxPositionLimits().y);
	spnLimits[SPIN_Z_MAX_POS] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_POS_Z_MAX, IDC_EDIT_LIMIT_POS_Z_MAX, -9999.0, 9999.0f, bone->IGetMaxPositionLimits().z);

	spnLimits[SPIN_X_MIN_ROT] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_ROT_X_MIN, IDC_EDIT_LIMIT_ROT_X_MIN, -9999.0, 9999.0f, bone->IGetMinRotationLimits().x);
	spnLimits[SPIN_Y_MIN_ROT] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_ROT_Y_MIN, IDC_EDIT_LIMIT_ROT_Y_MIN, -9999.0, 9999.0f, bone->IGetMinRotationLimits().y);
	spnLimits[SPIN_Z_MIN_ROT] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_ROT_Z_MIN, IDC_EDIT_LIMIT_ROT_Z_MIN, -9999.0, 9999.0f, bone->IGetMinRotationLimits().z);

	spnLimits[SPIN_X_MAX_ROT] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_ROT_X_MAX, IDC_EDIT_LIMIT_ROT_X_MAX, -9999.0, 9999.0f, bone->IGetMaxRotationLimits().x);
	spnLimits[SPIN_Y_MAX_ROT] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_ROT_Y_MAX, IDC_EDIT_LIMIT_ROT_Y_MAX, -9999.0, 9999.0f, bone->IGetMaxRotationLimits().y);
	spnLimits[SPIN_Z_MAX_ROT] = SetupFloatSpinner(hDlg, IDC_SPIN_LIMIT_ROT_Z_MAX, IDC_EDIT_LIMIT_ROT_Z_MAX, -9999.0, 9999.0f, bone->IGetMaxRotationLimits().z);

	Update();
}

void CATNodeControlHierarchyDlgCallBack::ReleaseControls()
{
	bone = NULL;
	for (int i = 0; i < NUM_LIMITS; i++)
		SAFE_RELEASE_SPIN(spnLimits[i]);
	hDlg = NULL;
}

void CATNodeControlHierarchyDlgCallBack::Update()
{
	SET_CHECKED(hDlg, IDC_CHK_DISPLAY_ONION_SKINS, bone->TestCCFlag(CNCFLAG_DISPLAY_ONION_SKINS));

	SET_CHECKED(hDlg, IDC_CHK_PINBONE, bone->TestCCFlag(CCFLAG_FB_IK_LOCKED));
	SET_CHECKED(hDlg, IDC_CHK_EFFECTHIERARCHY, bone->TestCCFlag(CCFLAG_EFFECT_HIERARCHY));

	SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_SETUPMODE_POS, bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS));
	SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_SETUPMODE_ROT, bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_ROT));
	SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_SETUPMODE_SCL, bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL));
	SET_CHECKED(hDlg, IDC_CHK_STRETCHYBONE, bone->TestCCFlag(CCFLAG_SETUP_STRETCHY));
	SET_CHECKED(hDlg, IDC_CHK_ANIM_STRETCHYBONE, bone->TestCCFlag(CCFLAG_ANIM_STRETCHY));

	if (layerTrans) {
		SET_CHECKED(hDlg, IDC_CHK_SETUPINHERIT_POS, !layerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT));
		SET_CHECKED(hDlg, IDC_CHK_SETUPINHERIT_ROT, !layerTrans->TestFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT));

		SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_POS, bone->TestCCFlag(CNCFLAG_LOCK_LOCAL_POS));
		SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_ROT, bone->TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT));
		SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_SCL, bone->TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL));

		SET_CHECKED(hDlg, IDC_CHK_ANIMINHERIT_POS, bone->TestCCFlag(CNCFLAG_INHERIT_ANIM_POS));
		SET_CHECKED(hDlg, IDC_CHK_ANIMINHERIT_ROT, bone->TestCCFlag(CNCFLAG_INHERIT_ANIM_ROT));
		SET_CHECKED(hDlg, IDC_CHK_ANIMINHERIT_SCL, bone->TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL));

		SET_CHECKED(hDlg, IDC_RDO_SETUP_VALUE, !layerTrans->GetUseSetupController());
		SET_CHECKED(hDlg, IDC_RDO_SETUP_CTRL, layerTrans->GetUseSetupController());
		SET_CHECKED(hDlg, IDC_CHK_REL_TO_SETUP_POSE, layerTrans->TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE));
		EnableWindow(GetDlgItem(hDlg, IDC_BTN_BAKE_LAYER_SETTINGS), bone->GetLayerTrans()->GetSelectedLayerMethod() == LAYER_ABSOLUTE);
	}
	else {
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_APPLYTRANSFORMS), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_SETUPINHERIT_POS), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_SETUPINHERIT_ROT), FALSE);

		// If we don't have a layer trans then we obviously cant do any of this
		SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_POS, TRUE);
		SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_ROT, TRUE);
		SET_CHECKED(hDlg, IDC_CHK_MANIP_LOCK_SCL, TRUE);

		EnableWindow(GetDlgItem(hDlg, IDC_CHK_MANIP_LOCK_POS), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_MANIP_LOCK_ROT), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_MANIP_LOCK_SCL), FALSE);

		EnableWindow(GetDlgItem(hDlg, IDC_CHK_ANIMINHERIT_POS), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_ANIMINHERIT_ROT), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_ANIMINHERIT_SCL), FALSE);

		SET_CHECKED(hDlg, IDC_RDO_SETUP_VALUE, TRUE);
		SET_CHECKED(hDlg, IDC_RDO_SETUP_CTRL, FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_RDO_SETUP_VALUE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_RDO_SETUP_CTRL), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_CHK_REL_TO_SETUP_POSE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_BTN_BAKE_LAYER_SETTINGS), FALSE);
	}

	SET_CHECKED(hDlg, IDC_CHK_POS_LIMITS, bone->TestCCFlag(CNCFLAG_IMPOSE_POS_LIMITS));
	SET_CHECKED(hDlg, IDC_CHK_ROT_LIMITS, bone->TestCCFlag(CNCFLAG_IMPOSE_ROT_LIMITS));
	SET_CHECKED(hDlg, IDC_CHK_SCL_LIMITS, bone->TestCCFlag(CNCFLAG_IMPOSE_SCL_LIMITS));

	spnLimits[SPIN_X_MAX_POS]->SetValue(bone->IGetMaxPositionLimits().x, FALSE);	spnLimits[SPIN_X_MAX_POS]->SetLimits(bone->IGetMinPositionLimits().x, 9999.0f);
	spnLimits[SPIN_Y_MAX_POS]->SetValue(bone->IGetMaxPositionLimits().y, FALSE);	spnLimits[SPIN_Y_MAX_POS]->SetLimits(bone->IGetMinPositionLimits().y, 9999.0f);
	spnLimits[SPIN_Z_MAX_POS]->SetValue(bone->IGetMaxPositionLimits().z, FALSE);	spnLimits[SPIN_Z_MAX_POS]->SetLimits(bone->IGetMinPositionLimits().z, 9999.0f);

	spnLimits[SPIN_X_MIN_POS]->SetValue(bone->IGetMinPositionLimits().x, FALSE);	spnLimits[SPIN_X_MIN_POS]->SetLimits(-9999.0f, bone->IGetMaxPositionLimits().x);
	spnLimits[SPIN_Y_MIN_POS]->SetValue(bone->IGetMinPositionLimits().y, FALSE);	spnLimits[SPIN_Y_MIN_POS]->SetLimits(-9999.0f, bone->IGetMaxPositionLimits().y);
	spnLimits[SPIN_Z_MIN_POS]->SetValue(bone->IGetMinPositionLimits().z, FALSE);	spnLimits[SPIN_Z_MIN_POS]->SetLimits(-9999.0f, bone->IGetMaxPositionLimits().z);

	spnLimits[SPIN_X_MAX_ROT]->SetValue(bone->IGetMaxRotationLimits().x, FALSE);
	spnLimits[SPIN_Y_MAX_ROT]->SetValue(bone->IGetMaxRotationLimits().y, FALSE);
	spnLimits[SPIN_Z_MAX_ROT]->SetValue(bone->IGetMaxRotationLimits().z, FALSE);

	spnLimits[SPIN_X_MIN_ROT]->SetValue(bone->IGetMinRotationLimits().x, FALSE);
	spnLimits[SPIN_Y_MIN_ROT]->SetValue(bone->IGetMinRotationLimits().y, FALSE);
	spnLimits[SPIN_Z_MIN_ROT]->SetValue(bone->IGetMinRotationLimits().z, FALSE);
}

INT_PTR CALLBACK CATNodeControlHierarchyDlgCallBack::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		InitControls(hWnd, (CATNodeControl*)lParam);
		break;
	case WM_DESTROY:
		ReleaseControls();
		return FALSE;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
		{
			switch (LOWORD(wParam)) {
				// the following accesses to layerTrans are only enabled if it exists (see Update());
			case IDC_CHK_SETUPINHERIT_POS:
			case IDC_CHK_SETUPINHERIT_ROT:
			{
				// We call the same accessor method as everyone else.
				HoldActions hold(IDS_HLD_BONESETTING);
				BitArray ba(2);
				if (IS_CHECKED(hWnd, IDC_CHK_SETUPINHERIT_POS)) ba.Set(0);
				if (IS_CHECKED(hWnd, IDC_CHK_SETUPINHERIT_ROT)) ba.Set(1);
				bone->SetSetupModeInheritance(&ba);
			}
			break;
			case IDC_CHK_DISPLAY_ONION_SKINS:	bone->SetCCFlag(CNCFLAG_DISPLAY_ONION_SKINS, IS_CHECKED(hWnd, IDC_CHK_DISPLAY_ONION_SKINS));					break;

			case IDC_CHK_PINBONE:				bone->SetCCFlag(CCFLAG_FB_IK_LOCKED, IS_CHECKED(hWnd, IDC_CHK_PINBONE));				break;
			case IDC_CHK_STRETCHYBONE:			bone->SetCCFlag(CCFLAG_SETUP_STRETCHY, IS_CHECKED(hWnd, IDC_CHK_STRETCHYBONE));			break;
			case IDC_CHK_ANIM_STRETCHYBONE:		bone->SetCCFlag(CCFLAG_ANIM_STRETCHY, IS_CHECKED(hWnd, IDC_CHK_ANIM_STRETCHYBONE));
				bone->SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL, !IS_CHECKED(hWnd, IDC_CHK_ANIM_STRETCHYBONE));
				break;
			case IDC_CHK_EFFECTHIERARCHY:		bone->SetCCFlag(CCFLAG_EFFECT_HIERARCHY, IS_CHECKED(hWnd, IDC_CHK_EFFECTHIERARCHY));	break;

				// Lock Setup Pos/Rot/Scale
			case IDC_CHK_MANIP_LOCK_SETUPMODE_POS:
			case IDC_CHK_MANIP_LOCK_SETUPMODE_ROT:
			case IDC_CHK_MANIP_LOCK_SETUPMODE_SCL:
			{
				// We call the same accessor method as everyone else.
				HoldActions hold(IDS_HLD_BONESETTING);
				BitArray modeLocks(3);
				if (IS_CHECKED(hWnd, IDC_CHK_MANIP_LOCK_SETUPMODE_POS)) modeLocks.Set(0);
				if (IS_CHECKED(hWnd, IDC_CHK_MANIP_LOCK_SETUPMODE_ROT)) modeLocks.Set(1);
				if (IS_CHECKED(hWnd, IDC_CHK_MANIP_LOCK_SETUPMODE_SCL)) modeLocks.Set(2);
				bone->SetSetupModeLocks(&modeLocks);
				break;
			}

			// Lock Animation mode Pos/Rot/Scale
			case IDC_CHK_MANIP_LOCK_POS:
			case IDC_CHK_MANIP_LOCK_ROT:
			case IDC_CHK_MANIP_LOCK_SCL:
			{
				// We call the same accessor method as everyone else.
				HoldActions hold(IDS_HLD_BONESETTING);
				BitArray modeLocks(3);
				if (IS_CHECKED(hWnd, IDC_CHK_MANIP_LOCK_POS)) modeLocks.Set(0);
				if (IS_CHECKED(hWnd, IDC_CHK_MANIP_LOCK_ROT)) modeLocks.Set(1);
				if (IS_CHECKED(hWnd, IDC_CHK_MANIP_LOCK_SCL)) modeLocks.Set(2);
				bone->SetAnimationLocks(&modeLocks);
				break;
			}

			// Animation Mode Inherit Pos/Rot/Scale
			case IDC_CHK_ANIMINHERIT_POS:
			case IDC_CHK_ANIMINHERIT_ROT:
			case IDC_CHK_ANIMINHERIT_SCL:
			{
				// We call the same accessor method as everyone else.
				HoldActions hold(IDS_HLD_BONESETTING);
				BitArray modeLocks(3);
				if (IS_CHECKED(hWnd, IDC_CHK_ANIMINHERIT_POS)) modeLocks.Set(0);
				if (IS_CHECKED(hWnd, IDC_CHK_ANIMINHERIT_ROT)) modeLocks.Set(1);
				if (IS_CHECKED(hWnd, IDC_CHK_ANIMINHERIT_SCL)) modeLocks.Set(2);
				bone->SetAnimModeInheritance(&modeLocks);
				break;
			}

			case IDC_RDO_SETUP_VALUE:			bone->CreateSetupController(FALSE);	SET_CHECKED(hWnd, IDC_RDO_SETUP_CTRL, FALSE);		break;
			case IDC_RDO_SETUP_CTRL:			bone->CreateSetupController(TRUE);	SET_CHECKED(hWnd, IDC_RDO_SETUP_VALUE, FALSE);		break;
			case IDC_CHK_REL_TO_SETUP_POSE:		bone->SetRelativeToSetupMode(IS_CHECKED(hWnd, IDC_CHK_REL_TO_SETUP_POSE));				break;

			case IDC_BTN_BAKE_LAYER_SETTINGS:	bone->GetLayerTrans()->BakeCurrentLayerSettings();														break;

			case IDC_CHK_POS_LIMITS:			bone->SetCCFlag(CNCFLAG_IMPOSE_POS_LIMITS, IS_CHECKED(hWnd, IDC_CHK_POS_LIMITS));	bone->Update();	break;
			case IDC_CHK_ROT_LIMITS:			bone->SetCCFlag(CNCFLAG_IMPOSE_ROT_LIMITS, IS_CHECKED(hWnd, IDC_CHK_ROT_LIMITS));	bone->Update();	break;
			case IDC_CHK_SCL_LIMITS:			bone->SetCCFlag(CNCFLAG_IMPOSE_SCL_LIMITS, IS_CHECKED(hWnd, IDC_CHK_SCL_LIMITS));	bone->Update();	break;
			}
			bone->GetCATParentTrans()->UpdateCharacter();
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			Update();
			break;
		}
		break;
		}
		break;
	case CC_SPINNER_BUTTONDOWN:
		if (!theHold.Holding()) theHold.Begin();
		break;
	case CC_SPINNER_CHANGE:
	{
		if (theHold.Holding()) {
			MaxReferenceMsgLock lockThis;
			theHold.Restore();
		}
		// TODO: The below line is almost certainly leaking stuff: Test it!
		DbgAssert(MAX_VERSION_MAJOR < 16); // This should be fixed in Tekken.
		if (theHold.Holding()) theHold.Put(new SetLimitRestore(bone));
		SetLimits(wParam);
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	}
	break;
	case CC_SPINNER_BUTTONUP:
		if (theHold.Holding()) {
			if (HIWORD(wParam)) {
				theHold.Accept(GetString(IDS_HLD_BONELIMITS));	break;
			}
			else theHold.Cancel();
		}
		break;
	case WM_CUSTEDIT_ENTER:
		if (!theHold.Holding()) theHold.Begin();
		SetLimits(wParam);
		if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_BONELIMITS));
		break;
	case WM_NOTIFY:
		break;

	default:
		return FALSE;
	}
	return TRUE;
};

void CATNodeControlHierarchyDlgCallBack::SetLimits(UINT wParam)
{
	Point3 maxPosLimit = bone->IGetMaxPositionLimits();
	Point3 minPosLimit = bone->IGetMinPositionLimits();
	Point3 maxRotLimit = bone->IGetMaxRotationLimits();
	Point3 minRotLimit = bone->IGetMinRotationLimits();
	switch (LOWORD(wParam)) {
	case IDC_SPIN_LIMIT_POS_X_MIN:
		minPosLimit.x = spnLimits[SPIN_X_MIN_POS]->GetFVal();
		bone->ISetMinPositionLimits(minPosLimit);
		break;
	case IDC_SPIN_LIMIT_POS_Y_MIN:
		minPosLimit.y = spnLimits[SPIN_Y_MIN_POS]->GetFVal();
		bone->ISetMinPositionLimits(minPosLimit);
		break;
	case IDC_SPIN_LIMIT_POS_Z_MIN:
		minPosLimit.z = spnLimits[SPIN_Z_MIN_POS]->GetFVal();
		bone->ISetMinPositionLimits(minPosLimit);
		break;
	case IDC_SPIN_LIMIT_POS_X_MAX:
		maxPosLimit.x = spnLimits[SPIN_X_MAX_POS]->GetFVal();
		bone->ISetMaxPositionLimits(maxPosLimit);
		break;
	case IDC_SPIN_LIMIT_POS_Y_MAX:
		maxPosLimit.y = spnLimits[SPIN_Y_MAX_POS]->GetFVal();
		bone->ISetMaxPositionLimits(maxPosLimit);
		break;
	case IDC_SPIN_LIMIT_POS_Z_MAX:
		maxPosLimit.z = spnLimits[SPIN_Z_MAX_POS]->GetFVal();
		bone->ISetMaxPositionLimits(maxPosLimit);
		break;
		//////////////////////////////////////////////////////////////////////////
	case IDC_SPIN_LIMIT_ROT_X_MIN:
		minRotLimit.x = spnLimits[SPIN_X_MIN_ROT]->GetFVal();
		bone->ISetMinRotationLimits(minRotLimit);
		break;
	case IDC_SPIN_LIMIT_ROT_Y_MIN:
		minRotLimit.y = spnLimits[SPIN_Y_MIN_ROT]->GetFVal();
		bone->ISetMinRotationLimits(minRotLimit);
		break;
	case IDC_SPIN_LIMIT_ROT_Z_MIN:
		minRotLimit.z = spnLimits[SPIN_Z_MIN_ROT]->GetFVal();
		bone->ISetMinRotationLimits(minRotLimit);
		break;
	case IDC_SPIN_LIMIT_ROT_X_MAX:
		maxRotLimit.x = spnLimits[SPIN_X_MAX_ROT]->GetFVal();
		bone->ISetMaxRotationLimits(maxRotLimit);
		break;
	case IDC_SPIN_LIMIT_ROT_Y_MAX:
		maxRotLimit.y = spnLimits[SPIN_Y_MAX_ROT]->GetFVal();
		bone->ISetMaxRotationLimits(maxRotLimit);
		break;
	case IDC_SPIN_LIMIT_ROT_Z_MAX:
		maxRotLimit.z = spnLimits[SPIN_Z_MAX_ROT]->GetFVal();
		bone->ISetMaxRotationLimits(maxRotLimit);
		break;
	}
}

static CATNodeControlSetupDlgCallBack CATNodeControlSetupCallback;

static INT_PTR CALLBACK CATNodeControlSetupProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return CATNodeControlSetupCallback.DlgProc(hWnd, message, wParam, lParam);
};

static CATNodeControlHierarchyDlgCallBack CATNodeControlHierarchyCallback;

static INT_PTR CALLBACK CATNodeControlHierarchyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return CATNodeControlHierarchyCallback.DlgProc(hWnd, message, wParam, lParam);
};

void CATNodeControl::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	TSTR classname;
	GetClassName(classname);
	flagsbegin = flags;
	ipbegin = ip;
	if (flags&BEGIN_EDIT_MOTION || flags&BEGIN_EDIT_LINKINFO)
	{
		if (flags&BEGIN_EDIT_LINKINFO)
			ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_CATBONE_HIERARCHY_CAT3), &CATNodeControlHierarchyProc, GetString(IDS_BONE_HIER), (LPARAM)this);
		// Remove the layer manager rollout
		if (GetLayerTrans()) GetLayerTrans()->BeginEditParams(ip, flags, prev);
	}
	else if (flagsbegin == 0)
	{
		ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_CATBONE_SETUP_CAT3), &CATNodeControlSetupProc, GetString(IDS_BONE_SETUP), (LPARAM)this);
	}
	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void CATNodeControl::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(next); UNREFERENCED_PARAMETER(flags);
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO)
	{
		if (GetLayerTrans()) GetLayerTrans()->EndEditParams(ip, END_EDIT_REMOVEUI);
		if (flagsbegin&BEGIN_EDIT_LINKINFO)
		{
			if (CATNodeControlHierarchyCallback.GetHWnd())
			{
				ip->DeleteRollupPage(CATNodeControlHierarchyCallback.GetHWnd());
			}
		}
	}
	else if (flagsbegin == 0)
	{
		if (CATNodeControlSetupCallback.GetHWnd())
		{
			ip->DeleteRollupPage(CATNodeControlSetupCallback.GetHWnd());
		}
	}
	ipbegin = NULL;
}

void CATNodeControl::UpdateUI() {
	if (!ipbegin) return;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO) {
		if ((flagsbegin&BEGIN_EDIT_LINKINFO) && !suppressLinkInfoUpdate) {
			if (CATNodeControlHierarchyCallback.IsDlgOpen())
				CATNodeControlHierarchyCallback.Update();
		}
		if (flagsbegin&BEGIN_EDIT_MOTION) {
			//TODO: make this compile
			//			if(GetLayerTrans()) GetLayerTrans()->GetRoot()->RefreshLayerRollout();
		}
	}
	else if (flagsbegin == 0) {
		if (CATNodeControlSetupCallback.IsDlgOpen())
			CATNodeControlSetupCallback.Update();
	}
}

void CATNodeControl::SetLengthAxis(int axis) {

	Matrix3 tmSetup = GetSetupMatrix();
	if (axis == X)	tmSetup = (RotateYMatrix(-HALFPI) * tmSetup) * RotateYMatrix(HALFPI);
	else			tmSetup = (RotateYMatrix(HALFPI)  * tmSetup) * RotateYMatrix(-HALFPI);
	SetSetupMatrix(tmSetup);

	ICATObject *iobj = GetICATObject();
	if (iobj) iobj->SetLengthAxis(axis);

	CATControl::SetLengthAxis(axis);
};

// user Properties.
// these will be used by the runtime libraries
void	CATNodeControl::UpdateUserProps() {

	INode *node = GetNode();
	if (!node) return;

	// Clear the user properties.
	// IMPORTANT: do not let this change go out with the next release
	// TODO: Add a UpdateUserProps Script function to the CATParent
	node->SetUserPropBuffer(_T(""));

	node->SetUserPropString(_T("CATProp_BoneAddress"), GetBoneAddress());

	CATGroup *pGroup = GetGroup();
	if (pGroup != NULL)
		node->SetUserPropString(_T("CATProp_LocalWeightGroup"), pGroup->AsCATControl()->GetBoneAddress());
}

void CATNodeControl::InvalidateTransform()
{
	mLocalValid.SetEmpty();
	mWorldValid.SetEmpty();

	INode* pNode = GetNode();
	// Don't invalidate while updating the transform.
	if (pNode != NULL && !pNode->TestAFlag(A_INODE_IN_UPDATE_TM))
		pNode->InvalidateTreeTM();
}
////////////////////
// Gimbal Madness //
////////////////////
// Wehenver the user selects a bone, or rotates it and then releases, Max rebuilds the
// Gimbal gizmo. At that point it calls GetRotationController on the TM controller.
// If it gets a Euler controler, it then goes a way and builds its gizmo, which is will
// get completely wrong, because we often don't use default inheritance. For example,
// the UpperArm inherits off the ribcage, not the collarbone, but the gizmo gets built
// around the collarbone, and we get no say in the matter. One way of getting a say, is to
// not return a rotation controller, and then Max is forced to use the other cryptic
// Control method GetLocalTMComponents. Here we get to specify a parent matrix.
// So far the GetLocalTMComponents method hasn't proved to be very usefull, as the
// PRSControl::Rotate method seems to do some wierd stuff.
// TODO: work your way through the PRSControl::Rotate method untill you totally understand
// the whole thing. figure out if there is a way to force the gimbal axes to work.
// The problem is that the best we can do is disable Gimbal, untill we can figure out a way
// to take over the actual building of the gimbal gizmo.

// Also - when reactor is setting values on us force it to setvalues through us... no accessing our sub-anims
#define REACTOR_SOLVING  ((GET_MAX_RELEASE(Get3DSMAXVersion())>=7000)&&(GetNode()&&!GetNode()->Selected()&&Animating()))

Control * CATNodeControl::GetPositionController()
{
	//	BOOL pm = GetCOREInterface()->InProgressiveMode();
	if (REACTOR_SOLVING)
		return NULL;
	if (GetNode() && GetNode()->Selected() && GetCOREInterface()->GetRefCoordSys() == COORDS_GIMBAL&& GetCOREInterface7()->GetPivotMode() != Interface7::kPIV_PIVOT_ONLY)
		return NULL;

	return GetLayerTrans() ? GetLayerTrans()->GetPositionController() : NULL;
}

Control * CATNodeControl::GetRotationController()
{
	//	BOOL pm = GetCOREInterface()->InProgressiveMode();
	if (REACTOR_SOLVING)
		return NULL;
	if (GetNode() && GetNode()->Selected() && GetCOREInterface()->GetRefCoordSys() == COORDS_GIMBAL&& GetCOREInterface7()->GetPivotMode() != Interface7::kPIV_PIVOT_ONLY)
		return NULL;

	return GetLayerTrans() ? GetLayerTrans()->GetRotationController() : NULL;
}

// See if this node, or any of its parents/ancestors, are
// from the rig pMyCPT
INode* FindNodeFromRig(INode* pNode, ICATParentTrans* pMyCPT)
{
	if (pMyCPT == NULL)
		return NULL;

	// search down the INode tree to find a node from this rig
	while (pNode != NULL)
	{
		CATNodeControl* pCtrl = dynamic_cast<CATNodeControl*>(pNode->GetTMController());
		if (pCtrl != NULL && pCtrl->GetCATParentTrans() == pMyCPT)
			return pNode; // always return false;

		pNode = pNode->GetParentNode();
	}

	return NULL;
}

void CATNodeControl::CalculateHasTransformFlag()
{
	INode* pNode = GetNode();
	ICATParentTrans* pMyCPT = GetCATParentTrans();
	CATClipMatrix3* pLayerTrans = GetLayerTrans();

#ifdef _DEBUG
	// We can only be run on a class that has been properly setup.
	DbgAssert(pNode != NULL);
	SpineData2* pMyOwner = dynamic_cast<SpineData2*>(GetParentCATControl());

	// The only time it is legal to not have a LayerTrans is
	// A) On a BoneSegTrans with ID != 0
	// B) on the SpineTrans in non-FK mode
	Class_ID myid = ClassID();
	if ((myid != BONESEGTRANS_CLASS_ID || GetBoneID() == 0) &&
		(myid != SPINETRANS2_CLASS_ID || (pMyOwner != NULL && pMyOwner->GetSpineFK())))
	{
		DbgAssert(pLayerTrans != NULL);
	}
#endif

	if (pNode == NULL || pLayerTrans == NULL)
		return;

	// Search for an ancestor that is part of this rig.
	if (FindNodeFromRig(pNode->GetParentNode(), pMyCPT) != NULL)
	{
		// We have an ancestor, and will inherit transforms appropriately.
		// Therefore, we don't need to re-apply transforms ourselves
		pLayerTrans->ClearFlag(CLIP_FLAG_HAS_TRANSFORM);
	}
	else
	{
		INode* pCatParentNode = pMyCPT ? pMyCPT->GetNode() : NULL;
		INode* pRootCatParent = pCatParentNode ? pCatParentNode->GetParentNode() : NULL;
		ICATParentTrans* pRootCPT = pRootCatParent ? dynamic_cast<ICATParentTrans*>(pRootCatParent->GetTMController()) : NULL;
		if (FindNodeFromRig(pNode->GetParentNode(), pRootCPT) != NULL)
		{
			// This rig's CATParent is a sub-rig of another CATParent
			// and the ancestor belongs to that CATParent.
			pLayerTrans->ClearFlag(CLIP_FLAG_HAS_TRANSFORM);
		}
		else
		{
			// If we get to here, we have no ancestors that are part of this rig, and will
			// need to modify our parent matrix in SetupMode to 'inherit' from the CATParent
			pLayerTrans->SetFlag(CLIP_FLAG_HAS_TRANSFORM);
		}
	}
}

BOOL CATNodeControl::ChangeParents(TimeValue t, const Matrix3& oldP, const Matrix3& newP, const Matrix3& tm)
{
	// Do not trigger CalculateHasTransform flag here, it
	// will be triggered by the NODE_LINKED callback in CATParent
	return FALSE; // always return false;
}

bool CATNodeControl::GetLocalTMComponents(TimeValue t, TMComponentsArg& cmpts, Matrix3Indirect& parentMatrix)
{
	if (GetLayerTrans())
	{
		// Modify the incoming parent appropriately
		Matrix3 tmOrigParent = parentMatrix();
		Matrix3 tmMyParent = tmOrigParent;
		Point3 p3ParentScale;
		CalcParentTransform(t, tmMyParent, p3ParentScale);
		parentMatrix.Set(tmMyParent);
		// Call our data for the result
		bool res = GetLayerTrans()->GetLocalTMComponents(t, cmpts, parentMatrix, tmOrigParent);
		if (cmpts.scale != NULL)
			cmpts.scale->s *= p3ParentScale;
		return  res;
	}
	return Control::GetLocalTMComponents(t, cmpts, parentMatrix);
}

//////////////////////////////////////////////////////////////////////////
// FPInterface stuff

BaseInterface* CATNodeControl::GetInterface(Interface_ID id)
{
	if (id == I_NODECONTROL)		return (INodeControlFP*)(this);
	if (id == I_EXTRARIGNODES_FP)	return (ExtraRigNodes*)this;
	else return CATControl::GetInterface(id);
}

FPInterfaceDesc* CATNodeControl::GetDescByID(Interface_ID id) {
	if (id == I_NODECONTROL)	return INodeControlFP::GetDesc();
	if (id == I_EXTRARIGNODES_FP)	return ExtraRigNodes::GetDesc();
	return CATControl::GetDescByID(id);
}

void CATNodeControl::OnRestoreDataChanged(DWORD val)
{
	UpdateUI();
}

//////////////////////////////////////////////////////////////////////////
// CATNodeControlClassDesc

CATNodeControlClassDesc::CATNodeControlClassDesc()
{
	// Add in interfaces for this class
	AddInterface(INodeControlFP::GetFnPubDesc());
	AddInterface(ExtraRigNodes::GetFnPubDesc());
}

SClass_ID CATNodeControlClassDesc::SuperClassID()
{
	return CTRL_MATRIX3_CLASS_ID;
}
