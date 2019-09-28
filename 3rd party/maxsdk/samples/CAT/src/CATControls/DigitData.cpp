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

// Storage Class for Digitgy stuff

#include "CatPlugins.h"
#include "iparamm2.h"

#include "ICATParent.h"
#include "CATClipValue.h"

#include "DigitData.h"
#include "LimbData2.h"
#include "PalmTrans2.h"
#include "DigitSegTrans.h"

class DigitDataClassDesc :public CATControlClassDesc
{
public:
	DigitDataClassDesc()
	{
		AddInterface(IBoneGroupManagerFP::GetFnPubDesc());
	}

	CATControl *	DoCreate(BOOL loading = FALSE) { return new DigitData(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_DIGITDATA); }
	Class_ID		ClassID() { return DIGITDATA_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("DigitData"); }	// returns fixed parsable name (scripter-visible name)
};

// SimCity - Add in a second class desc for DigitData.  This is to support loading of old files
// Old files will reference this ClassDesc to create instances of DigitData (under the old SClassID)
// The actual class of DigitData was irrelevant from CATs POV, and its methods were never called.
class DigitDataClassDescLegacy : public DigitDataClassDesc {
public:
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// Previous versions called this class a float controller
};

// Exported function to return the DigitData class description
static DigitDataClassDesc DigitDataDesc;
ClassDesc2* GetDigitDataDesc() { return &DigitDataDesc; }

static DigitDataClassDescLegacy DigitDataDescLegacy;
ClassDesc2* GetDigitDataDescLegacy() { return &DigitDataDescLegacy; }

//////////////////////////////////////////////////////////////////////////

class DigitDataParamDlgCallBack : public  ParamMap2UserDlgProc {

	DigitData* digit;

	ICustButton *btnCopy;
	ICustButton *btnPaste;
	ICustButton *btnPasteMirrored;

public:

	DigitDataParamDlgCallBack() : digit(NULL) {
		btnCopy = NULL;
		btnPaste = NULL;
		btnPasteMirrored = NULL;
	}
	void InitControls(HWND hWnd)
	{
		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;

		// Copy button
		btnCopy = GetICustButton(GetDlgItem(hWnd, IDC_BTN_COPY));
		btnCopy->SetType(CBT_PUSH);
		btnCopy->SetButtonDownNotify(TRUE);
		btnCopy->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnCopyDigit"), GetString(IDS_TT_COPYDIGIT)));
		btnCopy->SetImage(hIcons, 5, 5, 5 + 25, 5 + 25, 24, 24);

		// Paste button
		btnPaste = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE));
		btnPaste->SetType(CBT_PUSH);
		btnPaste->SetButtonDownNotify(TRUE);
		btnPaste->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteDigit"), GetString(IDS_TT_PASTEDIGIT)));
		btnPaste->SetImage(hIcons, 3, 3, 3 + 25, 3 + 25, 24, 24);

		// btnPasteMirrored button
		btnPasteMirrored = GetICustButton(GetDlgItem(hWnd, IDC_BTN_PASTE_MIRRORED));
		btnPasteMirrored->SetType(CBT_PUSH);
		btnPasteMirrored->SetButtonDownNotify(TRUE);
		btnPasteMirrored->SetTooltip(TRUE, catCfg.Get(_T("ToolTips"), _T("btnPasteDigitMirrored"), GetString(IDS_TT_PASTEDIGITMIRROR)));
		btnPasteMirrored->SetImage(hIcons, 4, 4, 4 + 25, 4 + 25, 24, 24);

		if (digit->GetCATMode() != SETUPMODE) {
			btnCopy->Disable();
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}
		else if (!digit->CanPasteControl())
		{
			btnPaste->Disable();
			btnPasteMirrored->Disable();
		}

	}

	void ReleaseControls()
	{
		SAFE_RELEASE_BTN(btnCopy);
		SAFE_RELEASE_BTN(btnPaste);
		SAFE_RELEASE_BTN(btnPasteMirrored);
	}

	virtual INT_PTR DlgProc(TimeValue, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		switch (msg) {
		case WM_INITDIALOG:
			digit = (DigitData*)map->GetParamBlock()->GetOwner();
			if (!digit) return FALSE;
			InitControls(hWnd);
			break;

		case WM_DESTROY:
			ReleaseControls();
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_COPY:					CATControl::SetPasteControl(digit);																			break;
				case IDC_BTN_PASTE:					digit->PasteRig(CATControl::GetPasteControl(), PASTERIGFLAG_DONT_PASTE_ROOTPOS, 1.0f);						break;
				case IDC_BTN_PASTE_MIRRORED:		digit->PasteRig(CATControl::GetPasteControl(), PASTERIGFLAG_MIRROR | PASTERIGFLAG_DONT_PASTE_ROOTPOS, 1.0f);	break;
				}
				break;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	virtual void DeleteThis() { }
};

