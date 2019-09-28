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

#include "math.h"
#include "decomp.h"
#include "iparamm2.h"

// Rig Structure
#include "Hub.h"
#include "LimbData2.h"
#include "BoneData.h"
#include "BoneSegTrans.h"
#include "ICATParent.h"
#include "PalmTrans2.h"

// Layer System
#include "CATWeight.h"

//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class BoneDataClassDesc : public CATNodeControlClassDesc {
public:
	BoneDataClassDesc()
	{
		AddInterface(IBoneGroupManagerFP::GetFnPubDesc());
	}

	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		BoneData* bonedata = new BoneData(loading);
		return bonedata;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_BONEDATA); }
	Class_ID		ClassID() { return BONEDATA_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("BoneData"); }		// returns fixed parsable name (scripter-visible name)
};

// Jan 2010 - Add in a second class desc for BoneData.  This is to support loading of old files
// Old files will reference this ClassDesc to create instances of BoneData (under the old SClassID)
// The actual class of BoneData was irrelevant from CATs POV, and its methods were never called.
class BoneDataClassDescLegacy : public BoneDataClassDesc
{
public:
	BoneDataClassDescLegacy(ClassDesc2* newDesc)
	{
		DbgAssert(newDesc != NULL);
		for (int i = 0; i < newDesc->NumParamBlockDescs(); i++)
		{
			AddParamBlockDesc(newDesc->GetParamBlockDesc(i));
		}
	}
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// This determines the type of our controller
	// All other functions we can leave to the BoneDataClassDesc
};

// I think we need two pblocks, one public that everyone can see with
// IKFK ratios, and IKTarget, and Name and colour.
// and one with all the info that the user doesn't need to see.
// we just don't return is as a reference

// our global instance of our classdesc class.
static BoneDataClassDesc BoneDataDesc;
ClassDesc2* GetBoneDataDesc() { return &BoneDataDesc; }

ClassDesc2* GetBoneDataDescLegacy()
{
	static BoneDataClassDescLegacy BoneDataDescLegacy(GetBoneDataDesc());
	return &BoneDataDescLegacy;
}

// Fixed the crash where the numSegs changed, and
// selected link deleted (deleted UI, which tried to do stuff = BAD)
volatile static BOOL bNumSegmentsChanging = FALSE;

//**********************************************************************//
// BoneData Setup Rollout                                               //
//**********************************************************************//
class BoneDataParamDlgCallBack : public  ParamMap2UserDlgProc
{
	BoneData*		bone;
	CATNodeControl*	boneseg;

	ICustEdit		*edtName;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;
	ICustButton *btnAddArbBone;
	ICustButton *btnAddERN;

public:

	BoneDataParamDlgCallBack() {
		bone = NULL;
		boneseg = NULL;

		edtName = NULL;

		btnCopy = NULL;
		btnPaste = NULL;
		btnPasteMirrored = NULL;
		btnAddArbBone = NULL;
		btnAddERN = NULL;
	}

	void InitControls(HWND hWnd, BoneData* bone)
	{
		this->bone = bone;
		for (int i = 0; i < bone->GetNumBones(); i++)
		{
			boneseg = bone->GetBone(i);
			if (boneseg != NULL && boneseg->IsThisBoneSelected()) {
				break;
			}
		}

		// Initialise custom Max controls
		edtName = GetICustEdit(GetDlgItem(hWnd, IDC_EDIT_SEGNAME));
		if (bone->GetNumBones() > 1 && boneseg)
			edtName->SetText(boneseg->GetName().data());
		else edtName->Disable();

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hWnd, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyBone"), GetString(IDS_TT_COPYBONE)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteBone"), GetString(IDS_TT_PASTERIG)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		// btnPasteMirrored button
		btnPasteMirrored = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE_MIRRORED));
		btnPasteMirrored->SetType(CBT_PUSH);
		btnPasteMirrored->SetButtonDownNotify(TRUE);
		btnPasteMirrored->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteBoneMirrored"), GetString(IDS_TT_PASTERIGMIRROR)));
		btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

		BOOL usingcustommesh = FALSE;
		for (int i = 0; i < bone->GetNumBones(); i++) {
			ICATObject* iobj = bone->GetBone(i)->GetICATObject();
			if (iobj && iobj->UsingCustomMesh()) usingcustommesh = TRUE;
		}

		SET_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH, usingcustommesh);

		// btnAddArbBone button
		btnAddArbBone = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADDARBBONE));	btnAddArbBone->SetType(CBT_PUSH);	btnAddArbBone->SetButtonDownNotify(TRUE);
		btnAddArbBone->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddExtraBone"), GetString(IDS_TT_ADDBONE)));

		btnAddERN = GetICustButton(GetDlgItem(hWnd, IDC_BTN_ADDRIGGING));		btnAddERN->SetType(CBT_CHECK);		btnAddERN->SetButtonDownNotify(TRUE);
		btnAddERN->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnAddERN"), GetString(IDS_TT_ADDRIGOBJS)));

		if (bone->GetCATParentTrans()->GetCATMode() != SETUPMODE) {
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
	}
	void ReleaseControls()
	{
		SAFE_RELEASE_EDIT(edtName);

		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnPasteMirrored);

		SAFE_RELEASE_BTN(btnAddArbBone);
		SAFE_RELEASE_BTN(btnAddERN);
		ExtraRigNodes *ern = (ExtraRigNodes*)boneseg->GetInterface(I_EXTRARIGNODES_FP);
		DbgAssert(ern);
		ern->IDestroyERNWindow();
	}

	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (BoneData*)map->GetParamBlock()->GetOwner());
			break;
		case WM_DESTROY:
			ReleaseControls();
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_BUTTONUP: {
				switch (LOWORD(wParam)) {
				case IDC_BTN_COPY:				CATControl::SetPasteControl(bone);												break;
				case IDC_BTN_PASTE:				bone->PasteFromCtrl(CATControl::GetPasteControl(), false);		break;
				case IDC_BTN_PASTE_MIRRORED:	bone->PasteFromCtrl(CATControl::GetPasteControl(), true);			break;
				case IDC_BTN_ADDARBBONE: {
					CATNodeControl* catnodectrl = (CATNodeControl*)boneseg->GetInterface(I_CATNODECONTROL);
					if (catnodectrl)
					{
						HoldActions actions(IDS_HLD_ADDBONE);
						catnodectrl->AddArbBone();
					}
					break;
				}
				case IDC_BTN_ADDRIGGING: {
					ExtraRigNodes* ern = (ExtraRigNodes*)boneseg->GetInterface(I_EXTRARIGNODES_FP);
					DbgAssert(ern);
					if (btnAddERN->IsChecked())
						ern->ICreateERNWindow(hWnd);
					else ern->IDestroyERNWindow();
					break;
				}
				}
			}
			break;
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_USECUSTOMMESH: {
					// collapse all the bone segments
					for (int i = 0; i < bone->GetNumBones(); i++) {
						ICATObject* iobj = bone->GetBone(i)->GetICATObject();
						if (iobj) iobj->SetUseCustomMesh(IS_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH));
					}
					break;
				}
				}
				break;
			}
			break;
		case WM_CUSTEDIT_ENTER:
		{
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_SEGNAME: {
				TCHAR strbuf[128];			edtName->GetText(strbuf, 64);// max 64 characters
				TSTR name(strbuf);			boneseg->SetName(name);
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

static BoneDataParamDlgCallBack BoneDataParamCallBack;

static ParamBlockDesc2 BoneData_t_param_blk(
	BoneData::pb_idParams, _T("Bone Setup"), 0, &BoneDataDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, BoneData::BONEDATAPB_REF,
	IDD_BONEDATA_SETUP_CAT3, IDS_BONEDATAPARAMS, 0, 0, &BoneDataParamCallBack,
	BoneData::PB_LIMBDATA, _T("Limb"), TYPE_REFTARG, P_SUBANIM, IDS_CL_LIMBDATA,
		p_end,

	BoneData::PB_BONEID_DEPRECATED, _T("BoneID"), TYPE_INDEX, 0, IDS_BONEID,
		p_default, 0,
		p_end,

	BoneData::PB_BONELENGTH, _T("Length"), TYPE_FLOAT, 0, 0,
		p_default, 40.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_LENGTH, IDC_SPIN_LENGTH, SPIN_AUTOSCALE,//0.01f,
		p_end,
	BoneData::PB_BONEWIDTH, _T("Width"), TYPE_FLOAT, 0, NULL,
		p_default, 12.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_WIDTH, IDC_SPIN_WIDTH, SPIN_AUTOSCALE,//0.01f,
		p_end,
	BoneData::PB_BONEDEPTH, _T("Depth"), TYPE_FLOAT, 0, NULL,
		p_default, 12.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_DEPTH, IDC_SPIN_DEPTH, SPIN_AUTOSCALE,//0.01f,
		p_end,
	BoneData::PB_TMSETUP_DEPRECATED, _T(""), TYPE_MATRIX3, P_OBSOLETE, NULL,
		p_end,
	// The lengths of all the children
	BoneData::PB_CHILD_LENGTHS_DEPRECATED, _T(""), TYPE_FLOAT, P_OBSOLETE, NULL,
		p_end,
	BoneData::PB_NUMSEGS, _T("NumSegs"), TYPE_INT, 0, NULL,
		p_default, 0,
		p_range, 1, 20,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_NUMSEGS, IDC_SPIN_NUMSEGS, SPIN_AUTOSCALE,
		p_end,
	BoneData::PB_SEGNODE_TAB_DEPRECATED, _T(""), TYPE_INODE_TAB, 0, P_NO_REF + P_OBSOLETE, NULL,
		p_end,

	BoneData::PB_SETUP_BONERATIO_DEPRECATED, _T(""), TYPE_FLOAT, P_OBSOLETE, NULL,
		p_end,

	BoneData::PB_ANGLERATIO_DEPRECATED, _T(""), TYPE_FLOAT, P_OBSOLETE + P_ANIMATABLE, NULL,
		p_end,
	BoneData::PB_SETUP_SWIVEL_DEPRECATED, _T(""), TYPE_FLOAT, 0, NULL,
		p_end,
	BoneData::PB_TWISTWEIGHT, _T("TwistWeight"), TYPE_FLOAT, P_ANIMATABLE, IDS_TWISTWEIGHT,
		p_end,
	// For GameDev only. To decrease the amount of data needed to
	// represent the transforms in all our segments, we publish the
	// swivel angle, this along with weights, and first seg tm, is
	// all that is needed to represent the entire bone
	BoneData::PB_TWISTANGLE, _T(""), TYPE_FLOAT, 0, NULL,
		p_end,

	BoneData::PB_SEGOBJ_PB_TAB_DEPRECATED, _T(""), TYPE_REFTARG_TAB, 0, P_OBSOLETE + P_NO_REF, 0,
		p_end,
	BoneData::PB_SCALE_DEPRECATED, _T(""), TYPE_POINT3, P_ANIMATABLE + P_OBSOLETE, 0,
		p_end,
	BoneData::PB_NAME, _T(""), TYPE_STRING, P_RESET_DEFAULT, NULL,
		p_default, _T(""),
		p_ui, TYPE_EDITBOX, IDC_EDIT_NAME,
		p_end,
	BoneData::PB_SEG_TAB, _T("BoneSegs"), TYPE_REFTARG_TAB, 0, P_NO_REF, 0,
		p_end,
	p_end
);

BoneData::BoneData(BOOL loading)
	: mIsEvaluating(false)
	, mdParentTwistAngle(0.0f)
	, mdChildTwistAngle(0.0f)
	, mTwistValid(FOREVER)
	, pblock(NULL)
	, mBendRatio(0.0f)
{
	// default bones to being stretchy
	ccflags |= CCFLAG_SETUP_STRETCHY | CNCFLAG_LOCK_LOCAL_POS | CNCFLAG_LOCK_SETUPMODE_LOCAL_POS | CCFLAG_EFFECT_HIERARCHY;

	if (!loading)
		BoneDataDesc.MakeAutoParamBlocks(this);
}

RefTargetHandle BoneData::Clone(RemapDir& remap)
{
	// make a new BoneData object to be the clone
	// call true for loading so the new BoneData doesn't
	// make new default subcontrollers.
	BoneData *newBoneData = new BoneData(TRUE);
	// We add this entry now so that the cloning system doesn't try to clone us again when
	// BoneSegTrans's pblock is cloned
	remap.AddEntry(this, newBoneData);

	newBoneData->ReplaceReference(BONEDATAPB_REF, CloneParamBlock(pblock, remap));

	// clone our subcontrollers and assign them to the new object.
	if (mLayerTrans)	newBoneData->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newBoneData, remap);

	// force the cloning of all the segments
	for (int i = 0; i < GetNumBones(); i++) {
		newBoneData->pblock->SetValue(PB_SEG_TAB, 0, remap.CloneRef(GetBone(i)), i);
		((BoneSegTrans*)newBoneData->GetBone(i))->SetBoneData(newBoneData);
	}

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newBoneData, remap);

	// now return the new object.
	return newBoneData;
}

