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

#include "IKTargController.h"
#include "CATClipValue.h"
#include "LimbData2.h"
#include "Hub.h"
#include "HDPivotTrans.h"

#include "../CATObjects/IKTargetObject.h"
#include "../DataRestoreObj.h"

class IKTargTransClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		IKTargTrans* iktargetttrans = new IKTargTrans(loading);
		return iktargetttrans;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_IKTARGET_TRANSFORM); }
	Class_ID		ClassID() { return IKTARGTRANS_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("IKTargTrans"); }	// returns fixed parsable name (scripter-visible name)
};

static IKTargTransClassDesc IKTargTransDesc;
ClassDesc2* GetIKTargTransDesc() { return &IKTargTransDesc; }

IKTargTrans::IKTargTrans(BOOL /*loading*/)
{
	limb = NULL;
	mRigID = idIKTargetValues;
}

IKTargTrans::~IKTargTrans()
{
	DeleteAllRefs();
}

void IKTargTrans::PostCloneNode()
{
}

RefTargetHandle IKTargTrans::Clone(RemapDir& remap)
{
	IKTargTrans *newIKTargTrans = new IKTargTrans(FALSE);

	// clone our subcontrollers and assign them to the new object.
	if (mLayerTrans)	newIKTargTrans->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));

	CloneCATNodeControl(newIKTargTrans, remap);

	DbgAssert(limb);
	remap.PatchPointer((RefTargetHandle*)&newIKTargTrans->limb, (RefTargetHandle)limb);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newIKTargTrans, remap);

	// now return the new object.
	return newIKTargTrans;
}

Animatable* IKTargTrans::SubAnim(int i)
{
	switch (i)
	{
	case SUBTRANS:		return mLayerTrans;
	default:			return NULL;
	}
}

TSTR IKTargTrans::SubAnimName(int i)
{
	switch (i)
	{
	case SUBTRANS:		return GetString(IDS_LAYERTRANS);
	default:			return _T("");
	}
}

void IKTargTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;

	if (limb) limb->BeginEditParams(ip, flags, prev);

	CATNodeControl::BeginEditParams(ip, flags, prev);
}

void IKTargTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);

	CATNodeControl::EndEditParams(ip, END_EDIT_REMOVEUI);

	if (limb) limb->EndEditParams(ip, END_EDIT_REMOVEUI);
}

RefResult IKTargTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	if (msg == REFMSG_TARGET_DELETED && hTarg == mLayerTrans) mLayerTrans = NULL;

	return REF_SUCCEED;
}

void IKTargTrans::SetLimbData(LimbData2 *owningLimb)
{
	HoldData(limb);
	limb = owningLimb;
}

CATControl* IKTargTrans::GetParentCATControl()
{
	return (CATControl*)GetLimbData();
}

void IKTargTrans::ClearParentCATControl()
{
	SetLimbData(NULL);
}

TSTR IKTargTrans::GetRigName()
{
	LimbData2 *limb = GetLimbData();
	assert(GetCATParentTrans() && limb);
	return GetCATParentTrans()->GetCATName() + limb->GetName() + GetName();
}

// Create a whole structure for CAT
INode* IKTargTrans::Initialise(LimbData2 *limb, CATClipMatrix3 *layers, USHORT rigID, BOOL loading)
{
	this->limb = (LimbData2*)limb;
	this->mRigID = rigID;
	ReplaceReference(LAYERTRANS, layers);

	IKTargetObject* obj = (IKTargetObject*)CreateInstance(HELPER_CLASS_ID, IKTARGET_OBJECT_CLASS_ID);
	obj->SetCross(TRUE);
	obj->SetCATUnits(GetCATUnits());

	INode* node = CreateNode(obj);
	node->SetWireColor(asRGB(limb->GetGroupColour()));

	if (!loading) {
		name = GetString(IDS_CL_IKTARGET_OBJECT);
		UpdateName();
	}

	return node;
}

// user Properties.
// these will be used by the runtime libraries
void	IKTargTrans::UpdateUserProps() {

	INode* node = GetNode();
	if (!node) return;

	CATNodeControl::UpdateUserProps();
}

