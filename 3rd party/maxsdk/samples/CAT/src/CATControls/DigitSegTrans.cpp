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
#include "DigitSegTrans.h"

 // Layers
#include "CATClipRoot.h"
#include "CATClipValues.h"

// Rig Structure
#include "DigitData.h"
#include "LimbData2.h"
#include "PalmTrans2.h"
#include "../CATObjects/ICATObject.h"

// CATMotion
#include "CATMotionDigitRot.h"
#include "CATMotionRot.h"
#include "CATGraph.h"

// Max Stuff
#include "math.h"
#include "Simpobj.h"
#include "decomp.h"
#include "iparamm2.h"

class DigitSegTransClassDesc :public CATNodeControlClassDesc
{
public:
	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		DigitSegTrans* bone = new DigitSegTrans(loading);
		return bone;
	}
	const TCHAR *	ClassName() { return _T("CATDigitSegTrans"); }
	Class_ID		ClassID() { return DIGITSEGTRANS_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("DigitSegTrans"); }	// returns fixed parsable name (scripter-visible name)
};

static DigitSegTransClassDesc DigitSegTransDesc;
ClassDesc2* GetDigitSegTransDesc() { return &DigitSegTransDesc; }

/************************************************************************/
/* DigitSegTrans Setup Rollout                                               */
/************************************************************************/
class DigitSegTransParamDlgCallBack : public  ParamMap2UserDlgProc
{
	DigitSegTrans*	bone;
	ICATObject*		iobj;
	ICustEdit		*edtName;
	ICustButton *btnAddArbBone;
	ICustButton *btnAddERN;

public:

	DigitSegTransParamDlgCallBack() : bone(NULL), iobj(NULL) {
		edtName = NULL;
		btnAddArbBone = NULL;
		btnAddERN = NULL;
	}

	void InitControls(HWND hWnd)
	{
		DbgAssert(bone);
		iobj = bone->GetICATObject();

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		// Initialise custom Max controls
		edtName = GetICustEdit(GetDlgItem(hWnd, IDC_EDIT_NAME));
		edtName->SetText(bone->GetName().data());

		if (iobj->CustomMeshAvailable()) {
			SET_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH, iobj->UsingCustomMesh());
		}
		else SendMessage(GetDlgItem(hWnd, IDC_CHK_USECUSTOMMESH), WM_ENABLE, (WPARAM)FALSE, 0);

		// btnCollapse button
		btnAddArbBone = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADDARBBONE));	btnAddArbBone->SetType(CBT_PUSH);	btnAddArbBone->SetButtonDownNotify(TRUE);
		btnAddArbBone->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddExtraBone"), GetString(IDS_TT_ADDBONE)));

		btnAddERN = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADDRIGGING));		btnAddERN->SetType(CBT_CHECK);		btnAddERN->SetButtonDownNotify(TRUE);
		btnAddERN->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddERN"), GetString(IDS_TT_ADDRIGOBJS)));

		if (bone->GetCATParentTrans()->GetCATMode() != SETUPMODE) {
			btnAddArbBone->Disable();
		}

		SetFocus(GetDlgItem(hWnd, IDC_CHK_USECUSTOMMESH));
	}
	void ReleaseControls()
	{
		SAFE_RELEASE_EDIT(edtName);
		SAFE_RELEASE_BTN(btnAddArbBone);
		SAFE_RELEASE_BTN(btnAddERN);
		ExtraRigNodes *ern = (ExtraRigNodes*)bone->GetInterface(I_EXTRARIGNODES_FP);
		DbgAssert(ern);
		ern->IDestroyERNWindow();
	}

	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{

		switch (msg) {
		case WM_INITDIALOG:
			bone = (DigitSegTrans*)map->GetParamBlock()->GetOwner();
			if (!bone) return FALSE;
			InitControls(hWnd);
			break;
		case WM_DESTROY:
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_BUTTONUP: {
				switch (LOWORD(wParam)) {
				case IDC_BTN_ADDARBBONE:
					if (!bone->GetDigit()->TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) {
						HoldActions actions(IDS_HLD_ADDBONE);
						bone->AddArbBone();
					}
					break;
				case IDC_BTN_ADDRIGGING: {
					ExtraRigNodes* ern = (ExtraRigNodes*)bone->GetInterface(I_EXTRARIGNODES_FP);
					DbgAssert(ern);
					if (btnAddERN->IsChecked())
						ern->ICreateERNWindow(hWnd);
					else ern->IDestroyERNWindow();
					break;
				}
				}
				break;
			}
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_USECUSTOMMESH:		if (iobj) iobj->SetUseCustomMesh(IS_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH));	break;
				}
				break;
			}
			break;
		case WM_CUSTEDIT_ENTER:
		{
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_NAME: {
				TCHAR strbuf[128];
				edtName->GetText(strbuf, 64);// max 64 characters
				TSTR name(strbuf);
				bone->SetName(name);
				break;
			}
			}
			break;
		}

		default:
			return FALSE;
		}
		return TRUE;
	}

	virtual void DeleteThis() { }//delete this; }
};