BoneData::~BoneData()
{
	DeleteAllRefs();
}

SClass_ID BoneData::SuperClassID()
{
	return CATNodeControl::SuperClassID();
}

void BoneData::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		BoneData *newctrl = (BoneData*)from;

		if (newctrl->mLayerTrans)
			ReplaceReference(LAYERTRANS, newctrl->mLayerTrans);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void BoneData::Update()
{
	for (int j = 0; j < GetNumBones(); j++)
	{
		CATNodeControl* pBone = GetBone(j);
		if (pBone != NULL)
		{
			INode *segnode = pBone->GetNode();
			if (segnode)
				segnode->InvalidateTM();
		}
	}
	LimbData2* pLimb = GetLimbData();
	if (pLimb)
		pLimb->UpdateLimb();
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

LimbData2* BoneData::GetLimbData()
{
	DbgAssert(pblock != NULL);
	if (pblock != NULL)
		return static_cast<LimbData2*>(pblock->GetReferenceTarget(PB_LIMBDATA));
	return NULL;
}

void BoneData::SetLimbData(LimbData2* l)
{
	DbgAssert(pblock != NULL);
	if (pblock != NULL)
		pblock->SetValue(PB_LIMBDATA, 0, l);
}

/************************************************************************/
/* New methods for the FK and IK system                                 */
/************************************************************************/

// Calculate the angle we need to twist our bones so that
// they soften the twist from their parents.
float BoneData::CalculateTwistAngle(Matrix3 tmLocal)
{
	// We are only interested in the twist around the point,
	// so its the rotation thats left when we point in
	// the same direction as our immediate ancestor.
	// This is in essence is when tmLocal's lengthAxis row
	// is aligned with the lengthAxis unit vector (eg, 0, 0, 1)
	int iLengthAxis = GetLengthAxis();
	Point3 p3NoRot = Point3::Origin;
	p3NoRot[iLengthAxis] = 1.0f;
	// This should leave us with 1 vector pointing in the
	// same direction as p3NoRot
	RotateMatrixToAlignWithVector(tmLocal, p3NoRot, iLengthAxis);

	// Now, our twist is clearly laid in front of us.  It's simply the Y rotation left.
	Point3 p3Twist = tmLocal.GetRow(Y);
	p3Twist.Unify();
	float dTwistRot = DotProd(p3Twist, Point3::YAxis);
	if ((-1 >= dTwistRot) || (dTwistRot >= 1))
		return 0.0f;
	float dTwist = acos(dTwistRot);

	// Last check, are we +ve or -ve this rotation?
	const Point3& p3OtherAxis = tmLocal.GetRow((iLengthAxis == X) ? Z : X);
	if (p3OtherAxis[Y] > 0)
		dTwist = -dTwist;

	return dTwist;
}

// This method will be evaluated if the pLimb is in FK and will also be evaluated
// during IK. This is because the FK system maintains the scale. Our new IK system
// uses the FK solution to drive the Bending of the pLimb while in IK so
// we are GUARANTEED to have the entire FK solution calculated before the IK solution.

void BoneData::GetFKValue(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Matrix3& tmValue, Point3& p3LocalScale, Interval& ivValid)
{
	LimbData2* pLimb = GetLimbData();
	DbgAssert(pLimb != NULL);

	// Hack: Caching only works for standard inheritance.
	// Disable if we have anything but that.
	if (validFK.InInterval(t) && TestCCFlag(CNCFLAG_INHERIT_ANIM_ALL))
	{
		Point3 mLocalPosOffset = mLocalFK.GetTrans();
		mLocalPosOffset *= p3ParentScale;
		Matrix3 tmLocalScaledPos = mLocalFK;
		tmLocalScaledPos.SetTrans(mLocalPosOffset);

		tmValue = tmLocalScaledPos * tmValue;
		p3LocalScale *= mLocalScale;
		// Update cache (in pure FK only)
		if (pLimb->GetIKFKRatio(t, ivValid) >= 1.0f)
			FKTM = tmValue;
	}
	else
	{
		// In order to calculate a pure FK solution, we need
		// to be evaluating here IFF we are in pure FK.
		// Otherwise, our incoming tmWorld will be contaminated
		// by including IK.  This assumption only holds true
		// because we LOCK_LIMB_FK and evaluate the entire
		// tree before attempting to solve FK
		DbgAssert(GetBoneID() == 0 || pLimb->GetIKFKRatio(t, ivValid) >= 1.0f);

		// Find our local FK solution validity
		validFK.SetInfinite();

		// Our FK value is simply our world transform
		FKTM = tmValue;
		mLocalScale.Set(1, 1, 1);
		CATNodeControl::CalcWorldTransform(t, tmParent, p3ParentScale, FKTM, mLocalScale, validFK);

		// Find the local FK result & cache
		mLocalFK = FKTM * Inverse(tmValue);

		// return the FK matrix
		tmValue = FKTM;
		p3LocalScale *= mLocalScale;
	}

	ivValid &= validFK;	// cache our local validity
}

#define COS_LIMIT(v) (max(min((v), 1.0f), -1.0f))

void BoneData::GetIKValue(TimeValue t, const Point3& p3CurrPos, const Matrix3& tmParent, Matrix3& tmValue, Point3& p3LocalScale, Interval& ivValid)
{
	//if(validIK.InInterval(t))
	//{
	//	tmWorld = IKTM;
	//	return;
	//}

	// Get the data our IK system relies on.
	Matrix3 tmIKTarget, tmFKTarget;
	int iBoneID = GetBoneID();
	LimbData2* pLimb = (LimbData2*)GetLimbData();
	validIK.SetInfinite();
	pLimb->GetIKTMs(t, validIK, p3CurrPos, tmParent, tmIKTarget, tmFKTarget, iBoneID);

	float child_lengths = pLimb->GetLimbIKLength(t, validIK, iBoneID);
	int numbones = pLimb->GetNumBones();
	numbones = min((int)floor(pLimb->GetLimbIKPos(t, validIK)) + 1, numbones);
	int lengthaxis = GetLengthAxis();

	// each bones IK Transform is based on the FK solution
	if (iBoneID == 0)
		IKTM = tmValue;
	else
	{
		pLimb->GetBoneFKTM(t, iBoneID, IKTM);

		// Remove scale
		Point3* mtxRows = reinterpret_cast<Point3*>(IKTM.GetAddr());
		mtxRows[0] = mtxRows[0].FNormalize();
		mtxRows[1] = mtxRows[1].FNormalize();
		mtxRows[2] = mtxRows[2].FNormalize();
	}

	// Find the initial bone position
	Point3 bonepos = tmValue.GetTrans();

	Point3 this_ik_bone_to_ik_target_vec = tmIKTarget.GetTrans() - bonepos;
	Point3 this_fk_bone_to_fk_target_vec = tmFKTarget.GetTrans() - IKTM.GetTrans();

	float this_bone_to_ik_target_length = Length(this_ik_bone_to_ik_target_vec);
	if (this_bone_to_ik_target_length > 0)
		this_ik_bone_to_ik_target_vec /= this_bone_to_ik_target_length;

	//////////////////////////////////////////////////////////////////////////////////////
	// The main goal of the following section is to force the entire IK chain to
	// Always solve on a unified plane.
	// The problem is that each bone evaluates totally independently and therefore
	// we need to ensure they behave as a whole.
	// You can take the following section of code out and the IK solution will work just fine,
	// but it will have a sloppy bends when you have more then 2 bones.
	// For example, if the FK pLimb is a pLimb of 4 bones and all the bones lie on a plane.
	// then after the IK solver does its thing, we would like our bones to still lie on a plane
	// but solved to hit the ik target correctly

	AngAxis ax;
	Matrix3 tmRetargetting;
	if (iBoneID == 0) {

		Point3 this_fk_bone_to_fk_target_vec = tmFKTarget.GetTrans() - bonepos;
		float this_fk_bone_to_fk_target_length = Length(this_fk_bone_to_fk_target_vec);
		if (this_fk_bone_to_fk_target_length > 0)
			this_fk_bone_to_fk_target_vec /= this_fk_bone_to_fk_target_length;

		// Define a general offset we would like to apply to the entire pLimb.
		// this offset points the FK hierarchy at our IKTarget before we start any IK stuff
		float axAngle = DotProd(this_ik_bone_to_ik_target_vec, this_fk_bone_to_fk_target_vec);
		axAngle = COS_LIMIT(axAngle);
		ax.angle = acos(axAngle);
		ax.axis = Normalize(CrossProd(this_ik_bone_to_ik_target_vec, this_fk_bone_to_fk_target_vec));

		// Apply the offset to this bone and continue
		RotateMatrix(IKTM, ax);
		pLimb->SetIKOffsetAX(ax);

		INode* pUpNode = pLimb->GetUpNode();
		if (pUpNode != NULL)
		{
			AngAxis upVax;

			Matrix3 upnode_tm = pUpNode->GetNodeTM(t, &validIK);
			Point3 this_bone_to_up_node = upnode_tm.GetTrans() - FKTM.GetTrans();
			Point3 vec1 = Normalize(CrossProd(this_bone_to_up_node, this_ik_bone_to_ik_target_vec));
			Point3 vec2 = Normalize(CrossProd(IKTM.GetRow(lengthaxis), this_ik_bone_to_ik_target_vec));
			float upAngle = DotProd(vec1, vec2);
			upAngle = COS_LIMIT(upAngle);
			upVax.angle = acos(upAngle);

			// Modulate the angle by IK ratio. This prevents
			// the arm from flipping out when blending on IK
			// ratio and the up axis is far from the current solution.
			upVax.angle *= 1.0f - pLimb->GetIKFKRatio(t, validIK, iBoneID);

			upVax.axis = Normalize(CrossProd(vec1, vec2));

			// Apply the offset to this bone and continue
			RotateMatrix(IKTM, upVax);
			pLimb->SetUpNodeOffsetAX(upVax);
		}

		IKTM.SetTrans(bonepos);
		tmRetargetting = IKTM;

		// If we need to stretch to meet our goal, then scale the bone to length.
		// After this, we need the IK to straighten the limb
		if (pLimb->IsLimbStretching(t, tmIKTarget.GetTrans()))
		{
			Interval validIK;
			float scale = this_bone_to_ik_target_length / pLimb->GetLimbIKLength(t, validIK, -1);
			p3LocalScale[lengthaxis] *= scale;
			pLimb->CacheLimbStretch(scale);
			tmValue = IKTM;
			// Set the validity to the IKValid.  Technically, this should not be
			// necessary.  Stretching should only occur in Interactive mode, therefore
			// the validity will never be used, but its better to be correct than assume.
			ivValid = validIK;
		}
	}
	else
	{
		RotateMatrix(IKTM, pLimb->GetIKOffsetAX());

		INode* pUpNode = pLimb->GetUpNode();
		if (pUpNode != NULL)
			RotateMatrix(IKTM, pLimb->GetUpNodeOffsetAX());

		// This code is to stop flickering in the IK Chain when it does not know which way to bend.
		// Build a tm based on FKTM that looks at the FK target
		this_fk_bone_to_fk_target_vec.Unify();

		Matrix3 tmFKTMLookingAtFKTarget = FKTM;
		float axAngle = DotProd(FKTM.GetRow(lengthaxis), this_fk_bone_to_fk_target_vec);
		axAngle = COS_LIMIT(axAngle);
		ax.angle = -acos(axAngle);
		ax.axis = Normalize(CrossProd(FKTM.GetRow(lengthaxis), this_fk_bone_to_fk_target_vec));
		RotateMatrix(tmFKTMLookingAtFKTarget, ax); tmFKTMLookingAtFKTarget.SetTrans(FKTM.GetTrans());

		// Now calc a tm based on IKTM, looking at the IKTarget
		Matrix3 tmIKTMLookingAtIKTarget = IKTM;
		axAngle = DotProd(IKTM.GetRow(lengthaxis), this_ik_bone_to_ik_target_vec);
		axAngle = COS_LIMIT(axAngle);
		ax.angle = -acos(axAngle);
		ax.axis = Normalize(CrossProd(IKTM.GetRow(lengthaxis), this_ik_bone_to_ik_target_vec));
		RotateMatrix(tmIKTMLookingAtIKTarget, ax); tmIKTMLookingAtIKTarget.SetTrans(IKTM.GetTrans());

		// Find the difference between FKTM and the FKTM matrix that looks at the FKTarget
		Matrix3 tmFKDelta = FKTM * Inverse(tmFKTMLookingAtFKTarget);
		// add on the above calculated Delta
		IKTM = tmFKDelta * tmIKTMLookingAtIKTarget;

		IKTM.SetTrans(bonepos);

		if (pLimb->IsLimbStretching(t, tmIKTarget.GetTrans()))
		{
			// If we do not inherit scale, we need to re-apply the limb stretching here.
			if (GetCATMode() == SETUPMODE || !TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL))
				p3LocalScale[lengthaxis] *= pLimb->GetLimbStretchCache();
		}
	}

	// Find the vector that points to the next bone in the chain
	Point3 this_ik_bone_vec;
	float bone_length;

	// Find the length between the root of this bone, and the root of the next.
	// This allows positional offsets in the IK chain, and includes scale etc.
	bone_length = (pLimb->GetBoneFKPosition(t, iBoneID) - pLimb->GetBoneFKPosition(t, iBoneID + 1)).Length();
	this_ik_bone_vec = IKTM.GetRow(lengthaxis);

	/////////////////////////////////////////////////////////////////
	// now for the core of the problem.
	// we are now going to generate an axis and an angle by which we will rotate
	// this bones matrix. The result will be to extend/retract the leg to reach the IK target perfectly
	// Angle
	// We use basic trig here to calculate an angle that rotate this bone so that its children bones
	// can reach the target. If we have 2 or more children bones, then we simplify them down to one imaginary bone
	// the length of this imaginary bone is determined by the stretch_ratio...
	// Axis
	// Previously we always rotated around the bones X Axis. Basically we took the parents matrix
	// and aimed it at the IKTarget, and then used a swivel angle to twist it until the x axis was
	// pointing in the right direction.
	// We aim the whole FK leg in the general direction of the IKTarget using axOffset angle axis.
	// Then, for each bone we simply adjust its rotation to extend, or retract the pLimb to meet the IKTarget..

	// we will use this AngAxis to solve our IK solution

	float desired_angle;
	float cos_angle;

	// what is the current angle between this bone, and the this_bone_to_target vector
	cos_angle = DotProd(this_ik_bone_to_ik_target_vec, this_ik_bone_vec);
	cos_angle = COS_LIMIT(cos_angle);
	ax.angle = -acos(cos_angle);
	ax.axis = CrossProd(this_ik_bone_vec, this_ik_bone_to_ik_target_vec);
	ax.axis.Unify();

	// if we cannot reach the target, or we are the last bone in the chain,
	// then just aim ourselves in the direction of the ik target by applying
	// the above computed transform.
	if (this_bone_to_ik_target_length >= (child_lengths + bone_length) ||
		bone_length > (this_bone_to_ik_target_length + child_lengths) ||
		((iBoneID == (numbones - 1)) && !pLimb->GetPalmTrans())) {
		// Simply apply the AngleAxis to the current transform
		RotateMatrix(IKTM, ax); IKTM.SetTrans(bonepos);
	}
	// if we have at least one child bone, or palm then do this....
	else
	{
		if (!(iBoneID == (numbones - 1) && pLimb->GetPalmTrans()))
		{
			if ((iBoneID <= (numbones - 2)) ||
				((iBoneID <= (numbones - 1)) && pLimb->GetPalmTrans())) {

				// this is the length of the bone that goes from the end of this bone to our IKTarget
				float imaginary_bone_length;

				if ((iBoneID == (numbones - 2)) ||
					((iBoneID == (numbones - 1) && pLimb->GetPalmTrans())))
				{
					// if we are the last bone then child lengths is the palms length
					// otherwise it should include 'extralength'
					imaginary_bone_length = child_lengths;
				}
				else
				{
					// shortest possible child length is where we look at the target
					float ik_short_child = this_bone_to_ik_target_length - bone_length;
					// the longest is where we look in the other direction
					float ik_long_child = min(child_lengths, this_bone_to_ik_target_length + bone_length);

					imaginary_bone_length = ik_short_child + ((ik_long_child - ik_short_child) * mBendRatio);

				}
				if (imaginary_bone_length > (bone_length + this_bone_to_ik_target_length)) {
					// look in the opposite direction of the IKTarget
					ax.angle += (float)M_PI;
				}
				else {
					float cos_desired_angle = (sq(bone_length) + sq(this_bone_to_ik_target_length) - sq(imaginary_bone_length)) / (2 * bone_length * this_bone_to_ik_target_length);
					// Do not let the float errors exceed 1.0f
					cos_desired_angle = COS_LIMIT(cos_desired_angle);
					desired_angle = -acos(cos_desired_angle);

					// This system simply applies and adjustment to the FK solution to make it reach for the IK target
					// so we calculate how much of a change would give us the right answer
					ax.angle = -(desired_angle - ax.angle);
				}
			}
			// if we really are the last bone in the pLimb, and
			// there is no palm/ankle, then just look at the target
			// Here we go!, apply the AngleAxis to the current transform
			RotateMatrix(IKTM, ax); IKTM.SetTrans(bonepos);
		}
		else // we are last bone in the pLimb and we have a palm
		{
			PalmTrans2 *palm = pLimb->GetPalmTrans();
			float dTargetAlign = palm->GetTargetAlign(t, validIK);
			float imaginary_bone_length = palm->GetBoneLength();

			// Rotate the target align matrix so that it is looking at the IKTarget matrix
			// that the rest of the limb looks at.
			RotateMatrix(IKTM, ax); IKTM.SetTrans(bonepos);

			/////////////////////////////////////////////////////////////////
			// palm rotates relative to the IKTarget
			Matrix3 tmTargetAlign = IKTM;

			// palm rotates relative to its parent
			if (dTargetAlign < 1.0f) {
				// Axis for when the palm is relative to it parent

				float cosAngle = (sq(bone_length) + sq(this_bone_to_ik_target_length) - sq(imaginary_bone_length)) / (2 * bone_length * this_bone_to_ik_target_length);
				cosAngle = COS_LIMIT(cosAngle);
				ax.angle = acos(cosAngle);

				// The angle is valid for any particular axis we care to chose.  We want to chose
				// the axis that ends up giving us the same rotation as the FK solution requested
				Matrix3 tmPalmLocal(1);
				pLimb->GettmFKTarget(t, validIK, NULL, tmPalmLocal);
				ax.axis = Normalize(CrossProd(IKTM.GetRow(lengthaxis), (tmPalmLocal * IKTM).GetRow(lengthaxis)));

				// Apply the IK rotation.
				RotateMatrix(IKTM, ax);

				/////////////////////////////////////////////////////////////////
				// Now we blend the result of each setting..
				// this will cause a bit of inaccuracy but get
				// rid of an ugly wobble when the two solutions are vastly different
				BlendRot(IKTM, tmTargetAlign, dTargetAlign);
			}

			IKTM.SetTrans(bonepos);
		}
	}

	if (iBoneID == 0 && pLimb->GetisLeg() && pLimb->TestFlag(LIMBFLAG_FFB_WORLDZ)) {
		float ffback = pLimb->GetForceFeedback(t, validIK);
		if (ffback > 0.0f) {
			BlendMat(IKTM, tmRetargetting, ffback);
		}
	}

	ivValid &= validIK;
	tmValue = IKTM;
}