// Limb Creation/Maintenance etc.
BOOL IKTargTrans::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(GetRigID());

	// if we are saving out a pose then we can
	// just save out the world space matrix for this bone
	if (!(flags&CLIPFLAG_CLIP)) {
		Matrix3 tm = GetNode()->GetNodeTM(timerange.Start());
		save->Write(idValMatrix3, tm);
	}
	else {
		if (mLayerTrans) {
			save->BeginGroup(idController);
			mLayerTrans->SaveClip(save, flags, timerange, layerindex);
			save->EndGroup();
		}
	}

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

BOOL IKTargTrans::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	assert(mLayerTrans);

	BOOL done = FALSE;
	BOOL ok = TRUE;

	INode *parentNode = GetParentNode();
	if (parentNode->IsRootNode())
		flags |= CLIPFLAG_WORLDSPACE;
	else flags &= ~CLIPFLAG_WORLDSPACE;

	// In the old days we didn't have an IKTarget controller,
	// We loaded data straight into the LayerTrans
	if (load->GetVersion() < CAT_VERSION_1210)
	{
		mLayerTrans->LoadClip(load, range, layerindex, dScale, flags);
		return TRUE;
	}

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idPlatform && load->CurGroupID() != GetRigID()) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idController:
				mLayerTrans->LoadClip(load, range, layerindex, dScale, flags);
				break;
			case idArbBones:
				LoadClipArbBones(load, range, layerindex, dScale, flags);
				break;
			case idExtraControllers:
				LoadClipExtraControllers(load, range, layerindex, dScale, flags);
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idValMatrix3: {
				flags |= CLIPFLAG_WORLDSPACE;
				Matrix3 val;
				// this method will do all the processing of the pose for us
				if (load->GetValuePose(flags, SuperClassID(), (void*)&val)) {
					INode* pNode = GetNode();
					if (pNode != NULL)
						pNode->SetNodeTM(range.Start(), val);
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

void IKTargTrans::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	switch (ctxt) {
	case kSNCClone:
		// A clone is either a limb, or the whole character.
		if (GetCATMode() == SETUPMODE)
		{
			LimbData2* pLimb = GetLimbData();
			if (pLimb != NULL)
				pLimb->AddSystemNodes(nodes, ctxt);
		}
		else
			GetCATParentTrans()->AddSystemNodes(nodes, ctxt);
		break;
	case kSNCDelete:
		// We always support deleting this element by itself.
		AddSystemNodes(nodes, ctxt);
		break;
	default:
		CATNodeControl::GetSystemNodes(nodes, ctxt);
	}
}

int IKTargTrans::Display(TimeValue t, INode*, ViewExp *vpt, int flags)
{
	Interval ivDontCare;
	if (limb && limb->GetIKFKRatio(t, ivDontCare) < 1.0f && limb->TestFlag(LIMBFLAG_DISPAY_FK_LIMB_IN_IK)) {
		theHold.Suspend();
		int data = -1;

		limb->SetFlag(LIMBFLAG_LOCKED_FK);
		limb->CATMessage(CAT_INVALIDATE_TM, data);

		// now force the character to dispaly the current layer
		Box3 bbox;
		limb->DisplayLayer(t, vpt, flags, bbox);

		// put things back the way they were
		limb->ClearFlag(LIMBFLAG_LOCKED_FK);
		limb->CATMessage(CAT_INVALIDATE_TM, data);

		theHold.Resume();
	}
	return 1;
}

// TODO: finnish this off
/*
void IKTargTrans::GetWorldBoundBox(TimeValue t, INode *inode, ViewExp* vpt, Box3& box)
{
	box += bbox;
}
*/

BOOL IKTargTrans::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (pastectrl->ClassID() != ClassID()) return FALSE;

	IKTargTrans* pasteiktarget = (IKTargTrans*)pastectrl;

	// if we are pasting a
	if (flags&PASTERIGFLAG_MIRROR) {
		SetMirrorBone(pasteiktarget->GetNode());
	}

	if (!(flags&PASTERIGFLAG_DONT_PASTE_CONTROLLER)) {
		Matrix3 tmSetup = pasteiktarget->GetSetupMatrix();
		TimeValue t = GetCOREInterface()->GetTime();
		if (pasteiktarget->GetLimbData()->GetHub() != GetLimbData()->GetHub()) {
			Matrix3 tmHubOffset = pasteiktarget->GetNodeTM(t) * Inverse(pasteiktarget->GetLimbData()->GetHub()->GetNodeTM(t));
			tmSetup = (tmHubOffset * pasteiktarget->GetLimbData()->GetHub()->GetNodeTM(t));
		}

		if (flags&PASTERIGFLAG_MIRROR) {
			if (GetLengthAxis() == Z)	MirrorMatrix(tmSetup, kXAxis);
			else					MirrorMatrix(tmSetup, kZAxis);
		}
		tmSetup.SetTrans(tmSetup.GetTrans() * scalefactor);
		SetSetupMatrix(tmSetup);

		mLayerTrans->SetFlags(pasteiktarget->GetLayerTrans()->GetFlags());
	}

	ICATObject* iobj = GetICATObject();
	ICATObject* pasteiobj = pasteiktarget->GetICATObject();
	if (iobj && pasteiobj)	iobj->PasteRig(pasteiobj, flags, scalefactor);

	return TRUE;
}

