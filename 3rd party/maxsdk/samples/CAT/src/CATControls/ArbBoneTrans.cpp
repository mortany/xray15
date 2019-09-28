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
#include <CATAPI/CATClassID.h>

#include "ArbBoneTrans.h"
#include "BoneSegTrans.h"
#include "CATClipWeights.h"

 //
 //	ArbBoneTransClassDesc
 //
 //	This gives the MAX information about our class
 //	before it has to actually implement it.
 //
class ArbBoneTransClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl *	DoCreate(BOOL loading = FALSE)
	{
		ArbBoneTrans* bone = new ArbBoneTrans(loading);
		return bone;
	}
	const TCHAR *	ClassName() { return _T("ArbBone"); }
	Class_ID		ClassID() { return ARBBONETRANS_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("ArbBoneTrans"); }			// returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
static ArbBoneTransClassDesc ArbBoneTransDesc;
ClassDesc2* GetArbBoneTransDesc() { return &ArbBoneTransDesc; }

ArbBoneTrans::ArbBoneTrans(BOOL /*loading*/) : dwColour(0L)
{
	mpGroup = NULL;
	weights = NULL;
}

ArbBoneTrans::~ArbBoneTrans()
{
	DeleteAllRefs();
}

class PostPatchArbBoneClone : public PostPatchProc
{
private:
	CATNodeControl* mpOwner;
	ArbBoneTrans* mpClonedBone;

public:
	PostPatchArbBoneClone(ArbBoneTrans* pClonedClass, CATNodeControl* pOwner)
		: mpOwner(pOwner)
		, mpClonedBone(pClonedClass)
	{
		DbgAssert(mpOwner != NULL && mpClonedBone != NULL);
	}

	~PostPatchArbBoneClone() {};

	// Our proc needs to set the cloned handles on the Cloned ERNode class
	int Proc(RemapDir& remap)
	{
		// Find the clone of the old parent.
		CATNodeControl* pNewOwner = (CATNodeControl*)remap.FindMapping(mpOwner);
		// If the parent wasn't cloned, keep the original pointer
		if (pNewOwner == NULL)
			pNewOwner = mpOwner;

		DbgAssert(pNewOwner != NULL);
		if (pNewOwner == NULL)
			return FALSE;

		// Give the owner pointers back to the Arb bones
		if (pNewOwner->GetArbBone(mpClonedBone->GetBoneID()) != mpClonedBone)
			pNewOwner->InsertArbBone(pNewOwner->GetNumArbBones(), mpClonedBone);
		else
			mpClonedBone->SetParentCATNodeControl(pNewOwner);

		// Just in case the CATParent has been cloned as well...
		mpClonedBone->SetCATParentTrans(pNewOwner->GetCATParentTrans());
		return TRUE;
	}
};

RefTargetHandle ArbBoneTrans::Clone(RemapDir& remap)
{
	// make a new ArbBoneTrans object to be the clone
	// call true for loading so the new ArbBoneTrans doesn't
	// make new default subcontrollers.
	ArbBoneTrans *newArbBoneTrans = new ArbBoneTrans(TRUE);

	// Hook up the parent pointer
	CATNodeControl* pOwner = GetParentCATNodeControl(true);

	// Hook up the CATParent pointer
	newArbBoneTrans->SetCATParentTrans(GetCATParentTrans());

	// The post clone proc handles setting the parent pointer properly
	// in case the parent is being cloned as well.
	remap.AddPostPatchProc(new PostPatchArbBoneClone(newArbBoneTrans, pOwner), true);

	if (mLayerTrans)
		newArbBoneTrans->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));
	if (weights != NULL)
		newArbBoneTrans->ReplaceReference(WEIGHTS, remap.CloneRef(weights));

	remap.PatchPointer((RefTargetHandle*)&newArbBoneTrans->mpGroup, mpGroup);

	newArbBoneTrans->dwColour = dwColour;

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newArbBoneTrans, remap);

	BaseClone(this, newArbBoneTrans, remap);

	// now return the new object.
	return newArbBoneTrans;
}