static DigitSegTransParamDlgCallBack DigitSegTransParamCallBack;

// An accessor that passes values directly throught to the object.
// This allows us to move parameters that used to be stored on the
// digit parameter block directly into the objects themselves.
class CATObjectParamAccessor : public PBAccessor
{
public:

	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid)
	{
		UNUSED_PARAM(id); UNUSED_PARAM(tabIndex); UNUSED_PARAM(valid); UNUSED_PARAM(t);
		DbgAssert(id == DigitSegTrans::PB_BONELENGTH);
		DigitSegTrans *pDigitSeg = dynamic_cast<DigitSegTrans *>(owner);
		if (NULL != pDigitSeg)
		{
			ICATObject* pSegObj = pDigitSeg->GetICATObject();
			if (pSegObj != NULL)
				v.f = pSegObj->GetZ();
		}

	}
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		UNUSED_PARAM(id); UNUSED_PARAM(tabIndex); UNUSED_PARAM(t);
		DbgAssert(id == DigitSegTrans::PB_BONELENGTH);
		DigitSegTrans *pDigitSeg = dynamic_cast<DigitSegTrans *>(owner);
		if (NULL != pDigitSeg)
		{
			ICATObject* pSegObj = pDigitSeg->GetICATObject();
			if (pSegObj != NULL)
				pSegObj->SetZ(v.f);
		}
	}
};
static CATObjectParamAccessor pbDigitLengthAcessor;

static ParamBlockDesc2 DigitSegTrans_param_blk(DigitSegTrans::PBLOCK_REF, _T("DigitSegTransParams"), 0, &DigitSegTransDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, DigitSegTrans::PBLOCK_REF,
	IDD_DIGITSEGTRANS_CAT3, IDS_DIGITSEGPARAMS, 0, 0, &DigitSegTransParamCallBack,

	DigitSegTrans::PB_DIGITDATA, _T(""), TYPE_FLOAT, P_ANIMATABLE, NULL,  // not really a float, really should be TYPE_REFTARG
		p_end,
	DigitSegTrans::PB_BONEID_DEPRECATED, _T("BoneID"), TYPE_INDEX, 0, NULL,
		p_end,
	DigitSegTrans::PB_BONELENGTH, _T("Length"), TYPE_FLOAT, 0, NULL,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LENGTH, IDC_SPIN_LENGTH, 0.01f,
		p_accessor, &pbDigitLengthAcessor,
		p_end,
	DigitSegTrans::PB_TMSETUP, _T(""), TYPE_MATRIX3, 0, NULL,
		p_end,
	DigitSegTrans::PB_OBJSEGREF, _T(""), TYPE_REFTARG, P_NO_REF, NULL,
		p_end,
	DigitSegTrans::PB_NODESEG, _T(""), TYPE_INODE, P_NO_REF, NULL,
		p_end,
	//////////////////////////////////////////////////////////////////////////
	DigitSegTrans::PB_BENDWEIGHT, _T("CATWeight"), TYPE_FLOAT, 0, NULL,
		p_default, 1.0f,
		p_range, -5.0f, 5.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_CATMOTIONWEIGHT, IDC_SPIN_CATMOTIONWEIGHT, SPIN_AUTOSCALE,
		p_end,
	p_end
);

DigitSegTrans::DigitSegTrans(BOOL /*loading*/)
	: pblock(NULL)
	, tmPreToePosHack(1)
{
	// defaut bones to being stretchy
	ccflags |= CCFLAG_SETUP_STRETCHY;
	ccflags |= CNCFLAG_LOCK_LOCAL_POS;
	ccflags |= CNCFLAG_LOCK_SETUPMODE_LOCAL_POS;
	ccflags |= CCFLAG_EFFECT_HIERARCHY;

	DigitSegTransDesc.MakeAutoParamBlocks(this);
}