void BoneData::CalcInheritance(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale)
{
	// Annoying legacy - if we are the first bone in a limb - flip our transform.
	if (GetBoneID() == 0)
		tmParent.PreRotateY(PI);
	CATNodeControl::CalcInheritance(t, tmParent, p3ParentScale);
}

void BoneData::ApplySetupOffset(TimeValue t, Matrix3 tmOrigParent, Matrix3& tmWorldTransform, Point3 &p3BoneLocalScale, Interval& ivValid)
{
	// Annoying legacy - if we are the first bone in a limb - flip our transform.
	// This is done to match the orig parents orientation with the calculated
	// orientation from CalcParentMatrix (which calls CalcInheritance above)

	// We could have flipped the tmOrigParent used by CATNodeControl, but I felt
	// it was better to do it only here where the flip is necessary - other functions
	// will still receive the original, untarnished tmOrigParent.
	if (GetBoneID() == 0)
		tmOrigParent.PreRotateY(PI);

	CATNodeControl::ApplySetupOffset(t, tmOrigParent, tmWorldTransform, p3BoneLocalScale, ivValid);
}

void BoneData::CalcWorldTransform(TimeValue t, const Matrix3& tmOrigParent, const Point3& p3ParentScale, Matrix3& tmValue, Point3& p3LocalScale, Interval& ivLocalValid)
{
	LimbData2* pLimb = GetLimbData();
	if (!pLimb)
		return;

	if (IsEvaluationBlocked() || pLimb->IsEvaluationBlocked())
		return;

	// Are we in IK or FK?
	int iBoneID = GetBoneID();
	float ikfkratio = pLimb->GetIKFKRatio(t, ivLocalValid, iBoneID);

	// Cache our calculated parent matrix.
	Matrix3 tmParent = tmValue;

	// Even when we are in IK, this is necessary
	GetFKValue(t, tmParent, p3ParentScale, tmValue, p3LocalScale, ivLocalValid);

	if (ikfkratio < 1.0f)
	{
		Matrix3 tmFKValue = tmValue;

		GetIKValue(t, tmParent.GetTrans(), tmOrigParent, tmValue, p3LocalScale, ivLocalValid);

		// If we are in the middle of an Num IK Bones blend, we need
		// to blend to reflect this.
		float limbIKPos = pLimb->GetLimbIKPos(t, ivLocalValid);
		if (limbIKPos > iBoneID && limbIKPos < (iBoneID + 1))
		{
			float blendRatio = limbIKPos - iBoneID;
			BlendRot(tmValue, tmFKValue, 1.0f - blendRatio);
		}
	}

	// While evaluating the twist, it's possible for
	// us to loop back and evaluate this (for example,
	// when the palm calls ApplyParentOffset, the BoneSegTrans
	// will reset the transform to remove the effect of twist).
	// This switch is to tell the BoneSegTrans not to do this,
	// because our tmBoneWorld hasn't been updated yet (it only
	// happens when this function exits), and also because
	// the input matrix is from right here - it has no twist.
	DbgAssert(mIsEvaluating == false);
	mIsEvaluating = true;

	// If we have more than 1 bone, we want to blend the twist on running down the arm
	if (GetNumBones() > 1)
	{
		// Calculate our twist from our parent
		// Bone motion is inherited from the Ribcage, which means
		// that tmParent is essentially tmRibcage but...
		// our blend is calculated from the collarbone, if there is one.
		// tmOrigParent is the non-modified transform (tmCollarbone)
		// For the 2nd bone etc, the tmParent == tmOrigParent for what
		// we need here.
		if (GetBoneID() == 0)
		{
			Matrix3 tmFixedParent = tmOrigParent;
			// Rotate around such that a right angle is a 0 rotation.
			tmFixedParent.PreRotateY((float)(M_PI_2 * pLimb->GetLMR()));
			mdParentTwistAngle = CalculateTwistAngle(tmValue * Inverse(tmFixedParent));
		}
		else
			mdParentTwistAngle = CalculateTwistAngle(tmValue * Inverse(tmOrigParent));

		// Now calculate the twist we will receive from our child.
		// TODO: We might be able to avoid the GetValue call here,
		// by directly referencing the mdFKBonePosition param on Limb
		CATNodeControl* pChildBone = GetChildCATNodeControl(0);
		if (pChildBone != NULL)
		{
			Matrix3 tmChild = tmValue;
			mTwistValid = FOREVER;
			pChildBone->GetValue(t, &tmChild, mTwistValid, CTRL_RELATIVE);
			Matrix3 tmTwistParent = tmValue;
			if (pChildBone->ClassID() == PALMTRANS2_CLASS_ID)
				tmTwistParent = FixTwistForPalm(tmValue, pLimb);

			mdChildTwistAngle = CalculateTwistAngle(tmChild * Inverse(tmTwistParent));
		}
	}
	else
	{
		// We cannot twist with only 1 bone, so skip the calculations.
		mdChildTwistAngle = mdParentTwistAngle = 0.0f;
		mTwistValid.SetInfinite();
	}

	DbgAssert(mIsEvaluating == true);
	mIsEvaluating = false;
}

