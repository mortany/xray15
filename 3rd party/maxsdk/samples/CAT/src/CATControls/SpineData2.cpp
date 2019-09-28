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

// Transform Controller to blend between CATMotion/MotionCapture/Freeform

#include "CatPlugins.h"
#include "CATRigPresets.h"

// Rig Structure
#include "SpineData2.h"
#include "SpineTrans2.h"
#include "ICATParent.h"
#include <CATAPI/CATClassID.h>
#include "CATWeight.h"
#include "Hub.h"

// Layers
#include "CATClipValues.h"
#include "CATClipRoot.h"

#include "CATCharacterRemap.h"

// Restore objects (for flags).
#include "../DataRestoreObj.h"

const float ABSREL_DEFAULT = 1.0f;

//
//	SpineData2ClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//

class SpineData2ClassDesc : public CATControlClassDesc
{
public:
	SpineData2ClassDesc()
	{
		AddInterface(IBoneGroupManagerFP::GetFnPubDesc());
		AddInterface(ISpineFP::GetFnPubDesc());
	}

	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		SpineData2* spinedata = new SpineData2(loading);
		return spinedata;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_SPINEDATA2); }
	Class_ID		ClassID() { return SPINEDATA2_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("SpineData2"); }			// returns fixed parsable name (scripter-visible name)
};

// SimCity - Add in a second class desc for SpineData2.  This is to support loading of old files
// Old files will reference this ClassDesc to create instances of SpineData2 (under the old SClassID)
// The actual class of SpineData2 was irrelevant from CATs POV, and its methods were never called.
class SpineData2ClassDescLegacy : public SpineData2ClassDesc {
public:
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }	// Previous versions called this class a float controller
};

// our global instance of our classdesc class.
static SpineData2ClassDesc SpineData2Desc;
ClassDesc2* GetSpineData2Desc() { return &SpineData2Desc; }

static SpineData2ClassDescLegacy SpineData2DescLegacy;
ClassDesc2* GetSpineData2DescLegacy() { return &SpineData2DescLegacy; }

//	SpineData2
//
//	Our class implementation.
//

#include "iparamm2.h"
class SpineSetupDlgCallBack : public ParamMap2UserDlgProc
{
	SpineData2* spine;
	HWND hWnd;

	ISpinnerControl *spnLength;
	ISpinnerControl *spnSize;

	ICustButton* btnAddArbBone;

public:

	SpineSetupDlgCallBack() : hWnd(NULL),
		spnSize(NULL),
		spnLength(NULL),
		btnAddArbBone(NULL)
	{
		spine = NULL;
	}

	void InitControls(HWND hDlg, IParamMap2 *map)
	{
		hWnd = hDlg;
		spine = (SpineData2*)map->GetParamBlock()->GetOwner();

		float val = spine->GetSpineLength();
		spnLength = SetupFloatSpinner(hDlg, IDC_SPIN_LENGTH, IDC_EDIT_LENGTH, 0.0f, 1000.0f, val);
		spnLength->SetAutoScale();

		val = spine->GetSpineWidth();
		spnSize = SetupFloatSpinner(hDlg, IDC_SPIN_SIZE, IDC_EDIT_SIZE, 0.0f, 1000.0f, val);
		spnSize->SetAutoScale();

		if (spine->TestFlag(SPINEFLAG_FKSPINE))
			SET_CHECKED(hDlg, IDC_RDO_FKSPINE, TRUE);
		else SET_CHECKED(hDlg, IDC_RDO_PROCEDURAL, TRUE);

		if (spine->TestCCFlag(CNCFLAG_RETURN_EXTRA_KEYS))
			SET_CHECKED(hDlg, IDC_DISP_EXTRA_KEYFRAMES, TRUE);
	}
	void ReleaseControls(HWND) {
		SAFE_RELEASE_SPIN(spnLength);
		SAFE_RELEASE_SPIN(spnSize);

		spine = NULL;
	}
	virtual void Update(TimeValue, Interval&, IParamMap2*) {
		if (spine->TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) return;

		spnLength->SetValue(spine->GetSpineLength(), FALSE);
		spnSize->SetValue((spine->GetSpineWidth() + spine->GetSpineDepth()) / 2.0f, FALSE);

		if (spine->TestFlag(SPINEFLAG_FKSPINE)) {
			SET_CHECKED(hWnd, IDC_RDO_PROCEDURAL, FALSE);
			SET_CHECKED(hWnd, IDC_RDO_FKSPINE, TRUE);
		}
		else {
			SET_CHECKED(hWnd, IDC_RDO_PROCEDURAL, TRUE);
			SET_CHECKED(hWnd, IDC_RDO_FKSPINE, FALSE);
		}
		if (spine->TestCCFlag(CNCFLAG_RETURN_EXTRA_KEYS))
			SET_CHECKED(hWnd, IDC_DISP_EXTRA_KEYFRAMES, TRUE);
		else SET_CHECKED(hWnd, IDC_DISP_EXTRA_KEYFRAMES, FALSE);
	}

	virtual INT_PTR DlgProc(TimeValue /*t*/, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, map);
			DbgAssert(spine);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			return FALSE;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_USECUSTOMMESH: {
					CATNodeControl* catcontrol = dynamic_cast<CATNodeControl*>(GetCOREInterface()->GetSelNode(0)->GetTMController());
					if (catcontrol) {
						ICATObject* iobj = catcontrol->GetICATObject();
						if (iobj) iobj->SetUseCustomMesh(IS_CHECKED(hWnd, IDC_CHK_USECUSTOMMESH));
					}
					break;
				}
				case IDC_RDO_PROCEDURAL:
				{
					HoldActions holdActions(IDS_HLD_BONESETTING);
					spine->SetSpineFK(FALSE);

					// SetSpineFK may be cancelled.  Ensure
					// our UI is updated appropriately
					Interval iv;
					Update(0, iv, NULL);
				}
				break;
				case IDC_RDO_FKSPINE:
				{
					HoldActions holdActions(IDS_HLD_BONESETTING);
					spine->SetSpineFK(TRUE);

					// SetSpineFK may be cancelled.  Ensure
					// our UI is updated appropriately
					Interval iv;
					Update(0, iv, NULL);
				}
				break;
				case IDC_DISP_EXTRA_KEYFRAMES:
					spine->SetCCFlag(CNCFLAG_RETURN_EXTRA_KEYS);
					break;
				}
				break;
			}
			break;
		case CC_SPINNER_BUTTONDOWN:
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SPIN_SIZE:
			case IDC_SPIN_LENGTH:
				if (!theHold.Holding()) theHold.Begin();
				break;
			}
			break;
		case CC_SPINNER_CHANGE:
			if (theHold.Holding()) theHold.Restore();
			switch (LOWORD(wParam)) {
			case IDC_SPIN_SIZE: {
				float ratio = spnSize->GetFVal() / ((spine->GetSpineDepth() + spine->GetSpineWidth()) / 2.0f);
				spine->SetSpineWidth(spine->GetSpineWidth() * ratio);
				spine->SetSpineDepth(spine->GetSpineDepth() * ratio);
				break;
			}
			case IDC_SPIN_LENGTH:
				spine->SetSpineLength(spnLength->GetFVal());
				break;
			}
			break;
		case CC_SPINNER_BUTTONUP:
			if (theHold.Holding()) {
				if (HIWORD(wParam))	theHold.Accept(GetString(IDS_HLD_SPINE));
				else				theHold.Cancel();
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	virtual void DeleteThis() { }//delete this; }
};

static SpineSetupDlgCallBack SpineSetupsCallback;

class SpineMotionDlgCallBack : public TimeChangeCallback
{
	SpineData2* spine;
	HWND hWnd;
	ISliderControl *sldAbsRel;

public:

	HWND GetHWnd() { return hWnd; }

	SpineMotionDlgCallBack() : hWnd(NULL), spine(NULL)
	{
		sldAbsRel = NULL;
	}
	void InitControls(HWND hDlg, SpineData2 *spine)
	{
		hWnd = hDlg;

		this->spine = spine;
		DbgAssert(spine);

		sldAbsRel = SetupFloatSlider(hDlg, IDC_SLIDER_LAYERABSREL, IDC_EDIT_LAYERABSREL, 0.0f, 1.0f, 0.0f, 4);

		TimeChanged(GetCOREInterface()->GetTime());
		GetCOREInterface()->RegisterTimeChangeCallback(this);
	}
	void ReleaseControls(HWND hDlg)
	{
		if (hWnd != hDlg) return;
		GetCOREInterface()->UnRegisterTimeChangeCallback(this);
		SAFE_RELEASE_SLIDER(sldAbsRel);
		hWnd = NULL;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (SpineData2*)lParam);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			break;
		case WM_NOTIFY:
			break;
		case CC_SLIDER_BUTTONDOWN:
			theHold.Begin();
			break;
		case CC_SLIDER_CHANGE: {
			BOOL newundo = FALSE;
			if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
			ISliderControl *ccSlider = (ISliderControl*)lParam;
			TimeValue t = GetCOREInterface()->GetTime();
			float val = ccSlider->GetFVal();
			switch (LOWORD(wParam)) { // Switch on ID
			case IDC_SLIDER_LAYERABSREL:
				if (spine->layerAbsRel) {
					spine->layerAbsRel->SetValue(t, (void*)&val);
				}
				break;
			}
			if (newundo) theHold.Accept(GetString(IDS_HLD_CHANGE));
			GetCOREInterface()->RedrawViews(t);
		}
		break;
		case CC_SLIDER_BUTTONUP:
			if (theHold.Holding())
			{
				if (HIWORD(wParam))	theHold.Accept(GetString(IDS_HLD_ABSREL));
				else				theHold.Cancel();
			}
			break;
		case WM_CUSTEDIT_ENTER:
			if (theHold.Holding()) theHold.Accept(GetString(IDS_HLD_ABSREL));
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	void TimeChanged(TimeValue t)
	{
		float val = 0.0;

		if (spine->layerAbsRel)
		{
			Interval iv = FOREVER;
			spine->layerAbsRel->GetValue(t, (void*)&val, iv);
			sldAbsRel->SetValue(val, FALSE);
			if (spine->layerAbsRel->IsKeyAtTime(t, 0))	sldAbsRel->SetKeyBrackets(TRUE);
			else										sldAbsRel->SetKeyBrackets(FALSE);
		}
	}

	virtual void DeleteThis() { }
};

static SpineMotionDlgCallBack SpineMotionCallback;
static INT_PTR CALLBACK SpineMotionProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return SpineMotionCallback.DlgProc(hWnd, message, wParam, lParam);
};

