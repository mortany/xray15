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

// Very simple transform. Ensures Collar bones point in opposite directions

#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>
#include "math.h"
#include "Simpobj.h"

 // CATRug Structure
#include "ICATParent.h"

#include "Hub.h"
#include "LimbData2.h"
#include "BoneData.h"
#include "CollarBoneTrans.h"
#include "ArbBoneTrans.h"

// Layer System
#include "CATClipHierarchy.h"
#include "CATClipValues.h"

//
//	CollarBoneTrans
//
//	Our class implementation.
//
//	Steve Nov 12 2002
//
#define COLLARBONE_OBJ_ID	Class_ID(0x423e5073, 0x78797166)
#define CATCOLLARBONE_CLASS_ID		Class_ID(0x617275ee, 0xb154b81)

//
//	CollarBoneTransClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class CollarBoneTransClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl *	DoCreate(BOOL loading = FALSE) { return new CollarBoneTrans(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_COLLARBONE); }
	Class_ID		ClassID() { return COLLARBONETRANS_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("CollarBoneTrans"); }			// returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
static CollarBoneTransClassDesc CollarBoneTransDesc;
ClassDesc2* GetCollarboneTransDesc() { return &CollarBoneTransDesc; }

//	CollarBoneTrans  Implementation.
//
//	Make it work
static ParamBlockDesc2 Collarbone_t_param_blk(CollarBoneTrans::pb_idParams, _T("CollarBoneTransParams"), 0, &CollarBoneTransDesc,
	P_AUTO_CONSTRUCT, CollarBoneTrans::REF_PBLOCK,

	// Maybe we don't really need a reference to
	CollarBoneTrans::PB_LIMBDATA, _T("Limb"), TYPE_REFTARG, 0, IDS_CL_LIMBDATA,
		p_end,
	CollarBoneTrans::PB_OBJCOLLARBONE_PB, _T(""), TYPE_REFTARG, 0, NULL,
		p_end,
	CollarBoneTrans::PB_NODECOLLARBONE, _T("Node"), TYPE_INODE, P_NO_REF, NULL,
		p_end,
	CollarBoneTrans::PB_TMSETUP, _T(""), TYPE_MATRIX3, 0, NULL,
		p_end,
	CollarBoneTrans::PB_SCALE, _T(""), TYPE_POINT3, P_ANIMATABLE, IDS_SCALE,
		p_default, Point3(1, 1, 1),
		p_end,

	// PT - V1.3 18/5/2004
	CollarBoneTrans::PB_NAME, _T(""), TYPE_STRING, P_RESET_DEFAULT, NULL,
		p_default, _T("Collarbone"),
		p_ui, TYPE_EDITBOX, IDC_EDIT_NAME,
		p_end,
	p_end
);

CollarBoneTrans::CollarBoneTrans(BOOL /*loading*/)
{
	pblock = NULL;

	// defaut bones to being stretchy
	ccflags |= CCFLAG_SETUP_STRETCHY;
	ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	ccflags |= CCFLAG_EFFECT_HIERARCHY;

	CollarBoneTransDesc.MakeAutoParamBlocks(this);
}

CollarBoneTrans::~CollarBoneTrans()
{
	DeleteAllRefs();
}

RefTargetHandle CollarBoneTrans::Clone(RemapDir& remap)
{
	// make a new CollarBoneTrans object to be the clone
	// call true for loading so the new CollarBoneTrans doesn't
	// make new default subcontrollers.
	CollarBoneTrans *newCollarBoneTrans = new CollarBoneTrans(TRUE);
	// We add this entry now so that we don't get cloned again by anyone cloning a reference to us
	remap.AddEntry(this, newCollarBoneTrans);

	// clone our subcontrollers and assign them to the new object.
	if (mLayerTrans)	newCollarBoneTrans->ReplaceReference(REF_LAYERTRANS, remap.CloneRef(mLayerTrans));

	newCollarBoneTrans->ReplaceReference(REF_PBLOCK, CloneParamBlock(pblock, remap));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newCollarBoneTrans, remap);

	BaseClone(this, newCollarBoneTrans, remap);

	// now return the new object.
	return newCollarBoneTrans;
}