static DigitDataParamDlgCallBack DigitDataParamCallBack;

static ParamBlockDesc2 DigitData_t_param_blk(
	DigitData::PBLOCK_REF, _T("DigitData Params"), 0, &DigitDataDesc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, DigitData::PBLOCK_REF,
	IDD_DIGITDATA, IDS_DIGITPARAMS, 0, 0, &DigitDataParamCallBack,

	////////////////////////////////////////////////////////////////////////////////////////////
	DigitData::PB_DIGITID_DEPRECATED, _T("DigitID"), TYPE_INDEX, P_OBSOLETE, NULL,
		p_default, -1,
		p_end,
	DigitData::PB_DIGITCOLOUR, _T(""), TYPE_RGBA, 0, NULL,
		p_end,
	DigitData::PB_DIGITNAME, _T(""), TYPE_STRING, 0, NULL,
		p_default, _T("Digit"),
		p_ui, TYPE_EDITBOX, IDC_EDIT_DIGITNAME,
		p_end,
	DigitData::PB_DIGITWIDTH, _T("Width"), TYPE_FLOAT, 0, NULL,
		p_default, 1.5f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_WIDTH, IDC_SPIN_WIDTH, SPIN_AUTOSCALE, //0.01f,
		p_end,
	DigitData::PB_DIGITDEPTH, _T("Depth"), TYPE_FLOAT, 0, NULL,
		p_default, 1.5f,
		p_range, 0.0f, 1000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_DEPTH, IDC_SPIN_DEPTH, SPIN_AUTOSCALE, //0.01f,
		p_end,
	DigitData::PB_ROOTPOS, _T("RootPos"), TYPE_POINT3, 0, NULL,
		p_end,

	DigitData::PB_PALM, _T("Palm"), TYPE_REFTARG, 0, 0,
		p_end,
	DigitData::PB_NUMBONES, _T("NumBones"), TYPE_INT, 0, IDS_NUMSEGS,
		p_default, 0,
		p_range, 0, 20,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_NUMSEGS, IDC_SPIN_NUMSEGS, SPIN_AUTOSCALE,
		p_end,
	DigitData::PB_SEGTRANSTAB, _T("Bones"), TYPE_REFTARG_TAB, 0, P_NO_REF + P_VARIABLE_SIZE, 0,
		p_end,
	DigitData::PB_SYMDIGIT, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	DigitData::PB_CATWEIGHT, _T("CurlWeight"), TYPE_FLOAT, 0, NULL,
		p_default, 1.0f,
		p_range, -5.0f, 5.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EDIT_CATMOTIONWEIGHT, IDC_SPIN_CATMOTIONWEIGHT, SPIN_AUTOSCALE,
		p_end,

	// NOTE - when cloning this class the parameter block as a whole is NOT cloned.
	// This means that any parameters added to this class may need to be copied by hand
	p_end
);

DigitData::DigitData(BOOL /*loading*/)
{
	pblock = NULL;

	FFweight = 1.0f;
	BAweight = 0.0f;
	FKWeight = 0.0f;
	BendAngle = 0.0f;

	p3PalmScale = Point3(1.0f, 1.0f, 1.0f);

	DigitDataDesc.MakeAutoParamBlocks(this);
	pblock->ResetAll(TRUE, FALSE);
}

DigitData::~DigitData() {
	DeleteAllRefs();
}

SClass_ID DigitData::SuperClassID()
{
	return DigitDataDesc.SuperClassID();
}

