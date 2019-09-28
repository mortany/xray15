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
#include "Simpobj.h"

 // Rig Structure
#include "BoneSegTrans.h"
#include "BoneData.h"
#include "LimbData2.h"
#include <CATAPI/CATClassID.h>
#include "PalmTrans2.h"

class BoneSegTransClassDesc : public CATNodeControlClassDesc {
public:
	CATControl *	DoCreate(BOOL /*loading = FALSE*/)
	{
		BoneSegTrans* bone = new BoneSegTrans();
		return bone;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_BONESEGTRANS); }
	Class_ID		ClassID() { return BONESEGTRANS_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("BoneSegTrans"); }	// returns fixed parsable name (scripter-visible name)
};

static BoneSegTransClassDesc BoneSegTransDesc;
ClassDesc2* GetBoneSegTransDesc() { return &BoneSegTransDesc; }

static ParamBlockDesc2 bonesegtrans_param_blk(BoneSegTrans::pb_params, _T("BoneSegTransParams"), 0, &BoneSegTransDesc,
	P_AUTO_CONSTRUCT, BoneSegTrans::BONEDATAPB_REF,

	BoneSegTrans::PB_BONEDATA, _T("Bone"), TYPE_REFTARG, P_SUBANIM, IDS_BONEDATA,
		p_end,
	BoneSegTrans::PB_SEGID_DEPRECATED, _T("SegID"), TYPE_INDEX, 0, IDS_SEGNUM,
		p_end,
	p_end
);

/**********************************************************************
 * BoneSegTrans...
 */

BoneSegTrans::BoneSegTrans()
	: pblock(NULL)
{
	BoneSegTransDesc.MakeAutoParamBlocks(this);

	// default bones to being stretchy
	ccflags |= CCFLAG_SETUP_STRETCHY;
	ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	ccflags |= CNCFLAG_LOCK_SETUPMODE_LOCAL_POS;
	ccflags |= CCFLAG_EFFECT_HIERARCHY;
}

BoneSegTrans::~BoneSegTrans()
{
	DeleteAllRefs();
}

RefTargetHandle BoneSegTrans::Clone(RemapDir& remap)
{
	BoneSegTrans *newBoneSegTrans = new BoneSegTrans();
	remap.AddEntry(this, newBoneSegTrans);

	newBoneSegTrans->ReplaceReference(BONEDATAPB_REF, CloneParamBlock(pblock, remap));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newBoneSegTrans, remap);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newBoneSegTrans, remap);

	// now return the new object.
	return newBoneSegTrans;
}

void BoneSegTrans::CalcBoneDataParent(TimeValue t, Matrix3& tmParent)
{
	// If we are the not the first bone, and if our parent had multiple segments,
	// then the matrix we are receiving here will have had twist added to it.
	// We can query BoneData for its calculated transform, which is the bones
	// transform before the twist was added in.
	BoneData* pBoneData = GetBoneData();
	if (pBoneData == NULL)
		return;

	if (pBoneData->GetBoneID() != 0)
	{
		// What is our 'real' parent, one without the twist baked in.
		BoneSegTrans* pParentSeg = dynamic_cast<BoneSegTrans*>(pBoneData->GetParentCATNodeControl());
		if (pParentSeg != NULL)
		{
			BoneData* pParentBone = pParentSeg->GetBoneData();
			if (pParentBone != NULL && pParentBone->GetNumBones() > 1)
			{
				Point3 p3Pos = tmParent.GetTrans();
				tmParent = pParentBone->GettmBoneWorld(t);
				// We inherit position from the last bone, and rotation from it's BoneData.
				tmParent.SetTrans(p3Pos);

#ifdef _DEBUG
				// Just to check - our input position is the same as the position for the last segment, huh?
				// This is not a 100% requirement - in fact, it shouldn't actually matter, but for completeness
				// we should make sure it is always correct
				Matrix3 tmParent = pParentSeg->GetNodeTM(t);
				DbgAssert(tmParent.GetTrans() == p3Pos);
#endif
			}
		}
	}
}

void BoneSegTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	BoneData* pBoneData = GetBoneData();
	DbgAssert(pBoneData != NULL);
	LimbData2* pLimb = pBoneData->GetLimbData();
	DbgAssert(pLimb != NULL);

	// Stretchy bones in IK is really hard.  To make it easier,
	// we simply switch to FK and calculate.
	SetXFormPacket* pXform = (SetXFormPacket*)val;
	Interval valid;
	Matrix3 tmAnkle(1);
	bool bKeyLimb = pLimb->GetIKFKRatio(t, valid) < 1.0f &&
		!pBoneData->GetLayerTrans()->TestFlag(CLIP_FLAG_KEYFREEFORM) &&
		(
		((pXform->command == XFORM_MOVE) || (pXform->command == XFORM_SET)) &&
			(GetCATMode() == SETUPMODE) &&
			pBoneData->TestCCFlag(CCFLAG_SETUP_STRETCHY)
			);

	// do NOT key limbs for 2nd bone onwards (math doesn't work yet)
	if (bKeyLimb && GetBoneID() != 0)
	{
		return;
	}

	if (bKeyLimb)
	{

		// And... yet another hack.  If we are a limb, when we go into
		// FK mode its not possible for the ankle to keep its current
		// position.  The problem is that when the limb does its
		// stretching, it resets the ankles transform to compensate
		// This unfortunately causes problems because it resets
		// the transform to be the current FK transform.
		if (pLimb->GetisLeg() && pLimb->GetPalmTrans() != NULL)
			tmAnkle = pLimb->GetPalmTrans()->GetNodeTM(t);

		pLimb->MatchIKandFK(t);
		pLimb->SetFlag(LIMBFLAG_LOCKED_FK);
	}

	// Our segment has no real transform of its own.  Reset
	// the incoming values to the correct parent, and pipe to the bonedata
	if (GetBoneID() != 0)
		pXform->tmParent = pBoneData->GetParentTM(t);
	pBoneData->SetValue(t, val, commit, method);

	if (bKeyLimb)
	{
		pLimb->ClearFlag(LIMBFLAG_LOCKED_FK);

		if (pLimb->GetisLeg() && pLimb->GetPalmTrans() != NULL)
			pLimb->GetPalmTrans()->SetNodeTM(t, tmAnkle);
	}
}

void BoneSegTrans::GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method)
{
	BoneData* pBoneData = GetBoneData();
	if (pBoneData == NULL)
		return;

	// The first segment calculates the transform
	// All remaining ones just offset from the parent.
	Matrix3& tmWorld = *(Matrix3*)val;
	int idSeg = GetBoneID();
	if (idSeg == 0)
		pBoneData->GetValue(t, val, valid, method);
	else
		CATNodeControl::ApplyChildOffset(t, tmWorld);

	// Data we'll use to calculate the twist
	float dTwist = pBoneData->GetTwistAngle(idSeg, valid);
	// We inherited the last bones twist, but what we
	// have calculated is the absolute twist.  Remove
	// the previous bones twist from the equation.
	if (idSeg != 0)
		dTwist -= pBoneData->GetTwistAngle(idSeg - 1, valid);

	int iLengthAxis = GetLengthAxis();
	if (iLengthAxis == X)
		tmWorld.PreRotateX(dTwist);
	else
		tmWorld.PreRotateZ(-dTwist);

	// Do not forget to assign this variable.
	mWorldValid = valid;
}

void BoneSegTrans::CalcInheritance(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	CalcBoneDataParent(t, tmParent);

	// If we are the first segment in a bone, apply our bones inheritance
	BoneData* pBoneData = GetBoneData();
	if (GetBoneID() == 0 && pBoneData != NULL)
		pBoneData->CalcInheritance(t, tmParent, p3ParentScale);
}

void BoneSegTrans::ApplyChildOffset(TimeValue t, Matrix3& tmChild) const
{
	// First, find the appropriate offset
	CATNodeControl::ApplyChildOffset(t, tmChild);
	// If we are the last segment in the bone, reset the transform to remove the twist effect
	// If we are the not the first bone, and if our parent had multiple segments,
	// then the matrix we are receiving here will have had twist added to it.
	// We can query BoneData for its calculated transform, which is the bones
	// transform before the twist was added in.
	const BoneData* pBoneData = GetBoneData();
	if (pBoneData == NULL)
		return;

	// If we are the last segment in the bone?
	int iBoneID = GetBoneID();
	if (iBoneID == pBoneData->GetNumBones() - 1)
	{
		if (!pBoneData->IsEvaluating())
		{
			// What is the 'real' transform to pass down, one without the twist baked in?
			Point3 p3Pos = tmChild.GetTrans();
			tmChild = pBoneData->GettmBoneWorld(t);
			tmChild.PreScale(pBoneData->GetWorldScale(t));
			// Reset the position though, that is inherited normal-like.
			tmChild.SetTrans(p3Pos);
		}
	}
}

