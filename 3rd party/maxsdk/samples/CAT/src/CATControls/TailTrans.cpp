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


// A very basic tail controller, can do Multiple-Seg Freeform
//	or CATMotion based on hub's extra rot.

#include "CatPlugins.h"

#include <CATAPI/CATClassID.h>
#include "decomp.h"

#include "CATRigPresets.h"

 // Rig Structure
#include "TailTrans.h"
#include "TailData2.h"
#include "Hub.h"

// Layer
#include "CATClipRoot.h"
#include "CATClipValue.h"

//
//	TailTransClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class TailTransClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		TailTrans* bone = new TailTrans(loading);
		return bone;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_TAILTRANS); }
	Class_ID		ClassID() { return TAILTRANS_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("TailTrans"); }			// returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
static TailTransClassDesc TailTransDesc;
ClassDesc2* GetTailTransDesc() { return &TailTransDesc; }

static ParamBlockDesc2 TailTrans_param_blk(TailTrans::PBLOCK_REF, _T("TailTransParams"), 0, &TailTransDesc,
	P_AUTO_CONSTRUCT, TailTrans::PBLOCK_REF,
	TailTrans::PB_TAILDATA, _T("Tail"), TYPE_REFTARG, 0, IDS_CL_TAILDATA,
		p_end,
	p_end
);

TailTrans::TailTrans(BOOL loading)
	: pblock(NULL)
{
	TailTransDesc.MakeAutoParamBlocks(this);

	ccflags |= CCFLAG_SETUP_STRETCHY;
	ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	ccflags |= CCFLAG_EFFECT_HIERARCHY;

	if (!loading)
	{
	}
}

TailTrans::~TailTrans()
{
	DeleteAllRefs();
}

RefTargetHandle TailTrans::Clone(RemapDir& remap)
{
	// make a new TailTrans object to be the clone
	// call true for loading so the new TailTrans doesn't
	// make new default subcontrollers.
	TailTrans *newTailTrans = new TailTrans(TRUE);
	remap.AddEntry(this, newTailTrans);

	newTailTrans->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));
	if (GetLayerTrans())	newTailTrans->ReplaceReference(LAYERTRANS, remap.CloneRef(GetLayerTrans()));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newTailTrans, remap);

	TailData2 *tail = (TailData2*)remap.FindMapping(GetTail());
	newTailTrans->pblock->SetValue(PB_TAILDATA, 0, tail);

	BaseClone(this, newTailTrans, remap);

	// now return the new object.
	return newTailTrans;
}

void TailTrans::Copy(Control *from)
{
	// We could only copy from existing tailTrans
	TailTrans *newTail = dynamic_cast<TailTrans*>(from);
	if (newTail != NULL && newTail->GetLayerTrans())
		ReplaceReference(LAYERTRANS, newTail->GetLayerTrans());
}

void TailTrans::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	// If we are being called, we can guarantee
	// that we have no further ancestors selected
	SetXFormPacket* xForm = (SetXFormPacket*)val;
	switch (xForm->command)
	{
	case XFORM_ROTATE:
	{
		TailData2* pTail = GetTail();
		if (pTail == NULL)
			break;
		Hub* pHub = pTail->GetHub();
		if (pHub == NULL)
			break;
		// Specifically for the english dragon, we need to support DangleRatio
		// This parameter is technically deprecated, but it still affects the way this
		// rig moves.
		if (pHub->GetDangleRatio(t) > 0.0f)
		{
			int iBoneID = GetBoneID();
			int nAncestorsSelected = NumSelectedBones(iBoneID);
			xForm->aa.angle *= nAncestorsSelected;
		}
	}
	case XFORM_SCALE:
	{
		// If we are the first bone to recieve this set value
		if (!IsAncestorSelected(GetBoneID()))
		{
			int nBoneSel = NumSelectedBones();
			xForm->p /= (float)nBoneSel;
		}
		break;
	}
	}

	CATNodeControlDistributed::SetValue(t, val, commit, method);
}