RefTargetHandle DigitData::Clone(RemapDir& remap)
{
	// make a new DigitData object to be the clone
	DigitData *pNewDigit = new DigitData();

	remap.AddEntry(this, pNewDigit);

	// Don't clone the parameter block directly, because this will trigger a clone
	// of the palm and all.  Copy over the affected information by hand.
	Color digitColor = pblock->GetColor(PB_DIGITCOLOUR);
	Point3 digitPos = pblock->GetPoint3(PB_ROOTPOS);
	pNewDigit->pblock->SetValue(PB_ROOTPOS, 0, digitPos);
	pNewDigit->pblock->SetValue(PB_DIGITCOLOUR, 0, digitColor);
	pNewDigit->pblock->SetValue(PB_DIGITNAME, 0, pblock->GetStr(PB_DIGITNAME));
	pNewDigit->pblock->SetValue(PB_DIGITWIDTH, 0, pblock->GetFloat(PB_DIGITWIDTH));
	pNewDigit->pblock->SetValue(PB_DIGITDEPTH, 0, pblock->GetFloat(PB_DIGITDEPTH));
	pNewDigit->pblock->SetValue(PB_CATWEIGHT, 0, pblock->GetFloat(PB_CATWEIGHT));

	// Now set the palm pointer
	PalmTrans2* pNewPalm = static_cast<PalmTrans2*>(remap.FindMapping(GetPalm()));
	if (pNewPalm == NULL)
		pNewPalm = GetPalm();
	pNewDigit->SetPalm(pNewPalm);

	// A palm is a requirement.  If NULL we just bail here.
	if (pNewPalm == NULL)
		return pNewDigit;

	// NOTE: Do NOT set the digit on the palm.
	// While we are mid-clone, we CANNOT modify
	// existing classes.  theHold is suspended,
	// and any changes will not be undone in case
	// of a cancel/undo.
	int id = GetDigitID();
	while (pNewPalm->GetDigit(id) != NULL)
		id++;
	pNewDigit->SetDigitID(id);

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATControl(pNewDigit, remap);

	// Force the cloning of all out child CATControls
	pNewDigit->pblock->SetCount(PB_SEGTRANSTAB, GetNumBones());
	for (int i = 0; i < GetNumBones(); i++) {
		pNewDigit->pblock->SetValue(PB_SEGTRANSTAB, 0, remap.CloneRef(GetBone(i)), i);
		pNewDigit->GetBone(i)->SetDigit(pNewDigit);
	}
	// update our additional pbNumDigits tab
	pNewDigit->SetNumBones(GetNumBones());

	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, pNewDigit, remap);
	// This base clone method allows our parent

	// now return the new object.
	return pNewDigit;
}

static void DigitCloneNotify(void *param, NotifyInfo *info)
{
	DigitData *pDigit = (DigitData*)param;
	INode *node = (INode*)info->callParam;
	if (pDigit->GetBone(0)->GetNode() == node)
	{
		PalmTrans2 *pPalm = pDigit->GetPalm();
		if (pPalm->GetDigit(pDigit->GetDigitID()) != pDigit)
		{
			pDigit->SetDigitID(pPalm->GetNumDigits());
			pPalm->SetDigit(pDigit->GetDigitID(), pDigit);
		}
		UnRegisterNotification(DigitCloneNotify, pDigit, NOTIFY_NODE_CLONED);
	}
}

void DigitData::PostCloneManager()
{
	RegisterNotification(DigitCloneNotify, this, NOTIFY_NODE_CLONED);
}

// This initialise function just sets up the CATMotion Hierarchy and the ClipHierarchy
void DigitData::Initialise(PalmTrans2* palm, int id, BOOL loading)
{
	SetDigitID(id);
	pblock->SetValue(PB_PALM, 0, palm);

	if (!loading) {
		// if we already have digits, lets copy off them
		int numdigits = palm->GetNumDigits();
		if (numdigits > 0 && (!palm->GetLimb()->GetisLeg() && id > 1))
		{
			DigitData* prevDigit = palm->GetDigit(id - 1);
			Point3 rootpos = prevDigit->GetRootPos();
			PasteRig(prevDigit, 0, 1.0f);
			prevDigit->SetRootPos(rootpos);
		}
		else
		{	// fingers default to 3 bones
			// toes default to 2 bones
			if (palm->GetLimb()->GetisArm())
				pblock->SetValue(DigitData::PB_NUMBONES, 0, 3);
			else pblock->SetValue(DigitData::PB_NUMBONES, 0, 2);
		}

		TSTR DigitName = GetString(IDS_DIGIT);
		TSTR DigitID;
		DigitID.printf(_T("%d"), id + 1);
		pblock->SetValue(PB_DIGITNAME, 0, TSTR(DigitName + DigitID));
	}
}