static ParamBlockDesc2 SpineData2_param_blk(SpineData2::PBLOCK_REF, _T("params"), 0, &SpineData2Desc,
	P_AUTO_CONSTRUCT + P_AUTO_UI, SpineData2::PBLOCK,
	IDD_SPINEDATA_SETUP, IDS_SPINEPARAMS, 0, 0, &SpineSetupsCallback,
	// With the P_NO_REF tag we are not recieving messages from the parent about CATUnits.
	// We do not need to know what mode we are in or anything else off the CATParent I believe.
	SpineData2::PB_HUBBASE, _T("BaseHub"), TYPE_REFTARG, P_NO_REF, IDS_TIPCONTROL,
		p_end,
	SpineData2::PB_HUBTIP, _T("TipHub"), TYPE_REFTARG, P_NO_REF, IDS_TIPCONTROL,
		p_end,
	SpineData2::PB_NAME, _T(""), TYPE_STRING, 0, IDS_SPINENAME,
		p_default, _T("Spine"),
		p_ui, TYPE_EDITBOX, IDC_EDIT_NAME,
		p_end,
	SpineData2::PB_SIZE, _T(""), TYPE_FLOAT, 0, 0,
		p_end,
	SpineData2::PB_LENGTH, _T(""), TYPE_FLOAT, 0, 0,
		p_default, 60.0f,
		p_end,
	SpineData2::PB_NUMLINKS, _T("NumBones"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 0,
		p_range, 1, 60,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_EDIT_NUMLINKS, IDC_SPIN_NUMLINKS, SPIN_AUTOSCALE,
		p_end,
	SpineData2::PB_ROOTPOS_DEPRECATED, _T(""), TYPE_POINT3, P_OBSOLETE, 0,
		p_end,
	SpineData2::PB_SPINETRANS_TAB, _T("Bones"), TYPE_REFTARG_TAB, 0, P_NO_REF, 0,
		p_end,
	SpineData2::PB_SPINENODE_TAB_DEPRECATED, _T(""), TYPE_INODE_TAB, 0, P_OBSOLETE + P_NO_REF, 0,
		p_end,

	SpineData2::PB_SPINEWEIGHT, _T("SpineWeight"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPINEWEIGHT,
		p_default, 0.0f,
		p_end,
	SpineData2::PB_SETUPABS_REL_DEPRECATED, _T(""), TYPE_INT, P_OBSOLETE, 0,
		p_default, 1,
		p_end,
	SpineData2::PB_BASE_TRANSFORM, _T("ProjectionVector"), TYPE_MATRIX3, 0, 0,
		p_end,
	p_end
);

SpineData2::SpineData2(BOOL loading)
	: m_flags(0)
{
	pblock = NULL;
	layerAbsRel = NULL;

	pre_clone_base_hub = NULL;
	mHoldAnimStretchy = false;

	bNumSegmentsChanging = FALSE;
	SpineData2Desc.MakeAutoParamBlocks(this);
	pblock->ResetAll(TRUE, FALSE);

	if (!loading) {

	}
}

SpineData2::~SpineData2()
{
	DeleteAllRefs();
}

RefTargetHandle SpineData2::Clone(RemapDir& remap)
{
	// make a new SpineData2 object to be the clone
	// call true for loading so the new SpineData2 doesn't
	// make new default sub-controllers.
	SpineData2 *newSpine = new SpineData2(TRUE);
	remap.AddEntry(this, newSpine);

	newSpine->m_flags = m_flags;

	newSpine->ReplaceReference(PBLOCK, CloneParamBlock(pblock, remap));
	if (layerAbsRel) {
		newSpine->ReplaceReference(LAYERABS_REL, remap.CloneRef(layerAbsRel));
	}
	// CATNondeControl can handle cloning the flags and things
	CloneCATControl(newSpine, remap);

	// We add this entry now so that when we clone the
	// spine bones they can patch their pointers immediately
	// because the pblocks don't patch pointers.

	for (int i = 0; i < GetNumBones(); i++) {
		if (!remap.FindMapping(GetSpineBone(i))) {
			SpineTrans2 *spinetrans = (SpineTrans2*)remap.CloneRef(GetSpineBone(i));
			newSpine->pblock->SetValue(PB_SPINETRANS_TAB, 0, spinetrans, i);
			spinetrans->SetSpine(newSpine);
		}
	}

	newSpine->SetTipHub((Hub*)remap.CloneRef(GetTipHub()));
	newSpine->GetTipHub()->SetInSpine(newSpine);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newSpine, remap);

	// now return the new object.
	return newSpine;
}

void SpineData2::PostCloneManager()
{
}

RefResult SpineData2::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_CHANGE:
		if (pblock == hTarg)
		{
			// table index if the parameter that changed was a table.
			// scince we have no tables in this param block its not likely
			int ParamTabIndex = 0;
			ParamID ParamIndex = pblock->LastNotifyParamID(ParamTabIndex);
			switch (ParamIndex)
			{
			case PB_NUMLINKS:
				CreateLinks();
				break;
			case PB_SPINEWEIGHT:
				UpdateSpineCurvatureWeights();
				UpdateSpineColours(GetCOREInterface()->GetTime());
				Update();
				break;
			case PB_NAME:
				RenameSpine();
				break;
			case PB_SPINETRANS_TAB:
				break;
			}
		}
		else if (layerAbsRel == hTarg) {
			Update();
		}
		break;

	case REFMSG_TARGET_DELETED:
		if (pblock == hTarg)
			pblock = NULL;
		break;
	}
	return REF_SUCCEED;
}

CATGroup* SpineData2::GetGroup()
{
	Hub* pTip = GetTipHub();
	if (pTip != NULL)
		return pTip->GetGroup();
	return NULL;
}

TSTR SpineData2::GetName()
{
	return pblock->GetStr(PB_NAME);
}

void SpineData2::SetName(TSTR newname, BOOL quiet/*=FALSE*/)
{
	if (newname != GetName()) {
		if (quiet) DisableRefMsgs();
		pblock->SetValue(PB_NAME, 0, newname);
		if (quiet) EnableRefMsgs();
	};
}

void SpineData2::CATMessage(TimeValue t, UINT msg, int data) //, void* CATData)
{
	int i;
	switch (msg)
	{
	case CAT_SHOWHIDE:
		for (i = 0; i < GetNumBones(); i++)
			GetSpineBone(i)->GetNode()->Hide(data);
		break;
	case CAT_NAMECHANGE:
		RenameSpine();
		break;
	case CAT_SET_LENGTH_AXIS:
		SetLengthAxis(data);
		break;
	case CAT_POST_RIG_LOAD:
		// Note: We do not want this message sent when a rg3 file is loaded
		UpdateSpineCurvatureWeights();
		break;
	}

	CATControl::CATMessage(t, msg, data);

	// Our Curvature weights, and spine colours depend on out children being updated first
	switch (msg)
	{
	case CAT_COLOUR_CHANGED:
		UpdateSpineColours(t);
		break;
	}
}

CATClipValue* SpineData2::GetLayerController(int i)
{
	return ((i == 0) ? layerAbsRel : NULL);
}

int SpineData2::NumChildCATControls()
{
	return GetNumBones() + (GetTipHub() ? 1 : 0);
}

CATControl* SpineData2::GetChildCATControl(int i)
{
	if (i < GetNumBones())	return GetSpineBone(i);
	else if (GetTipHub())	return GetTipHub();
	else					return NULL;
}

CATControl* SpineData2::GetParentCATControl()
{
	return GetBaseHub();
}

void SpineData2::SetLengthAxis(int axis)
{
	Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	if (axis == X)	tmSpineBase = (RotateYMatrix(-HALFPI) * tmSpineBase) * RotateYMatrix(HALFPI);
	else			tmSpineBase = (RotateYMatrix(HALFPI)  * tmSpineBase) * RotateYMatrix(-HALFPI);
	pblock->SetValue(PB_BASE_TRANSFORM, 0, tmSpineBase);
};

TSTR	SpineData2::GetBoneAddress() {
	TSTR bonerigname(IdentName(GetRigID()));
	TSTR boneid;
	boneid.printf(_T("[%i]"), GetSpineID());
	DbgAssert(GetBaseHub());
	return (GetBaseHub()->GetBoneAddress() + _T(".") + bonerigname + boneid);
};

BOOL SpineData2::LoadRig(CATRigReader *load)
{
	// Delete This (when everything goes)
	pblock->EnableNotifications(TRUE);

	BOOL done = FALSE;
	BOOL ok = TRUE;

	int spinelinkNumber = 0;
	Point3 rootpos;
	float spinelength, spinewidth;
	spinelength = spinewidth = 0.0f;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idSpineParams) &&
				(load->CurGroupID() != idSpine))return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idHubParams:
			case idHub:
			{
				DbgAssert(GetTipHub());
				GetTipHub()->LoadRig(load);
			}
			break;
			case idCATWeightParams:
			{
				Control *ctrlTwistWeight = pblock->GetControllerByID(PB_SPINEWEIGHT);
				if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
					((CATWeight*)ctrlTwistWeight)->LoadRig(load);
				else load->SkipNextTokenOrValue();
				break;
			}
			case idSpineLink:
				if (spinelinkNumber >= GetNumBones()) SetNumBones(GetNumBones() + 1);
				GetSpineBone(spinelinkNumber)->LoadRig(load);
				spinelinkNumber++;
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idBoneName:		load->ToParamBlock(pblock, PB_NAME);		break;
			case idFlags:			load->GetValue(m_flags);					break;
			case idAbsRel:
			{
				int val;
				load->GetValue(val);

				// now that our spine has changes, we need to update the tip hub
				// we need to tell
				if (load->GetVersion() < CAT_VERSION_1700) {
					if (val > 0)
						GetTipHub()->GetLayerTrans()->SetFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
					else GetTipHub()->GetLayerTrans()->SetFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT);
				}

				float dAbsRel = (float)val;
				layerAbsRel->SetSetupVal((void*)&dAbsRel);
				break;
			}
			case idWidth:		load->GetValue(spinewidth);		break;
			case idHeight:		load->GetValue(spinelength);	break;
			case idRootPos:
			{
				load->GetValue(rootpos);
				Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
				tmSpineBase.SetTrans(rootpos);
				pblock->SetValue(PB_BASE_TRANSFORM, 0, tmSpineBase);
				break;
			}
			case idSpineBase:	load->ToParamBlock(pblock, PB_BASE_TRANSFORM);	break;
			case idNumLinks:
				pblock->EnableNotifications(FALSE);
				load->ToParamBlock(pblock, PB_NUMLINKS);
				pblock->EnableNotifications(FALSE);
				CreateLinks(TRUE);

				if (load->GetVersion() < CAT_VERSION_1700) {
					for (int i = 0; i < GetNumBones(); i++) {
						TSTR boneid;
						boneid.printf(_T("%d"), (i + 1));
						GetSpineBone(i)->SetName(boneid);
					}
				}
				break;

			case idWeightOutTan:
			case idWeightInTan:
			case idWeightOutVal:
			case idWeightInVal:
			{
				Control *ctrlTwistWeight = pblock->GetControllerByID(PB_SPINEWEIGHT);
				if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID)
				{
					IParamBlock2 *twistBlock = ctrlTwistWeight->GetParamBlock(0);
					switch (load->CurIdentifier()) {
					case idWeightOutTan: load->ToParamBlock(twistBlock, CATWeight::PB_KEY1OUTTANLEN);	break;
					case idWeightInTan:  load->ToParamBlock(twistBlock, CATWeight::PB_KEY2INTANLEN);	break;
					case idWeightOutVal: load->ToParamBlock(twistBlock, CATWeight::PB_KEY1VAL);			break;
					case idWeightInVal:  load->ToParamBlock(twistBlock, CATWeight::PB_KEY2VAL);			break;
					}
				}
				else
					load->SkipNextTokenOrValue();
				break;
			}

			case idSetupTM:
			{
				if (spinelinkNumber >= GetNumBones()) SetNumBones(GetNumBones() + 1);
				Matrix3 tmSetup;
				load->GetValue(tmSetup);
				SetXFormPacket setVal(tmSetup);
				GetSpineBone(spinelinkNumber)->SetSetupMatrix(tmSetup);
				spinelinkNumber++;
				break;
			}
			default:		load->AssertOutOfPlace();
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

	if (ok && load->GetVersion() < CAT_VERSION_1720) {
		SetSpineLength(spinelength);
		SetSpineWidth(spinewidth);
		SetSpineDepth(spinewidth / 2.0f);
	}

	UpdateSpineCATUnits();
	UpdateSpineCurvatureWeights();
	UpdateSpineColours(0);
	return ok && load->ok();
}

BOOL SpineData2::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idSpine);

	int iNumBones = pblock->GetInt(PB_NUMLINKS);

	if (iNumBones < 1)
	{
		save->EndGroup();
		return TRUE;
	}
	save->FromParamBlock(pblock, idBoneName, PB_NAME);
	save->Write(idFlags, m_flags);
	Interval valid;
	float absRel = GetAbsRel(0, valid);
	save->Write(idAbsRel, absRel);
	save->FromParamBlock(pblock, idSpineBase, PB_BASE_TRANSFORM);

	Control *ctrlTwistWeight = pblock->GetControllerByID(PB_SPINEWEIGHT);
	if (ctrlTwistWeight && ctrlTwistWeight->ClassID() == CATWEIGHT_CLASS_ID) ((CATWeight*)ctrlTwistWeight)->SaveRig(save);

	save->Write(idNumLinks, iNumBones);
	for (int i = 0; i < iNumBones; i++)	GetSpineBone(i)->SaveRig(save);

	Hub* pTipHub = GetTipHub();
	if (pTipHub)	pTipHub->SaveRig(save);

	save->EndGroup();
	return TRUE;
}