void IKTargTrans::CATMessage(TimeValue t, UINT msg, int data)
{
	CATNodeControl::CATMessage(t, msg, data);

	// Note: Special case hack.
	// We want to apply a special conroller to the foot to handle
	// pivot positioning. After the layer has been
	if (msg == CLIP_LAYER_INSERT)
	{
		if (mLayerTrans->GetLayerMethod(data) == LAYER_ABSOLUTE)
		{
			// TODO: debug the pivot trans controller. Right now it has issues.
			// the 1st keyframes created don't seem to work at all with the pivot system.
			// there are a few wierdities when working with pivots. It needs to be rock solid
			Control* pivottrans = (Control*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HD_PIVOTTRANS_CLASS_ID);
			DbgAssert(pivottrans);
			mLayerTrans->SetLayer(data, pivottrans);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class IKTargPLCB : public PostLoadCallback {
protected:
	IKTargTrans *bone;
	ICATParentTrans	*catparent;

public:
	IKTargPLCB(IKTargTrans *pOwner) : catparent(NULL) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		catparent = bone->GetCATParentTrans();
		if (bone->GetFileSaveVersion() > 0) return bone->GetFileSaveVersion();

		DbgAssert(catparent);
		if (catparent)
			return catparent->GetFileSaveVersion();

		return -1;
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		//		if (GetFileSaveVersion() < CAT_VERSION_1700){
		//
		//			//	bone->catparent = catparent;
		//			//	LimbData2* limb = bone->GetLimbData();
		//			//	if(limb) bone->node = limb->GetIKTarget();
		//
		//				iload->SetObsolete();
		//			}

		if (GetFileSaveVersion() < CAT3_VERSION_3003) {
			if (bone->GetLimbData() && bone->GetLimbData()->GetUpNode() == bone->GetNode())
				bone->mRigID = idUpVectorValues;
		}

		if (GetFileSaveVersion() < CAT3_VERSION_3300) {
			if (bone->mRigID != idIKTargetValues && bone->mRigID != idUpVectorValues && bone->mRigID != idPlatform) {
				if (((LimbData2*)bone->GetLimbData())->GetisLeg()) {
					bone->mRigID = idPlatform;
				}
				else {
					bone->mRigID = idIKTargetValues;
				}
			}
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

#define		IKTARGCHUNK_CATNODECONTROL	0
#define		IKTARGCHUNK_LIMB			1
#define		IKTARGCHUNK_RIGID			2

IOResult IKTargTrans::Save(ISave *isave)
{
	IOResult res = IO_OK;
	ULONG id;
	DWORD nb;//, r

	isave->BeginChunk(IKTARGCHUNK_CATNODECONTROL);
	res = CATNodeControl::Save(isave);
	isave->EndChunk();

	isave->BeginChunk(IKTARGCHUNK_LIMB);
	id = isave->GetRefID(limb);
	isave->Write(&id, sizeof(ULONG), &nb);
	isave->EndChunk();

	isave->BeginChunk(IKTARGCHUNK_RIGID);
	isave->Write(&mRigID, sizeof(ULONG), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult IKTargTrans::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L;
	ULONG id = 0L;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case IKTARGCHUNK_CATNODECONTROL:
			CATNodeControl::Load(iload);
			break;

		case IKTARGCHUNK_LIMB:
			res = iload->Read(&id, sizeof(ULONG), &nb);
			if (res == IO_OK && id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&limb);
			break;
		case IKTARGCHUNK_RIGID:
			iload->Read(&mRigID, sizeof(ULONG), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new IKTargPLCB(this));

	return IO_OK;
}