void TailTrans::CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	CATNodeControl::CalcParentTransform(t, tmParent, p3ParentScale);

	// Do not apply OrientAlign if we are setting values automatically (KeyFreeform & CollapseLayers)
	if ((mLayerTrans != NULL && !mLayerTrans->TestFlag(CLIP_FLAG_KEYFREEFORM)) &&
		(GetCATParentTrans() && !GetCATParentTrans()->GetCATLayerRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS)))
	{
		TailData2* pTail = GetTail();
		Hub* hub = pTail->GetHub();
		float dOrientAlign = hub->GetDangleRatio(t);
		if (dOrientAlign > 0.0f)
		{
			Matrix3 tmHub = hub->GetNodeTM(t);
			Matrix3 tmOrient = tmHub;
			hub->GettmOrient(t, tmOrient);

			BlendRot(tmHub, tmOrient, pTail->GetTailStiffness(GetBoneID()));
			BlendRot(tmParent, tmHub, dOrientAlign);
		}
	}
}

RefTargetHandle TailTrans::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:	return pblock;
	case LAYERTRANS:	return mLayerTrans;
	default:			return NULL;
	}
}

void TailTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:	pblock = (IParamBlock2*)rtarg; break;
	case LAYERTRANS:	mLayerTrans = (CATClipMatrix3*)rtarg; break;
	}
}

// Manually returning TailData2's pblock, because ReferenceTargets arent
// asked for their subanims in trackview
// pblock subanim index our_max_plus_one... or NUMPARAMS
#define TAILDATA_PBLOCK NUMPARAMS

Animatable* TailTrans::SubAnim(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return GetTail();
	case LAYERTRANS:		return mLayerTrans;
		//	case SCALE:				return tail;
	default:				return NULL;
	}

}

TSTR TailTrans::SubAnimName(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return GetString(IDS_CL_TAILDATA); // GetString(IDS_PARAMS);
	case LAYERTRANS:		return GetString(IDS_LAYERTRANS);
	default:				return _T("");
	}
}

RefResult TailTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		break;
	case REFMSG_TARGET_DELETED:
		if (mLayerTrans == hTarg) mLayerTrans = NULL;
		break;
	}
	return REF_SUCCEED;

}

void TailTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	CATNodeControl::BeginEditParams(ip, flags, prev);

	TailData2 *tail = GetTail();
	if (tail) tail->BeginEditParams(ip, flags, prev);

	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void TailTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{

	TailData2 *tail = GetTail();
	if (tail) tail->EndEditParams(ip, flags, next);

	CATNodeControl::EndEditParams(ip, flags, next);
}

//////////////////////////////////////////////////////////////////////////

TSTR TailTrans::GetRigName()
{
	assert(GetCATParentTrans());
	TSTR bonename = GetName();
	// if the bonename has not been initialised, use the boneID
	TailData2* pTail = GetTail();
	if (pTail->GetNumBones() > 1 && bonename.isNull()) {
		bonename.printf(_T("%d"), (GetBoneID() + 1));
	}
	return GetCATParentTrans()->GetCATName() + pTail->GetName() + bonename;
}

void TailTrans::SetObjX(float val)
{
	TailData2* pTail = GetTail();
	if (pTail != NULL)
	{
		Point3 p3Scale(1, 1, 1);
		if (GetLengthAxis() == Z)
			p3Scale[X] = val / GetObjX();
		else
			p3Scale[Z] = val / GetObjX();
		pTail->ScaleTailSize(GetCOREInterface()->GetTime(), p3Scale);
	}
}

void TailTrans::SetObjY(float val)
{
	// Y is always the width of the tail (handled by the Tail)
	TailData2* pTail = GetTail();
	if (pTail != NULL)
	{
		float dYScale = val / GetObjY();
		pTail->ScaleTailSize(GetCOREInterface()->GetTime(), Point3(1.0f, dYScale, 1.0f));
	}
}

void TailTrans::SetObjZ(float val)
{
	if (GetLengthAxis() == X)
		CATNodeControl::SetObjZ(val);
	else
		CATNodeControl::SetObjZ(val);

}

CATGroup* TailTrans::GetGroup()
{
	TailData2* pTail = GetTail();
	if (pTail != NULL)
		return pTail->GetGroup();
	return NULL;
}

