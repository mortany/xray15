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

// Implements a basic PRS controller, except inherited scale is disregarded.
// CONTROLS:
//   Position
//   Rotation
//   Scale


 // CAT Project Stuff
#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>

// Max stuff
#include "math.h"
#include "decomp.h"
#include "CATRigPresets.h"

// CATRig Structure
#include "SpineTrans2.h"
#include "SpineData2.h"
#include "Hub.h"

//
//	SpineTrans2ClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class SpineTrans2ClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		SpineTrans2* bone = new SpineTrans2(loading);
		return bone;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_SPINETRANS2); }
	Class_ID		ClassID() { return SPINETRANS2_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("SpineTrans2"); }			// returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
static SpineTrans2ClassDesc SpineTrans2Desc;
ClassDesc2* GetSpineTrans2Desc() { return &SpineTrans2Desc; }

enum { SpineTrans2PBlock_params };

static ParamBlockDesc2 SpineTrans2_param_blk(SpineTrans2PBlock_params, _T("SpineLink params"), 0, &SpineTrans2Desc,
	P_AUTO_CONSTRUCT, SpineTrans2::PBLOCK_REF,
		SpineTrans2::PB_SPINEDATA, _T(""), TYPE_FLOAT, P_ANIMATABLE, IDS_CL_SPINEDATA2,  // not really a float, really should be TYPE_REFTARG
		p_end,
	SpineTrans2::PB_LINKNUM_DEPRECATED, _T("BoneID"), TYPE_INDEX, 0, 0, // I really mean LINKNUM
		p_default, 0,
		p_end,
	SpineTrans2::PB_TMSETUP, _T(""), TYPE_MATRIX3, 0, 0,
		p_end,
	SpineTrans2::PB_ROTWT, _T(""), TYPE_FLOAT, 0, 0,
		p_end,
	SpineTrans2::PB_POSWT, _T(""), TYPE_FLOAT, 0, 0,
		p_end,
	p_end
);

SpineTrans2::SpineTrans2(BOOL /*loading*/)
{

	pblock = NULL;
	mWorldTransform = Matrix3(1);
	mLayerTrans = NULL;

	dRotWt = 0.0f;
	dPosWt = 0.0f;

	mSpineStretch = 1.0f;
	mBoneLocalScale.Set(1, 1, 1);

	// defaut bones to being stretchy
	ccflags |= CCFLAG_SETUP_STRETCHY;
	ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	ccflags |= CNCFLAG_LOCK_SETUPMODE_LOCAL_POS;
	ccflags |= CCFLAG_EFFECT_HIERARCHY;

	SpineTrans2Desc.MakeAutoParamBlocks(this);
}

RefTargetHandle SpineTrans2::Clone(RemapDir& remap)
{
	SpineTrans2 *newSpineTrans2 = new SpineTrans2(TRUE);
	remap.AddEntry(this, newSpineTrans2);

	newSpineTrans2->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));
	if (mLayerTrans)	newSpineTrans2->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newSpineTrans2, remap);

	newSpineTrans2->dRotWt = dRotWt;
	newSpineTrans2->dPosWt = dPosWt;

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newSpineTrans2, remap);

	// now return the new object.
	return newSpineTrans2;
}

SpineTrans2::~SpineTrans2()
{
	DeleteAllRefs();
}

void SpineTrans2::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void SpineTrans2::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	// Let the CATNode manage the UIs for motion and hierarchy panel
	CATNodeControl::BeginEditParams(ip, flags, prev);

	SpineData2* pSpine = GetSpine();
	if (pSpine != NULL)
		pSpine->BeginEditParams(ip, flags, prev);

	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();
}

void SpineTrans2::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	SpineData2* pSpine = GetSpine();
	if (pSpine != NULL)
		pSpine->EndEditParams(ip, flags, next);

	// Let the CATNode manage the UIs for motion and hierarchy panel
	CATNodeControl::EndEditParams(ip, flags, next);
}

//////////////////////////////////////////////////////////////////////////
// Spine eval methods

