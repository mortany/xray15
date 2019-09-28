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
 // Max includes
#include "decomp.h"

#include "ICATParent.h"
#include "CATNodeControl.h"
#include "CATUnitsPosition.h"

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc proxytransform_FPinterface(
	I_CATUNITSCTRL_FP, _T("ICATUnitsCtrlFPInterface"), 0, NULL, FP_MIXIN,

	properties,

	ICATUnitsCtrlFP::propGetCATParentNode, ICATUnitsCtrlFP::propSetCATParentNode, _T("CATParentNode"), 0, TYPE_INODE,

	p_end
);

/**********************************************************************
 * UI
 */
 // ------------------ CommandMode classes for node picking ----------------------- */

class CATUnitsCtrlPickMode : public PickModeCallback
{
private:
	CATUnitsCtrl *ctrlCATUnitsCtrl;
	const TCHAR*				msg;
	ICustButton*		pnb;

public:

	PickNodeCallback *GetFilter() { return NULL; }
	BOOL	RightClick(IObjParam *, ViewExp *) { return TRUE; }

	void EnterMode(IObjParam *ip)
	{
		if (pnb != NULL)	pnb->SetCheck(TRUE);
		if (msg != NULL)	ip->PushPrompt(msg);
	}

	void ExitMode(IObjParam *ip)
	{
		if (pnb != NULL)	pnb->SetCheck(FALSE);
		if (msg != NULL)	ip->PopPrompt();
	}

	// the user is moving the mouse over the window
	BOOL HitTest(IObjParam *ip, HWND hWnd, ViewExp *, IPoint2 m, int flags)
	{
		UNREFERENCED_PARAMETER(flags);
		return ip->PickNode(hWnd, m, NULL) ? TRUE : FALSE;
	}

	// The user has picked something
	BOOL Pick(IObjParam *, ViewExp *vpt)
	{
		ctrlCATUnitsCtrl->SetCATParentNode(vpt->GetClosestHit());
		return TRUE;
	}

	void setup(CATUnitsCtrl *ctrlCATUnitsCtrl, const TCHAR* msg, ICustButton* pnb)
	{
		this->ctrlCATUnitsCtrl = ctrlCATUnitsCtrl;
		this->msg = msg;
		this->pnb = pnb;
	}
};

static CATUnitsCtrlPickMode pick_mode;

class CATUnitsCtrlRolloutData : TimeChangeCallback
{
	HWND hDlg;
	CATUnitsCtrl		*ctrlCATUnitsCtrl;
	ICustButton		*btnCATParentNode;
	CatDotIni		*catCfg;

public:

	CATUnitsCtrlRolloutData() : catCfg(NULL), hDlg(NULL)
	{
		ctrlCATUnitsCtrl = NULL;
		btnCATParentNode = NULL;
	}

	HWND GetHWnd() { return hDlg; }

	void TimeChanged(TimeValue)
	{
	}

	BOOL InitControls(HWND hWnd, CATUnitsCtrl* CATUnitsCtrl)
	{
		hDlg = hWnd;

		ctrlCATUnitsCtrl = CATUnitsCtrl;

		// Initialise the INI files so we can read button text and tooltips
		catCfg = new CatDotIni;

		// PathNode
		btnCATParentNode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_TARGETNODE));
		btnCATParentNode->SetType(CBT_CHECK);
		btnCATParentNode->SetButtonDownNotify(TRUE);
		btnCATParentNode->SetTooltip(TRUE, catCfg->Get(_T("ToolTips"), _T("btnGizmoCATParentNode"), GetString(IDS_TT_PICKNODEGIZMO_XFM)));

		INode* node = ctrlCATUnitsCtrl->GetCATParentNode();
		TSTR strbuttontext = catCfg->Get(_T("ButtonTexts"), _T("btnCATParentNode"), GetString(IDS_BTN_CATPARENT));
		if (node) strbuttontext = strbuttontext + _T(":") + node->GetName();
		btnCATParentNode->SetText(strbuttontext);

		TimeChanged(GetCOREInterface()->GetTime());
		return TRUE;
	}

	void ReleaseControls(HWND hWnd) {

		if (hDlg != hWnd) return;

		SAFE_RELEASE_BTN(btnCATParentNode);
		SAFE_DELETE(catCfg);

		hDlg = NULL;
		ctrlCATUnitsCtrl = NULL;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (CATUnitsCtrl*)lParam);
			GetCOREInterface()->RegisterTimeChangeCallback(this);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			GetCOREInterface()->UnRegisterTimeChangeCallback(this);
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_BUTTONUP:
				switch (LOWORD(wParam)) {
				case IDC_BTN_TARGETNODE:
					if (btnCATParentNode->IsChecked())
					{
						TSTR msg = catCfg->Get(_T("ToolTips"), _T("btnCATParentNode"), GetString(IDS_TT_PICKNODEGIZMO_CTRL));
						pick_mode.setup(ctrlCATUnitsCtrl, msg, btnCATParentNode);
						GetCOREInterface()->ClearPickMode();
						GetCOREInterface()->SetPickMode(&pick_mode);
					}
					break;
				}
				break;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
};