BOOL SpineData2::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	int  spinelinkNumber = 0;

	// by default bones are not in world space
	flags &= ~CLIPFLAG_WORLDSPACE;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if ((load->CurGroupID() != idSpineParams) &&
				(load->CurGroupID() != idSpine))return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idSpineLink:
				if (spinelinkNumber < GetNumBones()) {
					GetSpineBone(spinelinkNumber)->LoadClip(load, range, layerindex, dScale, flags);
					spinelinkNumber++;
				}
				else {
					load->SkipGroup();
				}
				break;
			case idAbsRelCtrl:
				if (layerAbsRel) {
					int newflags = flags;
					newflags = newflags&~CLIPFLAG_APPLYTRANSFORMS;
					newflags = newflags&~CLIPFLAG_MIRROR;
					newflags = newflags&~CLIPFLAG_SCALE_DATA;
					layerAbsRel->LoadClip(load, range, layerindex, dScale, newflags);
				}
				break;
			case idHubParams:
			case idHub:
			{
				Hub* pTipHub = GetTipHub();
				if (pTipHub) pTipHub->LoadClip(load, range, layerindex, dScale, flags);
				else load->SkipGroup();
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
	return ok && load->ok();
}

BOOL SpineData2::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(idSpine);

	save->Comment(GetName());

	int iNumBones = GetNumBones();
	save->Write(idNumLinks, iNumBones);
	for (int i = 0; i < iNumBones; i++)	GetSpineBone(i)->SaveClip(save, flags, timerange, layerindex);

	if (layerAbsRel) {
		save->BeginGroup(idAbsRelCtrl);
		layerAbsRel->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}

	Hub* pTipHub = GetTipHub();
	if (pTipHub)	pTipHub->SaveClip(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

void SpineData2::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	TSTR classname = _T("SpineData");
	flagsbegin = flags;
	ipbegin = ip;

	if (flags&BEGIN_EDIT_MOTION || flags&BEGIN_EDIT_LINKINFO) {
		if (!TestFlag(SPINEFLAG_FKSPINE)) {
			//////////////////////////////////////////////////////
			// Initialise Sub-Object mode
			moveMode = new MoveCtrlApparatusCMode(this, ip);
			rotateMode = new RotateCtrlApparatusCMode(this, ip);

			TSTR type(GetString(IDS_SPINEDIR_VEC));
			const TCHAR *ptype = type;
			ip->RegisterSubObjectTypes((const TCHAR**)&ptype, 1);

			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			ip->RedrawViews(ip->GetTime());
			//////////////////////////////////////////////////////

			if (flags&BEGIN_EDIT_MOTION)
				GetCATParentTrans()->GetCATLayerRoot()->BeginEditParams(ip, flags, prev);

			if (flags&BEGIN_EDIT_MOTION)
				ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_SPINEDATA_MOTION), &SpineMotionProc, GetString(IDS_SPINE_MOTION), (LPARAM)this);
		}
	}
	else if (flagsbegin == 0) {
		if (!TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) {

			pblock->EnableNotifications(FALSE);// this is to stop the name edit box sending a message when it is initilaised

			SpineData2Desc.BeginEditParams(ip, this, flags, prev);

			Control* ctrlWeights = pblock->GetControllerByID(PB_SPINEWEIGHT);
			if (ctrlWeights) {
				((CATWeight*)ctrlWeights)->DisplayRollout(ip, flags, prev, GetString(IDS_SPINECURVE_GRAPH));
			}
			pblock->EnableNotifications(TRUE);
		}
	}
}

void SpineData2::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	ipbegin = NULL;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO) {
		if (!TestFlag(SPINEFLAG_FKSPINE)) {
			///////////////////////////////////
			// End the subobject mode
			ip->DeleteMode(moveMode);
			if (moveMode) delete moveMode;
			moveMode = NULL;

			ip->DeleteMode(rotateMode);
			if (rotateMode) delete rotateMode;
			rotateMode = NULL;
			///////////////////////////////////

			if (flagsbegin&BEGIN_EDIT_MOTION)
				ip->DeleteRollupPage(SpineMotionCallback.GetHWnd());

			if (flagsbegin&BEGIN_EDIT_MOTION)
				GetCATParentTrans()->GetCATLayerRoot()->EndEditParams(ip, flags, next);
		}
	}
	else if (flagsbegin == 0) {
		if (!TestCCFlag(CNCFLAG_KEEP_ROLLOUTS)) {
			SpineData2Desc.EndEditParams(ip, this, flags, next);

			Control* ctrlWeights = pblock->GetControllerByID(PB_SPINEWEIGHT);
			if (ctrlWeights) ctrlWeights->EndEditParams(ip, flags, next);
		}
	}
}

void SpineData2::UpdateUI() {
	if (!ipbegin) return;
	if (flagsbegin&BEGIN_EDIT_MOTION || flagsbegin&BEGIN_EDIT_LINKINFO) {
		GetCATParentTrans()->GetCATLayerRoot()->UpdateUI();
	}
	else if (flagsbegin == 0) {
		IParamMap2* pmap = pblock->GetMap();
		if (pmap) pmap->Invalidate();
	}
}

void SpineData2::SetValueHubBase(TimeValue t, SetXFormPacket *ptr, int commit, GetSetMethod method)
{
	// If the tip hub is not currently pinned, then return...
	Hub* pTipHub = GetTipHub();
	Hub* pBaseHub = GetBaseHub();
	DbgAssert(pTipHub != NULL && pBaseHub != NULL);
	if (pTipHub == NULL || pBaseHub == NULL)
		return;

	if (!pTipHub->TestCCFlag(CCFLAG_FB_IK_LOCKED) || GetCATMode() == SETUPMODE)
		return;

	// Don't 'pin' when we are recieving an absolute value
	// This is to prevent issues when re-parenting or keying freeform
	// because the calls to GetNodeTM are not reliable in these cases.
	if (ptr->command == XFORM_SET)
		return;

	// A much simpler implementation.
	// Cache the tip hubs matrix.
	Matrix3 tmCurrTipHub = pTipHub->GetNodeTM(t);
	// Apply the SetValue to the base hub
	pBaseHub->CATNodeControl::SetValue(t, ptr, commit, method);
	// Attempt to set the old TM back on tip
	pTipHub->SetNodeTM(t, tmCurrTipHub);

	// Find the new tm of the tip hub.  This will
	// trigger a complete spine evaluation
	Matrix3 tmNewTipHub = pTipHub->GetNodeTM(t);
	// Find the offset from cached tip
	// tm to the resulting tm
	Point3 p3TipOffset = tmCurrTipHub.GetTrans() - tmNewTipHub.GetTrans();

	// Offset the base by that amount (this is the value returned
	// and applied by our calling function).
	*ptr = SetXFormPacket(p3TipOffset);
}

#ifdef _DEBUG
#include <vector>
static std::vector<std::pair<Point3, Point3>> dPosAttempts;
static std::vector<float> dPosAttemptErrorPct;
static Point3 p3SpineBase;
void DisplayStuff(GraphicsWindow* gw)
{
	if (dPosAttempts.size() == 0) return;
	Point3 pAttempt, pResult;
	static int idx = -1;

	// Draw Starting point and Dest first
	dLine2Pts(gw, p3SpineBase, dPosAttempts[0].first);
	gw->text(&(dPosAttempts[0].first), _T("Start"));
	dLine2Pts(gw, p3SpineBase, dPosAttempts[1].second);
	gw->text(&(dPosAttempts[0].second), _T("Desired"));

	// Draw 1 particular attempt only
	if (idx >= 1 && idx < (int)dPosAttempts.size())
	{
		pAttempt = dPosAttempts[idx].first;
		pResult = dPosAttempts[idx].second;
		gw->setColor(LINE_COLOR, float(idx) / dPosAttempts.size(), 0.0f, 0.0f);

		dLine2Pts(gw, p3SpineBase, pAttempt);
		dLine2Pts(gw, p3SpineBase, pResult);

		MSTR str;
		str.printf(_T("A(%i)"), idx);
		gw->text(&pAttempt, str);
		str.printf(_T("R(%i)%f"), idx, dPosAttemptErrorPct[idx]);
		gw->text(&pResult, str);
	}
	else
	{
		for (int i = 1; i < (int)dPosAttempts.size(); i++)
		{
			pAttempt = dPosAttempts[i].first;
			pResult = dPosAttempts[i].second;
			gw->setColor(LINE_COLOR, float(i) / dPosAttempts.size(), 0.3f, 0.0f);

			dLine2Pts(gw, p3SpineBase, pAttempt);
			gw->setColor(LINE_COLOR, 0.0f, 0.3f, float(i) / dPosAttempts.size());
			dLine2Pts(gw, p3SpineBase, pResult);

			MSTR str;
			str.printf(_T("A(%i)"), i);
			gw->text(&pAttempt, str);

			str.printf(_T("R(%i)%f"), i, dPosAttemptErrorPct[i]);
			gw->text(&pResult, str);
		}
	}
}
#endif

bool SpineData2::CalculateGuestimateHubPos(const Point3& vToCurrResult, const Point3& vToDesiredResult, Point3& p3HubPos, float& dError)
{
	// Calculate our error - what rotation did we achieve with that position?
	float dCosError = DotProd(vToCurrResult, vToDesiredResult);

	// If we are reaching the limit - there is no more error!
	if (dCosError >= ACOS_LIMIT)
		return false;

	// Offset our evaluation to include this error
	dError = acos(dCosError);
	//DbgAssert(dAngleError < dLastError);	// Just to ensure we are on the right track...
	//dLastError = dAngleError;

	// Now scale the angle by ratio that our last attempt missed by
	//DbgAssert(dTotalPosOffsetAngle > dAngleError);
	//float dAngleErrorRatio = dTotalPosOffsetAngle / (dTotalPosOffsetAngle - dAngleError);
	//if (dAngleErrorRatio > 1.01) // Don't scale the angle if the difference is small
	//	dAngleError = min(M_PI_2, dAngleError * dAngleErrorRatio);
	Point3 p3AxisError = Normalize(CrossProd(vToCurrResult, vToDesiredResult));
	Matrix3 tmErrorCorrector = RotAngleAxisMatrix(p3AxisError, dError);

	// Rotate the offset from tmSpineBase to tmAxis to account for this error
	p3HubPos = tmErrorCorrector.VectorTransform(p3HubPos);
	return true;
}