RefResult DigitData::NotifyRefChanged(
	const Interval&,
	RefTargetHandle hTarget,
	PartID&,
	RefMessage message,
	BOOL)
{
	switch (message)
	{
	case REFMSG_CHANGE:
	{
		if (pblock == hTarget)
		{
			int tabIndex = 0;
			ParamID index = pblock->LastNotifyParamID(tabIndex);
			switch (index)
			{
			case PB_NUMBONES:
				CreateDigitBones();
				break;
			case PB_DIGITNAME:
				CATMessage(0, CAT_NAMECHANGE);
				break;
			case PB_ROOTPOS:
				break;
			case PB_DIGITWIDTH:
			case PB_DIGITDEPTH:
				CATMessage(GetCOREInterface()->GetTime(), CAT_CATUNITS_CHANGED);
				break;
			}
		}
		break;
	}
	}
	return REF_SUCCEED;
}

void DigitData::CATMessage(TimeValue t, UINT msg, int data) //, void* CATData)
{
	switch (msg)
	{
		// This message is not handled by CATControl normally
	case CAT_SET_LENGTH_AXIS:
		SetLengthAxis(data);
		break;
	}

	CATControl::CATMessage(t, msg, data);
}

void DigitData::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	if (TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) return;
	flagsbegin = flags;
	ipbegin = ip;
	DigitDataDesc.BeginEditParams(ip, this, flags, prev);
}

void DigitData::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) return;
	ipbegin = NULL;
	DigitDataDesc.EndEditParams(ip, this, flags, next);
}

void DigitData::UpdateUI() {
	if (!ipbegin) return;
	//	DigitDataDesc.InvalidateUI();
	IParamMap2* pmap = pblock->GetMap();
	if (pmap) pmap->Invalidate();
}

//////////////////////////////////////////////////////////////////////////

CATGroup* DigitData::GetGroup()
{
	PalmTrans2* pPalm = GetPalm();
	if (pPalm != NULL)
		return pPalm->GetGroup();
	return NULL;
}

CATControl* DigitData::GetParentCATControl()
{
	return GetPalm();
}

CATControl* DigitData::GetChildCATControl(int i)
{
	return GetBone(i);
}

TSTR	DigitData::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetDigitID());

	return (GetPalm()->GetBoneAddress() + _T(".") + bonerigname + boneid);
};

BOOL DigitData::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	int digitBoneNumber = 0;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idDigitParams) &&
				(load->CurGroupID() != idDigit))return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idDigitSegParams:
			case idDigitSeg:
				if (digitBoneNumber >= GetNumBones()) {
					int numBones = digitBoneNumber + 1;
					pblock->SetValue(PB_NUMBONES, 0, numBones);
				}
				GetBone(digitBoneNumber)->LoadRig(load);
				digitBoneNumber++;
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier()) {
			case idSetupPos:		load->ToParamBlock(pblock, PB_ROOTPOS);				break;
			case idBoneName:		load->ToParamBlock(pblock, PB_DIGITNAME);			break;
			case idFlags:			load->GetValue(ccflags);											break;
			case idCATMotionWeight:	load->ToParamBlock(pblock, PB_CATWEIGHT);			break;
			case idNumBones:		load->ToParamBlock(pblock, PB_NUMBONES);			break;
			case idWidth:			load->ToParamBlock(pblock, PB_DIGITWIDTH);			break;
			case idHeight:			load->ToParamBlock(pblock, PB_DIGITDEPTH);			break;
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

	if (ok) CATMessage(GetCOREInterface()->GetTime(), CAT_CATUNITS_CHANGED);

	return ok && load->ok();
}