Matrix3 BoneData::FixTwistForPalm(const Matrix3& tmBoneWorld, LimbData2* pLimb)
{
	Matrix3 res = tmBoneWorld;
	if (pLimb != NULL && !pLimb->GetisLeg())
	{
		if (GetLengthAxis() == Z_AXIS)
		{
			if (pLimb->GetLMR() == -1)
			{
				res.SetRow(Y_AXIS, tmBoneWorld.GetRow(X_AXIS));
				res.SetRow(X_AXIS, -tmBoneWorld.GetRow(Y_AXIS));
			}
			else
			{
				res.SetRow(Y_AXIS, -tmBoneWorld.GetRow(X_AXIS));
				res.SetRow(X_AXIS, tmBoneWorld.GetRow(Y_AXIS));
			}
		}
		else // X_AXIS
		{
			if (pLimb->GetLMR() == -1)
			{
				res.SetRow(Y_AXIS, -tmBoneWorld.GetRow(Z_AXIS));
				res.SetRow(Z_AXIS, tmBoneWorld.GetRow(Y_AXIS));
			}
			else
			{
				res.SetRow(Y_AXIS, tmBoneWorld.GetRow(Z_AXIS));
				res.SetRow(Z_AXIS, -tmBoneWorld.GetRow(Y_AXIS));
			}
		}
	}
	return res;
}