void BoneSegTrans::ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid)
{
	// This function is not called in normal operation, but is left in here
	// in case another CATNodeControl needs to call this?
	DbgAssert(false && _T("Should Not Be Called"));

	// Only the first segment in a bone applies Setup
	if (GetBoneID() == 0)
		CATNodeControl::ApplySetupOffset(t, tmOrigParent, tmWorldTransform, p3BoneLocalScale, ivValid);
}

void BoneSegTrans::CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid)
{
	// This function is not called in normal operation, but is left in here
	// in case another CATNodeControl needs to call this?
	DbgAssert(false && _T("Should Not Be Called"));

	BoneData* pBoneData = GetBoneData();
	if (!pBoneData)
		return;

	int nSegNum = GetBoneID();

	// Our first segment applies the rotation from BoneData
	if (nSegNum == 0)
		pBoneData->CalcWorldTransform(t, tmParent, p3ParentScale, tmWorld, p3LocalScale, ivLocalValid);

	// Data we'll use to calculate the twist
	float dTwist = pBoneData->GetTwistAngle(nSegNum, ivLocalValid);
	// We inherited the last bones twist, but what we
	// have calculated is the absolute twist.  Remove
	// the previous bones twist from the equation.
	if (nSegNum != 0)
		dTwist -= pBoneData->GetTwistAngle(nSegNum - 1, ivLocalValid);
}

void BoneSegTrans::CalcPositionStretch(TimeValue t, Matrix3& tmCurrent, Point3 p3Pivot, const Point3& p3Initial, const Point3& p3Final)
{
	// Only bone 0 is capable of doing position stretching.  The rest of us need to pass this on to BoneData.
	if (GetBoneID() != 0)
	{
		BoneData* pData = GetBoneData();
		CATNodeControl* pSeg0 = pData->GetBone(0);
		if (pSeg0 != NULL)
		{
			tmCurrent = pSeg0->GetNodeTM(t);
			pSeg0->CalcPositionStretch(t, tmCurrent, tmCurrent.GetTrans(), p3Initial, p3Final);
		}
	}
	else
		CATNodeControl::CalcPositionStretch(t, tmCurrent, p3Pivot, p3Initial, p3Final);
}

//////////////////////////////////////////////////////////////////////////

void BoneSegTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	BoneData *bonedata = GetBoneData();
	if (bonedata) bonedata->BeginEditParams(ip, flags, prev);

	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void BoneSegTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	ReferenceTarget *bonedata = pblock->GetReferenceTarget(PB_BONEDATA);
	if (bonedata) bonedata->EndEditParams(ip, flags, next);
}

RefTargetHandle BoneSegTrans::GetReference(int)
{
	return pblock;
}

// On Bone Segs we basicly just want to save out the arbitrary bones
BOOL BoneSegTrans::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(GetRigID());

	save->Comment(GetNode()->GetName());

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

RefResult BoneSegTrans::NotifyRefChanged(const Interval&, RefTargetHandle /*hTarg*/, PartID&, RefMessage, BOOL)
{
	return REF_SUCCEED;
}

//////////////////////////////////////////////////////////////////////////
// CATNodeControl functions.

void BoneSegTrans::SetNode(INode* n)
{
	CATNodeControl::SetNode(n);

	// If we are bone 0, clone to our BoneData -
	// this allows BoneData to have a pointer
	// to a node that best represents its own
	// transform
	if (GetBoneID() == 0)
	{
		BoneData* pBone = GetBoneData();
		if (pBone != NULL)
			pBone->SetNode(GetNode());
	}
}

CATNodeControl* BoneSegTrans::FindParentCATNodeControl()
{
	CATNodeControl* pParent = NULL;
	BoneData* pBone = GetBoneData();
	DbgAssert(pBone != NULL);
	if (pBone == NULL)
		return NULL;

	int iBoneSegID = GetBoneID();
	if (iBoneSegID > 0)
		pParent = pBone->GetBone(iBoneSegID - 1);
	else
	{
		CATNodeControl *pBoneParent = pBone->GetParentCATNodeControl(true);

		BoneData* pLimbBoneParent = dynamic_cast<BoneData*>(pBoneParent);
		if (pLimbBoneParent != NULL)
			pParent = pLimbBoneParent->GetBone(pLimbBoneParent->GetNumBones() - 1);
		else
			pParent = pBoneParent;
	}

	DbgAssert(pParent != NULL);
	return pParent;
}