static CATUnitsCtrlRolloutData staticCATUnitsCtrlRollout;

static INT_PTR CALLBACK CATUnitsCtrlRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticCATUnitsCtrlRollout.DlgProc(hWnd, message, wParam, lParam);
};

/**********************************************************************
 * CATUnitsCtrl
 */

FPInterfaceDesc* CATUnitsCtrl::GetDescByID(Interface_ID id) {
	if (id == I_CATUNITSCTRL_FP) return &proxytransform_FPinterface;
	return NULL;
}

void CATUnitsCtrl::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;

	//	if(flags&BEGIN_EDIT_LINKINFO){
	ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_GIZMOTRANSFORM), &CATUnitsCtrlRolloutProc, GetString(IDS_CATUNITS_CTRL_PARAMS), (LPARAM)this);
	//	}
	if (ctrl) ctrl->BeginEditParams(ip, flags, prev);
}

void CATUnitsCtrl::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (staticCATUnitsCtrlRollout.GetHWnd())
		ip->DeleteRollupPage(staticCATUnitsCtrlRollout.GetHWnd());

	if (ctrl) ctrl->EndEditParams(ip, flags, next);
	ipbegin = NULL;
}

void CATUnitsCtrl::Init() {
	catparent_node = NULL;
	ctrl = NULL;
	//	tmOffset = Matrix3(1);
	ipbegin = NULL;

}

void CATUnitsCtrl::Copy(Control *from)
{
	// Find the current node using the handlemessage system
	INode *node = FindReferencingClass<INode>(this);

	if (node) {
		CATNodeControl *ctrl = (CATNodeControl*)node->GetTMController()->GetInterface(I_CATNODECONTROL);
		if (ctrl) {
			ReplaceReference(REF_CATPARENTNODE, ctrl->GetCATParentTrans()->GetNode());
			Point3 val;
			Interval iv = FOREVER;
			from->GetValue(GetCOREInterface()->GetTime(), (void*)&val, iv, CTRL_ABSOLUTE);
			val /= ctrl->GetCATParentTrans()->GetCATUnits();
			from->SetValue(GetCOREInterface()->GetTime(), (void*)&val, 1, CTRL_ABSOLUTE);
		}
	}

	//	GetValu
		// If someone tries to clone us, then we will clone the whole CATRig
	ReplaceReference(REF_CTRL, from);
}

RefTargetHandle CATUnitsCtrl::Clone(RemapDir& remap)
{
	// make a new datamusher object to be the clone
	CATUnitsPos *newCATUnitsPos = new CATUnitsPos(FALSE);

	newCATUnitsPos->ReplaceReference(REF_CTRL, remap.CloneRef(ctrl));
	newCATUnitsPos->ReplaceReference(REF_CATPARENTNODE, catparent_node);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATUnitsPos, remap);

	// now return the new object.
	return newCATUnitsPos;
}

RefResult CATUnitsCtrl::NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL)
{
	//	if(msg == REFMSG_TARGET_DELETED && hTarg == prs) prs = NULL;

	return REF_SUCCEED;
}

RefTargetHandle CATUnitsCtrl::GetReference(int i) {
	switch (i) {
	case REF_CTRL:			return ctrl;
	case REF_CATPARENTNODE:	return catparent_node;
	default:			return NULL;
	}
}