CATNodeControl* ArbBoneTrans::FindParentCATNodeControl()
{
#ifdef _DEBUG
	if (theHold.Holding() && !TestAFlag(A_LOCK_TARGET))
	{
		// This function -should- never be called.  It is not possible
		// to find your way back to your owner.
		DbgAssert(!"This function -should- never be called");
	}
#endif
	return NULL;
}

void ArbBoneTrans::CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	CATNodeControl* pParentCtrl = GetParentCATNodeControl();
	if (pParentCtrl != NULL)
	{
		if (pParentCtrl->ClassID() == BONESEGTRANS_CLASS_ID)
			pParentCtrl->CATNodeControl::ApplyChildOffset(t, tmParent);
		else
			pParentCtrl->ApplyChildOffset(t, tmParent);
	}
	CalcInheritance(t, tmParent, p3ParentScale);
}

void ArbBoneTrans::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	switch (ctxt)
	{
		// if we are merging, force the whole character to be merged
	case kSNCDelete:
		if (GetCATMode() == SETUPMODE) {
			AddSystemNodes(nodes, ctxt);
		}
		else
		{
			ICATParentTrans* pCATParent = GetCATParentTrans();
			if (pCATParent != NULL)
				pCATParent->AddSystemNodes(nodes, ctxt);
		}
		break;
	default:
		CATNodeControl::GetSystemNodes(nodes, ctxt);
	}
}

void ArbBoneTrans::Copy(Control *from)
{
	// This copy function is a little bit junk.  It doesn't handle things like
	// copying between groups, copying between rigs, or basically any copying
	// at all.  I don't think it will ever get called, however if you see
	// this assert please contact someone on the CAT team to take a look at it.
	DbgAssert(FALSE && _T("Please Contact CAT Team to fix this bug"));

	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		ArbBoneTrans *newctrl = (ArbBoneTrans*)from;

		if (newctrl->mLayerTrans)	ReplaceReference(ArbBoneTrans::LAYERTRANS, newctrl->mLayerTrans);
		if (newctrl->weights)		ReplaceReference(WEIGHTS, newctrl->weights);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

RefTargetHandle ArbBoneTrans::GetReference(int i)
{
	switch (i)
	{
	case LAYERTRANS:		return mLayerTrans;
	case WEIGHTS:			return weights;
	default:				return NULL;
	}
}

void ArbBoneTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case LAYERTRANS:		mLayerTrans = dynamic_cast<CATClipMatrix3*>(rtarg);		break;
	case WEIGHTS:			weights = dynamic_cast<CATClipWeights*>(rtarg);			break;
	}

}

Animatable* ArbBoneTrans::SubAnim(int i)
{
	switch (i)
	{
	case LAYERTRANS:		return mLayerTrans;
	case WEIGHTS:			return weights;
	default:				return NULL;
	}
}

TSTR ArbBoneTrans::SubAnimName(int i)
{
	switch (i)
	{
	case LAYERTRANS:		return GetString(IDS_LAYERTRANS);
	case WEIGHTS:			return GetString(IDS_WEIGHTS);
	default:				return _T("");
	}
}

RefResult ArbBoneTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:

		break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (mLayerTrans == hTarg)	mLayerTrans = NULL;
		break;
	}
	return REF_SUCCEED;
}

void* ArbBoneTrans::GetInterface(ULONG id)
{
	// Do NOT return a master control.  The problem with this is
	// that Max will automatically delete the master and all siblings
	// with this controller if we returned it.
	if (id == I_MASTER)
		return NULL;
	// Otherwise, the default implementation is good!
	return CATNodeControl::GetInterface(id);
}