// we really can't find a way to use CATNodeControlDistributed::SetValue to do our rotations...
void SpineTrans2::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	// Our spine is unique in that it (really) doesn't inherit from its parent.
	// (as opposed to tails, which say they don't, but really do, in order to get distributed SetValues)
	// Each bone inherits from the spine base.  That means that when we do a setvalue,
	// all our children need to do the same SetValue too to 'fake' the inheritance.
	// If multiple elements are selected, then increment the rotation angle each time
	// to get that neat bendy effect.
	SetXFormPacket* xForm = (SetXFormPacket*)val;
	SpineData2* pSpine = GetSpine();
	if (pSpine == NULL)
		return;

	// compensate for spine links weird inheritance for rotation,
	// unless we are an FK spine (in which case we devolve to normal operation).
	if (xForm->command == XFORM_ROTATE && !pSpine->GetSpineFK())
	{
		//  Do we have
		int iBoneID = GetBoneID();
		int iSelAncestors = NumSelectedBones(iBoneID);
		int iSelTotal = NumSelectedBones(-1);

		// Increment the angle rotated to get that neat bendy effect
		// Modifications to the xForm will not be persisted between
		// SetValue calls on different links.  Therefore, we need to
		// increment the angle each time
		if (iSelTotal > 0)
			xForm->aa.angle *= float(iSelAncestors) / iSelTotal;

		SpineData2* pSpine = GetSpine();
		DbgAssert(pSpine != NULL);
		if (pSpine == NULL)
			return;

		// Apply this SetValue over all spine links
		// Selected bone links recieve their own SetValue
		CATNodeControl* pLink = this;
		do {
			pLink->CATNodeControl::SetValue(t, xForm, commit, method);
			pLink = pSpine->GetSpineBone(++iBoneID);
		} while (pLink != NULL && !pLink->IsThisBoneSelected());
	}
	else
		CATNodeControlDistributed::SetValue(t, xForm, commit, method);
}

Matrix3 SpineTrans2::CalcPosTM(Matrix3 tmParent, AngAxis axPos)
{
	// new method now is way more faster
	AngAxis ax = axPos;
	ax.angle *= dPosWt;

	RotateMatrix(mWorldTransform, (Quat)ax);
	mWorldTransform.SetTrans(tmParent.GetTrans());

	// Prepare a matrix to pass on to the next link
	Matrix3 tmChildParent = mWorldTransform;
	CATNodeControl::ApplyChildOffset(0, tmChildParent);

	// Ensure our position is scaled.
	Point3 p3Offset = tmChildParent.GetTrans() - mWorldTransform.GetTrans();
	p3Offset *= mBoneLocalScale;
	tmChildParent.SetTrans(mWorldTransform.GetTrans() + p3Offset);

	return tmChildParent;
}

// We pass in the matrix positioned at the start of this link.
// this is consistent with the way the limbs process themselves
Matrix3 SpineTrans2::CalcRotTM(const Matrix3& tmParent, const Matrix3& tmBaseHub, const Matrix3& tmTipHub, const Point3& p3ScaleIncr, Point3& p3LastBoneScale)
{
	mWorldTransform = tmBaseHub;
	// Make sure this blends the included scale
	BlendMat(mWorldTransform, tmTipHub, dRotWt);

	// Blend between base scale to tip scale based on rotation weight
	if (p3ScaleIncr != P3_IDENTITY_SCALE)
	{
		// what is our desired, world-space scale
		Point3 p3MyWorldScale = P3_IDENTITY_SCALE + (p3ScaleIncr - P3_IDENTITY_SCALE) * dRotWt;
		// We inherit scale, so remove the effect of the last bones scale.
		// to find our local scale
		mBoneLocalScale = p3MyWorldScale / p3LastBoneScale;
		// Return the p3LastBoneScale
		p3LastBoneScale = p3MyWorldScale;
	}
	// else, we have constant scale from base to tip, so just use that
	else
		mBoneLocalScale = P3_IDENTITY_SCALE;

	mWorldTransform = pblock->GetMatrix3(PB_TMSETUP) * mWorldTransform;
	mWorldTransform.SetTrans(tmParent.GetTrans());

	// Prepare a matrix to pass on to the next link
	Matrix3 tmChildParent = mWorldTransform;
	CATNodeControl::ApplyChildOffset(0, tmChildParent);

	// Ensure our position is scaled.
	Point3 p3Offset = tmChildParent.GetTrans() - mWorldTransform.GetTrans();
	p3Offset *= p3LastBoneScale;
	tmChildParent.SetTrans(mWorldTransform.GetTrans() + p3Offset);

	return tmChildParent;
}