RefTargetHandle BoneData::GetReference(int i)
{
	switch (i)
	{
	case BONEDATAPB_REF:	return pblock;
	case LAYERTRANS:		return mLayerTrans;
	default:				return NULL;
	}
}

void BoneData::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case BONEDATAPB_REF:	pblock = (IParamBlock2*)rtarg;			break;
	case LAYERTRANS:		mLayerTrans = (CATClipMatrix3*)rtarg;	break;
	}

}

Animatable* BoneData::SubAnim(int i)
{
	switch (i)
	{
	case SUBLIMB:		return GetLimbData();
	case SUBTRANS:		return mLayerTrans;
	default:			return NULL;
	}
}

TSTR BoneData::SubAnimName(int i)
{
	switch (i)
	{
	case SUBLIMB:		return GetString(IDS_LIMBDATA);
	case SUBTRANS:		return GetString(IDS_LAYERTRANS);
	default:			return _T("");
	}
}

RefResult BoneData::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if (pblock == hTarg)
		{
			int tabIndex = 0;
			ParamID index = pblock->LastNotifyParamID(tabIndex);
			switch (index)
			{
			case PB_BONELENGTH:
			{
				LimbData2* pLimb = GetLimbData();
				if (pLimb && !pLimb->IsEvaluationBlocked())
					pLimb->UpdateLimb();
			}
			case PB_BONEWIDTH:
			case PB_BONEDEPTH:
				UpdateCATUnits();
				break;
			case PB_NUMSEGS:
				UpdateNumSegBones();
				break;
			case PB_NAME:
				CATMessage(0, CAT_NAMECHANGE);
				break;
			}
			break;
		}
		else if (mLayerTrans == hTarg)
		{
			validFK = NEVER;
			LimbData2* pLimb = GetLimbData();
			if (pLimb != NULL)
			{
				TimeValue t = GetCOREInterface()->GetTime();
				if (!pLimb->IsEvaluationBlocked() && pLimb->GetIKFKRatio(t, FOREVER) < 1)
					pLimb->UpdateLimb();
				else
				{
					if (GetBoneID() > 0)
					{
						// Find the last BoneData, and update it.
						BoneData* pParentBone = pLimb->GetBoneData(GetBoneID() - 1);
						if (pParentBone != NULL)
							pParentBone->InvalidateTransform();
					}
					else
						InvalidateTransform();
				}
			}
			break;
		}

	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (mLayerTrans == hTarg)
			mLayerTrans = NULL;
		break;
	}
	return REF_SUCCEED;
}

// CATs Internal messaging system, cause using
// Max's system is like chasing mice with a mallet
void BoneData::CATMessage(TimeValue t, UINT msg, int data) //, void* CATData)
{
	switch (msg)
	{
		//case CAT_KEYFREEFORM:
		//case CAT_KEYLIMBBONES:
		//	{
		//		// Set our current value straight back in
		//		// We can't use KeyFreeform here, as it does funny stuff.
		//		// and we have a strange relationship with our bone segs
		//		SetXFormPacket xform;
		//		xform.tmAxis = GetNodeTM(t);
		//		xform.tmParent = GetParentTM(t);

		//		KeyFreeformMode SetMode(GetCATParentTrans()->GetLayerRoot());
		//		//CATNodeControl* pBone = GetBone(0);
		//		//if (pBone != NULL)
		//		//	pBone->KeyFreeform(t);

		//		//KeyFreeform(t);

		//		SetValue(t, &xform, 1, CTRL_ABSOLUTE);

		//		// Turn on KeyFreeform for the SetNode bit
		//		//Matrix3 tmValue = GetNodeTM(t);

		//		//KeyFreeformMode keyMode(GetLayerTrans()->GetRoot());
		//		//SetNodeTM(t, tmValue);
		//		break;
		//	}
	case CAT_NAMECHANGE:
		// Just let all the segments update thir names
		CATControl::CATMessage(t, msg, data);
		return;
	case CAT_UPDATE:
	case CAT_CATMODE_CHANGED:
		validIK = validFK = NEVER;
		break;
	}

	CATNodeControl::CATMessage(t, msg, data);
}

BOOL BoneData::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	int segNumber = 0;
	LimbData2* pLimb = GetLimbData();

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idLimbBone) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idBoneSeg:		GetBone(segNumber)->LoadRig(load); segNumber++;					break;
			case idController:	DbgAssert(GetLayerTrans()); GetLayerTrans()->LoadRig(load);		break;
			case idCATWeightParams: {
				Control *ctrlTwistWeight = pblock->GetControllerByID(PB_TWISTWEIGHT);
				if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
					((CATWeight*)ctrlTwistWeight)->LoadRig(load);
				break;
			}
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idBoneName:
				load->GetValue(name);
				if (name.Length() == 0) name.printf(_T("%d"), GetBoneID() + 1);
				SetName(name);
				break;
			case idFlags:		load->GetValue(ccflags);											break;
			case idSetupTM:
			{
				Matrix3 tmSetup;
				load->GetValue(tmSetup);

				// Converting old school bend rations to FK bending, as now the IK system
				// is driven by the FK system rather than bend ratio
				if (load->GetVersion() < CAT_VERSION_1700)
				{
					BoneData* pBoneParent = dynamic_cast<BoneData*>(GetParentCATNodeControl(true));
					if (pBoneParent && pLimb->GetisLeg() && !pLimb->TestFlag(LIMBFLAG_SETUP_FK))// && tmSetup.IsIdentity())
					{
						//if(pBoneParent->GetBendRatio() > 0.0f)
						tmSetup.PreRotateX(1.0f);
						//else tmSetup.PreRotateX(-1.0f);
					}
					if (GetBoneID() == 0 && !pLimb->GetCollarbone())
						tmSetup.SetTrans(tmSetup.GetTrans() * RotateYMatrix(PI));
					else tmSetup.NoTrans();
				}
				SetSetupMatrix(tmSetup);
			}
			break;
			case idNumBoneSegs:
				load->ToParamBlock(pblock, PB_NUMSEGS);
				break;
			case idSetupSwivel: {
				float dSwivel;
				load->GetValue(dSwivel);
				Matrix3 tmSetup = GetSetupMatrix();
				tmSetup.PreRotateZ(dSwivel);
				SetSetupMatrix(tmSetup);
				break;
			}
			case idWidth:			load->ToParamBlock(pblock, PB_BONEWIDTH);		break;
			case idLength:			load->ToParamBlock(pblock, PB_BONELENGTH);		break;
			case idHeight:			load->ToParamBlock(pblock, PB_BONEDEPTH);		break;

			case idWeightOutTan:
			case idWeightInTan:
			case idWeightOutVal:
			case idWeightInVal:
			{
				Control *ctrlTwistWeight = pblock->GetControllerByID(PB_TWISTWEIGHT);
				if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
				{
					IParamBlock2 *twistBlock = ctrlTwistWeight->GetParamBlock(0);
					switch (load->CurIdentifier()) {
					case idWeightOutTan: load->ToParamBlock(twistBlock, CATWeight::PB_KEY1OUTTANLEN); break;
					case idWeightInTan:  load->ToParamBlock(twistBlock, CATWeight::PB_KEY2INTANLEN); break;
					case idWeightOutVal: load->ToParamBlock(twistBlock, CATWeight::PB_KEY1VAL); break;
					case idWeightInVal:  load->ToParamBlock(twistBlock, CATWeight::PB_KEY2VAL); break;
					}
				}
				else
					load->SkipNextTokenOrValue();
				break;
			}
			case idParentNode: {
				TSTR parent_address;
				load->GetValue(parent_address);
				load->AddParent(GetNode(), parent_address);
				break;
			}
			default:
				load->AssertOutOfPlace();
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