CATNodeControl* BoneSegTrans::GetChildCATNodeControl(int i)
{
	UNUSED_PARAM(i);
	DbgAssert(i == 0);

	BoneData* pBone = GetBoneData();
	if (pBone == NULL)
		return NULL;

	// If we are not the last seg in the bone, return next seg.
	int iSegID = GetBoneID();
	if (iSegID < (pBone->GetNumBones() - 1))
		return pBone->GetBone(iSegID + 1);

	LimbData2* pLimb = pBone->GetLimbData();
	if (pLimb == NULL)
		return NULL;

	// If we are not the last bone in the limb, return the first
	// seg of the next bone.
	int iBoneID = pBone->GetBoneID();
	if (iBoneID < (pLimb->GetNumBones() - 1))
	{
		BoneData* pNextBone = pLimb->GetBoneData(iBoneID + 1);
		if (pNextBone != NULL)
			return pNextBone->GetBone(0);
	}

	// If none of the above (we are the last seg in the last bone) return the palm
	return pLimb->GetPalmTrans();
}

INode* BoneSegTrans::Initialise(BoneData* bonedata, int id, BOOL loading)
{
	SetBoneID(id);
	DisableRefMsgs();

	SimpleObject2* obj = (SimpleObject2*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	pblock->SetValue(PB_BONEDATA, 0, (ReferenceTarget*)bonedata);

	// Old code needs this value here...
	pblock->SetValue(PB_SEGID_DEPRECATED, 0, id);

	if (!loading) {
		name.printf(_T("%d"), GetBoneID() + 1);
	}

	INode* node = CreateNode(obj);

	EnableRefMsgs();

	return node;
}

// user Properties.
// these will be used by the runtime libraries
void	BoneSegTrans::UpdateUserProps() {

	CATNodeControl::UpdateUserProps();

	// Generate a unique identifier for the limb.
	// we will borrow the node handle from the 1st bone in the limb
//	ULONG limbid = GetBoneData()->GetLimb()->GetBone(0)->GetNode()->GetHandle();
//	node->SetUserPropInt(_T("CATProp_LimbID"), limbid);
	INode* node = GetNode();

	node->SetUserPropInt(_T("CATProp_LimbBoneID"), GetBoneData()->GetBoneID());

	if (GetBoneData()->GetNumBones() > 1) {
		if (GetBoneID() == 0) {
			node->SetUserPropFloat(_T("CATProp_BoneLength"), GetBoneData()->GetBoneLength());
		}
		node->SetUserPropInt(_T("CATProp_NumLimbBoneSegs"), GetBoneData()->GetNumBones());
		node->SetUserPropInt(_T("CATProp_LimbBoneSegID"), GetBoneID());

		float SegRatio = ((float)GetBoneID()) / (float)GetBoneData()->GetNumBones();
		node->SetUserPropFloat(_T("CATProp_LimbBoneSegTwistWeight"), GetBoneData()->GetTwistWeight(SegRatio));
	}

	if (GetBoneID() == 0 && GetBoneData()->GetBoneID() == 0) {
		node->SetUserPropString(_T("CATProp_LimbAddress"), GetBoneData()->GetLimbData()->GetBoneAddress());
		node->SetUserPropInt(_T("CATProp_LimbLMR"), GetBoneData()->GetLimbData()->GetLMR());

		ILimb* symlimb = GetBoneData()->GetLimbData()->GetSymLimb();
		if (symlimb) {
			LimbData2* pSymLimb = static_cast<LimbData2*>(symlimb);
			node->SetUserPropString(_T("CATProp_SymLimbAddress"), pSymLimb->GetBoneAddress());
		}

		node->SetUserPropInt(_T("CATProp_NumLimbBones"), GetBoneData()->GetLimbData()->GetNumBones());

		// now we save the IKFK key times to the user properties
		WriteControlToUserProps(node, GetBoneData()->GetLimbData()->GetLayerController(3)->GetSelectedLayer(), _T("LimbIKFKRatio"));

		// now we save the Local weights key times and values to the user properties
		WriteControlToUserProps(node, ((CATClipValue*)GetBoneData()->GetLimbData()->GetWeights())->GetSelectedLayer(), _T("LocalWeight"));
	}
}

TSTR	BoneSegTrans::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetBoneID());
	BoneData* bonedata = GetBoneData();
	DbgAssert(bonedata);
	TSTR bonename = _T("<<unknown>>");
	if (bonedata)
		bonename = bonedata->GetBoneAddress();
	return (bonename + _T(".") + bonerigname + boneid);
};