void CATUnitsCtrl::SetReference(int i, RefTargetHandle rtarg) {
	switch (i) {
	case REF_CTRL:			ctrl = (Control*)rtarg;			break;
	case REF_CATPARENTNODE:	catparent_node = (INode*)rtarg;	break;
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void CATUnitsCtrl::SetCATParentNode(INode* n)
{
	if (n)
	{
		ICATParentTrans *catparenttrans = (ICATParentTrans*)n->GetTMController()->GetInterface(I_CATPARENTTRANS);
		if (catparenttrans) {
			initCATUnits = catparenttrans->GetCATUnits();
		}
	}
	ReplaceReference(REF_CATPARENTNODE, n);
};

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class CATUnitsCtrlPLCB : public PostLoadCallback {
protected:
	CATUnitsCtrl *bone;

public:
	CATUnitsCtrlPLCB(CATUnitsCtrl *pOwner) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		return bone->GetVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *) {
		delete this;
	}
};

#define		CATUNITSPOS_VERSION			0
#define		CATUNITSPOS_INITCATUNITS	1

IOResult CATUnitsCtrl::Save(ISave *isave)
{
	DWORD nb;//, r

	isave->BeginChunk(CATUNITSPOS_VERSION);
	isave->Write(&dwVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(CATUNITSPOS_INITCATUNITS);
	isave->Write(&initCATUnits, sizeof(float), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult CATUnitsCtrl::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;
	//	ULONG id;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case CATUNITSPOS_VERSION:
			res = iload->Read(&dwVersion, sizeof(DWORD), &nb);
			break;
		case CATUNITSPOS_INITCATUNITS:
			res = iload->Read(&initCATUnits, sizeof(float), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new CATUnitsCtrlPLCB(this));

	return IO_OK;
}

/**********************************************************************
 * CATUnitsPos
 */

static bool pos_interfaceAdded = false;
class CATUnitsPosClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE)// {return new CATUnitsPos(loading); }
	{
		CATUnitsPos* ctrl = new CATUnitsPos(loading);
		if (!pos_interfaceAdded)
		{
			AddInterface(ctrl->GetDescByID(I_CATUNITSCTRL_FP));
			pos_interfaceAdded = true;
		}
		return ctrl;
	}
	const TCHAR *	ClassName() { return _T("CATUnitsPosition"); } //GetString(IDS_CL_LAYER_TRANSFORM); }
	SClass_ID		SuperClassID() { return CTRL_POSITION_CLASS_ID; }
	Class_ID		ClassID() { return CATUNITSPOS_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATUnitsPosition"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle

};

static CATUnitsPosClassDesc CATUnitsPosDesc;
ClassDesc2* GetCATUnitsPosDesc() { return &CATUnitsPosDesc; }

CATUnitsPos::CATUnitsPos(BOOL loading)
{
	Init();
	if (!loading) {
		ReplaceReference(REF_CTRL, NewDefaultPositionController());
	}
}

CATUnitsPos::~CATUnitsPos() {
	DeleteAllRefs();
}

void CATUnitsPos::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	if (catparent_node) {
		Point3 *p = ((Point3*)val);
		*p /= ((ICATParentTrans*)catparent_node->GetTMController()->GetInterface(I_CATPARENTTRANS))->GetCATUnits() / initCATUnits;
	}

	if (ctrl) {
		ctrl->SetValue(t, val, commit, method);
	}

}

void CATUnitsPos::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	if (ctrl) {
		Matrix3 tmParent;
		if (method == CTRL_RELATIVE) tmParent = *(Matrix3*)val;
		ctrl->GetValue(t, val, valid, method);
		if (catparent_node) {
			ICATParentTrans *catparenttrans = (ICATParentTrans*)catparent_node->GetTMController()->GetInterface(I_CATPARENTTRANS);
			// apply CATUnits to the position
			if (catparenttrans) {
				if (method == CTRL_ABSOLUTE)
					(*(Point3*)val) *= catparenttrans->GetCATUnits() / initCATUnits;
				else (*(Matrix3*)val).SetTrans(tmParent.GetTrans() + (((*(Matrix3*)val).GetTrans() - tmParent.GetTrans()) * (catparenttrans->GetCATUnits() / initCATUnits)));
			}
		}
	}
}

/**********************************************************************
 * CATUnitsScl
 */

static bool scl_interfaceAdded = false;
class CATUnitsSclClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE)// {return new CATUnitsScl(loading); }
	{
		CATUnitsScl* ctrl = new CATUnitsScl(loading);
		if (!scl_interfaceAdded)
		{
			AddInterface(ctrl->GetDescByID(I_CATUNITSCTRL_FP));
			scl_interfaceAdded = true;
		}
		return ctrl;
	}
	const TCHAR *	ClassName() { return _T("CATUnitsScale"); } //GetString(IDS_CL_LAYER_TRANSFORM); }
	SClass_ID		SuperClassID() { return CTRL_SCALE_CLASS_ID; }
	Class_ID		ClassID() { return CATUNITSSCL_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATUnitsScale"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle

};