void SpineData2::SetValueHubTip(TimeValue t, SetXFormPacket *ptr, int commit, GetSetMethod &method)
{
	Hub* pBaseHub = GetBaseHub();
	Hub* pTipHub = GetTipHub();
	if (pBaseHub == NULL || pTipHub == NULL)
		return;

	Interval iv = FOREVER;
	float dAbsRel = GetAbsRel(t, iv);

	// Find the bounding values used to calculate the spine.
	// We need to find what values will cause the spine
	// to evaluate to find the desired result.
	Matrix3 tmBaseNode = GetBaseHub()->GetNodeTM(t);
	Matrix3 tmSpineBase = tmBaseNode;
	Matrix3 tmSpineTip(1);
	Point3 p3BaseScale(1, 1, 1);
	Point3 p3TipScale(1, 1, 1); // TODO - Test scaling.  I don't know if this will work with scale

	CalcSpineAnchors(t, tmBaseNode, p3BaseScale, tmSpineBase, tmSpineTip, p3TipScale, iv);

	// How do we want this SetValue to proceed...
	BOOL bDoFKSetValue = GetSpineFK();

	// If we are FK, pass the value off.
	// TODO: Fix these divergent paths.
	if (bDoFKSetValue)
		SetValueTipHubFK(t, ptr, tmSpineBase, commit, method);
	else
	{
		switch (ptr->command)
		{
		case XFORM_ROTATE:
		{
			break;
		}
		case XFORM_SCALE:
		{
			break;
		}
		case XFORM_MOVE:
		{
			// if the tip hub is being positioned out of range of the current spine, then lengthen the spine to suit
			// NOTE - To do this in Animation mode, we need to do a re-design of the spinal evaluation system.
			// The problem is that procedural spine does not have a matrix3 controller, so the simple system
			// of just scaling the spine doesn't work.
			if (CanScaleSpine())
			{
				Matrix3 tmTipPosition = GetTipHub()->GetNodeTM(t);
				// Where is the new hub position.
				Point3 p3NewTipPos = tmTipPosition.GetTrans() + ptr->tmAxis.VectorTransform(ptr->p);

				// In setup mode, change the actual length of the spine
				if (GetCATMode() == SETUPMODE)
				{
					// What is the % increase to the spine length?
					float ratio = Length(p3NewTipPos - tmSpineBase.GetTrans()) /
						Length(tmTipPosition.GetTrans() - tmSpineBase.GetTrans());

					MACRO_DISABLE;
					ScaleSpineLength(t, ratio);
					MACRO_ENABLE;
				}

				// Now, enforce that position we calculated!
				tmTipPosition.SetTrans(p3NewTipPos);
				// Force the spine to re-evaluate

				ptr->tmAxis = tmTipPosition;
				ptr->command = XFORM_SET;

				// reset the parent TM, and incidentally, update the spine.
				ptr->tmParent = GetTipHub()->GetParentTM(t);
				// Allow fall through to the XFORM_SET below

			}
			else
			{
				// here we remove the amount of translation that the spine could not reach
				if (GetTipHub()->TestCCFlag(HUBFLAG_INSPINE_RESTRICTS_MOVEMENT))
				{
					// Where was the old tip of the spine?
					Matrix3 tmTipPosition = GetTipHub()->GetNodeTM(t);

					// Where is the new hub position.
					Point3 p3NewTipPos = tmTipPosition.GetTrans() + ptr->tmAxis.VectorTransform(ptr->p);

					// Now, enforce that position we calculated!
					tmTipPosition.SetTrans(p3NewTipPos);
					ptr->tmAxis = tmTipPosition;
					ptr->command = XFORM_SET;

					// reset the parent TM, and incidentally, update the spine.
					ptr->tmParent = GetTipHub()->GetParentTM(t);

					// Allow fallthrough to the XFORM_SET below

					// Redo the SetValue, but we want to trigger the Absolute setvalue below.
				}
				else
				{
					Point3 p3NewTipPos = tmSpineTip.GetTrans() + ptr->tmAxis.VectorTransform(ptr->p);

					Point3 p3BaseToTip = p3NewTipPos - tmSpineBase.GetTrans();
					// Restrict the length of this vector
					p3BaseToTip = p3BaseToTip.Normalize() * (GetSpineLength() * GetCATUnits());
					// Where is this point?
					p3NewTipPos = tmSpineBase.GetTrans() + p3BaseToTip;
					// Now set the vector back
					ptr->p = Inverse(ptr->tmAxis).VectorTransform(p3NewTipPos - tmSpineTip.GetTrans());

					break;
				}
			}
			// Allow fall through to the XFORM_SET case here.
		}
		case XFORM_SET:
		{
			// Where would we end up if we evaluated just the rotation of the incoming value?
			Matrix3 tmDesired = ptr->tmAxis;

			// FOR RELATIVE SPINE: Positions are offset relative to the mtmHubTipParent.
			// Find the appropriate position to move us so we at least lie on the vector to our desired transform
			// This is the position that we want to store

			// The 2 vectors to measure our success each iteration.
			Point3 vToCurrResult, vToDesiredResult;
			vToDesiredResult = tmDesired.GetTrans() - tmSpineBase.GetTrans();
			float desiredLength = vToDesiredResult.LengthUnify();
#ifdef _DEBUG
			// Debugging info, cause I need some visual cues here...
			p3SpineBase = tmSpineBase.GetTrans();
#endif
			const int iMaxIterations = 20;
			float dLastError = TWOPI;
			float dAngleError = TWOPI;

			for (int iErrorIterations = 0; iErrorIterations < iMaxIterations; iErrorIterations++) // How many times have we tried to eliminate error?
			{
				Matrix3 tmTipWorld = EvalSpineRotation(tmSpineBase, ptr->tmAxis, p3BaseScale, Point3(1, 1, 1));
#ifdef _DEBUG
				dPosAttempts.resize(iErrorIterations + 2);
				dPosAttemptErrorPct.resize(iErrorIterations + 2);
#endif
				if (iErrorIterations == 0)
				{
					// Instead of measuring how much we missed by, the first time we
					// actually just measure how much we rotated by
					vToCurrResult = tmTipWorld.GetTrans() - tmSpineBase.GetTrans();
					vToCurrResult.Unify();

					Point3 p3WorldBaseToAxis = mtmHubTipParent.GetTrans() - tmSpineBase.GetTrans();
					if ((GetCATMode() == SETUPMODE) &&
						!CalculateGuestimateHubPos(vToCurrResult, vToDesiredResult, p3WorldBaseToAxis, dAngleError))
					{
						// Just using the rotation, we have hit the correct position. Set our
						// current position to the parents position, because that is correct.
						ptr->tmAxis.SetTrans(mtmHubTipParent.GetTrans());
						// Our axis gave us the correct result. return
						return;
					}

					// Rotate the offset from tmSpineBase to tmAxis to account for this error
					// Maintain the length of the desired transform.  This ensures that any scaling is maintained.
					if (CanScaleSpine())
						p3WorldBaseToAxis = p3WorldBaseToAxis.Normalize() * desiredLength;

					ptr->tmAxis.SetTrans(tmSpineBase.GetTrans() + p3WorldBaseToAxis);
					dLastError = dAngleError;

#ifdef _DEBUG
					// Save the points we are working with (Seeing is understanding)
					dPosAttempts[0].first = tmTipWorld.GetTrans();
					dPosAttempts[0].second = tmDesired.GetTrans();
#endif
				}

				// Lets just try chucking the current offset through, to see what we get
				Matrix3 tmResult = EvalSpinePosition(dAbsRel, tmTipWorld.GetTrans(), tmSpineBase, p3BaseScale, ptr->tmAxis, Point3(1, 1, 1));

#ifdef _DEBUG
				dPosAttempts[iErrorIterations + 1].first = ptr->tmAxis.GetTrans();
				dPosAttempts[iErrorIterations + 1].second = tmResult.GetTrans();
#endif

				// Now, we probably missed, but how much by?  Calculate our offset
				// Error difference in angle between the desired positions (in World Coords)
				Point3 vToCurrResult = tmResult.GetTrans() - tmSpineBase.GetTrans();
				vToCurrResult.Unify();

				// Using the error we missed by, generate a new guess.
				Point3 p3WorldBaseToAxis = ptr->tmAxis.GetTrans() - tmSpineBase.GetTrans();
				if (!CalculateGuestimateHubPos(vToCurrResult, vToDesiredResult, p3WorldBaseToAxis, dAngleError))
					break;

				// If we've really taken this many iterations to solve, we've messed up.
				DbgAssert(iErrorIterations <= (iMaxIterations - 1));

				// Set the new position, and lets go for another round
				ptr->tmAxis.SetTrans(tmSpineBase.GetTrans() + p3WorldBaseToAxis);

				// Store our last attempt, for future
				dLastError = dAngleError;
			}
			break; // break the switch statement
		}
		}
	}
}

void SpineData2::SetValueTipHubFK(TimeValue t, SetXFormPacket *ptr, const Matrix3& tmSpineBase, int commit, GetSetMethod &method)
{
	int iNumBones = GetNumBones();
	if (iNumBones == 0)
		return;

	Hub* pTipHub = GetTipHub();

	switch (ptr->command)
	{
	case XFORM_MOVE:
	{
		if (!pTipHub->TestCCFlag(CCFLAG_EFFECT_HIERARCHY))
			return;

		Matrix3 tmDesired = pTipHub->GetNodeTM(t);

		Point3 vec1 = Normalize(tmDesired.GetTrans() - tmSpineBase.GetTrans());
		tmDesired.Translate(VectorTransform(ptr->tmAxis, ptr->p));
		Point3 vec2 = Normalize(tmDesired.GetTrans() - tmSpineBase.GetTrans());

		AngAxis ax;
		ax.angle = -(float)acos(DotProd(vec1, vec2));
		ax.axis = Normalize(CrossProd(vec1, vec2));

		SetXFormPacket boneptr;
		for (int i = 0; i < iNumBones; i++) {
			SpineTrans2 *bone = (SpineTrans2*)GetSpineBone(i);
			boneptr = SetXFormPacket(ax, 1, bone->GetNode()->GetParentTM(t));
			if (i == 0) boneptr.aa.angle *= bone->GetPosWt();
			else	 boneptr.aa.angle *= bone->GetPosWt() - ((SpineTrans2*)GetSpineBone(i - 1))->GetPosWt();
			bone->GetLayerTrans()->SetValue(t, (void*)&boneptr, commit, method);
		}
		*ptr = SetXFormPacket(ax, 1, GetSpineBone(iNumBones - 1)->GetNode()->GetParentTM(t));
		ptr->aa.angle *= -GetSpineBone(iNumBones - 1)->GetPosWt();

		// Calculate the stretch required for us to reach this position.
		Matrix3 tm = pTipHub->GetNodeTM(t);
		vec1 = tmDesired.GetTrans() - tmSpineBase.GetTrans();
		vec2 = tm.GetTrans() - tmSpineBase.GetTrans();
		float stretch = (Length(vec1) / Length(vec2)) * DotProd(Normalize(vec1), Normalize(vec2));

		// Set on all applicable bones
		Interval iv;
		for (int i = 0; i < iNumBones; i++)
		{
			SpineTrans2* bone = GetSpineBone(i);
			if (bone == NULL || !bone->CanStretch(GetCATMode()))
				continue;

			CATClipMatrix3* pLayerTrans = bone->GetLayerTrans();
			if (pLayerTrans == NULL) // We cannot scale without a layer trans
				continue;

			// Construct the local scale packet
			// We want to apply in the controllers local space
			// so ensure our parent TM is the same as the
			// controllers value.
			SetXFormPacket pckt;
			pckt.command = XFORM_SCALE;
			pckt.p = P3_IDENTITY_SCALE;
			pckt.p[GetLengthAxis()] = stretch;
			pckt.tmParent.IdentityMatrix();
			pckt.tmAxis.IdentityMatrix();

			// This get value puts the axis system local to the controller.
			pLayerTrans->GetValue(t, &pckt.tmAxis, iv);
			pLayerTrans->SetValue(t, &pckt, 1, CTRL_RELATIVE);
		}
	}
	case XFORM_ROTATE:
	{
		if (iNumBones == 0) return;
		Matrix3 tmDesired;

		AngAxis axSetVal = ptr->aa;
		axSetVal.axis = Normalize(VectorTransform(ptr->tmAxis*Inverse(ptr->tmParent), axSetVal.axis));

		tmDesired = pTipHub->GetNodeTM(t);
		Point3 currSetPos = tmDesired.GetTrans();
		RotateMatrix(tmDesired, axSetVal);
		tmDesired.SetTrans(currSetPos);

		for (int i = 0; i < iNumBones; i++) {
			SpineTrans2 *bone = GetSpineBone(i);
			SetXFormPacket boneptr = *ptr;
			boneptr.tmParent = bone->GetNode()->GetParentTM(t);
			if (i == 0) boneptr.aa.angle *= bone->GetRotWt();
			else	 boneptr.aa.angle *= bone->GetRotWt() - GetSpineBone(i - 1)->GetRotWt();
			bone->GetLayerTrans()->SetValue(t, (void*)&boneptr, commit, method);
		}
		ptr->aa.angle *= (1.0f - GetSpineBone(iNumBones - 1)->GetRotWt());
	}
	}
}