DigitSegTrans::~DigitSegTrans()
{
	DeleteAllRefs();
}

void DigitSegTrans::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		DigitSegTrans *newctrl = (DigitSegTrans*)from;

		if (newctrl->mLayerTrans)
			ReplaceReference(LAYERTRANS, newctrl->mLayerTrans);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

RefTargetHandle DigitSegTrans::Clone(RemapDir& remap)
{
	// make a new DigitSegTrans object to be the clone
	// call true for loading so the new DigitSegTrans doesn't
	// make new default subcontrollers.
	DigitSegTrans *newDigitSegTrans = new DigitSegTrans(TRUE);

	//	newDigitSegTrans->pre_clone_palm = GetDigit()->GetPalm();

	remap.AddEntry(this, newDigitSegTrans);

	newDigitSegTrans->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// clone our subcontrollers and assign them to the new object.
	if (mLayerTrans)	newDigitSegTrans->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newDigitSegTrans, remap);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newDigitSegTrans, remap);

	// now return the new object.
	return newDigitSegTrans;
}

//////////////////////////////////////////////////////////////////////////

void DigitSegTrans::CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	CATNodeControl::CalcParentTransform(t, tmParent, p3ParentScale);

	// If we are the first finger bone?
	if (GetBoneID() == 0)
	{
		// Calculate a new pos using the root pos ratio
		CATNodeControl* pMyParent = GetParentCATNodeControl();
		if (pMyParent)
		{
			DigitData* pDigit = GetDigit();
			if (pDigit == NULL)
				return;

			tmParent.PreTranslate(pMyParent->GetBoneDimensions() * pMyParent->GetWorldScale(t) * pDigit->GetRootPos());
		}
	}
}

void DigitSegTrans::ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid)
{
	// This part is essentially a hack to correct the parent transform calculated above.
	// Translations have been applied in the space of the parent.
	// Re-apply those transforms in the space of the IK target now.
	if (GetBoneID() == 0)
	{
		// Pre CAT3 we had a stupid inheritance system where the toes inherited rotation off the iktarget.
		// This cause problems because the toes translation would be relative to the iK target.
		// This only showed up on rigs where the toes were offset from the tip of the ankle bone.
		LimbData2 *pLimb = GetLimbData();
		if (pLimb != NULL && pLimb->GetisLeg())
		{
			Matrix3 tmOrigWorld = tmWorldTransform;
			Point3 pos = tmWorldTransform.GetTrans();

			// TODO: Good place to bake transforms (rotate by PI is simple axis flip)
			float ikfkratio = pLimb->GetIKFKRatio(t, ivValid, pLimb->GetNumBones());

			// TODO: This seems like a problem - we should really be passing the original FK parent here?
			tmWorldTransform = pLimb->GettmIKTarget(t, ivValid);
			if (GetLengthAxis() == Z)
				tmWorldTransform.PreRotateX(-(float)M_PI_2);
			else
				tmWorldTransform.PreRotateZ((float)M_PI_2);
			BlendRot(tmWorldTransform, tmOrigWorld, ikfkratio);

			tmWorldTransform.SetTrans(pos);

			// Apply our setup value in our world space.
			Matrix3 tmNewParent = tmWorldTransform;
			CATNodeControl::ApplySetupOffset(t, tmOrigParent, tmWorldTransform, p3BoneLocalScale, ivValid);

			if (ikfkratio < 1.0f)
			{
				// Find out how much position offset has been accumulated.
				Point3 p = (tmWorldTransform * Inverse(tmNewParent)).GetTrans();
				// Now put the pos offset back in the space of the actual parent
				tmWorldTransform.SetTrans(tmOrigWorld.GetTrans() + tmOrigWorld.VectorTransform(p));
			}

			return;
		}
	}

	// Otherwise, just procede with the regular stuff
	CATNodeControl::ApplySetupOffset(t, tmOrigParent, tmWorldTransform, p3BoneLocalScale, ivValid);
}