TSTR	BoneData::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetBoneID());
	LimbData2* pLimb = GetLimbData();
	TSTR boneAddress = _T("<<unknown>>");
	if (pLimb)
		boneAddress = pLimb->GetBoneAddress();
	return (boneAddress + _T(".") + bonerigname + boneid);
};

BOOL BoneData::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idLimbBone);
	save->Write(idBoneName, GetName());
	save->Write(idFlags, ccflags);

	if (GetLayerTrans()) GetLayerTrans()->SaveRig(save);

	save->FromParamBlock(pblock, idNumBoneSegs, PB_NUMSEGS);
	save->FromParamBlock(pblock, idWidth, PB_BONEWIDTH);
	save->FromParamBlock(pblock, idLength, PB_BONELENGTH);
	save->FromParamBlock(pblock, idHeight, PB_BONEDEPTH);

	Control *ctrlTwistWeight = pblock->GetControllerByID(PB_TWISTWEIGHT);
	if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
		((CATWeight*)ctrlTwistWeight)->SaveRig(save);

	INode* parentnode = GetParentNode();
	save->Write(idParentNode, parentnode);

	for (int i = 0; i < GetNumBones(); i++) {
		GetBone(i)->SaveRig(save);
	}

	save->EndGroup();
	return TRUE;
}

BOOL BoneData::LoadClip(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	int segNumber = 0;
	int newflags;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idHubParams) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idController:
				newflags = flags;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)) newflags |= CLIPFLAG_SKIPPOS;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)) newflags |= CLIPFLAG_SKIPROT;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) newflags |= CLIPFLAG_SKIPSCL;

				if (mLayerTrans) mLayerTrans->LoadClip(load, timerange, layerindex, dScale, newflags);
				break;
			case idBoneSeg:
				if (segNumber < GetNumBones()) {
					GetBone(segNumber)->LoadClip(load, timerange, layerindex, dScale, flags);
					segNumber++;
				}
				else {
					load->SkipGroup();
				}
				break;
			case idArbBones:
				LoadClipArbBones(load, timerange, layerindex, dScale, flags);
				break;
			case idExtraControllers:
				LoadClipExtraControllers(load, timerange, layerindex, dScale, flags);
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idValMatrix3:
			{
				Matrix3 val;
				load->GetValue(val);

				// Mirror first then apply transforms
				if (flags&CLIPFLAG_MIRROR) {

					val = val * Inverse(load->GettmFilePathNodeGuess());

					Point3 p3Pos = val.GetTrans();
					Quat qt(val);
					qt.y *= -1.0f;
					qt.z *= -1.0f;
					val.SetRotate(qt);
					val.SetTrans(p3Pos * Point3(-1.0f, 1.0f, 1.0f));

					// put it back where you found it
					val = val * load->GettmFilePathNodeGuess();
				}
				// Scale data first.
				if (flags&CLIPFLAG_SCALE_DATA) val.SetTrans(val.GetTrans() * load->GetScale());

				// apply the offset, it has already been scaled in CATParent::LoadClip
				if (flags&CLIPFLAG_APPLYTRANSFORMS)
					val = val * load->GettmTransform();

				SetXFormPacket XFormSet(val);
				SetValue(timerange.Start(), &XFormSet, 1, CTRL_ABSOLUTE);
				break;
			}
			case idSubNum:
			{
				int subNum;
				load->GetValue(subNum);

				load->NextClause();
				if (load->CurIdentifier() == idController)
				{
					int numSubs = NumSubs();
					if (subNum <= numSubs)
					{
						if (SubAnim(subNum)) ((CATClipValue*)SubAnim(subNum))->LoadClip(load, timerange, layerindex, dScale, flags);
						else load->SkipGroup();
					}
					else
					{
						subNum -= numSubs;
						ParamID pid = (ParamID)subNum;
						CATClipValue *ctrlClipValue = (CATClipValue*)pblock->GetControllerByID(pid);
						if (ctrlClipValue) ctrlClipValue->LoadClip(load, timerange, layerindex, dScale, flags);
						else load->SkipGroup();
					}
				}
			}
			break;
			default:
				load->AssertOutOfPlace();
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