CATControl* TailTrans::GetParentCATControl()
{
	return GetTail();
}

// this function is called to recolur the bones accurding to he current colour mode
Color TailTrans::GetCurrentColour(TimeValue t)
{
	Hub *baseHub = GetTail()->GetHub();
	DbgAssert(baseHub);

	Color baseColour = baseHub->GetCurrentColour(t);
	Color tipColour = CATNodeControl::GetCurrentColour(t);
	if (GetCATMode() != SETUPMODE &&
		GetCATParentTrans()->GetEffectiveColourMode() != COLOURMODE_CLASSIC) return tipColour;

	return baseColour + ((tipColour - baseColour) * GetTail()->GetTailStiffness(GetBoneID()));
}

INode *TailTrans::Initialise(TailData2 *tail, int id, BOOL loading)
{
	SetBoneID(id);
	pblock->SetValue(PB_TAILDATA, 0, tail);

	CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), tail->GetClipWeights(), GetCATParentTrans(), loading);
	ReplaceReference(LAYERTRANS, layerController);

	if (id == 0)	ClearCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
	else		SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);

	Interface *ip = GetCOREInterface();

	Object *obj = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	DbgAssert(obj);

	INode* pNode = CreateNode(obj);

	// If there are existing tail segments, extract the setup TM of
	// the last segment, to be used as the setup TM for the new tail
	// segment.  Otherwise the setup TM will remain as the identity.
	if (!loading) {
		if (id > 1) {
			Matrix3 tmSetup;
			tail->GetBone(id - 1)->GetLayerTrans()->GetSetupVal((void*)&tmSetup);
			mLayerTrans->SetSetupVal((void*)&tmSetup);
		}

		TSTR bonename;
		bonename.printf(_T("%d"), id + 1);
		SetName(bonename);
	}

	return pNode;
};

CATNodeControl* TailTrans::FindParentCATNodeControl()
{
	TailData2* pTail = GetTail();
	if (pTail != NULL)
	{
		CATNodeControl* pParent = NULL;
		int iBoneID = GetBoneID();
		if (iBoneID > 0)
			pParent = pTail->GetBone(iBoneID - 1);
		else
			pParent = pTail->GetHub();

		return pParent;
	}
	return NULL;
}

int	TailTrans::NumChildCATNodeControls() {
	TailData2* pTail = GetTail();
	if (pTail != NULL && GetBoneID() < (pTail->GetNumBones() - 1)) return 1;
	return 0;
};

CATNodeControl*	TailTrans::GetChildCATNodeControl(int i)
{
	if (i == 0)
	{
		int iBoneID = GetBoneID();
		TailData2* pTail = GetTail();
		if (pTail != NULL && iBoneID < (pTail->GetNumBones() - 1)) 	return pTail->GetBone(iBoneID + 1);
	}
	return NULL;
};

TailData2* TailTrans::GetTail()
{
	return (TailData2*)pblock->GetReferenceTarget(PB_TAILDATA);
}

IBoneGroupManager* TailTrans::GetManager()
{
	return GetTail();
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class TailTransPLCB : public PostLoadCallback {
protected:
	TailTrans	*bone;

public:
	TailTransPLCB(TailTrans *pOwner) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		if (bone->GetFileSaveVersion() > 0) return bone->GetFileSaveVersion();

		ICATParentTrans *catparent = bone->GetCATParentTrans();
		if (catparent) return catparent->GetFileSaveVersion();
		// oldschool
		catparent = bone->GetTail()->GetCATParentTrans();
		DbgAssert(catparent);
		return catparent->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		if (GetFileSaveVersion() < CAT_VERSION_2447) {
			bone->UpdateObjDim();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

/**********************************************************************
 * Loading and saving....
 */
#define		TAILTRANSCHUNK_CATNODECONTROL		0

IOResult TailTrans::Save(ISave *isave)
{
	IOResult res = IO_OK;

	isave->BeginChunk(TAILTRANSCHUNK_CATNODECONTROL);
	CATNodeControl::Save(isave);
	isave->EndChunk();

	return res;
}

IOResult TailTrans::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case TAILTRANSCHUNK_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new TailTransPLCB(this));

	return IO_OK;
}