void DigitSegTrans::SetWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, SetXFormPacket* packet, int commit, GetSetMethod method)
{
	// This part is essentially a hack to correct the parent transform calculated above.
	// Translations have been applied in the space of the parent.
	// Re-apply those transforms in the space of the IK target now.
	if (GetBoneID() == 0 && GetCATMode() == SETUPMODE)
	{
		switch (packet->command)
		{
		case XFORM_ROTATE:
		{
			// Pre CAT3 we had a stupid inheritance system where the toes inherited rotation off the iktarget.
			// This cause problems because the toes translation would be relative to the iK target.
			// This only showed up on rigs where the toes were offset from the tip of the ankle bone.
			LimbData2 *pLimb = GetLimbData();
			if (pLimb != NULL && pLimb->GetisLeg())
			{
				Matrix3 tmOrigWorld = packet->tmParent;
				Point3 pos = tmOrigWorld.GetTrans();

				// TODO: Good place to bake transforms (rotate by PI is simple axis flip)
				Interval iValid = FOREVER;
				float ikfkratio = pLimb->GetIKFKRatio(t, iValid, pLimb->GetNumBones());

				// TODO: This seems like a problem - we should really be passing the original FK parent here?
				packet->tmParent = pLimb->GettmIKTarget(t, iValid);
				if (GetLengthAxis() == Z)
					packet->tmParent.PreRotateX(-(float)M_PI_2);
				else
					packet->tmParent.PreRotateZ((float)M_PI_2);
				BlendRot(packet->tmParent, tmOrigWorld, ikfkratio);

				packet->tmParent.SetTrans(pos);
			}
			break;
		}
		case XFORM_MOVE:
		{
			// Calculate a new pos using the root pos ratio
			// Until we deprecate this stuff, the digit will continue
			// to store its root position on the hand.
			CATNodeControl* pMyParent = GetParentCATNodeControl();
			if (pMyParent)
			{
				DigitData* pDigit = GetDigit();
				if (pDigit == NULL)
					break;

				Point3 p3ParentDim = pMyParent->GetBoneDimensions();
				Point3 p3Offset = (packet->tmAxis * Inverse(packet->tmParent)).VectorTransform(packet->p);
				p3Offset = p3Offset / p3ParentDim + pDigit->GetRootPos();
				pDigit->SetRootPos(p3Offset);

				// Do NOT propagate this SetValue through.
				return;
			}
		}
		case XFORM_SET:
		{
			// Pre CAT3 we had a stupid inheritance system where the toes inherited rotation off the iktarget.
			// This cause problems because the toes translation would be relative to the iK target.
			// This only showed up on rigs where the toes were offset from the tip of the ankle bone.
			LimbData2 *pLimb = GetLimbData();
			if (pLimb != NULL && pLimb->GetisLeg())
			{
				Matrix3 tmOrigWorld = packet->tmParent;
				Point3 pos = tmOrigWorld.GetTrans();

				// TODO: Good place to bake transforms (rotate by PI is simple axis flip)
				Interval iValid = FOREVER;
				float ikfkratio = pLimb->GetIKFKRatio(t, iValid, pLimb->GetNumBones());

				// TODO: This seems like a problem - we should really be passing the original FK parent here?
				packet->tmParent = pLimb->GettmIKTarget(t, iValid);
				if (GetLengthAxis() == Z)
					packet->tmParent.PreRotateX(-(float)M_PI_2);
				else
					packet->tmParent.PreRotateZ((float)M_PI_2);
				BlendRot(packet->tmParent, tmOrigWorld, ikfkratio);

				// Calculate a position from this
				DigitData* pDigit = GetDigit();
				if (pDigit == NULL)
					break;

				CATNodeControl* pMyParent = GetParentCATNodeControl();
				if (pMyParent == NULL)
					break;

				Matrix3 tmRelToParent = packet->tmAxis * Inverse(tmOrigWorld);
				Point3 p3Offset = tmRelToParent.GetTrans();
				// Include the RootPos here again, because GetParentTransform has added it to tmParent
				// In other words, our parent includes part of our currently setting value
				// TODO: Clean all this up.
				p3Offset = p3Offset / pMyParent->GetBoneDimensions(t) + pDigit->GetRootPos();
				pDigit->SetRootPos(p3Offset);

				// Position has been set.  Remove it from the SetValue
				packet->tmAxis.SetTrans(packet->tmParent.GetTrans());
			}
		}
		}
	}

	CATNodeControl::SetWorldTransform(t, tmOrigParent, p3ParentScale, packet, commit, method);
}