void SpineTrans2::CalcWorldTransform(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmWorld, Point3& p3LocalScale, Interval& ivLocalValid)
{
	SpineData2* iSpine = GetSpine();
	if (iSpine == NULL)
		return;

	if (iSpine->GetSpineFK())
		CATNodeControl::CalcWorldTransform(t, tmParent, p3ParentScale, tmWorld, p3LocalScale, ivLocalValid);
	else
	{
		// Bone 0 needs to evaluate for the entire spine, but sequential bones simply take the cached value
		if (GetBoneID() == 0)
			iSpine->EvalSpine(t, tmWorld, p3ParentScale, ivLocalValid);
		else
			mWorldTransform.SetTrans(tmWorld.GetTrans());

		tmWorld = mWorldTransform;
		p3LocalScale = mBoneLocalScale;
	}
	p3LocalScale[GetLengthAxis()] *= mSpineStretch;
}

void SpineTrans2::SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method)
{
	// FK spines get to skip this next bit.
	if (GetCATMode() == SETUPMODE && mLayerTrans == NULL)
	{
		SpineData2* pSpine = (SpineData2*)GetSpine();
		if (pSpine == NULL)
			return;

		Hub* pTipHub = pSpine->GetTipHub();
		if (pTipHub == NULL)
			return;

		Matrix3 tmTipHub = pTipHub->GetNodeTM(t);

		//////////////////////////////////////////////////////////////////////////
		Matrix3 tmSetup = pblock->GetMatrix3(PB_TMSETUP);
		Matrix3 tmOpSpace = packet->tmAxis * Inverse(packet->tmParent);

		switch (packet->command)
		{
		case XFORM_MOVE:
		{
			Point3 pos = VectorTransform(tmOpSpace, packet->p);
			pos /= GetCATUnits();

			// The first bones position comes from tmBaseTransform
			if (GetBoneID() == 0)
			{
				Matrix3 tmBaseTransform = pSpine->GetBaseTransform();
				tmBaseTransform.Translate(pos);
				pSpine->SetBaseTransform(tmBaseTransform);

				// No need to set tmSetup after this
				return;
			}

			tmSetup.Translate(pos);
			break;
		}
		case XFORM_ROTATE:
		{
			AngAxis rot(packet->aa);
			rot.axis = Normalize(VectorTransform(tmOpSpace, rot.axis));

			// apply the new rotation
			Point3 p3SetupPos = tmSetup.GetTrans();
			RotateMatrix(tmSetup, rot);
			tmSetup.SetTrans(p3SetupPos);
			break;

		}
		case XFORM_SET:
		{
			// This is difficult.  Our Setup matrix is applied
			// before the position offset, so make sure we remove that.

			SpineData2* pSpine = (SpineData2*)GetSpine();

			// Calculate our desired start & end points
			Matrix3 tmBaseNode = pSpine->GetBaseHub()->GetNodeTM(t);
			Matrix3 tmSpineBase = tmBaseNode;
			Matrix3 tmSpineTip;
			Point3 p3BaseScale(1, 1, 1);
			Point3 p3TipScale(1, 1, 1);

			CATNodeControl* pBone = pSpine->GetSpineBone(0);
			pBone->CalcParentTransform(t, tmSpineBase, p3BaseScale);

			Interval iv;
			pSpine->CalcSpineAnchors(t, tmBaseNode, p3BaseScale, tmSpineBase, tmSpineTip, p3TipScale, iv);

			// Calculate our actual parent.
			Matrix3 tmLinkParent = tmSpineBase;
			BlendRot(tmLinkParent, tmSpineTip, dRotWt);
			packet->tmParent = tmLinkParent;

			// Remove any GetPosTM effects from this?  Can we assume this cache is valid?
			Matrix3 tmPosOffset = mWorldTransform * Inverse(pblock->GetMatrix3(PB_TMSETUP) * tmLinkParent);
			tmSetup = Inverse(tmPosOffset) * packet->tmAxis * Inverse(tmLinkParent);

			// The first bones position comes from tmBaseTransform
			if (GetBoneID() == 0)
			{
				Matrix3 tmBaseTransform = pSpine->GetBaseTransform();
				tmBaseTransform.SetTrans(tmOpSpace.GetTrans() / GetCATUnits());
				pSpine->SetBaseTransform(tmBaseTransform);

				// Remove translations from tmSetup, and continue.
				tmSetup.NoTrans();
			}
			else
				tmSetup.SetTrans(tmOpSpace.GetTrans() / GetCATUnits());
			break;
		}
		}
		SetSetupMatrix(tmSetup);
	}
	else CATNodeControlDistributed::SetWorldTransform(t, tmOrigParent, p3ParentScale, packet, commit, method);
}
//////////////////////////////////////////////////////////////////////////