void CollarBoneTrans::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		CollarBoneTrans *newctrl = (CollarBoneTrans*)from;

		//		ReplaceReference(REF_PBLOCK, CloneRefHierarchy(newctrl->pblock));

		if (newctrl->mLayerTrans)	ReplaceReference(CollarBoneTrans::REF_LAYERTRANS, newctrl->mLayerTrans);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void CollarBoneTrans::ApplyChildOffset(TimeValue t, Matrix3& tmChild) const
{
	// First - we need to find the correct position (for our child,
	// the 1st bone in the limb)
	CATNodeControl::ApplyChildOffset(t, tmChild);

	// We reset the parent transform to the hub's orientation;
	// but keep the calculated position for our child.
	Point3 p3ChildPos = tmChild.GetTrans();

	// We inherit rotations from our parent
	tmChild = GetParentTM(t);
	// But we need to apply our own scale
	Point3 p3LocalScale = const_cast<CollarBoneTrans*>(this)->GetLocalScale(t);
	tmChild.PreScale(p3LocalScale);
	// And we keep the calculated translation
	tmChild.SetTrans(p3ChildPos);
}

void CollarBoneTrans::CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	// Allow standard calculation first
	CATNodeControl::CalcParentTransform(t, tmParent, p3ParentScale);

	// Modify our value slightly
	LimbData2* pLimb = GetLimbData();
	if (pLimb != NULL)
	{
		// TODO: Bake out this transformation
		tmParent.PreRotateY((float)(M_PI_2 * pLimb->GetLMR()));

		// Now switch around the scale to match the rotation
		float temp = p3ParentScale.z;
		p3ParentScale.z = p3ParentScale.x;
		p3ParentScale.x = temp;
	}
}

RefTargetHandle CollarBoneTrans::GetReference(int i)
{
	switch (i)
	{
	case REF_PBLOCK:	return pblock;
	case REF_LAYERTRANS:		return mLayerTrans;
	default:				return NULL;
	}
}

void CollarBoneTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case REF_PBLOCK:	pblock = (IParamBlock2*)rtarg;		break;
	case REF_LAYERTRANS:		mLayerTrans = (CATClipMatrix3*)rtarg;	break;
	}

}

Animatable* CollarBoneTrans::SubAnim(int i)
{
	//	return layerTrans;
	switch (i)
	{
	case SUBROTATION:	return mLayerTrans;
	case SUBSCALE:		return pblock->GetControllerByID(PB_SCALE);
	default:			return NULL;
	}
}

TSTR CollarBoneTrans::SubAnimName(int i)
{
	//	return GetString(IDS_layerTrans);
	switch (i)
	{
	case SUBROTATION:	return GetString(IDS_LAYERTRANS);
	case SUBSCALE:		return GetString(IDS_SCALE);
	default:			return _T("");
	}
}

RefResult CollarBoneTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID& /*partID*/, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if (pblock == hTarg)
		{
			int tabIndex = 0;
			int index = pblock->LastNotifyParamID(tabIndex);
			switch (index)
			{
			case PB_NAME:
				CATMessage(0, CAT_NAMECHANGE);
				break;
			}
			break;
		}
		else if (mLayerTrans == hTarg)
			InvalidateTransform();

		break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (pblock == hTarg)			pblock = NULL;
		else if (mLayerTrans == hTarg)	mLayerTrans = NULL;
		break;
	}
	return REF_SUCCEED;
}

// CATs Internal messaging system, cause using
// Max's system is like chasing mice with a mallet
void CollarBoneTrans::CATMessage(TimeValue t, UINT msg, int data) //, void* CATData)
{
	CATNodeControl::CATMessage(t, msg, data);
	/*	switch(msg)
		{
		case CAT_CATUNITS_CHANGED:
			UpdateCATUnits();
			break;
		case CAT_VISIBILITY:
			UpdateVisibility();
			break;
		case CAT_SHOWHIDE:
			GetNode()->Hide(data);
			break;
		case CAT_NAMECHANGE:
			SetCollarName();
			break;
		}

		if(layerTrans)
			layerTrans->CATMessage(t, msg, data);

		for(int i = 0; i < tabArbBones.Count(); i++)
			if(tabArbBones[i])	tabArbBones[i]->CATMessage(t, msg, data);
	*/
}

void CollarBoneTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	LimbData2* pLimb = GetLimbData();
	if (!pLimb)
		return;

	if (pLimb) pLimb->BeginEditParams(ip, flags, prev);

	// Let the CATNode manage the UIs for motino and hierarchy panel
	CATNodeControl::BeginEditParams(ip, flags, prev);

	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void CollarBoneTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	LimbData2* pLimb = GetLimbData();
	if (!pLimb)
		return;

	// Let the CATNode manage the UIs for motion and hierarchy panel
	CATNodeControl::EndEditParams(ip, flags, next);

	if (pLimb) pLimb->EndEditParams(ip, END_EDIT_REMOVEUI); // next);

}

CATGroup* CollarBoneTrans::GetGroup()
{
	LimbData2* pLimbData = GetLimbData();
	if (pLimbData != NULL)
		return pLimbData->GetGroup();
	return NULL;
}

void CollarBoneTrans::Initialise(CATControl* owner, BOOL loading)
{
	LimbData2* pLimb = dynamic_cast<LimbData2*>(owner);
	if (!pLimb)
		return;
	SetLimbData(pLimb);

	CATClipValue* layerCollarbone = CreateClipValueController(GetCATClipMatrix3Desc(), pLimb->GetClipWeights(), GetCATParentTrans(), loading);
	ReplaceReference(REF_LAYERTRANS, layerCollarbone);

	Interface* ip = GetCOREInterface();
	Object* obj = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	INode* node = CreateNode(obj);

	pblock->EnableNotifications(FALSE);
	pblock->SetValue(PB_NODECOLLARBONE, 0, node);
	//	pblock->SetValue(PB_OBJCOLLARBONE_PB, 0, (ReferenceTarget*)pblockCollarboneObj);

		//////////////////////////////////////////////////////////////////////////
		// Clip stuff
		// Plug into the Clip Hierarchy

	pblock->EnableNotifications(TRUE);

	if (!loading)
	{
		LimbData2 *symlimb = static_cast<LimbData2*>(pLimb->GetSymLimb());
		if (symlimb && symlimb->GetCollarbone())
		{
			PasteRig(symlimb->GetCollarbone(), PASTERIGFLAG_MIRROR, 1.0f);
			CATNodeControl* pBone0 = pLimb->GetBoneData(0);
			CATNodeControl* pSymBone0 = symlimb->GetBoneData(0);
			if (pBone0 != NULL && pSymBone0 != NULL)
			{
				Matrix3 tmSetup = pBone0->GetSetupMatrix();
				tmSetup.SetTrans(pSymBone0->GetSetupMatrix().GetTrans());
				pBone0->SetSetupMatrix(tmSetup);
				pBone0->SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS, pSymBone0->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS));
				pBone0->SetCCFlag(CNCFLAG_LOCK_LOCAL_POS, pSymBone0->TestCCFlag(CNCFLAG_LOCK_LOCAL_POS));
			}
		}
		else
		{
			// this bone does not need a positionional offset any more
			if (pLimb->GetNumBones() > 0) {
				CATNodeControl* pBone0 = pLimb->GetBoneData(0);
				Matrix3 tmSetup = pBone0->GetSetupMatrix();
				tmSetup.NoTrans();
				pBone0->SetSetupMatrix(tmSetup);
				pBone0->SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
				pBone0->SetCCFlag(CNCFLAG_LOCK_LOCAL_POS);
			}

			ICATObject* iobj = (ICATObject*)obj->GetInterface(I_CATOBJECT);
			float size = 5, length = 20;
			iobj->SetCATUnits(GetCATUnits());
			iobj->SetX(size);
			iobj->SetY(size);
			iobj->SetZ(length);

			// init the collarbone position
			Object* hubobj = pLimb->GetHub()->GetObject();
			if (hubobj) {
				ICATObject* ihubobj = (ICATObject*)hubobj->GetInterface(I_CATOBJECT);
				if (ihubobj) {
					Point3 p3RootPos(0.0f, 0.0f, 0.0f);
					p3RootPos[GetLengthAxis()] = (ihubobj->GetX() / 4.0f) * -(float)pLimb->GetLMR();
					Matrix3 tmSetup = GetSetupMatrix();
					tmSetup.SetTrans(p3RootPos);
					SetSetupMatrix(tmSetup);
				}
			}

			// collarbones bones are stretchy
			ccflags |= CCFLAG_SETUP_STRETCHY;
		}
	}
	UpdateObjDim();
	UpdateColour(ip->GetTime());
}