void DigitSegTrans::SetObjX(float val)
{
	DigitData* pDigit = GetDigit();
	if (pDigit != NULL)
		pDigit->SetDigitWidth(val);
}

void DigitSegTrans::SetObjY(float val)
{
	DigitData* pDigit = GetDigit();
	if (pDigit != NULL)
		pDigit->SetDigitDepth(val);
}

//////////////////////////////////////////////////////////////////////////

RefTargetHandle DigitSegTrans::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case LAYERTRANS:		return mLayerTrans;
	default:				return NULL;
	}
}

void DigitSegTrans::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:		pblock = (IParamBlock2*)rtarg;			break;
	case LAYERTRANS:		mLayerTrans = (CATClipMatrix3*)rtarg;		break;
	}

}

Animatable* DigitSegTrans::SubAnim(int i)
{

	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case LAYERTRANS:		return mLayerTrans;
	default:				return NULL;
	}
}

TSTR DigitSegTrans::SubAnimName(int i)
{

	switch (i)
	{
	case PBLOCK_REF:		return GetString(IDS_PARAMS);
	case LAYERTRANS:		return GetString(IDS_LAYERTRANS);
	default:				return _T("");
	}
}

RefResult DigitSegTrans::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if (mLayerTrans == hTarg)
			InvalidateTransform();
		break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		if (mLayerTrans == hTarg)
			mLayerTrans = NULL;
		break;
	}
	return REF_SUCCEED;
}

void DigitSegTrans::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY) {
		// Let the CATNode manage the UIs for motino and hierarchy panel
		CATNodeControl::BeginEditParams(ip, flags, prev);
	}
	else if (flagsbegin == 0)
	{
		DigitData* pDigit = GetDigit();
		if (pDigit)
			pDigit->BeginEditParams(ip, flags, prev);
		DigitSegTransDesc.BeginEditParams(ip, this, flags, prev);
	}
	// Make the page-up/page-down buttons go after the rollout has been displayed
	EnableAccelerators();

}

void DigitSegTrans::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	TSTR nodename = GetNode()->GetName();

	ipbegin = NULL;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY) {
		// Let the CATNode manage the UIs for motion and hierarchy panel
		CATNodeControl::EndEditParams(ip, flags, next);
	}
	else if (flagsbegin == 0) {
		DigitData* pDigit = GetDigit();
		if (pDigit)
			pDigit->EndEditParams(ip, flags, next);
		DigitSegTransDesc.EndEditParams(ip, this, flags, next);
	}
}

void DigitSegTrans::UpdateUI() {
	if (!ipbegin) return;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO || flagsbegin&BEGIN_EDIT_HIERARCHY) {
		// Let the CATNode manage the UIs for motion and hierarchy panel
		CATNodeControl::UpdateUI();
	}
	else if (flagsbegin == 0) {
		DigitData* pDigit = GetDigit();
		if (pDigit)
			pDigit->UpdateUI();
		//	DigitSegTransDesc.InvalidateUI();
		IParamMap2* pmap = pblock->GetMap();
		if (pmap) pmap->Invalidate();
	}
}

TSTR	DigitSegTrans::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetBoneID());
	DbgAssert(GetDigit());
	TSTR boneAddress = _T("<<unknown>>");
	if (GetDigit())
		boneAddress = GetDigit()->GetBoneAddress();
	return (boneAddress + _T(".") + bonerigname + boneid);
};

BOOL DigitSegTrans::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	ICATObject* iobj = (ICATObject*)GetObject()->GetInterface(I_CATOBJECT);
	//	float val;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idDigitSegParams) &&
				(load->CurGroupID() != idDigitSeg)) {
				ok = FALSE;
				continue;
			}
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idObjectParams:	if (iobj) iobj->LoadRig(load);									break;
			case idController:		DbgAssert(GetLayerTrans()); GetLayerTrans()->LoadRig(load);		break;
			case idArbBones:		LoadRigArbBones(load);											break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idBoneName:	load->GetValue(name);			SetName(name);						break;
			case idFlags:		load->GetValue(ccflags);											break;
			case idSetupTM:
			{
				Matrix3 tmSetup;
				load->GetValue(tmSetup);
				tmSetup.NoTrans();
				mLayerTrans->SetSetupVal((void*)&tmSetup);
				break;
			}
			//	case idLength:			load->GetValue(val);		if(iobj) iobj->SetZ(val);			break;
			case idLength:			break; //load->ToParamBlock(pblock, PB_BONELENGTH);							break;
			case idCATMotionWeight:	load->ToParamBlock(pblock, PB_BENDWEIGHT);						break;
			default:				load->AssertOutOfPlace();
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

