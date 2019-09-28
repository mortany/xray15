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

#include "ProxyTransform.h"

static bool interfaceAdded = false;
class GizmoTransformClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE)// {return new GizmoTransform(loading); }
	{
		GizmoTransform* ctrl = new GizmoTransform(loading);
		if (!interfaceAdded)
		{
			AddInterface(ctrl->GetDescByID(I_GIZMOTRANSFORM_FP));
			interfaceAdded = true;
		}
		return ctrl;
	}
	const TCHAR *	ClassName() { return _T("CATGizmoTransform"); } //GetString(IDS_CL_LAYER_TRANSFORM); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID		ClassID() { return GIZMOTRANSFORM_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATGizmoTransform"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle

};

static GizmoTransformClassDesc GizmoTransformDesc;
ClassDesc2* GetGizmoTransformDesc() { return &GizmoTransformDesc; }

/* ------------------ CommandMode classes for node picking ----------------------- */

class GizmoTransformPickMode : public PickModeCallback
{
private:
	GizmoTransform *ctrlGizmoTransform;
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
		ctrlGizmoTransform->SetTargetNode(vpt->GetClosestHit());
		return TRUE;
	}

	void setup(GizmoTransform *ctrlGizmoTransform, const TCHAR* msg, ICustButton* pnb)
	{
		this->ctrlGizmoTransform = ctrlGizmoTransform;
		this->msg = msg;
		this->pnb = pnb;
	}
};

static GizmoTransformPickMode pick_mode;

class GizmoTransformRolloutData : TimeChangeCallback
{
	HWND hDlg;
	GizmoTransform	*ctrlGizmoTransform;
	ICustButton		*btnTargetNode;
	CatDotIni		*catCfg;

public:

	GizmoTransformRolloutData() : catCfg(NULL), hDlg(NULL)
	{
		ctrlGizmoTransform = NULL;
		btnTargetNode = NULL;
	}

	HWND GetHWnd() { return hDlg; }

	void TimeChanged(TimeValue)
	{
	}

	BOOL InitControls(HWND hWnd, GizmoTransform* GizmoTransform)
	{
		hDlg = hWnd;

		ctrlGizmoTransform = GizmoTransform;

		// Initialise the INI files so we can read button text and tooltips
		catCfg = new CatDotIni;

		// PathNode
		btnTargetNode = GetICustButton(GetDlgItem(hDlg, IDC_BTN_TARGETNODE));
		btnTargetNode->SetType(CBT_CHECK);
		btnTargetNode->SetButtonDownNotify(TRUE);
		btnTargetNode->SetTooltip(TRUE, catCfg->Get(_T("ToolTips"), _T("btnGizmoTargetNode"), GetString(IDS_TT_PICKNODEGIZMO_XFM)));

		INode* node = ctrlGizmoTransform->GetTargetNode();
		TSTR strbuttontext = catCfg->Get(_T("ButtonTexts"), _T("btnGizmoTargetNode"), GetString(IDS_BTN_TARGOBJ));
		if (node) strbuttontext = strbuttontext + _T(":") + node->GetName();
		btnTargetNode->SetText(strbuttontext);

		TimeChanged(GetCOREInterface()->GetTime());
		return TRUE;
	}