BOOL BoneData::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(idLimbBone);

	save->Comment(GetNode()->GetName());

	save->BeginGroup(idController);
	mLayerTrans->SaveClip(save, flags, timerange, layerindex);
	save->EndGroup();

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	for (int i = 0; i < GetNumBones(); i++)
		if (GetBone(i))	GetBone(i)->SaveClip(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

void BoneData::UpdateNumSegBones(BOOL loading/*=FALSE*/)
{
	if (theHold.RestoreOrRedoing()) return;

	int newNumSegs = pblock->GetInt(PB_NUMSEGS);
	int iNumSegs = GetNumBones();

	if (newNumSegs < 1)
	{
		newNumSegs = 1;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(PB_NUMSEGS, 0, 1);
		pblock->EnableNotifications(TRUE);
	}

	if (newNumSegs == iNumSegs)
		return;

	// Create the new segments
	if (iNumSegs > newNumSegs)
		DeleteSegBones(newNumSegs, iNumSegs);
	else
		CreateSegBones(newNumSegs, iNumSegs, loading);

	if (!loading) {
		CATMessage(0, CAT_CATUNITS_CHANGED);
		CATMessage(0, CAT_NAMECHANGE);
		CATMessage(0, CAT_COLOUR_CHANGED);
	}
};

void BoneData::DeleteSegBones(int newNumSegs, int iNumSegs)
{
	DbgAssert(newNumSegs > 0);
	if (newNumSegs < 1)
		newNumSegs = 1;

	DbgAssert(newNumSegs < iNumSegs);

	BlockEvaluation block(this);

	// delete excess,
	Interface* ip = GetCOREInterface();
	int SelCount = ip->GetSelNodeCount();

	INode* pBone0Node = NULL;
	if (GetBone(0) != NULL)
		pBone0Node = GetBone(0)->GetNode();

	for (int j = (iNumSegs - 1); j >= newNumSegs; j--)
	{
		CATNodeControl* pBone = GetBone(j);
		if (pBone != NULL)
		{
			INode* nodeBoneSeg = pBone->GetNode();
			if (nodeBoneSeg)
			{
				if ((j != 0) && (SelCount == 1) && (nodeBoneSeg == ip->GetSelNode(0))) // we must be being called from the modifier panel
				{
					//	The node being deleted is the one that was displaying the ui
					bNumSegmentsChanging = TRUE;
					ip->SelectNode(pBone0Node);
					ip->DeSelectNode(nodeBoneSeg);
					bNumSegmentsChanging = FALSE;
				}
			}
			// Delete the Seg Node, and any extra bones
			pBone->DeleteBoneHierarchy();
		}
		// as segments are removed, they will re-assign the child to the
		// next remaining segment. We need to update as we go so that the
		// next remaining segment can be found correctly (otherwise the child
		// can be detached)
		pblock->SetCount(PB_SEG_TAB, j);
	}
}

void BoneData::CreateSegBones(int newNumSegs, int iNumSegs, BOOL bLoading)
{
	BlockEvaluation block(this);

	// Resize the array BEFORE we start putting things into it
	pblock->SetCount(PB_SEG_TAB, newNumSegs);

	BoneSegTrans *ctrlSegTrans = NULL;
	Interface* ip = GetCOREInterface();

	for (int j = iNumSegs; j < newNumSegs; j++)
	{

		ctrlSegTrans = (BoneSegTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, BONESEGTRANS_CLASS_ID);
		ctrlSegTrans->Initialise(this, j, bLoading);

		// Put it into our parameter block
		pblock->SetValue(PB_SEG_TAB, 0, ctrlSegTrans, j);
	}
}

/*
 *	Set up the BoneSegments to all be the right size
 */
void BoneData::UpdateCATUnits()
{
	// Disable Macrorecorder for this bit
	MacroRecorder::MacroRecorderDisable disableGuard;

	float bonelength = pblock->GetFloat(PB_BONELENGTH);
	int iNumSegs = GetNumBones();
	float dSegLength = bonelength / iNumSegs;

	float dBoneWidth = pblock->GetFloat(PB_BONEWIDTH);
	float dBoneDepth = pblock->GetFloat(PB_BONEDEPTH);

	float dCATUnits = GetCATUnits();

	for (int i = 0; i < GetNumBones(); i++)
	{
		CATNodeControl* pSeg = GetBone(i);
		if (pSeg != NULL)
		{
			// Access the obj directly, as calling
			// SetObjX on our BoneSegTrans loops back to our
			// SetObjX functions, which ends up calling UpdateCATUnits (loop)
			ICATObject* pSegObj = pSeg->GetICATObject();
			if (pSegObj == NULL)
				continue;

			pSegObj->SetZ(dSegLength);
			pSegObj->SetX(dBoneWidth);
			pSegObj->SetY(dBoneDepth);
			pSegObj->SetCATUnits(dCATUnits);

			pSeg->UpdateObjDim();
		}
	}
	UpdateObjDim();
}

void BoneData::UpdateObjDim()
{
	if (GetLengthAxis() == Z)
	{
		obj_dim.x = pblock->GetFloat(PB_BONEWIDTH);	obj_dim.y = pblock->GetFloat(PB_BONEDEPTH); obj_dim.z = pblock->GetFloat(PB_BONELENGTH);
	}
	else { obj_dim.x = pblock->GetFloat(PB_BONELENGTH);	obj_dim.y = pblock->GetFloat(PB_BONEDEPTH); obj_dim.z = pblock->GetFloat(PB_BONEWIDTH); };
	obj_dim *= GetCATUnits();
}

//////////////////////////////////////////////////////////////////////////
// CATNodecontrol fn's

CatAPI::IBoneGroupManager* BoneData::GetManager()
{
	return GetLimbData();
}

CATNodeControl* BoneData::FindParentCATNodeControl()
{
	LimbData2* pLimb = GetLimbData();
	DbgAssert(GetLimbData() != NULL);
	if (pLimb == NULL)
		return NULL;

	// If we are not the first bone, try immediately above us
	CATNodeControl* pParent = NULL;
	int iBoneID = GetBoneID();
	if (iBoneID > 0)
		pParent = pLimb->GetBoneData(iBoneID - 1);
	else
	{
		pParent = pLimb->GetCollarbone();

		if (pParent == NULL)
			pParent = pLimb->GetHub();
	}

	DbgAssert(pParent != NULL);
	return pParent;
}

void BoneData::UpdateObject() {
	ICATObject *iobj;
	for (int i = 0; i < GetNumBones(); i++)
	{
		GetBone(i)->GetNode()->InvalidateWS();
		iobj = GetBone(i)->GetICATObject();
		if (iobj) iobj->Update();
	}
}

// rips through all the segments and collpsese the object modifier stack
// using our super cool collapser
void BoneData::CollapseBone()
{
	ICATObject *iobj;
	for (int i = 0; i < GetNumBones(); i++)
	{
		CATNodeControl* pBone = GetBone(i);
		if (pBone != NULL)
		{
			INode* segnode = pBone->GetNode();
			iobj = pBone->GetICATObject();
			if (iobj) iobj->CopyMeshFromNode(segnode);
		}
	}
}

int	BoneData::NumChildCATNodeControls() {
	int iBoneID = GetBoneID();
	LimbData2* pLimb = GetLimbData();
	if (pLimb &&
		(iBoneID < (pLimb->GetNumBones() - 1) ||
		(iBoneID == (pLimb->GetNumBones() - 1) && pLimb->GetPalmTrans())))
		return 1;
	return 0;
};

CATNodeControl*	BoneData::GetChildCATNodeControl(int i)
{
	UNUSED_PARAM(i);
	DbgAssert(i == 0);

	LimbData2* pLimb = GetLimbData();
	if (!pLimb)
		return NULL;

	int iBoneID = GetBoneID();
	if (iBoneID < (pLimb->GetNumBones() - 1)) {
		return pLimb->GetBoneData(iBoneID + 1);
	}
	if (iBoneID == (pLimb->GetNumBones() - 1) && pLimb->GetPalmTrans()) {
		return pLimb->GetPalmTrans();
	}
	return NULL;
};

void BoneData::SetTransformLock(int type, BOOL onOff)
{
	for (int i = 0; i < GetNumBones(); i++) {
		CATNodeControl* pBone = GetBone(i);
		if (pBone != NULL)
		{
			INode* pBoneNode = pBone->GetNode();
			if (pBoneNode) {
				pBoneNode->SetTransformLock(type, INODE_LOCK_X, onOff);
				pBoneNode->SetTransformLock(type, INODE_LOCK_Y, onOff);
				pBoneNode->SetTransformLock(type, INODE_LOCK_Z, onOff);
			}
		}
	}
}

void BoneData::DeleteBone()
{
	// Access to max's core
	Interface *ip = GetCOREInterface();
	int SelCount = ip->GetSelNodeCount();

	// Delete the Nodes
	INodeTab nodes;
	AddSystemNodes(nodes, kSNCDelete);

	LimbData2* pLimb = GetLimbData();
	DbgAssert(pLimb != NULL);

	for (int i = (nodes.Count() - 1); i >= 0; i--) {
		if (nodes[i]) {
			// we must be being called from the modifier panel
			if ((SelCount == 1) && (nodes[i] == ip->GetSelNode(0))) {
				//	The node being deleted is the one that was displaying the ui
				ip->DeSelectNode(nodes[i]);
				if (GetBoneID() > 0)
					ip->SelectNode(pLimb->GetBoneData(0)->GetNode());
			}
			nodes[i]->Delete(0, FALSE);
		}
	}
	return;
}

// This initialize function just sets up the CATMotion Hierarchy and the ClipHierarchy
void BoneData::Initialise(LimbData2* pLimb, int id, BOOL loading)
{
	DbgAssert(pLimb != NULL);

	SetLimbData(pLimb);
	SetBoneID(id);
	int iBoneID = GetBoneID();

	// To support old files, we maintain the pblock ID
	pblock->SetValue(PB_BONEID_DEPRECATED, 0, iBoneID);

	CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), (CATClipWeights*)GetLimbData()->GetWeights(), GetCATParentTrans(), FALSE);

	// this means that the pLimb will hang when in setup pose
	if (iBoneID == 0) {
		layerController->SetFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT);
		layerController->SetFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
		layerController->SetFlag(CLIP_FLAG_SETUP_FLIPROT_FROM_CATPARENT);
	}

	Control* ctrlTistWeight = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATWEIGHT_CLASS_ID);
	pblock->SetControllerByID(PB_TWISTWEIGHT, 0, ctrlTistWeight, FALSE);

	ReplaceReference(LAYERTRANS, layerController);

	if (id == 0) {
		if (pLimb->GetCollarbone())
			SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
		else ClearCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
	}
	else SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);

	// we need at least one bone segment to stop the
	// autodeletion system cleaning us up.
	int defaultBoneSegs = 1;
	pblock->SetValue(PB_NUMSEGS, 0, defaultBoneSegs);
	UpdateNumSegBones();

	if (!loading)
	{
		pblock->EnableNotifications(FALSE);
		TSTR bonename;
		bonename.printf(_T("%d"), id + 1);
		SetName(bonename);

		int lengthaxis = GetLengthAxis();

		if (GetBoneID() == 0)
		{
			if (pLimb->GetCollarbone()) {
				Matrix3 tmSetup = GetSetupMatrix();
				tmSetup.NoTrans();
				SetSetupMatrix(tmSetup);
			}
			else {
				// init the root bone position
				Object* hubobj = nullptr;
				if (pLimb->GetHub())
					hubobj = pLimb->GetHub()->GetObject();
				if (hubobj) {
					ICATObject* iobj = (ICATObject*)hubobj->GetInterface(I_CATOBJECT);
					if (iobj) {
						Point3 p3RootPos;
						if (lengthaxis == Z) {
							p3RootPos = Point3(iobj->GetX() / 2.0f, 0.0f, 0.0f);
							p3RootPos.x *= -pLimb->GetLMR();
						}
						else {
							p3RootPos = Point3(0.0f, 0.0f, iobj->GetX() / 2.0f);
							p3RootPos.z *= pLimb->GetLMR();
						}
						Matrix3 tmSetup = GetSetupMatrix();
						tmSetup.SetTrans(p3RootPos);
						SetSetupMatrix(tmSetup);

					}
				}
			}
		}
		// This is just to make sure arms bend with the elbows pointing behind the person
		Matrix3 tmSetup = GetSetupMatrix();
		if (pLimb->GetisArm()) {
			// arm bones should default to being smaller
			pblock->SetValue(PB_BONEWIDTH, 0, 9.0f);
			pblock->SetValue(PB_BONEDEPTH, 0, 9.0f);
			pblock->SetValue(PB_BONELENGTH, 0, 30.0f);

			if (lengthaxis == Z) {
				if (iBoneID == 0)
					tmSetup.PreRotateX(0.5f);
				else tmSetup.PreRotateX(-1.0f);
			}
			else {
				if (iBoneID == 0)
					tmSetup.PreRotateZ(-0.5f);
				else tmSetup.PreRotateZ(1.0f);
			}
			SetSetupMatrix(tmSetup);
		}
		else {
			// arb bones should default to being smaller
			pblock->SetValue(PB_BONEWIDTH, 0, 12.0f);
			pblock->SetValue(PB_BONEDEPTH, 0, 12.0f);

			// Calculate a length of rht eleg so that it can touch the ground
			int numbones = pLimb->GetNumBones();
			ICATParentTrans *catparent = GetCATParentTrans();
			Hub* pHub = pLimb->GetHub();
			float hubheight = 1.;
			if (catparent && pHub)
				hubheight = Length((pHub->GetNodeTM(0) * Inverse(catparent->GettmCATParent(GetCOREInterface()->GetTime()))).GetTrans()) / GetCATUnits();
			pblock->SetValue(PB_BONELENGTH, 0, hubheight / (float)numbones);

			// Configure the default bend angles for  the new pLimb bones
			if (lengthaxis == Z) {
				if ((iBoneID == 0 && pHub && !pHub->GetInSpine()) || (iBoneID > 0 && pHub && pHub->GetInSpine()))
					tmSetup.PreRotateX(-0.5f);
				else tmSetup.PreRotateX(0.5f);
			}
			else {
				if ((iBoneID == 0 && pHub && !pHub->GetInSpine()) || (iBoneID > 0 && pHub && pHub->GetInSpine()))
					tmSetup.PreRotateZ(0.5f);
				else tmSetup.PreRotateZ(-0.5f);
			}
			SetSetupMatrix(tmSetup);

		}

		// pLimb bones are stretchy
		ccflags |= CCFLAG_SETUP_STRETCHY;

		pblock->EnableNotifications(TRUE);

		UpdateCATUnits();
	}
}