BOOL DigitSegTrans::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idDigitSegParams);
	save->Write(idBoneName, GetName());
	save->Write(idFlags, ccflags);

	//	save->FromParamBlock(pblock, idLength, PB_BONELENGTH);
	//	save->FromParamBlock(pblock, idCATMotionWeight, PB_BENDWEIGHT);

	//	Matrix3 tmSetup;
	//	layerTrans->GetSetupVal((void*)&tmSetup);
	//	save->Write(idSetupTM, tmSetup);
	if (GetLayerTrans()) GetLayerTrans()->SaveRig(save);

	ICATObject* iobj = (ICATObject*)GetObject()->GetInterface(I_CATOBJECT);
	if (iobj) iobj->SaveRig(save);

	SaveRigArbBones(save);

	save->EndGroup();
	return TRUE;
}

TSTR DigitSegTrans::GetRigName()
{
	// Sanity check - I don't think we need
	// to localize these error messages, they
	// should _never_ get shown.
	ICATParentTrans* pCPTrans = GetCATParentTrans();
	if (pCPTrans == NULL)
		return _M("ERROR: No CATParentTrans");

	LimbData2* pLimb = GetLimbData();
	if (pLimb == NULL)
		return _M("ERROR: No Limb");

	DigitData* pDigit = GetDigit();
	if (pDigit == NULL)
		return _M("ERROR: No Digit");;

	TSTR bonename = GetName();
	if (pDigit != NULL)
	{
		if (pDigit->GetNumBones() > 1 && bonename.isNull()) {
			bonename.printf(_T("%d"), (GetBoneID() + 1));
		}
		return pCPTrans->GetCATName() + pLimb->GetName() + pDigit->GetName() + bonename;
	}
	return GetString(IDS_ERROR);
}

void DigitSegTrans::UpdateCATUnits()
{
	DigitData* pDigit = GetDigit();
	ICATObject *iobj = GetICATObject();
	if (iobj && pDigit && !IsXReffed()) {
		iobj->SetX(pDigit->GetDigitWidth());
		iobj->SetY(pDigit->GetDigitDepth());
		//iobj->SetZ(pblock->GetFloat(PB_BONELENGTH));
		iobj->SetCATUnits(GetCATUnits());
		iobj->Update();

		UpdateObjDim();
	}
}

CATControl* DigitSegTrans::GetParentCATControl()
{
	return GetDigit();
}

BOOL DigitSegTrans::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	// propogate to any ARBBones
	CATNodeControl::PasteRig(pastectrl, flags, scalefactor);

	UpdateCATUnits();
	return TRUE;
}

INode *DigitSegTrans::Initialise(DigitData* pDigit, int id)
{
	if (!(pDigit))
		return NULL;
	SetDigit(pDigit);
	SetBoneID(id);

	// Old code needs this value here...
	pblock->SetValue(PB_BONEID_DEPRECATED, 0, id);

	CATClipWeights* weights = nullptr;
	if (pDigit->GetPalm())
		weights = pDigit->GetPalm()->GetWeights();
	CATClipValue* layerDigitSeg = CreateClipValueController(GetCATClipMatrix3Desc(), weights, GetCATParentTrans(), FALSE);
	ReplaceReference(LAYERTRANS, (RefTargetHandle)layerDigitSeg);

	Interface *ip = GetCOREInterface();
	Object* objDigitSeg = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	pblock->SetValue(PB_OBJSEGREF, 0, (ReferenceTarget*)objDigitSeg);
	INode* node = CreateNode(objDigitSeg);

	// Find a smart initial length.  This is the length of
	// our last bone, or 0.3f if we don't have a parent.
	float dInitLength = 0.3f;
	if (id > 0)
	{
		DigitSegTrans* pParentSeg = pDigit->GetBone(id - 1);
		if (pParentSeg != NULL)
			dInitLength = pParentSeg->GetObjZ();
	}
	SetObjZ(dInitLength);

	if (id == 0)	ClearCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
	else		SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);

	BOOL loading = FALSE;
	if (!loading) {
		TSTR bonename;
		bonename.printf(_T("%d"), id + 1);
		SetName(bonename);
	}

	return node;
}