TSTR BoneSegTrans::GetRigName()
{
	DbgAssert(GetCATParentTrans());

	TSTR catname = _T("<<unknown>>");
	if (GetCATParentTrans())
		catname = GetCATParentTrans()->GetCATName();
	TSTR bonename = GetBoneData()->GetName();
	TSTR limbname = _T("<<unknown>>"); 
	if (GetBoneData()->GetLimbData())
		limbname = GetBoneData()->GetLimbData()->GetName();
	if (bonename.Length() == 0) bonename.printf(_T("%d"), GetBoneData()->GetBoneID() + 1);

	int iNumSegs = GetBoneData()->GetNumBones();
	if (iNumSegs > 1) {
		if (name.Length() == 0) { name.printf(_T("%d"), GetBoneID() + 1); }
		return catname + limbname + bonename + name;
	}
	else return catname + limbname + bonename;
}
BOOL BoneSegTrans::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(GetRigID());

	// For non-hierarchical naming
	save->Write(idBoneName, GetName());
	save->Write(idFlags, ccflags);

	ICATObject* iobj = GetICATObject();
	if (iobj) iobj->SaveRig(save);

	SaveRigArbBones(save);

	save->EndGroup();
	return TRUE;
}

BOOL BoneSegTrans::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	ICATObject* iobj = GetICATObject();

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
			case idArbBones:		LoadRigArbBones(load);					break;
			case idObjectParams:	iobj->LoadRig(load);					break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idBoneName:
				load->GetValue(name);
				// This is a fix to a problem where on old rigs,
				// Bone segs were saving out the bondatas name to rig presets
				if (name == GetBoneData()->GetName()) name = _T("");
				SetName(name);
				break;
			case idFlags:		load->GetValue(ccflags);					break;
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

void BoneSegTrans::CollapsePoseToCurrLayer(TimeValue t)
{
	GetBoneData()->CollapsePoseToCurrLayer(t);
};

void BoneSegTrans::SetObjX(float val)
{
	BoneData* pBone = GetBoneData();
	if (pBone != NULL)
		pBone->SetObjX(val);
}

void BoneSegTrans::SetObjY(float val)
{
	BoneData* pBone = GetBoneData();
	if (pBone != NULL)
		pBone->SetObjY(val);
}

void BoneSegTrans::SetObjZ(float val)
{
	BoneData* pBone = GetBoneData();
	if (pBone != NULL)
	{
		// Don't forget to re-scale to the total length
		val *= pBone->GetNumBones();
		pBone->SetObjZ(val);
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class BoneSegTransPLCB : public PostLoadCallback {
protected:
	BoneSegTrans *bone;
	ICATParentTrans *catparent;

public:
	BoneSegTransPLCB(BoneSegTrans *pOwner) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		if (bone->GetFileSaveVersion() > 0) return bone->GetFileSaveVersion();

		catparent = bone->GetCATParentTrans();
		if (catparent) return catparent->GetFileSaveVersion();
		catparent = bone->GetBoneData()->GetCATParentTrans();
		DbgAssert(catparent);
		return catparent->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad* /*iload*/) {

		if (!bone->GetBoneData() || !bone->GetBoneData()->GetLimbData()) {
			bone->MaybeAutoDelete();
			delete this;
			return;
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

BoneData*	BoneSegTrans::GetBoneData()
{
	return dynamic_cast<BoneData*>(pblock->GetReferenceTarget(PB_BONEDATA));
}

void		BoneSegTrans::SetBoneData(BoneData* b)
{
	pblock->SetValue(PB_BONEDATA, 0, b);
}

/**********************************************************************
 * Loading and saving....
 */
#define BONESEGTRANSCHUNK_CATNODECONTROL		0
IOResult BoneSegTrans::Save(ISave *isave)
{
	isave->BeginChunk(BONESEGTRANSCHUNK_CATNODECONTROL);
	CATNodeControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult BoneSegTrans::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case BONESEGTRANSCHUNK_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new BoneSegTransPLCB(this));

	return IO_OK;
}

CATGroup* BoneSegTrans::GetGroup()
{
	BoneData* pLimbData = GetBoneData();
	if (pLimbData != NULL)
		return pLimbData->GetGroup();
	return NULL;
}