BOOL DigitData::SaveRig(CATRigWriter *save)
{
	int numDigitBones = pblock->GetInt(PB_NUMBONES);
	save->BeginGroup(idDigit);
	save->FromParamBlock(pblock, idBoneName, PB_DIGITNAME);
	save->FromParamBlock(pblock, idSetupPos, PB_ROOTPOS);
	save->Write(idFlags, ccflags);

	save->FromParamBlock(pblock, idCATMotionWeight, PB_CATWEIGHT);
	save->FromParamBlock(pblock, idWidth, PB_DIGITWIDTH);
	save->FromParamBlock(pblock, idHeight, PB_DIGITDEPTH);
	save->FromParamBlock(pblock, idNumBones, PB_NUMBONES);

	for (int i = 0; i < numDigitBones; i++)
	{
		if (GetBone(i))
			GetBone(i)->SaveRig(save);
	}
	save->EndGroup();
	return TRUE;
}

BOOL DigitData::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	int digitBoneNumber = 0;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idDigitParams) &&
				(load->CurGroupID() != idDigit)) {
				ok = FALSE;
				continue;
			}
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idDigitSegParams:
			case idDigitSeg:
				if (digitBoneNumber < GetNumBones()) {
					DigitSegTrans *ctrlBoneTrans = GetBone(digitBoneNumber);
					DbgAssert(ctrlBoneTrans);
					ctrlBoneTrans->LoadClip(load, range, layerindex, dScale, flags);
					digitBoneNumber++;
				}
				else {
					load->SkipGroup();
				}
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
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

BOOL DigitData::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	int numDigitBones = pblock->GetInt(PB_NUMBONES);
	save->BeginGroup(idDigit);

	for (int i = 0; i < numDigitBones; i++)
	{
		if (GetBone(i))
			GetBone(i)->SaveClip(save, flags, timerange, layerindex);
	}

	save->EndGroup();
	return TRUE;
}

PalmTrans2* DigitData::GetPalm()
{
	return (PalmTrans2*)pblock->GetReferenceTarget(PB_PALM);
}

void DigitData::SetPalm(PalmTrans2 *p)
{
	pblock->SetValue(PB_PALM, 0, p);
}

CatAPI::INodeControl* DigitData::GetBoneINodeControl(int BoneID)
{
	return GetBone(BoneID);
}

DigitSegTrans* DigitData::GetBone(int BoneID)
{
	return (BoneID >= 0 && BoneID < GetNumBones()) ? (DigitSegTrans*)pblock->GetReferenceTarget(PB_SEGTRANSTAB, 0, BoneID) : NULL;
}

// This is the Class ID of the generic DigitBone Obj (in script);