CATNodeControl* CollarBoneTrans::FindParentCATNodeControl()
{
	LimbData2* pLimb = GetLimbData();
	DbgAssert(pLimb != NULL);
	if (pLimb)
		return pLimb->GetHub();
	return NULL;
}

CATNodeControl* CollarBoneTrans::GetChildCATNodeControl(int i)
{
	LimbData2* pLimb = GetLimbData();
	if (pLimb)
	{
		// We have 2 children: we have the first bone, and its first seg.
		BoneData* pBone = pLimb->GetBoneData(0);
		if (pBone != NULL)
		{
			if (i == 0)
				return pBone;
			else if (i == 1)
			{
				return pBone->GetBone(0);
			}
		}
	}

	return NULL;
}

TSTR CollarBoneTrans::GetRigName()
{
	LimbData2* pLimb = GetLimbData();
	assert(GetCATParentTrans() && pLimb);
	if (pLimb != NULL)
		return GetCATParentTrans()->GetCATName() + pLimb->GetName() + GetName();
	return _T("YoBeAllMessedUp");
}

/**********************************************************************
 * Loading and saving....
 */

#define		COLLARBONECHUNK_CATNODECONTROL	0

IOResult CollarBoneTrans::Save(ISave *isave)
{
	IOResult res = IO_OK;

	isave->BeginChunk(COLLARBONECHUNK_CATNODECONTROL);
	res = CATNodeControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult CollarBoneTrans::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case COLLARBONECHUNK_CATNODECONTROL:
			CATNodeControl::Load(iload);
			break;

		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	return IO_OK;
}

BOOL CollarBoneTrans::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	ICATObject* iobj = GetICATObject();
	float val;

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
				break;
			case idArbBones:		LoadRigArbBones(load);											break;
			case idController:		assert(GetLayerTrans()); GetLayerTrans()->LoadRig(load);		break;
			case idObjectParams:	iobj->LoadRig(load);											break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idBoneName:	load->GetValue(name);		SetName(name);						break;
			case idFlags:		load->GetValue(ccflags);										break;
			case idParentNode: {
				TSTR parent_address;
				load->GetValue(parent_address);
				load->AddParent(GetNode(), parent_address);
				break;
			}
			case idSetupTM: {
				Matrix3 tmSetup;
				load->GetValue(tmSetup);
				if (load->GetVersion() < CAT_VERSION_1700)
				{
					ILimb* pLimb = GetLimbData();
					if (pLimb != NULL)
					{
						int lmr = pLimb->GetLMR();
						Point3 rootpos = tmSetup.GetTrans() * RotateYMatrix(PI);
						if (lmr == 1)
							tmSetup.SetTrans(rootpos*RotateYMatrix((float)M_PI_2));
						else tmSetup.SetTrans(rootpos*RotateYMatrix((float)-M_PI_2));
					}
				}
				SetSetupMatrix(tmSetup);
				break;
			}
			case idWidth:		load->GetValue(val);		if (iobj) iobj->SetX(val);			break;
			case idHeight:		load->GetValue(val);		if (iobj) iobj->SetY(val);			break;
			case idLength:		load->GetValue(val);		if (iobj) iobj->SetZ(val);			break;
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

void CollarBoneTrans::SetLengthAxis(int axis) {

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
void CollarBoneTrans::UpdateUserProps() {

	CATNodeControl::UpdateUserProps();

	// Generate a unique identifier for the pLimb.
	// we will borrow the node handle from the 1st bone in the pLimb
//	ULONG limbid = GetLimbData()->GetBone(0)->GetNode()->GetHandle();
//	node->SetUserPropInt(_T("CATProp_LimbID"), limbid);
	GetNode()->SetUserPropInt(_T("CATProp_CollarBone"), true);
}

CATControl* CollarBoneTrans::GetParentCATControl()
{
	return GetLimbData();
}

LimbData2* CollarBoneTrans::GetLimbData()
{
	return static_cast<LimbData2*>(pblock->GetReferenceTarget(PB_LIMBDATA));
}

void CollarBoneTrans::SetLimbData(LimbData2* l)
{
	pblock->SetValue(PB_LIMBDATA, 0, l);
}