void DigitSegTrans::GetRotation(TimeValue t, Matrix3* tmLocalRot)
{
	DbgAssert(tmLocalRot != NULL);
	*tmLocalRot = GetNodeTM(t) * Inverse(GetParentTM(t));
}

void DigitSegTrans::SetRotation(TimeValue t, const Matrix3& tmLocalRot)
{
	Matrix3 tmIdent(1);
	Point3 p3Ident(P3_IDENTITY_SCALE);
	SetXFormPacket val(tmLocalRot);
	SetWorldTransform(t, tmIdent, p3Ident, &val, 1, CTRL_ABSOLUTE);
}

CATNodeControl* DigitSegTrans::FindParentCATNodeControl()
{
	DigitData* pDigit = GetDigit();
	DbgAssert(pDigit != NULL);
	if (pDigit == NULL)
		return NULL;

	int iBoneID = GetBoneID();
	if (iBoneID > 0)
		return pDigit->GetBone(iBoneID - 1);

	return (CATNodeControl*)pDigit->GetParentCATControl();
}

int	DigitSegTrans::NumChildCATNodeControls() {
	DigitData* pDigit = GetDigit();
	if (pDigit != NULL)
	{
		if (GetBoneID() < (pDigit->GetNumBones() - 1))
			return 1;
	}
	return 0;
}

CATNodeControl*	DigitSegTrans::GetChildCATNodeControl(int i)
{
	UNUSED_PARAM(i);
	DbgAssert(i == 0);

	int iBoneID = GetBoneID();
	DigitData* pDigit = GetDigit();
	if (iBoneID < (pDigit->GetNumBones() - 1))
		return pDigit->GetBone(iBoneID + 1);
	return NULL;
};