/////////////////////////////////////////////////////////////
// Create Destroy ect Digit segments
// -ST 26/03/2003
void BailCreateDigitBones(DigitData* digit) {
	int oldNumBones = digit->pblock->Count(DigitData::PB_SEGTRANSTAB);
	int newNumBones = digit->pblock->GetInt(DigitData::PB_NUMBONES);
	if (newNumBones != oldNumBones) {
		digit->pblock->EnableNotifications(FALSE);
		digit->pblock->SetValue(DigitData::PB_NUMBONES, 0, oldNumBones);
		digit->pblock->EnableNotifications(TRUE);
	}
}
void DigitData::CreateDigitBones()
{
	if (theHold.RestoreOrRedoing())
		return;

	//////////////////////////////////////////////////////////////////////////
	// Layers Stuff
	// Plug into the Clip Hierarchy
//	ICATClipPalm* pClipPalm = (ICATClipPalm*)palm->GetClipPalm();
//	DbgAssert(pClipPalm);

	// Access to max's core
	Interface *ip = GetCOREInterface();

	int oldNumBones = GetNumBones();
	int newNumBones = pblock->GetInt(PB_NUMBONES);
	if (newNumBones < 1)
	{
		newNumBones = 1;
		pblock->EnableNotifications(FALSE);
		pblock->SetValue(PB_NUMBONES, 0, 1);
		pblock->EnableNotifications(TRUE);
	}
	if (newNumBones == oldNumBones)
		return;

	// Cache the current length of the digit.
	float dOldDigitLength = GetDigitLength();
	if (dOldDigitLength <= 0)
		dOldDigitLength = CalcDefaultDigitLength();

	// Remove old bones
	if (newNumBones < oldNumBones)
	{
		int numSelected = ip->GetSelNodeCount();
		for (int j = (oldNumBones - 1); j >= newNumBones; j--)	// If newNumBones < oldNumBones, delete some
		{
			DigitSegTrans* digitbone = GetBone(j);
			if (digitbone)
			{
				INode *digitNode = digitbone->GetNode();
				if (digitNode)
				{
					if (numSelected == 1 && digitNode == ip->GetSelNode(0))
					{
						SetCCFlag(CNCFLAG_KEEP_ROLLOUTS);

						DigitSegTrans *nextDigitBone = GetBone(j - 1);
						if (nextDigitBone && nextDigitBone->GetNode())
							ip->SelectNode(nextDigitBone->GetNode(), 1);

						ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);
					}
					digitbone->DeleteBoneHierarchy();
					pblock->SetValue(PB_SEGTRANSTAB, 0, (ReferenceTarget*)NULL, j);
				}
			}
		}
	}

	pblock->Resize(PB_SEGTRANSTAB, newNumBones);
	pblock->SetCount(PB_SEGTRANSTAB, newNumBones);

	// Create new bones
	if (newNumBones > oldNumBones)
	{
		for (int j = oldNumBones; j < newNumBones; j++)
		{
			DigitSegTrans* digitbone = (DigitSegTrans*)CreateInstance(CTRL_MATRIX3_CLASS_ID, DIGITSEGTRANS_CLASS_ID);
			digitbone->Initialise(this, j);

			// Put it into our parameter block
			pblock->SetValue(PB_SEGTRANSTAB, 0, (ReferenceTarget*)digitbone, j);
		}
	}

	// Try to keep the same length
	if (dOldDigitLength > 0.0f)
		SetDigitLength(dOldDigitLength);

	CATMessage(0, CAT_CATUNITS_CHANGED);
	CATMessage(0, CAT_COLOUR_CHANGED);
	CATMessage(0, CAT_NAMECHANGE);
}

void DigitData::SetRootPos(Point3 val, GetSetMethod method)
{
	if (method == CTRL_RELATIVE)
	{
		Point3 p3rootPos = pblock->GetPoint3(PB_ROOTPOS);
		Point3 palmDimensions = GetPalm()->GetBoneDimensions();
		val = val / palmDimensions;

		val += p3rootPos;
	}

	pblock->SetValue(PB_ROOTPOS, 0, val);
}

void DigitData::PoseLikeMe(TimeValue t, int nCATMode, DigitData* digit)
{
	UNREFERENCED_PARAMETER(nCATMode);
	int numBones = min(GetNumBones(), digit->GetNumBones());
	Matrix3 tmBoneRot(1);
	for (int i = 0; i < numBones; i++) {
		GetBone(i)->GetRotation(t, &tmBoneRot);
		digit->GetBone(i)->SetRotation(t, tmBoneRot);
	}
}

float DigitData::GetDigitLength()
{
	float dDigitLength = 0.0f;
	for (int j = 0; j < GetNumBones(); j++)
	{
		// Get the Digit's length
		DigitSegTrans* digitbone = GetBone(j);
		dDigitLength += digitbone->GetBoneLength();
	}
	return dDigitLength;
}

void DigitData::SetDigitLength(float dNewDigitLength)
{
	int j, nNumBones = GetNumBones();
	Tab <float> tabBoneLengths;
	tabBoneLengths.SetCount(nNumBones);

	float sizeDiff = dNewDigitLength / GetDigitLength();
	for (j = 0; j < nNumBones; j++) {
		// Get the Digit's length
		DigitSegTrans* digitbone = GetBone(j);
		digitbone->SetBoneLength(GetBone(j)->GetBoneLength()*sizeDiff);
	}
}