RefTargetHandle SpineTrans2::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:	return pblock;
	case LAYERTRANS:	return mLayerTrans;
	default:			return NULL;
	}
}

void SpineTrans2::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:	pblock = (IParamBlock2*)rtarg; break;
	case LAYERTRANS:	mLayerTrans = (CATClipMatrix3*)rtarg; break;
	}
}

Animatable* SpineTrans2::SubAnim(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case LAYERTRANS:		return mLayerTrans;
	default:				return NULL;
	}

}

TSTR SpineTrans2::SubAnimName(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return GetString(IDS_PARAMS);
	case LAYERTRANS:		return GetString(IDS_LAYERTRANS);
	default:				return _T("");
	}
}

RefResult SpineTrans2::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:

		InvalidateTransform();

		if (mLayerTrans == hTarg) {
			GetSpine()->Update();
		}

		if (pblock == hTarg) {
			// table index if the parameter that changed was a table.
			// since we have no tables in this param block its not likely
			int ParamTabIndex = 0;
			ParamID ParamIndex = pblock->LastNotifyParamID(ParamTabIndex);
			switch (ParamIndex)
			{
			case PB_TMSETUP:
			{
				if (GetBoneID() != 0)
				{
					SpineData2* pSpine = GetSpine();
					if (pSpine != NULL)
					{
						pSpine->Update();
					}
				}
			}
			break;
			}
		}
		break;
	}

	return REF_SUCCEED;
}

/***********************************************************************************************\
	CATControl Functions
\***********************************************************************************************/

CATGroup* SpineTrans2::GetGroup()
{
	SpineData2* pData = GetSpine();
	if (pData != NULL)
		return pData->GetGroup();
	return NULL;
}

CATControl* SpineTrans2::GetParentCATControl()
{
	return GetSpine();
}

// this function is not currently in use. SpineData colours the spine links now
Color SpineTrans2::GetCurrentColour(TimeValue t/*=0*/)
{
	SpineData2* pSpine = GetSpine();
	DbgAssert(pSpine);
	Hub *hubBase = pSpine->GetBaseHub();
	Hub *hubTip = pSpine->GetTipHub();
	if (!hubBase || !hubTip) return Color(0.9, 0.1, 0.1);
	Color clrBase(hubBase->GetCurrentColour(t));
	Color clrTip(hubTip->GetCurrentColour(t));;
	//	Color clr;
	//	clrBase(hubBase->GetCurrentColour(t));
	//	if(hubTip)	clrTip(spine->GetTipHub()->GetCurrentColour(t));
	Color clr = (clrBase + ((clrTip - clrBase) * dRotWt));
	clr.ClampMinMax();

	return  clr;
}

TSTR SpineTrans2::GetRigName()
{
	SpineData2* pSpine = GetSpine();
	ICATParentTrans* pCATParentTrans = GetCATParentTrans();
	DbgAssert(pSpine && pCATParentTrans);

	int nNumLinks = pSpine->GetNumBones();
	if (nNumLinks == 1) {
		return pCATParentTrans->GetCATName() + pSpine->GetName();
	}

	TSTR bonename = GetName();
	// if the bonename has not been initialised, use the boneID
	if (pSpine->GetNumBones() > 1 && bonename.isNull()) {
		bonename.printf(_T("%d"), (GetBoneID() + 1));
	}
	return pCATParentTrans->GetCATName() + pSpine->GetName() + bonename;
}