	void ReleaseControls(HWND hWnd) {

		if (hDlg != hWnd) return;

		SAFE_RELEASE_BTN(btnTargetNode);

		hDlg = NULL;
		ctrlGizmoTransform = NULL;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (GizmoTransform*)lParam);
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
					if (btnTargetNode->IsChecked())
					{
						TSTR msg = catCfg->Get(_T("ToolTips"), _T("btnTargetNode"), GetString(IDS_TT_PICKNODEGIZMO_CTRL));
						pick_mode.setup(ctrlGizmoTransform, msg, btnTargetNode);
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

static GizmoTransformRolloutData staticGizmoTransformRollout;

static INT_PTR CALLBACK GizmoTransformRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticGizmoTransformRollout.DlgProc(hWnd, message, wParam, lParam);
};

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc proxytransform_FPinterface(
	I_GIZMOTRANSFORM_FP, _T("IGizmoTransformFPInterface"), 0, NULL, FP_MIXIN,

	properties,

	IGizmoTransformFP::propGetTargetNode, IGizmoTransformFP::propSetTargetNode, _T("TargetNode"), 0, TYPE_INODE,

	p_end
);

FPInterfaceDesc* GizmoTransform::GetDescByID(Interface_ID id) {
	if (id == I_GIZMOTRANSFORM_FP) return &proxytransform_FPinterface;
	return NULL;
}

static void GizmoNodeDeleted(void *param, NotifyInfo *info)
{
	GizmoTransform *gizmotm = (GizmoTransform*)param;
	if (gizmotm == NULL) return;
	if (!CATEvaluationLock::IsEvaluationLocked() && (info != NULL) && (gizmotm->GetTargetNode() != NULL)) {
		INode* deleted_node = (INode*)info->callParam;
		if (deleted_node == gizmotm->GetTargetNode())
			gizmotm->SetTargetNode(NULL);
	}
}

/*
static void GizmoDeSelected(void *param, NotifyInfo *info)
{
	GizmoTransform *gizmotm = (GizmoTransform*)param;
	Interface *ip = GetCOREInterface();
	if(!gizmotm) return;

	ULONG handle;
	gizmotm->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
	INode *node = ip->GetINodeByHandle(handle);
	if(node){
		if(ip->GetSelNodeCount()==1 && node->Selected()){
			// We have just been selected
		//	gizmotm->isselected = TRUE;

			int cid = ip->GetCommandMode()->ID();
			// Only change the mode if we are in
			if(gizmotm->commandmode!=PICK_COMMAND && (cid==CID_OBJMOVE || cid==CID_OBJROTATE)){
				ip->SetRefCoordSys(gizmotm->coordsys);
				ip->SetStdCommandMode(gizmotm->commandmode);
			}
		}else if (gizmotm->isselected && !node->Selected()){
			gizmotm->isselected = FALSE;
		}
	}
}
*/

GizmoTransform::GizmoTransform(BOOL loading) {
	UNREFERENCED_PARAMETER(loading);
	target_node = NULL;
	tmOffset = Matrix3(1);
	ipbegin = NULL;
	isselected = FALSE;
	commandmode = PICK_COMMAND;
	coordsys = COORDS_LOCAL;

	RegisterNotification(GizmoNodeDeleted, this, NOTIFY_SCENE_PRE_DELETED_NODE);
	//	RegisterNotification(GizmoDeSelected,		this, NOTIFY_SELECTIONSET_CHANGED);
}

GizmoTransform::~GizmoTransform()
{
	UnRegisterNotification(GizmoNodeDeleted, this, NOTIFY_SCENE_PRE_DELETED_NODE);
	//	UnRegisterNotification(GizmoDeSelected,		this, NOTIFY_SELECTIONSET_CHANGED);

	DeleteAllRefs();
}

RefTargetHandle GizmoTransform::Clone(RemapDir& remap)
{
	// make a new datamusher object to be the clone
	GizmoTransform *newGizmoTransform = new GizmoTransform(FALSE);

	// If someone tries to clone us, then we will clone the whole CATRig
	newGizmoTransform->ReplaceReference(TARGETNODE, target_node);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newGizmoTransform, remap);

	// now return the new object.
	return newGizmoTransform;
}

void GizmoTransform::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	/*	if(!isselected){
			Interface *ip = GetCOREInterface();
			ULONG handle;
			NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
			INode *node = ip->GetINodeByHandle(handle);

			if(node && node->Selected()){
				// We have just been selected
				isselected = TRUE;

				int cid = ip->GetCommandMode()->ID();
				// Only change the mode if we are in
			//	if(commandmode!=PICK_COMMAND && (cid==CID_OBJMOVE || cid==CID_OBJROTATE)){
			//		ip->SetRefCoordSys(coordsys);
			//		ip->SetStdCommandMode(commandmode);
				if(cid==CID_OBJMOVE || cid==CID_OBJROTATE){
					commandmode = cid;
					coordsys = ip->GetRefCoordSys();
				}
			}
		}
	*/	if (target_node) {
		SetXFormPacket *ptr = (SetXFormPacket*)val;
		if (target_node->GetParentNode()->IsRootNode()) {
			ptr->tmParent.IdentityMatrix();
		}
		else {
			Interval iv = FOREVER;
			ptr->tmParent = target_node->GetParentNode()->GetNodeTM(t, &iv);
		}
		target_node->GetTMController()->SetValue(t, val, commit, method);
	}
}