float DigitData::CalcDefaultDigitLength()
{
	// By default, we chose the same length as another digit, or if
	// another digit does not exist, we'll go for 1/2 the length of
	// the palm
	PalmTrans2* pPalm = GetPalm();
	if (pPalm != NULL)
	{
		int iDigitID = GetDigitID();
		if (iDigitID > 0)
		{
			DigitData* pOtherDigit = pPalm->GetDigit(iDigitID - 1);
			if (pOtherDigit != NULL)
				return pOtherDigit->GetDigitLength();
		}

		// We still don't have a length (no other digit maybe)
		// We are the same length of the palm, or 1/3 the length
		// of an ankle
		float dDefaultDigitLength = pPalm->GetObjZ();
		ILimb* pLimb = pPalm->GetLimb();
		if (pLimb != NULL && pLimb->GetisLeg())
			dDefaultDigitLength *= 0.333f;
		return dDefaultDigitLength;
	}
	return 1.5f;
}

BOOL DigitData::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	DigitData* pastedigit = (DigitData*)pastectrl;

	int numBones = GetNumBones();
	int pasteBones = pastedigit->GetNumBones();
	//	Force the number of bones to be the same
	if (numBones != pasteBones)
		pblock->SetValue(PB_NUMBONES, 0, pasteBones);

	if (!(flags&PASTERIGFLAG_DONT_PASTE_ROOTPOS)) {
		Point3 rootpos = pastedigit->pblock->GetPoint3(PB_ROOTPOS);
		if (flags&PASTERIGFLAG_MIRROR) {
			if (GetLengthAxis() == Z)
				rootpos.x *= -1.0f;
			else rootpos.z *= -1.0f;
		}
		pblock->SetValue(PB_ROOTPOS, 0, rootpos);
		ccflags &= ~PASTERIGFLAG_DONT_PASTE_ROOTPOS;
	}

	SetDigitWidth(pastedigit->GetDigitWidth()* scalefactor);
	SetDigitDepth(pastedigit->GetDigitDepth()* scalefactor);
	//	UpdateCATUnits();

	CATControl::PasteRig(pastectrl, flags, scalefactor);

	return TRUE;
}

void DigitData::GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	switch (ctxt)
	{
	case kSNCDelete:
	{
		ICATParentTrans* pCatParent = GetCATParentTrans();
		if (pCatParent != NULL)
		{
			if (pCatParent->GetCATMode() == SETUPMODE)
			{
				//					if (theHold.Holding()&&!TestAFlag(A_HELD)) {
				//						theHold.Put(new DigitDeleteRestore(this, GetPalm(), ctxt));
				//						SetAFlag(A_HELD);
				//					}
				AddSystemNodes(nodes, ctxt);
			}
			else
				pCatParent->AddSystemNodes(nodes, ctxt);
		}
		break;
	}
	default:
		CATControl::GetSystemNodes(nodes, ctxt);
	}
}

void DigitData::SetLengthAxis(int axis) {

	Point3 rootpos = GetRootPos();
	if (axis == X) {
		float x = rootpos.x;
		rootpos.x = rootpos.z;
		rootpos.z = -x;
	}
	else {
		float z = rootpos.z;
		rootpos.z = rootpos.x;
		rootpos.x = -z;
	}
	SetRootPos(rootpos);

	CATControl::SetLengthAxis(axis);
};

/**********************************************************************
 * Loading and saving....
 */

#define		DIGITDATACHUNK_CATCTRLCHUNK		1

IOResult DigitData::Save(ISave *isave)
{
	//	DWORD nb;//, refID;
	//	ULONG id;
	IOResult res = IO_OK;

	isave->BeginChunk(DIGITDATACHUNK_CATCTRLCHUNK);
	res = CATControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult DigitData::Load(ILoad *iload)
{
	IOResult res = IO_OK;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case DIGITDATACHUNK_CATCTRLCHUNK:
			res = CATControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	iload->RegisterPostLoadCallback(new BoneGroupPostLoadIDPatcher(this, PB_DIGITID_DEPRECATED));
	return IO_OK;
}

void DigitData::DestructAllCATMotionLayers()
{
	// We don't have any CATMotion controllers directly assigned to us,
	// so the default implementation of this function won't actually work

	// Do we still have a palm?  (We need this)
	PalmTrans2* pPalm = GetPalm();
	if (pPalm == NULL)
		return;

	// Our first digit has the CATMotion on it
	DigitSegTrans* pSeg = GetBone(0);
	if (pSeg == NULL)
		return;

	// So Destruct it first...
	pSeg->DestructAllCATMotionLayers();
}