void SpineTrans2::CreateLayerTransformController()
{
	SpineData2* pSpine = GetSpine();
	if (pSpine == NULL)
		return;

	CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), pSpine->GetClipWeights(), GetCATParentTrans(), FALSE);

	Matrix3 tmSetup = pblock->GetMatrix3(PB_TMSETUP);
	layerController->SetSetupVal((void*)&tmSetup);

	ReplaceReference(LAYERTRANS, layerController);

	// We need to calculate the transform flag specially here,
	// as the LayerTrans for spines can be created at any time
	// so the trigger may not be set by the ChangeParents notify
	CalculateHasTransformFlag();
}

void SpineTrans2::DestroyLayerTransformController() {

	Matrix3 tmSetup(1);
	if (GetLayerTrans()) GetLayerTrans()->GetSetupVal((void*)&tmSetup);
	pblock->SetValue(PB_TMSETUP, 0, tmSetup);
	DeleteReference(LAYERTRANS);
}

void SpineTrans2::Initialise(CATControl* owner, int id/*=0*/, BOOL loading/*=TRUE*/)
{
	BlockEvaluation block(this);

	Interface* ip = GetCOREInterface();

	// save a pointer to the Spine
	SpineData2 *pSpine = dynamic_cast<SpineData2 *>(owner);
	DbgAssert(NULL != pSpine);
	SetSpine(pSpine);
	SetBoneID(id);

	// Old code needs this value here...
	pblock->SetValue(PB_LINKNUM_DEPRECATED, 0, id);

	Object *obj = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	DbgAssert(obj);

	if (pSpine->GetSpineFK()) {
		CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), pSpine->GetClipWeights(), GetCATParentTrans(), loading);
		ReplaceReference(LAYERTRANS, layerController);
	}

	if (id == 0) {
		ClearCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
	}

	// Make the INode*
	CreateNode(obj);

	if (!loading) {
		TSTR bonename;
		bonename.printf(_T("%d"), id + 1);
		SetName(bonename);
	}
}

CATNodeControl* SpineTrans2::FindParentCATNodeControl()
{
	SpineData2* pSpine = GetSpine();
	DbgAssert(pSpine);
	if (pSpine == NULL)
		return NULL;

	CATNodeControl* pParent = NULL;
	int iSpineID = GetBoneID();
	if (iSpineID > 0)
		pParent = pSpine->GetSpineBone(iSpineID - 1);
	else
		pParent = pSpine->GetBaseHub();

	return pParent;
}

// Cache our position effect weight
void SpineTrans2::UpdateRotPosWeights()
{
	SpineData2* pSpine = GetSpine();
	if (!pSpine)	return;

	float id = (float)GetBoneID();
	// when a spine Bends, this value is the interpolated values
	// between the base of the spine and the Tip of the spine
	dRotWt = pSpine->GetRotWeight(id + 0.5f);

	// 1 based link index
	float dRotWtThis = pSpine->GetRotWeight(id);
	float dRotWtNext = pSpine->GetRotWeight(id + 1);
	dPosWt = (dRotWtNext - dRotWtThis) * pSpine->GetNumBones();
}

// user Properties.
// these will be used by the runtime libraries
void	SpineTrans2::UpdateUserProps() {

	INode *node = GetNode();
	SpineData2* pSpine = GetSpine();
	if (!(node && pSpine)) return;

	CATNodeControl::UpdateUserProps();

	if (GetBoneID() == 0) {
		node->SetUserPropString(_T("CATProp_SpineAddress"), pSpine->GetBoneAddress());
		node->SetUserPropInt(_T("CATProp_NumSpineBones"), pSpine->GetNumBones());
		node->SetUserPropString(_T("CATProp_SpineBaseHub"), pSpine->GetBaseHub()->GetBoneAddress());
	}

	node->SetUserPropInt(_T("CATProp_SpineBoneID"), GetBoneID());
	node->SetUserPropFloat(_T("CATProp_SpineBoneRotWeight"), dRotWt);
	node->SetUserPropFloat(_T("CATProp_SpineBonePosWeight"), dPosWt);

	//////////////////////////////////////////////////////////////////////
	if (GetLengthAxis() == Z)
		node->SetUserPropFloat(_T("CATProp_LinkLength"), GetObjZ() * GetCATUnits());
	else node->SetUserPropFloat(_T("CATProp_LinkLength"), GetObjX() * GetCATUnits());
	//////////////////////////////////////////////////////////////////////
}