class SpineEditRestore : public RestoreObj {
public:
	SpineData2	*spine;
	SpineEditRestore(SpineData2 *s) { spine = s; }
	void Restore(int isUndo) {
		for (int i = 0; i < spine->GetNumBones(); i++) {
			spine->GetSpineBone(i)->UpdateObjDim();
		}
		if (isUndo) {
			spine->Update();
			spine->UpdateUI();
		}
	}
	void Redo() {	}
	int Size() { return 1; }
	void EndHold() { spine->ClearAFlag(A_HELD); }
};

class SpineEditRestoreRedo : public RestoreObj {
public:
	SpineData2	*spine;
	SpineEditRestoreRedo(SpineData2 *s) { spine = s; }
	void Restore(int isUndo) { UNREFERENCED_PARAMETER(isUndo); }
	void Redo() {
		for (int i = 0; i < spine->GetNumBones(); i++) {
			spine->GetSpineBone(i)->UpdateObjDim();
		}
		spine->Update();
		spine->UpdateUI();
	}
	int Size() { return 1; }
	void EndHold() { spine->ClearAFlag(A_HELD); }
};

// GetSpineLength should return the length of the unscaled spine.
float SpineData2::GetSpineLength()
{
	float spinelength = 0.0f;
	for (int i = 0; i < GetNumBones(); i++)
	{
		SpineTrans2* pSpine = GetSpineBone(i);
		if (!pSpine)
			break;

		spinelength += pSpine->GetObjZ();
	}
	return spinelength;
};

void  SpineData2::SetSpineLength(float val) {
	if (val < 1.0f) return;
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	if (theHold.Holding() && !TestAFlag(A_HELD)) theHold.Put(new SpineEditRestore(this));

	float oldspinelength = GetSpineLength();
	float ratio = (oldspinelength > 0.0f) ? (val / oldspinelength) : 1.0f;
	ScaleValue sv;
	sv.s = Point3(1.0f, 1.0f, ratio);
	ModVec(sv.s, GetLengthAxis());
	SetXFormPacket xform_scl(sv.s, 1, Matrix3(1), Matrix3(1));
	int iNumBones = GetNumBones();
	for (int i = 0; i < iNumBones; i++) {
		CATNodeControl *bone = GetSpineBone(i);
		if (!bone || bone->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL) || bone->IsXReffed()) continue;
		ICATObject *iobj = GetSpineBone(i)->GetICATObject();
		if (iobj) {
			if (oldspinelength > 0.0f)
				iobj->SetZ(iobj->GetZ() * ratio);
			else iobj->SetZ(val / (float)iNumBones);
			bone->UpdateObjDim();
		}
	}

	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		theHold.Put(new SpineEditRestoreRedo(this));
		SetAFlag(A_HELD);
	}
	//	if(theHold.Holding()) theHold.Put(new SpineEditRestoreRedo(this));
		// a new undo has been started by someone else. If we accept here, then thier updo won't be saved
	Update();
	if (newundo)	theHold.Accept(GetString(IDS_HLD_LENSIZE));
}

bool SpineData2::CanScaleSpine()
{
	// If some spine links are set to 'Manipulation causes stretching'
	// then we can scale the spine to reach our position
	int nLinks = GetNumBones();
	Hub* pTipHub = GetTipHub();
	if (pTipHub == NULL)
		return FALSE;

	// Get CATMode from the tip hub.  This
	// ensures that we get the correct mode
	// (including KeyFreeform).  This may
	// need tweaking for FK Spines.
	CATMode iCATMode = pTipHub->GetCATMode();
	for (int i = 0; i < nLinks; i++)
	{
		CATNodeControl* pLink = GetSpineBone(i);
		if (pLink != NULL && pLink->IsStretchy(iCATMode))
			return true;
	}
	return false;
}

void SpineData2::ScaleSpineLength(TimeValue t, float scale)
{
	if (scale <= 0)
		return;

	int iNumBones = GetNumBones();
	CATMode iCATMode = GetCATMode();
	if (iCATMode != SETUPMODE)
		return;

	for (int i = 0; i < iNumBones; i++)
	{
		SpineTrans2 *bone = GetSpineBone(i);

		if (bone == NULL)
			continue;

		bool bIsStretchy = bone->CanStretch(iCATMode);
		if (!bIsStretchy)
			continue;

		if (iCATMode == SETUPMODE)
		{
			ICATObject *iobj = bone->GetICATObject();
			if (iobj)
				iobj->SetZ(iobj->GetZ() * scale);
		}
	}
}

float SpineData2::GetSpineWidth() {
	float spinesize = 0.0f;
	for (int i = 0; i < GetNumBones(); i++) {
		ICATObject *iobj = GetSpineBone(i)->GetICATObject();
		if (iobj) {
			spinesize += (iobj->GetX() + iobj->GetY());
		}
	}
	spinesize /= (GetNumBones() * 2);
	return spinesize;
};

void  SpineData2::SetSpineWidth(float val) {
	if (val < 0.01) return;
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	if (theHold.Holding()) theHold.Put(new SpineEditRestore(this));

	float ratio = val / GetSpineWidth();
	for (int i = 0; i < GetNumBones(); i++) {
		if (GetSpineBone(i)->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL)) continue;
		ICATObject *iobj = GetSpineBone(i)->GetICATObject();
		if (iobj) {
			iobj->SetX(iobj->GetX() * ratio);
			GetSpineBone(i)->UpdateObjDim();
		}
	}
	if (newundo) theHold.Accept(GetString(IDS_HLD_SPINEWID));
}

float SpineData2::GetSpineDepth() {
	float spinedepth = 0.0f;
	for (int i = 0; i < GetNumBones(); i++) {
		ICATObject *iobj = GetSpineBone(i)->GetICATObject();
		if (iobj) spinedepth += (iobj->GetY());
	}
	spinedepth /= (GetNumBones() * 2);
	return spinedepth;
};

void  SpineData2::SetSpineDepth(float val) {
	if (val < 0.01) return;
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	float ratio = val / GetSpineDepth();
	for (int i = 0; i < GetNumBones(); i++) {
		if (GetSpineBone(i)->TestCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_SCL)) continue;
		ICATObject *iobj = GetSpineBone(i)->GetICATObject();
		if (iobj) {
			iobj->SetY(iobj->GetY() * ratio);
			GetSpineBone(i)->UpdateObjDim();
		}
	}
	if (newundo) theHold.Accept(GetString(IDS_HLD_SPINEDEPTH));
}

float SpineData2::GetAbsRel(TimeValue t, Interval& valid)
{
	float val = ABSREL_DEFAULT;
	DbgAssert(layerAbsRel != NULL);
	if (layerAbsRel) {
		layerAbsRel->GetValue(t, (void*)&val, valid, CTRL_ABSOLUTE);
	}
	return val;
}

void SpineData2::SetAbsRel(TimeValue t, float val)
{
	val = min(1.0f, max(val, 0.0f));
	if (layerAbsRel) layerAbsRel->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
}

const Matrix3& SpineData2::GetSpineTipTM() const
{
	return mtmHubTipParent;
}

void SpineData2::CalcSpineBaseAnchor(const Matrix3& tmBaseNode, const Point3& p3BaseScale,
	Matrix3& tmSpineBase, Matrix3& tmSpineTipParent)
{
	float dCATUnits = GetCATUnits();
	float dSpineLength = GetSpineLength()*dCATUnits;
	int lengthaxis = GetLengthAxis();
	dSpineLength *= p3BaseScale[lengthaxis];

	// Calculate the base of the spine
	Matrix3 tmBaseOffset = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	tmBaseOffset.SetTrans(p3BaseScale * dCATUnits * tmBaseOffset.GetTrans());
	tmSpineBase = tmBaseOffset * tmBaseNode;

	// Get the global position of the tips parent.
	tmSpineTipParent = tmSpineBase;
	tmSpineTipParent.PreTranslate(FloatToVec(dSpineLength, lengthaxis));
}

void SpineData2::CalcSpineAnchors(TimeValue t, const Matrix3& tmBaseNode, const Point3& p3BaseScale,
	Matrix3& tmSpineBase, Matrix3& tmSpineTip, Point3& p3TipScale, Interval& valid)
{
	CalcSpineBaseAnchor(tmBaseNode, p3BaseScale, tmSpineBase, tmSpineTip);
	tmSpineTip.PreScale(p3BaseScale);
	mtmHubTipParent = tmSpineTip;

	// Bypass the normal evaluation pipeline: Get tips desired value without
	// allowing it to limit us! The tip directly inherits the base's scale
	Hub* pTipHub = GetTipHub();
	if (pTipHub == NULL)
		return;

	Point3 p3TipLocalScale = P3_IDENTITY_SCALE;
	pTipHub->CalcInheritance(t, tmSpineTip, p3TipScale);
	pTipHub->ApplySetupOffset(t, tmBaseNode, tmSpineTip, p3TipScale, valid);
	pTipHub->CATNodeControl::CalcWorldTransform(t, tmBaseNode, p3TipScale, tmSpineTip, p3TipLocalScale, valid);
	p3TipScale *= p3TipLocalScale;
}

// Given all the inputs for the spine, we now force an evaluation in the spine bones. We calculate a new tmHubTip
void SpineData2::EvalSpine(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, Interval &valid)
{
	if (IsEvaluationBlocked())
		return;

	// This should never happen
	DbgAssert(!TestFlag(SPINEFLAG_FKSPINE));
	if (TestFlag(SPINEFLAG_FKSPINE))
		return;

	// It is possible that we recurse into this function if the hub
	// has reach applied.  This is unnecessary - each evaluation of
	// the while below will calculate valid values for the spine links.
	// The spine links can just return their current values and be correct.
	BlockEvaluation block(this);

	// Calculate our desired start & end points
	Matrix3 tmSpineBase, tmSpineTip;
	Point3 p3TipScale;
	CalcSpineAnchors(t, tmParent, p3ParentScale, tmSpineBase, tmSpineTip, p3TipScale, valid);

	Hub* pHub = GetTipHub();
	if (pHub == NULL)
		return;

	float dAbsRel = GetAbsRel(t, valid);
	int i = 0;
	pHub->ResetReachCache();
	Matrix3 tmSpineResult(1);
	do {
		// Calculate the affect of applying the rotation
		tmSpineResult = EvalSpineRotation(tmSpineBase, tmSpineTip, p3ParentScale, p3TipScale);

		tmSpineResult = EvalSpinePosition(dAbsRel, tmSpineResult.GetTrans(), tmSpineBase, p3ParentScale, tmSpineTip, p3TipScale);

		// Interesting issue!!! Evaluating with reach can cause
		// a pre-evaluation of the hierarchy, meaning that the
		// node thinks its current value is valid.  Adding this
		// line basically means that if we evaluate, the node is
		// always going to update to the new value.
		// NOTE - Don't call this on Bone 0, as that will
		// trigger call this fn again (infinite loop). Also
		// its unnecessary, bone0 GetValue is calling us
		// right now!
		if (i != 0 && GetNumBones() > 1)
			GetSpineBone(1)->InvalidateTransform();

		// We are only ever relative for the 1st iteration
		dAbsRel = 0;

		// Apply reach to our result...
	} while (i++ < 3 && pHub->ApplyReach(t, valid, tmSpineTip, tmSpineResult, p3TipScale));
}

Matrix3 SpineData2::EvalSpineRotation(const Matrix3& tmSpineBase, const Matrix3& tmSpineTip, const Point3& p3BaseScale, const Point3& p3TipScale)
{
	////////////////////////////////////////////////////////////////////////////////////////////
	// Apply the Root tm to the base matrix to get it to the base of the spine
	Matrix3 tmSpineLink = tmSpineBase;
	Point3 p3IncrScale = p3TipScale / p3BaseScale;
	Point3 p3BoneScale = P3_IDENTITY_SCALE;

	int iNumBones = GetNumBones();
	for (int i = 0; i < iNumBones; i++)
	{
		SpineTrans2* pBone = GetSpineBone(i);
		DbgAssert(pBone != NULL);
		if (pBone)
			tmSpineLink = pBone->CalcRotTM(tmSpineLink, tmSpineBase, tmSpineTip, p3IncrScale, p3BoneScale);
	}
	return tmSpineLink;
}