bool GizmoTransform::GetLocalTMComponents(TimeValue t, TMComponentsArg& cmpts, Matrix3Indirect& parentMatrix)
{
	if (target_node) {
		return target_node->GetTMController()->GetLocalTMComponents(t, cmpts, parentMatrix);
	}
	return false;
};

void GizmoTransform::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod)
{
	if (target_node) {
		*(Matrix3*)val = target_node->GetNodeTM(t, &valid);
	}
}

void GizmoTransform::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;
	if (flags&BEGIN_EDIT_LINKINFO) {
		if (!target_node) {
			ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_GIZMOTRANSFORM), &GizmoTransformRolloutProc, GetString(IDS_GIZMO_TRANS_PARAMS), (LPARAM)this);
		}
	}
	if (target_node) {
		target_node->GetTMController()->BeginEditParams(ip, flags, prev);
	}
}

void GizmoTransform::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (staticGizmoTransformRollout.GetHWnd())
		ip->DeleteRollupPage(staticGizmoTransformRollout.GetHWnd());

	if (target_node) {
		target_node->GetTMController()->EndEditParams(ip, flags, next);
	}
	ipbegin = NULL;
}

void GizmoTransform::SetReference(int i, RefTargetHandle rtarg) {
	if (i == TARGETNODE) target_node = (INode*)rtarg;
	if (target_node && ipbegin) {
		target_node->GetTMController()->BeginEditParams(ipbegin, flagsbegin, NULL);
	}
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

RefResult GizmoTransform::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	if (msg == REFMSG_TARGET_DELETED && hTarg == target_node) target_node = NULL;

	return REF_SUCCEED;
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class GizmoTransformPLCB : public PostLoadCallback {
protected:
	GizmoTransform *bone;

public:
	GizmoTransformPLCB(GizmoTransform *pOwner) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		return bone->GetVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *) {
		delete this;
	}
};

#define		GIZMOTRANSFORM_VERSION			0
#define		GIZMOTRANSFORM_OFFSETTM			1
#define		GIZMOTRANSFORM_COMMANDMODE		2
#define		GIZMOTRANSFORM_COORDSYS			3

IOResult GizmoTransform::Save(ISave *isave)
{
	//	ULONG id;
	DWORD nb;//, r

	isave->BeginChunk(GIZMOTRANSFORM_VERSION);
	isave->Write(&dwVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	isave->BeginChunk(GIZMOTRANSFORM_OFFSETTM);
	isave->Write(&tmOffset, sizeof(Matrix3), &nb);
	isave->EndChunk();

	isave->BeginChunk(GIZMOTRANSFORM_COMMANDMODE);
	isave->Write(&commandmode, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(GIZMOTRANSFORM_COORDSYS);
	isave->Write(&coordsys, sizeof(int), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult GizmoTransform::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;
	//	ULONG id;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case GIZMOTRANSFORM_VERSION:
			res = iload->Read(&dwVersion, sizeof(DWORD), &nb);
			break;
		case GIZMOTRANSFORM_OFFSETTM:
			res = iload->Read(&tmOffset, sizeof(Matrix3), &nb);
			break;
		case GIZMOTRANSFORM_COMMANDMODE:
			res = iload->Read(&commandmode, sizeof(int), &nb);
			break;
		case GIZMOTRANSFORM_COORDSYS:
			res = iload->Read(&coordsys, sizeof(int), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new GizmoTransformPLCB(this));

	return IO_OK;
}