// Our setup matrix is defined a little differently.
Matrix3 SpineTrans2::GetSetupMatrix()
{
	if (mLayerTrans != NULL)
		return mLayerTrans->GetSetupMatrix();
	else
		return pblock->GetMatrix3(PB_TMSETUP);
}

void SpineTrans2::SetSetupMatrix(Matrix3 tmSetup)
{
	// Ensure that there is no scale in our setup mtx.
	tmSetup.NoScale();

	if (mLayerTrans != NULL)
		mLayerTrans->SetSetupMatrix(tmSetup);
	else
		pblock->SetValue(PB_TMSETUP, 0, tmSetup);
}

BOOL SpineTrans2::SaveRig(CATRigWriter *save)
{
	SpineData2* pSpine = GetSpine();
	DbgAssert(pSpine);

	save->BeginGroup(GetRigID());

	save->Write(idBoneName, GetName());
	save->Write(idFlags, ccflags);

	INode* parentnode = GetParentNode();
	save->Write(idParentNode, parentnode);

	ICATObject* iobj = GetICATObject();
	if (iobj) iobj->SaveRig(save);

	if (!pSpine->GetSpineFK()) {
		Matrix3 tmSetup = GetSetupMatrix();
		save->Write(idSetupTM, tmSetup);
	}
	else if (GetLayerTrans()) GetLayerTrans()->SaveRig(save);

	SaveRigArbBones(save);

	save->EndGroup();
	return TRUE;
}

BOOL SpineTrans2::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	float val;
	Matrix3 tmSetup;
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
				break;
			}
			case idFlags:		load->GetValue(ccflags);											break;
			case idSetupTM:		load->GetValue(tmSetup);		SetSetupMatrix(tmSetup);			break;
			case idWidth:		load->GetValue(val);			if (iobj) iobj->SetX(val);			break;
			case idHeight:		load->GetValue(val);			if (iobj) iobj->SetY(val);			break;
			case idLength:		load->GetValue(val);			if (iobj) iobj->SetZ(val);			break;
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
		if (load->GetVersion() < CAT3_VERSION_2707)
		{
			HoldSuspend hs;
			SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_POS);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL);
			ClearCCFlag(CCFLAG_ANIM_STRETCHY);
		}
	}
	return ok && load->ok();
}

// For the procedural spine sub-objet selection
int SpineTrans2::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->Display(t, inode, vpt, flags);

	return CATNodeControl::Display(t, inode, vpt, flags);
};

void SpineTrans2::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->GetWorldBoundBox(t, inode, vpt, box);
	CATNodeControl::GetWorldBoundBox(t, inode, vpt, box);
};

int  SpineTrans2::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		return GetSpine()->HitTest(t, inode, type, crossing, flags, p, vpt);
	else	return CATNodeControl::HitTest(t, inode, type, crossing, flags, p, vpt);
};
void SpineTrans2::ActivateSubobjSel(int level, XFormModes& modes) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->ActivateSubobjSel(level, modes);
	else	CATNodeControl::ActivateSubobjSel(level, modes);
};
void SpineTrans2::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->SelectSubComponent(hitRec, selected, all, invert);
	else	CATNodeControl::SelectSubComponent(hitRec, selected, all, invert);
};
void SpineTrans2::ClearSelection(int selLevel) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->ClearSelection(selLevel);
	else	CATNodeControl::ClearSelection(selLevel);
};
int  SpineTrans2::SubObjectIndex(CtrlHitRecord *hitRec) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		return	GetSpine()->SubObjectIndex(hitRec);
	else	return	CATNodeControl::SubObjectIndex(hitRec);
};