void ArbBoneTrans::Initialise(CATGroup* group, CATNodeControl* ctrlParent, int boneid, BOOL loading, bool bCreateNode /*= false*/)
{
	if (loading)
		bCreateNode = true;

	SetBoneID(boneid);
	SetParentCATNodeControl(ctrlParent);

	if (group != NULL)
		this->mpGroup = group->AsCATControl();
	else
	{
		// We are creating a new group.  In this case, we can either
		// be a new sub-group (if we have a parent) or we can be a new
		// root group.  If we are a root group, we have no parent, but
		// need to find the CATParentTrans
		ICATParentTrans* pCATParent = NULL;
		CATClipWeights* pParentWeights = NULL;
		if (ctrlParent != NULL)
		{
			pCATParent = ctrlParent->GetCATParentTrans();
			CATGroup* pParentGroup = ctrlParent->GetGroup();
			if (pParentGroup != NULL)
				pParentWeights = pParentGroup->GetWeights();
		}
		else
		{
			// We _need_ to have a CATParent set here.  If we do not, we be pooched
			pCATParent = GetCATParentTrans(false);
			DbgAssert(pCATParent);
		}
		// We do not have an owning group.  This isn't a good thing for a
		// CAT bone, but arb bones are special in that they have no pre-defined
		// group, so its possible to be created without one.  In this case,
		// this bone will form its own group, and any bones created from this bone will
		// reference it for as their group controller.
		CATClipWeights* newweights = CreateClipWeightsController(pParentWeights, pCATParent, loading);
		ReplaceReference(WEIGHTS, newweights);
	}
	CATClipValue* layerArbBone = CreateClipValueController(GetCATClipMatrix3Desc(), GetClipWeights(), GetCATParentTrans(), loading);
	ReplaceReference(LAYERTRANS, layerArbBone);

	Interface* ip = GetCOREInterface();

	//////////////////////////////////////////////////////////////////////////

	if (bCreateNode)
	{
		Object* objArbBone = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
		INode* node = CreateNode(objArbBone);

		// If we do not have our own group, then we
		// can simply cache our new bones wire-colour
		if (group == NULL)
			dwColour = node->GetWireColor();
		else
		{
			dwColour = asRGB(group->GetGroupColour());
			node->SetWireColor(dwColour);
		}
	}

	if (!loading)
	{
		// If our parent was another ArbBone, clone its values
		if (ctrlParent != NULL && ctrlParent->ClassID() == ClassID())
			PasteRig(ctrlParent, PASTERIGFLAG_DONT_PASTE_CHILDREN | PASTERIGFLAG_DONT_PASTE_FLAGS | PASTERIGFLAG_DONT_PASTE_CONTROLLER | PASTERIGFLAG_DONT_PASTE_MESHES, 1.0f);
		else
		{
			// give the object some default values
			ICATObject* pCatObject = GetICATObject();
			if (pCatObject != NULL)
			{
				pCatObject->SetZ(10.0f);
				pCatObject->SetY(5.0f);
				pCatObject->SetX(5.0f);
			}
		}

		UpdateCATUnits();

		TSTR bonename;
		bonename.printf(GetString(IDS_BONESTR), boneid);
		if (ctrlParent != NULL)
			bonename = ctrlParent->GetName() + bonename;
		SetName(bonename);
		UpdateName();

		// Make sure this name hasn't been used.
		bonename = GetRigName();
		TSTR uniquename = bonename;
		ip->MakeNameUnique(uniquename);
		if (bonename != uniquename)
		{
			// Take the CATName back off
			TSTR catname = GetCATParentTrans()->GetCATName();
			bonename = uniquename.Substr(catname.Length(), uniquename.Length() - catname.Length());
			SetName(bonename);
		}

		INode* pNode = GetNode();
		if (pNode != NULL)
		{
			INodeTab flash;
			flash.Append(1, &pNode, 10);
			ip->FlashNodes(&flash);
		}

		ip->RedrawViews(GetCurrentTime());
	}
	else
	{
		UpdateCATUnits();
	}
}

int ArbBoneTrans::NumLayerControllers()
{
	// The bone adds 1 new layer controller (the weights)
	return 1 + CATNodeControl::NumLayerControllers();
}

CATClipValue* ArbBoneTrans::GetLayerController(int i)
{
	// Other controllers with weights assign them to index 0
	if (i == 0) return weights;
	return CATNodeControl::GetLayerController(i - 1);
}