BOOL BoneData::IsThisBoneSelected()
{
	BOOL isSelected = FALSE;
	for (int i = 0; (i < GetNumBones()) && !isSelected; i++)
	{
		CATNodeControl* pBone = GetBone(i);
		if (pBone != NULL)
			isSelected = pBone->IsThisBoneSelected();
	}
	return isSelected;
}

float BoneData::GetTwistWeight(float SegRatio) {
	return pblock->GetFloat(PB_TWISTWEIGHT, (int)(SegRatio * STEPTIME100));
};

float BoneData::GetTwistAngle(int nSegID, Interval& ivValid)
{
	ivValid &= mTwistValid;

	float dSegRatio = float(nSegID) / GetNumBones();
	float dTwistWeight = GetTwistWeight(dSegRatio);
	// Old CAT twist was limited from 0 to 1.0f
	// This has been changed to be from -1 to 1,
	// to reflect the ability to include twist from the parent bone
	float dInitWeight = GetTwistWeight(0);
	float dFinalWeight = GetTwistWeight(1.0f);
	dTwistWeight = (dTwistWeight - dInitWeight) / (dFinalWeight - dInitWeight);

	float dAngleFromParent = (dInitWeight < 0) ? mdParentTwistAngle * dInitWeight : 0.0f;
	float dAngleToChild = (dFinalWeight > 0) ? mdChildTwistAngle * dFinalWeight : 0.0f;
	return BlendFloat(dAngleFromParent, dAngleToChild, dTwistWeight);
}

void BoneData::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	if (bNumSegmentsChanging) return;

	flagsbegin = flags;
	ipbegin = ip;

	LimbData2* pLimb = GetLimbData();
	if (pLimb)
		pLimb->BeginEditParams(ip, flags, prev);

	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO /* ||flagsbegin&BEGIN_EDIT_HIERARCHY*/) {
		// Let the CATNode manage the UIs for motino and hierarchy panel
		CATNodeControl::BeginEditParams(ip, flags, prev);
	}
	else if (flagsbegin == 0)
	{
		pblock->EnableNotifications(FALSE);// this is to stop the name edit box sending a message when it is initilaised
		BoneDataDesc.BeginEditParams(ip, this, flags, prev);
		Control *ctrlTwistWeight = pblock->GetControllerByID(PB_TWISTWEIGHT);
		if (ctrlTwistWeight) {
			((CATWeight*)ctrlTwistWeight)->DisplayRollout(ip, flags, prev, GetString(IDS_BONE_TWIST_WGT));
		}
		pblock->EnableNotifications(TRUE);
	}
}

void BoneData::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (bNumSegmentsChanging) return;

	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO /*|| flagsbegin&BEGIN_EDIT_HIERARCHY*/) {
		// Let the CATNode manage the UIs for motion and hierarchy panel
		CATNodeControl::EndEditParams(ip, flags, next);
	}
	else if (flagsbegin == 0) {
		BoneDataDesc.EndEditParams(ip, this, END_EDIT_REMOVEUI, NULL);//next);

		Control* ctrlTwistWeight = pblock->GetControllerByID(PB_TWISTWEIGHT);
		if (ctrlTwistWeight) ctrlTwistWeight->EndEditParams(ip, END_EDIT_REMOVEUI); // next);
	}

	LimbData2* pLimb = GetLimbData();
	if (pLimb)
		pLimb->EndEditParams(ip, flags, next); // next);

	ipbegin = NULL;
}
//////////////////////////////////////////////////////////////////////////
// CATNodeControl

CATGroup* BoneData::GetGroup()
{
	LimbData2* pLimbData = GetLimbData();
	if (pLimbData != NULL)
		return pLimbData->GetGroup();
	return NULL;
}

CATControl* BoneData::GetParentCATControl()
{
	return GetLimbData();
}

int BoneData::NumChildCATControls()
{
	return GetNumBones();
}

CATControl* BoneData::GetChildCATControl(int i)
{
	if (i < GetNumBones())	return (CATControl*)GetBone(i);
	i -= GetNumBones();
	return NULL;
}

int BoneData::NumLayerControllers()
{
	return 1 + NumLayerFloats();
}

CATClipValue* BoneData::GetLayerController(int i)
{
	return (i == 0 ? GetLayerTrans() : GetLayerFloat(i - 1));
}

BOOL BoneData::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	BoneData* pasteBone = (BoneData*)pastectrl;

	pblock->EnableNotifications(FALSE);
	pblock->SetValue(PB_BONEWIDTH, 0, pasteBone->pblock->GetFloat(PB_BONEWIDTH) * scalefactor);
	pblock->SetValue(PB_BONEDEPTH, 0, pasteBone->pblock->GetFloat(PB_BONEDEPTH) * scalefactor);
	pblock->SetValue(PB_BONELENGTH, 0, pasteBone->pblock->GetFloat(PB_BONELENGTH) * scalefactor);

	pblock->SetValue(PB_NUMSEGS, 0, pasteBone->pblock->GetInt(PB_NUMSEGS));

	CATWeight* twistWeight = (CATWeight*)pblock->GetControllerByID(PB_TWISTWEIGHT);
	CATWeight* pasteTwistWeight = (CATWeight*)pasteBone->pblock->GetControllerByID(PB_TWISTWEIGHT);
	twistWeight->PasteRig(pasteTwistWeight);

	pblock->EnableNotifications(TRUE);

	UpdateNumSegBones();
	UpdateCATUnits();

	// propogate to any ARBBones
	CATNodeControl::PasteRig(pastectrl, flags, scalefactor);

	return TRUE;
}

void BoneData::SetLengthAxis(int axis) {

	Matrix3 tmSetup = GetSetupMatrix();
	if (axis == X)	tmSetup = (RotateYMatrix(-HALFPI) * tmSetup) * RotateYMatrix(HALFPI);
	else			tmSetup = (RotateYMatrix(HALFPI)  * tmSetup) * RotateYMatrix(-HALFPI);
	SetSetupMatrix(tmSetup);

	for (int i = 0; i < GetNumBones(); i++) {
		ICATObject *iobj = GetBone(i)->GetICATObject();
		if (iobj) iobj->SetLengthAxis(axis);
	}

	CATControl::SetLengthAxis(axis);
};

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class BoneDataPLCB : public PostLoadCallback {
protected:
	BoneData *bone;

public:
	BoneDataPLCB(BoneData *pOwner) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		if (bone->GetFileSaveVersion() > 0) return bone->GetFileSaveVersion();

		ICATParentTrans *catparent = bone->GetCATParentTrans();
		if (catparent) return catparent->GetFileSaveVersion();
		// old method of getting to the catparent
		LimbData2* pLimb = bone->GetLimbData();
		assert(pLimb);
		catparent = pLimb->GetCATParentTrans();
		DbgAssert(catparent);
		return catparent->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *) {

		if (!bone->GetLimbData()) {
			bone->MaybeAutoDelete();
			delete this;
			return;
		}

		if (GetFileSaveVersion() < CAT_VERSION_2000) {

			int boneid = bone->GetBoneID();
			if (bone->name.Length() == 0 && bone->GetLimbData()) {
				TSTR bone_num_str;
				bone_num_str.printf(_T("%d"), (boneid + 1));
				DisableRefMsgs();
				bone->pblock->SetValue(BoneData::PB_NAME, 0, bone_num_str);
				EnableRefMsgs();
			}
		}

		if (GetFileSaveVersion() < CAT_VERSION_2420) {
			// any ArbBones that have been added to the BoneData controller
			// need to be moved off onto the  1st bone segment
			if (bone->tabArbBones.Count() > 0) {
				CATNodeControl *seg1 = bone->GetBone(0);
				DbgAssert(seg1);
				for (int i = 0; i < bone->tabArbBones.Count(); i++) {
					seg1->InsertArbBone(seg1->GetNumArbBones(), bone->GetArbBone(i));
					//	((ArbBoneTrans*)bone->GetArbBone(i))->SetOwner(seg1);
				}
				bone->tabArbBones.SetCount(0);
			}
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

/**********************************************************************
 * Loading and saving....
 */
#define BONEDATACHUNK_CATNODECONTROL		0
IOResult BoneData::Save(ISave *isave)
{
	isave->BeginChunk(BONEDATACHUNK_CATNODECONTROL);
	CATNodeControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult BoneData::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case BONEDATACHUNK_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new BoneDataPLCB(this));

	return IO_OK;
}

int BoneData::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (GetBoneID() == 0)
	{
		GetLimbData()->Display(t, inode, vpt, flags);
	}
	return CATNodeControl::Display(t, inode, vpt, flags);
}

CATNodeControl* BoneData::GetBone(int id)
{
	if (0 <= id && id < GetNumBones())
		return (CATNodeControl*)pblock->GetReferenceTarget(PB_SEG_TAB, 0, id);
	return NULL;
}

CatAPI::INodeControl* BoneData::GetBoneINodeControl(int id)
{
	return GetBone(id);
}