void SpineTrans2::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->GetSubObjectCenters(cb, t, node);
	else	CATNodeControl::GetSubObjectCenters(cb, t, node);
};
void SpineTrans2::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->GetSubObjectTMs(cb, t, node);
	else	CATNodeControl::GetSubObjectTMs(cb, t, node);
};
void SpineTrans2::SubMove(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->SubMove(t, partm, tmAxis, val, localOrigin);
	else	CATNodeControl::SubMove(t, partm, tmAxis, val, localOrigin);
};
void SpineTrans2::SubRotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->SubRotate(t, partm, tmAxis, val, localOrigin);
	else	CATNodeControl::SubRotate(t, partm, tmAxis, val, localOrigin);
};
void SpineTrans2::SubScale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) {
	if (GetSpine() && !GetSpine()->GetSpineFK())
		GetSpine()->SubScale(t, partm, tmAxis, val, localOrigin);
	else	CATNodeControl::SubScale(t, partm, tmAxis, val, localOrigin);
};

CATNodeControl* SpineTrans2::GetChildCATNodeControl(int i) {
	if (i != 0)
		return NULL;

	SpineData2* pSpine = GetSpine();
	if (pSpine == NULL)
		return NULL;

	if (GetBoneID() < (pSpine->GetNumBones() - 1))
		return pSpine->GetSpineBone(GetBoneID() + 1);
	return pSpine->GetTipHub();
}

void	SpineTrans2::Update() {
	SpineData2* pSpine = GetSpine();
	if (pSpine == nullptr || pSpine->GetSpineFK()) {
		CATMessage(GetCOREInterface()->GetTime(), CAT_UPDATE);
	}
	else pSpine->CATMessage(GetCOREInterface()->GetTime(), CAT_UPDATE);
};

void SpineTrans2::UpdateObjDim()
{
	CATNodeControl::UpdateObjDim();

	// If we are not the first bone in the spine,
	// trigger an update to force re-evaluation
	if (GetBoneID() != 0)
		Update();
}

void* SpineTrans2::GetInterface(ULONG id) {
	if (id == I_MASTER)			return (ReferenceTarget*)GetSpine()->GetTipHub();
	return CATNodeControl::GetInterface(id);
}

SpineData2* SpineTrans2::GetSpine()
{
	return (SpineData2*)pblock->GetControllerByID(PB_SPINEDATA);
}

const SpineData2* SpineTrans2::GetSpine() const
{
	return (const SpineData2*)pblock->GetControllerByID(PB_SPINEDATA);
}

void SpineTrans2::SetSpine(SpineData2* sp)
{
	pblock->SetControllerByID(PB_SPINEDATA, 0, static_cast<Control*>(sp), FALSE);
}

CatAPI::IBoneGroupManager* SpineTrans2::GetManager()
{
	return GetSpine();
}

void SpineTrans2::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	DbgAssert(GetSpine() && GetSpine()->GetTipHub());
	GetSpine()->GetTipHub()->GetSystemNodes(nodes, ctxt);
}

//////////////////////////////////////////////////////////////////////
// Save etc
//

#define		SPINETRANSCHUNK_CATNODECONTROL		1
#define		SPINETRANSCHUNK_POSWEIGHT			2
#define		SPINETRANSCHUNK_ROTWEIGHT			3

IOResult SpineTrans2::Save(ISave *isave)
{
	DWORD nb;
	IOResult res = IO_OK;

	isave->BeginChunk(SPINETRANSCHUNK_CATNODECONTROL);
	res = CATNodeControl::Save(isave);
	isave->EndChunk();

	isave->BeginChunk(SPINETRANSCHUNK_POSWEIGHT);
	res = isave->Write(&dPosWt, sizeof(float), &nb);
	isave->EndChunk();

	isave->BeginChunk(SPINETRANSCHUNK_ROTWEIGHT);
	res = isave->Write(&dRotWt, sizeof(float), &nb);
	isave->EndChunk();

	return res;
}

IOResult SpineTrans2::Load(ILoad *iload)
{
	DWORD nb;
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case SPINETRANSCHUNK_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		case SPINETRANSCHUNK_POSWEIGHT:
			res = iload->Read(&dPosWt, sizeof(float), &nb);
			break;
		case SPINETRANSCHUNK_ROTWEIGHT:
			res = iload->Read(&dRotWt, sizeof(float), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	return IO_OK;
}