BOOL ArbBoneTrans::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;
	ArbBoneTrans *pastearb = (ArbBoneTrans*)pastectrl;

	if (!(flags&PASTERIGFLAG_DONT_CHECK_FOR_LOOP)) {
		// check to see if we are int the subhierarchy of the pastctrl
		INodeTab nodes;
		pastearb->AddSystemNodes(nodes, (SysNodeContext)-1);
		for (int i = (nodes.Count() - 1); i >= 0; i--)
			if (nodes[i] && nodes[i]->GetTMController() == this) {
				flags |= PASTERIGFLAG_DONT_PASTE_CHILDREN;
				flags |= PASTERIGFLAG_DONT_PASTE_CONTROLLER;
			}
		// check to see if the pastctrl is in our subhierarchy
		nodes.SetCount(0);
		AddSystemNodes(nodes, (SysNodeContext)-1);
		for (int i = (nodes.Count() - 1); i >= 0; i--)
			if (nodes[i] && nodes[i]->GetTMController() == pastectrl) {
				flags |= PASTERIGFLAG_DONT_PASTE_CHILDREN;
				flags |= PASTERIGFLAG_DONT_PASTE_CONTROLLER;
			}
		flags &= ~PASTERIGFLAG_DONT_CHECK_FOR_LOOP;
	}

	return CATNodeControl::PasteRig(pastectrl, flags, scalefactor);
}

/**********************************************************************
 * Loading and saving....
 */
 //////////////////////////////////////////////////////////////////////
 // Backwards compatibility
 //

#define		ARBBONECHUNK_CATNODECONTROL			1
#define		ARBBONECHUNK_OWNER_CATNODECONTROL	2
#define		ARBBONECHUNK_ICATOBJECT				3
#define		ARBBONECHUNK_GROUP					4
#define		ARBBONECHUNK_INDEX					5
#define		ARBBONECHUNK_COLOUR					8

IOResult ArbBoneTrans::Save(ISave *isave)
{
	IOResult res = IO_OK;
	DWORD nb;//, refID;
	ULONG id;

	isave->BeginChunk(ARBBONECHUNK_CATNODECONTROL);
	res = CATNodeControl::Save(isave);
	isave->EndChunk();

	isave->BeginChunk(ARBBONECHUNK_GROUP);
	id = isave->GetRefID((void*)mpGroup);
	res = isave->Write(&id, sizeof(ULONG), &nb);
	isave->EndChunk();

	// One day move the colour loading and saving into CATNodeControl
	// 1 step closer to erradicating parameter blocks
	isave->BeginChunk(ARBBONECHUNK_COLOUR);
	res = isave->Write(&dwColour, sizeof(DWORD), &nb);
	isave->EndChunk();

	return res;
}

IOResult ArbBoneTrans::Load(ILoad *iload)
{

	IOResult res = IO_OK;
	DWORD nb;//, refID;
	ULONG id = 0L;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case ARBBONECHUNK_CATNODECONTROL:
			CATNodeControl::Load(iload);
			break;
		case ARBBONECHUNK_GROUP:
			res = iload->Read(&id, sizeof(int), &nb);
			if (res == IO_OK && id != (DWORD)-1)
				iload->RecordBackpatch(id, (void**)&mpGroup);
			break;
		case ARBBONECHUNK_INDEX:
		{
			int anID = -1;
			res = iload->Read(&anID, sizeof(anID), &nb);
			DbgAssert(GetBoneID() == -1 || GetBoneID() == anID);
			if (anID != -1)
				SetBoneID(anID);
		}
		break;
		case ARBBONECHUNK_COLOUR:
			res = iload->Read(&dwColour, sizeof(DWORD), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	return IO_OK;
}

Color ArbBoneTrans::GetGroupColour()
{
	return Color(dwColour);
}

void ArbBoneTrans::SetGroupColour(Color clr)
{
	dwColour = clr.toRGB();
}