// user Properties.
// these will be used by the runtime libraries
void DigitSegTrans::UpdateUserProps() {

	CATNodeControl::UpdateUserProps();
	LimbData2* pLimb = GetLimbData();
	if (pLimb != NULL && pLimb->GetisLeg())
	{
		INode* pNode = GetNode();
		if (pNode != NULL)
			pNode->SetUserPropInt(_T("CATProp_ToeBone"), true);
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class DigitSegTransPLCB : public PostLoadCallback {
protected:
	DigitSegTrans	*bone;
	ICATParentTrans	*catparent;

public:
	DigitSegTransPLCB(DigitSegTrans *pOwner) : catparent(NULL) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		catparent = bone->GetCATParentTrans();
		if (bone->GetFileSaveVersion() > 0) return bone->GetFileSaveVersion();
		if (catparent) return catparent->GetFileSaveVersion();
		return -1;
	}

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		if (GetFileSaveVersion() < CAT_VERSION_2430 && GetFileSaveVersion() > CAT_VERSION_2000) {

			for (int i = 0; i < bone->mLayerTrans->GetNumLayers(); i++) {
				// This upgrade is because between versions 2.0 and 2.5 CATMition layers were being set up incorectly on digits
				// the wrong controllers were bing assigned. CATMotionRot, instead of CATMotionDigitRot
				if (bone->mLayerTrans->GetRoot()->GetLayer(i)->GetMethod() == LAYER_CATMOTION &&
					bone->mLayerTrans->GetLayer(i)->GetRotationController()->ClassID() == CATMOTIONROT_CLASS_ID) {
					CATMotionRot* ctrlCATMotionRot = (CATMotionRot*)bone->mLayerTrans->GetLayer(i)->GetRotationController();
					Control* p3 = ctrlCATMotionRot->pblock->GetControllerByID(CATMotionRot::PB_P3CATMOTION);

					// the controllers were assigned wrong, swap them back.
					if (bone->GetBoneID() == 0) {
						if (bone->GetLengthAxis() == Z) {
							CATGraph* ctrlZ = (CATGraph*)p3->GetReference(Z);
							ctrlZ->SetAFlag(A_LOCK_TARGET);
							p3->AssignController(p3->GetReference(Y), Z);
							p3->AssignController(ctrlZ, Y);
							ctrlZ->ClearAFlag(A_LOCK_TARGET);
						}
						else {
							CATGraph* ctrlX = (CATGraph*)p3->GetReference(X);
							ctrlX->SetAFlag(A_LOCK_TARGET);
							p3->AssignController(p3->GetReference(Y), X);
							p3->AssignController(ctrlX, Y);
							ctrlX->ClearAFlag(A_LOCK_TARGET);

							// Flip the spread value
							((CATGraph*)p3->GetReference(Y))->SetFlipVal(-1.0f);
						}
						//
						((CATGraph*)p3->GetReference(X))->SetUnits(DEFAULT_DIM);
						((CATGraph*)p3->GetReference(Y))->SetUnits(DEFAULT_DIM);
						((CATGraph*)p3->GetReference(Z))->SetUnits(DEFAULT_DIM);
					}

					CATMotionDigitRot* ctrlCATMotionDigitRot = (CATMotionDigitRot*)CreateInstance(CTRL_ROTATION_CLASS_ID, CATMOTIONDIGITROT_CLASS_ID); //CATMOTIONROT_CLASS_ID);
					ctrlCATMotionDigitRot->Initialise(bone, p3);
					bone->GetLayerTrans()->GetLayer(i)->SetRotationController(ctrlCATMotionDigitRot);;
				}
			}
			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_2431 && GetFileSaveVersion() > CAT_VERSION_2000) {

			for (int i = 0; i < bone->mLayerTrans->GetNumLayers(); i++) {
				// This upgrade is because between versions 2.0 and 2.5 CATMition layers were being set up incorectly on digits
				// the wrong controllers were bing assigned. CATMotionRot, instead of CATMotionDigitRot
				if (bone->mLayerTrans->GetRoot()->GetLayer(i)->GetMethod() == LAYER_CATMOTION &&
					bone->mLayerTrans->GetLayer(i)->GetRotationController()->ClassID() == CATMOTIONDIGITROT_CLASS_ID) {
					CATMotionDigitRot* ctrlCATMotionDigitRot = (CATMotionDigitRot*)bone->mLayerTrans->GetLayer(i)->GetRotationController();
					Control* p3 = ctrlCATMotionDigitRot->pblock->GetControllerByID(CATMotionDigitRot::PB_P3CATMOTION);

					// Make suer the controlelrs have the right dimensiotn.
					if (bone->GetBoneID() == 0) {
						((CATGraph*)p3->GetReference(X))->SetUnits(DEFAULT_DIM);
						((CATGraph*)p3->GetReference(Y))->SetUnits(DEFAULT_DIM);
						((CATGraph*)p3->GetReference(Z))->SetUnits(DEFAULT_DIM);
					}
				}
			}
			iload->SetObsolete();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

//*********************************************************************
// Loading and saving....
//
#define		DIGITBONE_CATNODECONTROL	0
IOResult DigitSegTrans::Save(ISave *isave)
{
	// For backwards compatibility, we need to set this value
	// back on the parameter block
	pblock->SetValue(PB_BONELENGTH, 0, GetObjZ());

	isave->BeginChunk(DIGITBONE_CATNODECONTROL);
	CATNodeControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult DigitSegTrans::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case DIGITBONE_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}
	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new DigitSegTransPLCB(this));

	return IO_OK;
}

DigitData* DigitSegTrans::GetDigit()
{
	return static_cast<DigitData*>(pblock->GetControllerByID(PB_DIGITDATA));
}

void DigitSegTrans::SetDigit(DigitData* d)
{
	pblock->SetControllerByID(PB_DIGITDATA, 0, d);
}

CatAPI::IBoneGroupManager* DigitSegTrans::GetManager()
{
	return GetDigit();
}

LimbData2* DigitSegTrans::GetLimbData()
{
	DigitData* pDigit = GetDigit();
	if (pDigit != NULL)
	{
		CATControl* pParentCtrl = pDigit->GetParentCATControl();
		if (pParentCtrl != NULL)
		{
			return dynamic_cast<LimbData2*>(pParentCtrl->GetParentCATControl());
		}
	}
	return NULL;
}

CATGroup* DigitSegTrans::GetGroup()
{
	CATControl* pDigit = GetDigit();
	if (pDigit != NULL)
		return pDigit->GetGroup();
	return NULL;
}