static CATUnitsSclClassDesc CATUnitsSclDesc;
ClassDesc2* GetCATUnitsSclDesc() { return &CATUnitsSclDesc; }

CATUnitsScl::CATUnitsScl(BOOL loading)
{
	Init();
	initCATUnits = 1.0f;
	if (!loading) {
		ReplaceReference(REF_CTRL, NewDefaultScaleController());
	}
}

CATUnitsScl::~CATUnitsScl() {
	DeleteAllRefs();
}

void CATUnitsScl::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	if (ctrl) {
		ctrl->SetValue(t, val, commit, method);
	}
}

void CATUnitsScl::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	if (ctrl) {
		Matrix3 tmParent;
		if (method == CTRL_RELATIVE) tmParent = *(Matrix3*)val;
		ctrl->GetValue(t, val, valid, method);
		if (catparent_node) {
			ICATParentTrans *catparenttrans = (ICATParentTrans*)catparent_node->GetTMController()->GetInterface(I_CATPARENTTRANS);
			// apply CATUnits to the position
			if (catparenttrans) {
				if (method == CTRL_ABSOLUTE)
					(*(Point3*)val) *= catparenttrans->GetCATUnits() / initCATUnits;
				else {
					Matrix3 tmLocal = *(Matrix3*)val * Inverse(tmParent);
					AffineParts parts;
					decomp_affine(tmLocal, &parts);

					parts.k *= catparenttrans->GetCATUnits() / initCATUnits;

					ScaleValue sv = ScaleValue(parts.k*parts.f);
					ApplyScaling(tmLocal, sv);

					*(Matrix3*)val = tmLocal * Inverse(tmParent);
				}
			}
		}
	}
}

/**********************************************************************
 * CATUnitsFlt
 */

static bool flt_interfaceAdded = false;
class CATUnitsFltClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE)// {return new CATUnitsFlt(loading); }
	{
		CATUnitsFlt* ctrl = new CATUnitsFlt(loading);
		if (!flt_interfaceAdded)
		{
			AddInterface(ctrl->GetDescByID(I_CATUNITSCTRL_FP));
			flt_interfaceAdded = true;
		}
		return ctrl;
	}
	const TCHAR *	ClassName() { return _T("CATUnitsFloat"); } //GetString(IDS_CL_LAYER_TRANSFORM); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return CATUNITSFLT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATUnitsFloat"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle

};

static CATUnitsFltClassDesc CATUnitsFltDesc;
ClassDesc2* GetCATUnitsFltDesc() { return &CATUnitsFltDesc; }

CATUnitsFlt::CATUnitsFlt(BOOL loading)
{
	Init();
	initCATUnits = 1.0f;
	if (!loading) {
		ReplaceReference(REF_CTRL, NewDefaultScaleController());
	}
}

CATUnitsFlt::~CATUnitsFlt() {
	DeleteAllRefs();
}

void CATUnitsFlt::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	if (catparent_node) {
		float *f = ((float*)val);
		*f /= ((ICATParentTrans*)catparent_node->GetTMController()->GetInterface(I_CATPARENTTRANS))->GetCATUnits() / initCATUnits;
	}
	if (ctrl) {
		ctrl->SetValue(t, val, commit, method);
	}
}

void CATUnitsFlt::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	if (ctrl) {
		float fParent = 0.0f;
		if (method == CTRL_RELATIVE) fParent = *(float*)val;
		ctrl->GetValue(t, val, valid, method);
		if (catparent_node) {
			ICATParentTrans *catparenttrans = (ICATParentTrans*)catparent_node->GetTMController()->GetInterface(I_CATPARENTTRANS);
			// apply CATUnits to the position
			if (catparenttrans) {
				if (method == CTRL_ABSOLUTE)
					(*(float*)val) *= catparenttrans->GetCATUnits() / initCATUnits;
				else {
					(*(float*)val) = fParent + (((*(float*)val) - fParent) * (catparenttrans->GetCATUnits() / initCATUnits));
				}
			}
		}
	}
}