Matrix3 SpineData2::EvalSpinePosition(float absrel, const Point3& p3CurrentPos, const Matrix3& tmSpineBase, const Point3& /*p3BaseScale*/, const Matrix3& tmSpineTip, const Point3& /*p3TipScale*/)
{
	/////////////////////////////////////////////////////////////////
	// Here we calculate the AngAxis that will be used to create the positional offset down the spine
	// For a relative spine, we leave translation so far accumulated and returned from the hub tip.
	// For an absolute spine, we remove the accumulated translation created from the rotations
	// and add on the translation returned from the tip hub.
	// absolute spines will maintain thier positions as accurately as possible
	// for a absolute spine, we subtract off the currently accumulated translation
	Point3 pAbsDir = p3CurrentPos - tmSpineBase.GetTrans();
	Point3 pRelDir = tmSpineBase.GetRow(GetLengthAxis());
	Point3 vec1 = BlendVector(Normalize(pAbsDir), pRelDir, absrel);
	Point3 vec2 = tmSpineTip.GetTrans() - tmSpineBase.GetTrans();

	vec1.Unify();	// Unify == fancy sounding normalize
	vec2.Unify();

	Matrix3 tmTipHub = tmSpineTip;
	AngAxis axOffsetPos;
	float dCosAngle = DotProd(vec1, vec2);
	if (dCosAngle < ACOS_LIMIT)	// is significant?  Largest number which gives non 0 result in acos operation
	{
		Matrix3 tmSpineLink = tmSpineBase;
		axOffsetPos.angle = -acos(dCosAngle);
		axOffsetPos.axis = Normalize(CrossProd(vec1, vec2));
		int nBones = GetNumBones();
		for (int i = 0; i < nBones; i++)
		{
			SpineTrans2* pSpineBone = GetSpineBone(i);
			if (pSpineBone != NULL)
				tmSpineLink = pSpineBone->CalcPosTM(tmSpineLink, axOffsetPos);
		}
		tmTipHub.SetTrans(tmSpineLink.GetTrans());
	}
	else
		tmTipHub.SetTrans(p3CurrentPos);

	// now calculate the stretch factor on the spine
	if (!mHoldAnimStretchy && (GetCATMode() != SETUPMODE)) {
		float spinestretch = Length(tmSpineTip.GetTrans() - tmSpineBase.GetTrans()) / Length(tmTipHub.GetTrans() - tmSpineBase.GetTrans());
		BOOL stretchy = FALSE;
		for (int i = 0; i < GetNumBones(); i++)
		{
			SpineTrans2* bone = GetSpineBone(i);
			if (bone->TestCCFlag(CCFLAG_ANIM_STRETCHY))
			{
				// If the previous bone didn't stretch, or if we don't inherit its scale, set the scale here.
				// NOTE: We skip the check for InheritAnim because it's a given we are in non-FK spine here
				// (we wouldn't be here if we were).  In non-FK spine, we have no LayerTrans, and the
				// the Inheritance function in CATNodeControl dosn't run.
				DbgAssert(bone->GetLayerTrans() == NULL);
				if (!stretchy) // || (!bone->TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL) && )
					bone->SetSpineStretch(spinestretch);
				else // We are simply inheriting the previous scale.  We're good :-)
					bone->SetSpineStretch(1.0f);
				// This bone should stretch
				stretchy = TRUE;

			}
			else
			{
				// If the previous bone stretched, and we _don't_ inherit its scale, cancel it out.
				if (stretchy)// && !bone->TestCCFlag(CNCFLAG_INHERIT_ANIM_SCL)) (ignore inheritance, it doesn't apply to procedural spines
					bone->SetSpineStretch(1.0f / spinestretch);
				else
					// We have already canceled out the parents scale, so just return here.
					bone->SetSpineStretch(1.0f);
				stretchy = FALSE;
			}
		}
	}
	return tmTipHub;
}

float SpineData2::GetRotWeight(float index) {

	float numlinks = (float)GetNumBones();

	// Calcuatle the length of the spine up to the index specified
	// Start by adding all the bone lengths together like we do in GetSpineLength()
	int lastbone = (int)floor(index);
	float prev_spine_length = 0.0f;
	float bonelength = 0.0f;
	int i;
	for (i = 0; i < numlinks && i < lastbone; i++) {
		ICATObject*	iobj = GetSpineBone(i)->GetICATObject();
		if (iobj) {
			bonelength = iobj->GetZ();
		}
		else {
			bonelength = GetSpineBone(i)->GetBoneLength() / GetCATUnits();
		}
		prev_spine_length += bonelength;
	}
	// Then because our index is a float value we may need to add on a portion of the last bone
	if (lastbone < numlinks) {
		ICATObject*	iobj = GetSpineBone(lastbone)->GetICATObject();
		if (iobj) {
			bonelength = iobj->GetZ();
		}
		else {
			bonelength = GetSpineBone(i)->GetBoneLength() / GetCATUnits();
		}
		prev_spine_length += (index - (float)lastbone) * bonelength;
	}

	// link ratio is basically how far along the length of the spine this index placed us
	float linkratio = prev_spine_length / (GetSpineLength()/*GetCATUnits()*/);

	// Magic STEPTIME100 number, SpineData generates the weight graphs 19200 ticks long
	return pblock->GetFloat(PB_SPINEWEIGHT, (int)(linkratio * STEPTIME100));
}

// this function will be called by the Ribcage when someone is fiddling with one of his controllers
void SpineData2::Update()
{
	SpineTrans2* pFirstBone = GetSpineBone(0);
	if (pFirstBone != NULL)
		pFirstBone->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

// We need this method because some times we want
// to update the objects but not let the message propogate to the tip hub
void SpineData2::UpdateSpineCATUnits()
{
	TimeValue t = GetCOREInterface()->GetTime();
	for (int j = 0; j < GetNumBones(); j++)
	{
		GetSpineBone(j)->CATMessage(t, CAT_CATUNITS_CHANGED);
	}
}

void SpineData2::UpdateSpineCurvatureWeights()
{
	int iNumBones = GetNumBones();
	for (int j = 0; j < iNumBones; j++)
	{
		SpineTrans2 *vertebret = GetSpineBone(j);
		if (vertebret) {
			vertebret->CATMessage(0, CAT_CATUNITS_CHANGED);
			vertebret->UpdateRotPosWeights();
		}
	}
}

void SpineData2::UpdateSpineColours(TimeValue t)
{
	for (int j = 0; j < GetNumBones(); j++)
	{
		SpineTrans2 *vertebret = GetSpineBone(j);
		if (vertebret) vertebret->UpdateColour(t);
	}
}

void SpineData2::RenameSpine()
{
	int iNumBones = GetNumBones();
	for (int j = 0; j < iNumBones; j++)
	{
		GetSpineBone(j)->UpdateName();
	}
}

//////////////////////////////////////////////////////////////////////////

Hub* SpineData2::GetBaseHub()
{
	return static_cast<Hub*>(pblock->GetReferenceTarget(PB_HUBBASE));
}

void SpineData2::SetBaseHub(Hub* hub)
{
	pblock->SetValue(PB_HUBBASE, 0, (ReferenceTarget*)hub);
}

Hub* SpineData2::GetTipHub()
{
	return static_cast<Hub*>(pblock->GetReferenceTarget(PB_HUBTIP));
}

void SpineData2::SetTipHub(Hub* hub)
{
	pblock->SetValue(PB_HUBTIP, 0, (ReferenceTarget*)hub);
}

CatAPI::IHub* SpineData2::GetTipIHub()
{
	return GetTipHub();
}

CatAPI::IHub* SpineData2::GetBaseIHub()
{
	return GetBaseHub();
}

SpineTrans2* SpineData2::GetSpineBone(int id)
{
	return (id < GetNumBones()) ? (SpineTrans2*)pblock->GetReferenceTarget(PB_SPINETRANS_TAB, 0, id) : NULL;
}

//////////////////////////////////////////////////////////////////////////

void SpineData2::CreateLinks(BOOL loading/*=FALSE*/)
{
	if (theHold.RestoreOrRedoing()) return;

	int iNumBones = pblock->GetInt(PB_NUMLINKS);
	int oldNumLinks = GetNumBones();
	if (iNumBones == oldNumLinks)
		return;

	Interface *ip = GetCOREInterface();
	int t = ip->GetTime();

	// Do not update while creating links
	{
		HoldActions hold(IDS_HLD_SPINEBONES);
		BlockEvaluation block(this);
		SetCCFlag(CNCFLAG_KEEP_ROLLOUTS);

		if (iNumBones < 1) {
			// Remove the inspine from the Tip Hub
			GetTipHub()->SetInSpine(NULL);
			GetBaseHub()->RemoveSpine(GetSpineID(), FALSE);
			// Add the Tip hub to the list of arbitrary bones
			int numarbbones = GetBaseHub()->GetNumArbBones();
			GetBaseHub()->InsertArbBone(numarbbones, GetTipHub());
			ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);
		}

		// Get the old spine length if we are not loading a rig file
		float oldspinelength = loading ? 0.0f : GetSpineLength();

		int j;
		if (oldNumLinks > iNumBones)
		{
			// delete excess,
			for (j = (oldNumLinks - 1); j >= iNumBones; j--)
			{
				INode *nodeSeg = GetSpineBone(j)->GetNode();
				if (nodeSeg) {
					if (ip->GetSelNodeCount() == 1 && nodeSeg == ip->GetSelNode(0)) {
						// Deselect this node, as its about to be deleted
						// Do this to prevent crash when useing Object panel to delete stuff
						if (iNumBones > 0) {
							ip->SelectNode(GetSpineBone(0)->GetNode());
						}
						ip->DeSelectNode(nodeSeg);
					}
					// Delete the Bone Node, and any extra bones
					GetSpineBone(j)->DeleteBoneHierarchy();
					pblock->SetCount(PB_SPINETRANS_TAB, j);
				}
			}
		}

		pblock->SetCount(PB_SPINETRANS_TAB, iNumBones);

		if (oldNumLinks < iNumBones)
		{
			for (j = oldNumLinks; j < iNumBones; j++)
			{
				SpineTrans2* ctrl = (SpineTrans2*)CreateInstance(CTRL_MATRIX3_CLASS_ID, SPINETRANS2_CLASS_ID);
				ctrl->Initialise(this, j, loading);
				if (!loading && j > 0 && oldNumLinks > 0) {
					ctrl->PasteRig(GetSpineBone(j - 1), 0, 1.0f);
				}
				pblock->SetValue(PB_SPINETRANS_TAB, 0, (ReferenceTarget*)ctrl, j);
			}
		}

		ClearCCFlag(CNCFLAG_KEEP_ROLLOUTS);

		if (!loading && iNumBones > 0) {
			if (oldNumLinks > 0)
				SetSpineLength(oldspinelength);

			CATMessage(GetCOREInterface()->GetTime(), CAT_CATUNITS_CHANGED);
			UpdateSpineCurvatureWeights();

			// the Max node colour swatch crashes if the previously selected node
			// is deleted, and it tries to update the colour swatch
			UpdateSpineColours(t);
			RenameSpine();
		}
	}

	if (iNumBones > 0)
	{
		Update();
	}
};

void SpineData2::SetNumBones(int newNumLinks)
{
	pblock->SetValue(PB_NUMLINKS, 0, newNumLinks);
}

int SpineData2::GetNumBones() const
{
	return pblock->Count(PB_SPINETRANS_TAB);
}

CatAPI::INodeControl* SpineData2::GetBoneINodeControl(int id)
{
	return GetSpineBone(id);
}

/*
I now build the spine up from the tip to the base.
This means that I don't have to keep a pointer to the Ribcage/Head, or the Pelvis.
The spineWeights therfore have to be reversed and this makes it a bit confusing.
Spine link 1 in the array is the one that the ribcage is parented to
and the last in the array is the base of the spine
*/
void SpineData2::SetSpineWeights()
{
	pblock->EnableNotifications(FALSE);
	// Set up a weigthing graph for the spines bending
	Control *ctrlTwistWeight = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATWEIGHT_CLASS_ID);
	pblock->SetControllerByID(PB_SPINEWEIGHT, 0, ctrlTwistWeight, FALSE);
	pblock->EnableNotifications(TRUE);
}

// here we configure this spine and maybe setup some
// default values if we are not loading a rig file
void SpineData2::Initialise(Hub* hub, int nSpineID, BOOL loading, int iNumBones)
{
	{ // Scope the BlockEvaluation for setup
		BlockEvaluation block(this);

		pblock->SetValue(PB_HUBBASE, 0, (ReferenceTarget*)hub);
		SetSpineID(nSpineID);

		Control *ctrlTwistWeight = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATWEIGHT_CLASS_ID);
		pblock->SetControllerByID(PB_SPINEWEIGHT, 0, ctrlTwistWeight, FALSE);

		// we absolutely must have a tip hub, so make one now
		Hub* pTipHub = (Hub*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HUB_CLASS_ID);
		pTipHub->Initialise(GetCATParentTrans(), loading, this);
		pblock->SetValue(PB_HUBTIP, 0, (ReferenceTarget*)pTipHub);

		CATClipValue* layerAbsRel = CreateClipValueController(GetCATClipFloatDesc(), GetClipWeights(), GetCATParentTrans(), FALSE);
		ReplaceReference(LAYERABS_REL, layerAbsRel);
	}

	if (!loading)
	{
		layerAbsRel->SetSetupVal((void*)&ABSREL_DEFAULT);

		SetNumBones(iNumBones);
		SetSpineLength(60);
		SetSpineWidth(10);
		SetSpineDepth(8);

		CATMessage(GetCOREInterface()->GetTime(), CAT_CATUNITS_CHANGED);
		UpdateSpineColours(GetCOREInterface()->GetTime());
	}
}

SClass_ID SpineData2::SuperClassID()
{
	return SpineData2Desc.SuperClassID();
}

RefTargetHandle SpineData2::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case LAYERABS_REL:		return (ReferenceTarget*)layerAbsRel;
	default:				return NULL;
	}
}

void SpineData2::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:		pblock = (IParamBlock2*)rtarg;			break;
	case LAYERABS_REL:		layerAbsRel = (CATClipFloat*)rtarg;		break;
	}
}

Animatable* SpineData2::SubAnim(int i)
{
	// currently corresponds directly to SubAnim()
	switch (i)
	{
	case PBLOCK_REF:		return pblock;
	case LAYERABS_REL:		return layerAbsRel;
	default:				return NULL;
	}
}

TSTR SpineData2::SubAnimName(int i)
{
	// currently corresponds directly to SubAnim()
	switch (i)
	{
	case PBLOCK_REF:		return GetString(IDS_PARAMS);
	case LAYERABS_REL:		return GetString(IDS_LYR_ABSREL);
	default:				return _T("");
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class SpineDataPLCB : public PostLoadCallback {
protected:
	SpineData2 *spine;

public:
	SpineDataPLCB(SpineData2 *pOwner) { spine = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(spine);
		if (spine->GetFileSaveVersion() > 0) return spine->GetFileSaveVersion();

		ICATParentTrans *catparent = spine->GetCATParentTrans();
		DbgAssert(catparent);
		return catparent->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *iload) {

		if (GetFileSaveVersion() < CAT_VERSION_1210)
		{
			// here we are changing the PB_SPINETRANS_TAB to store SpineTrans's instead if pblocks
			int numNodes = spine->pblock->Count(SpineData2::PB_SPINENODE_TAB_DEPRECATED);
			spine->pblock->SetCount(SpineData2::PB_SPINETRANS_TAB, numNodes);

			for (int i = 0; i < numNodes; i++) {
				INode* linknode = spine->pblock->GetINode(SpineData2::PB_SPINENODE_TAB_DEPRECATED, 0, i);
				if (linknode) spine->pblock->SetValue(SpineData2::PB_SPINETRANS_TAB, 0, (ReferenceTarget*)linknode->GetTMController(), i); ;
			}
			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_2100)
		{
			Matrix3 tmSpineBase = spine->pblock->GetMatrix3(SpineData2::PB_BASE_TRANSFORM);
			tmSpineBase.SetTrans(spine->pblock->GetPoint3(SpineData2::PB_ROOTPOS_DEPRECATED));
			spine->pblock->SetValue(SpineData2::PB_BASE_TRANSFORM, 0, tmSpineBase);
			iload->SetObsolete();
		}

		if (GetFileSaveVersion() < CAT_VERSION_2445)
		{
			CATClipValue* layerAbsRel = CreateClipValueController(GetCATClipFloatDesc(), spine->GetClipWeights(), spine->GetCATParentTrans(), FALSE);

			// set the controller up with the same settings as we have been using
			float dDefaultVal = (float)spine->pblock->GetInt(SpineData2::PB_SETUPABS_REL_DEPRECATED);
			layerAbsRel->SetSetupVal((void*)&dDefaultVal);
			for (int i = 0; i < layerAbsRel->GetNumLayers(); i++) {
				// Wemay be in the middle of upgrading from CAT1, so don't access the layer properties directly
				// Call GetLayerMethod
				if (layerAbsRel->GetRoot()->GetLayerMethod(i) == LAYER_ABSOLUTE || layerAbsRel->GetRoot()->GetLayerMethod(i) == LAYER_CATMOTION || (GetFileSaveVersion() < CAT_VERSION_1700 && i == 0)) {
					layerAbsRel->GetLayer(i)->SetValue(0, (void*)&dDefaultVal);
				}
			}
			spine->ReplaceReference(SpineData2::LAYERABS_REL, (RefTargetHandle)layerAbsRel);

			iload->SetObsolete();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

/**********************************************************************
 * Loading and saving....
 */
#define SPINEDATA_FLAGS				1
#define SPINEDATA_CATCONTROL		2

IOResult SpineData2::Save(ISave *isave)
{
	DWORD nb;

	isave->BeginChunk(SPINEDATA_CATCONTROL);
	CATControl::Save(isave);
	isave->EndChunk();

	// Save out our flags
	isave->BeginChunk(SPINEDATA_FLAGS);
	isave->Write(&m_flags, sizeof(m_flags), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult SpineData2::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case SPINEDATA_CATCONTROL:
			res = CATControl::Load(iload);
			break;

		case SPINEDATA_FLAGS:
			res = iload->Read(&m_flags, sizeof(m_flags), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new SpineDataPLCB(this));
	iload->RegisterPostLoadCallback(new BoneGroupPostLoadIDPatcher(this, PB_SPINEID_DEPRECATED));

	return IO_OK;
}

BOOL SpineData2::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	SpineData2* pasteSpine = (SpineData2*)pastectrl;

	// paste any setting such as Abs/Rel
	this->m_flags = pasteSpine->m_flags;

	pblock->EnableNotifications(FALSE);
	layerAbsRel->SetSetupVal(pasteSpine->layerAbsRel->GetSetupVal());

	Matrix3 tmSpineBase = pasteSpine->pblock->GetMatrix3(PB_BASE_TRANSFORM);
	if (flags&PASTERIGFLAG_MIRROR) {
		if (GetLengthAxis() == Z)	MirrorMatrix(tmSpineBase, kXAxis);
		else									MirrorMatrix(tmSpineBase, kZAxis);
	}
	tmSpineBase.SetTrans(tmSpineBase.GetTrans() * scalefactor);
	pblock->SetValue(PB_BASE_TRANSFORM, 0, tmSpineBase);

	int newnumlinks = pasteSpine->pblock->GetInt(PB_NUMLINKS);
	pblock->SetValue(PB_NUMLINKS, 0, newnumlinks);

	CATWeight* twistWeight = (CATWeight*)pblock->GetControllerByID(PB_SPINEWEIGHT);
	CATWeight* pasteTwistWeight = (CATWeight*)pasteSpine->pblock->GetControllerByID(PB_SPINEWEIGHT);
	twistWeight->PasteRig(pasteTwistWeight);

	UpdateSpineCurvatureWeights();

	pblock->EnableNotifications(TRUE);

	CreateLinks();

	// To avoid an infinite loop, we avoid pasting onto the tip hub
	int iNumBones = GetNumBones();
	for (int j = 0; j < iNumBones; j++)
		if (GetSpineBone(j)) GetSpineBone(j)->PasteRig(pasteSpine->GetSpineBone(j), flags, scalefactor);

	return TRUE;
}

BOOL SpineData2::BuildMapping(CATControl* pastectrl, CATCharacterRemap &remap, BOOL includeERNnodes)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;

	SpineData2 *pastespine = (SpineData2*)pastectrl;

	remap.AddEntry(pastectrl, this);
	int i;
	// Build a mapping o
	for (i = 0; i < NumLayerControllers(); i++) {
		remap.AddEntry(pastectrl->GetLayerController(i), GetLayerController(i));
	}
	int iNumBones = min(GetNumBones(), pastespine->GetNumBones());
	for (i = 0; i < iNumBones; i++) {
		SpineTrans2* spinebone = GetSpineBone(i);
		SpineTrans2* pastespinebone = pastespine->GetSpineBone(i);
		if (spinebone && pastespinebone)
			spinebone->BuildMapping(pastespinebone, remap, includeERNnodes);
	}

	if (GetTipHub() && pastespine->GetTipHub()) {
		GetTipHub()->BuildMapping(pastespine->GetTipHub(), remap, includeERNnodes);
	}

	return TRUE;
}

BOOL SpineData2::PasteLayer(CATControl* pastectrl, int fromindex, int toindex, DWORD flags, RemapDir &remap)
{
	if (ClassID() != pastectrl->ClassID()) return FALSE;
	// you cannot paste a layer onto its self
	if ((pastectrl == this) && (fromindex == toindex))return FALSE;

	SpineData2 *pastespine = (SpineData2*)pastectrl;

	int i;
	for (i = 0; i < NumLayerControllers(); i++) {
		CATClipValue* pastelayerctrl = pastectrl->GetLayerController(i);
		if (GetLayerController(i) && pastelayerctrl) {
			GetLayerController(i)->PasteLayer(pastelayerctrl, fromindex, toindex, flags, remap);
		}
	}

	// some bones may have more or less arbitrary bones attached, adn so we can only paste the minimum
	int iNumBones = min(GetNumBones(), pastespine->GetNumBones());
	for (i = 0; i < iNumBones; i++) {
		SpineTrans2* spinebone = GetSpineBone(i);
		SpineTrans2* pastespinebone = pastespine->GetSpineBone(i);
		if (spinebone && pastespinebone)
			spinebone->PasteLayer(pastespinebone, fromindex, toindex, flags, remap);
	}
	if (GetTipHub() && pastespine->GetTipHub())
		GetTipHub()->PasteLayer(pastespine->GetTipHub(), fromindex, toindex, flags, remap);
	return TRUE;
}

void SpineData2::SetSpineFK(BOOL toFK)
{
	int iNumBones = GetNumBones();
	Hub* pTipHub = GetTipHub();
	if (pTipHub == NULL)
		return;

	// Do not pop dialogs in silent mode
	if (!GetCOREInterface()->GetQuietMode())
	{
		// Do we really want to go ahead with this?
		if (toFK && !TestFlag(SPINEFLAG_FKSPINE))
		{
			TCHAR buf[512] = { 0 };
			_tcscpy(buf, GetString(IDS_ERR_FKSPINES1));
			_tcscat(buf, GetString(IDS_ERR_FKSPINES2));
			_tcscat(buf, GetString(IDS_ERR_FKSPINES3));
			_tcscat(buf, GetString(IDS_ERR_FKSPINES4));
			_tcscat(buf, GetString(IDS_ERR_FKSPINES5));
			_tcscat(buf, GetString(IDS_ERR_FKSPINES6));
			_tcscat(buf, GetString(IDS_ERR_FKSPINES7));
			DWORD dwResult = ::MessageBox(GetCOREInterface()->GetMAXHWnd(), buf,
				GetString(IDS_ERR_FKSPINES8), MB_YESNO | MB_ICONASTERISK);

			if (dwResult == IDNO) {
				UpdateUI();
				return;
			}
		}
	}

	// Before making changes, cache the current transforms
	TimeValue t = GetCOREInterface()->GetTime();
	Tab<Matrix3> dAllSpineTMs;
	dAllSpineTMs.SetCount(iNumBones);
	for (int j = 0; j < iNumBones; j++)
	{
		SpineTrans2* pSpineLink = GetSpineBone(j);
		if (pSpineLink == NULL)
			continue;

		dAllSpineTMs[j] = pSpineLink->GetNodeTM(t);
	}
	// Cache the tip tm, so we can set it back later.
	Matrix3 tmTipHub = pTipHub->GetNodeTM(0);

	// Make the switch
	if (toFK)
	{
		if (ipbegin && flagsbegin == 0) {
			// Kill the subobject modes
			ipbegin->DeleteMode(moveMode);
			delete moveMode; moveMode = NULL;

			ipbegin->DeleteMode(rotateMode);
			delete rotateMode; rotateMode = NULL;
		}

		SetFlag(SPINEFLAG_FKSPINE);

		// all hubs inherit rot fromm the CATParent
		pTipHub->GetLayerTrans()->ClearFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
		pTipHub->GetLayerTrans()->ClearFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT);
		// make it work like any other bone
		pTipHub->SetCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
		pTipHub->SetCCFlag(CNCFLAG_LOCK_LOCAL_POS);
		pTipHub->SetCCFlag(CCFLAG_EFFECT_HIERARCHY);

		for (int j = 0; j < iNumBones; j++)
		{
			SpineTrans2* pSpineLink = GetSpineBone(j);
			if (pSpineLink == NULL)
				continue;

			pSpineLink->CreateLayerTransformController();
		}
	}
	else
	{
		// all hubs inherit rot from the CATParent
		pTipHub->GetLayerTrans()->SetFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT);
		// make it work like any other bone
		pTipHub->ClearCCFlag(CNCFLAG_LOCK_SETUPMODE_LOCAL_POS);
		pTipHub->ClearCCFlag(CNCFLAG_LOCK_LOCAL_POS);
		pTipHub->ClearCCFlag(CCFLAG_EFFECT_HIERARCHY);

		for (int j = 0; j < iNumBones; j++)
		{
			SpineTrans2* pSpineLink = GetSpineBone(j);
			if (pSpineLink == NULL)
				continue;

			// Cache tmSetup before and after creation
			pSpineLink->DestroyLayerTransformController();
		}

		ClearFlag(SPINEFLAG_FKSPINE);
		ClearFlag(SPINEFLAG_RELATIVESPINE);

	}

	// For procedural spines, we need to set the ribcages transform first
	// This is because once the ribcages rotation is set correctly,
	// the spine links will correctly set their requested SetupTM
	// However if we set the ribcage 2nd, the rotation change
	// would modify the positions, and it not guaranteed that the
	// following SetValue would find the correct position again.
	if (!toFK)
		pTipHub->SetNodeTM(t, tmTipHub);

	for (int j = 0; j < iNumBones; j++)
	{
		SpineTrans2* pSpineLink = GetSpineBone(j);
		if (pSpineLink == NULL)
			continue;

		pSpineLink->SetNodeTM(t, dAllSpineTMs[j]);
	}

	// For FK spines, we need to set this value after the spine links
	// have all set their values correctly.  This is because it inherits
	// in the usual way from the spinelinks, and setting its value before
	// the spinelinks would mean it inherits the wrong values.
	if (toFK)
	{
		// Note: Remove current transform from Hub
		// This is a nasty hack.  The problem is because a procedural
		// spine uses the Hubs transform in a different way, and it
		// will likely have a lot of position offset on it.  When
		// we switch to FK, its possible for the Hub to query its own
		// NodeTM in the SetValue (in ApplySetValueLock, if any locks are set)
		// and this will evaluate the current NodeTM including the procedural-style
		// position offset in a FK style straight transform.  This
		// means our procedural positional offset can become baked
		// into the FK SetValue, because the Lock will replace the incoming
		// position with the newly calculated one.
		// Resetting the transform to the identity, we remove the positional
		// offset (and any other offsets) in case the Hub queries its
		// own node position.
		//
		// Note:
		// If the node doesn't query its own transform, this will have no effect.
		Point3 p3ParentScale(P3_IDENTITY_SCALE);
		SetXFormPacket hackPacket(Matrix3(1));
		if (pTipHub->GetLayerTrans() != NULL)
			pTipHub->GetLayerTrans()->SetValue(t, Matrix3(1), p3ParentScale, hackPacket);

		// Now safely set.
		pTipHub->SetNodeTM(t, tmTipHub);
	}

	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED, NOTIFY_ALL, FALSE);
}

// this function isint as fast as it could be but its a trickey problem
int SpineData2::GetKeyTimes(Tab<TimeValue>& times, Interval range, DWORD flags)
{
	if (!TestCCFlag(CNCFLAG_RETURN_EXTRA_KEYS))
		return CATControl::GetKeyTimes(times, range, flags);
	return GetTipHub()->GetKeyTimes(times, range, flags);
}

BOOL SpineData2::IsKeyAtTime(TimeValue t, DWORD flags) {
	if (!TestCCFlag(CNCFLAG_RETURN_EXTRA_KEYS))
		return CATControl::IsKeyAtTime(t, flags);
	return GetTipHub()->IsKeyAtTime(t, flags);
}

BOOL SpineData2::GetNextKeyTime(TimeValue t, DWORD flags, TimeValue &nt) {
	if (!TestCCFlag(CNCFLAG_RETURN_EXTRA_KEYS))
		return CATControl::GetNextKeyTime(t, flags, nt);
	return GetTipHub()->GetNextKeyTime(t, flags, nt);
}

//////////////////////////////////////////////////////////////////////
// Sub-Object Selection
MoveCtrlApparatusCMode*    SpineData2::moveMode = NULL;
RotateCtrlApparatusCMode*    SpineData2::rotateMode = NULL;
static GenSubObjType SOT_ProjVector(23);

// selection levels:
#define SEL_OBJECT	0
#define SEL_PIVOT	1

void SpineData2::DrawGizmo(TimeValue /*t*/, GraphicsWindow *gw, const Matrix3& tmParent) {

	gw->setTransform(Matrix3(1));		// sets the graphicsWindow to world

	bbox.Init();

	////////
	gw->setColor(LINE_COLOR, GetUIColor(COLOR_TARGET_LINE));

	DbgAssert(GetBaseHub());
	float length = GetSpineLength() * GetCATUnits();
	Point3 p1, p2;

	Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	tmSpineBase.SetTrans(GetCATUnits() * tmSpineBase.GetTrans());
	Matrix3 tm = tmSpineBase * tmParent;

	p1 = tm.GetTrans();

	// move the matrix up the spine
	if (GetLengthAxis() == Z)
		tm.PreTranslate(Point3(0.0f, 0.0f, length));
	else tm.PreTranslate(Point3(length, 0.0f, 0.0f));

	p2 = tm.GetTrans();

	dLine2Pts(gw, p1, p2); bbox += p1; bbox += p2;

}

int SpineData2::HitTest(
	TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	UNREFERENCED_PARAMETER(flags);
	Matrix3 tm(1);
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0, 0, 0);

	GraphicsWindow *gw = vpt->getGW();
	gw->setTransform(tm);
	tm = inode->GetObjectTM(t);
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	//	DrawAndHit(t, inode, vpt);
	DrawGizmo(t, gw, inode->GetParentTM(t));

	gw->setRndLimits(savedLimits);

	return gw->checkHitCode();
}

int SpineData2::Display(TimeValue /*t*/, INode*, ViewExp* /*vpt*/, int /*flags*/)
{
	return 1;
}

void SpineData2::GetWorldBoundBox(TimeValue, INode *, ViewExp*, Box3& box)
{
	box += bbox;
}

void SpineData2::ActivateSubobjSel(int level, XFormModes& modes)
{
	// Set the meshes level
	selLevel = level;

	if (level == SEL_PIVOT) {
		modes = XFormModes(moveMode, rotateMode, NULL, NULL, NULL, NULL);
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void SpineData2::SelectSubComponent(CtrlHitRecord *hitRec, BOOL selected, BOOL all, BOOL invert)
{
	UNREFERENCED_PARAMETER(selected); UNREFERENCED_PARAMETER(all); UNREFERENCED_PARAMETER(invert);
	//	HoldTrack();
	while (hitRec) {
		//		if (selected) {
		//			sel.Set(hitRec->hitInfo);
		//		} else {
		//			sel.Clear(hitRec->hitInfo);
		//			}
		//		if (all)
		hitRec = hitRec->Next();
		//		else break;
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void SpineData2::ClearSelection(int selLevel)
{
	UNREFERENCED_PARAMETER(selLevel);
	//	HoldTrack();
	//	sel.ClearAll();
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

int SpineData2::SubObjectIndex(CtrlHitRecord *hitRec)
{
	UNREFERENCED_PARAMETER(hitRec);
	//	for (ulong i=0; i<hitRec->hitInfo; i++) {
	//		if (sel[i]) count++;
	//		}
	return 1;//count;
}
void SpineData2::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node)
{
	if (selLevel == SEL_OBJECT) return;	// shouldn't happen.

	Point3 center(0, 0, 0);

	Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	tmSpineBase.SetTrans(GetCATUnits() * tmSpineBase.GetTrans());
	Matrix3 tm = tmSpineBase * node->GetParentTM(t);

	center = tm.GetTrans();
	cb->Center(center, 0);
}

void SpineData2::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node)
{
	Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	tmSpineBase.SetTrans(GetCATUnits() * tmSpineBase.GetTrans());
	Matrix3 tm = tmSpineBase * node->GetParentTM(t);

	cb->TM(tm, 0);
}

void SpineData2::SubMove(TimeValue, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL /*localOrigin*/)
{
	Point3 pos = VectorTransform(tmAxis*Inverse(partm), val);
	Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	pos /= GetCATParentTrans()->GetCATUnits();
	tmSpineBase.Translate(pos);
	pblock->SetValue(PB_BASE_TRANSFORM, 0, tmSpineBase);
}

void SpineData2::SubRotate(TimeValue, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL /*localOrigin*/)
{
	AngAxis axSetVal = AngAxis(val);
	axSetVal.axis = Normalize(VectorTransform(tmAxis*Inverse(partm), axSetVal.axis));

	Matrix3 tmSpineBase = pblock->GetMatrix3(PB_BASE_TRANSFORM);
	Point3 currSetPos = tmSpineBase.GetTrans();
	RotateMatrix(tmSpineBase, axSetVal);
	tmSpineBase.SetTrans(currSetPos);
	pblock->SetValue(PB_BASE_TRANSFORM, 0, tmSpineBase);

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

BaseInterface* SpineData2::GetInterface(Interface_ID id)
{
	if (id == I_SPINE_FP) return static_cast<ISpineFP*>(this);
	else if (id == BONEGROUPMANAGER_INTERFACE_FP) return static_cast<IBoneGroupManagerFP*>(this);
	return CATControl::GetInterface(id);
}

void SpineData2::SetFlag(DWORD f)
{
	HoldData(m_flags);
	m_flags |= f;
};

void SpineData2::ClearFlag(DWORD f)
{
	HoldData(m_flags);
	m_flags = m_flags & ~f;
}
