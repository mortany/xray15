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

/**********************************************************************
	Simple controller attached to the limb's IK target
	All it is is a wrapper for the CATClipValue transform
	on PalmTrans, so we can call the right version of
	GetValue on it.
 **********************************************************************/

#include "CatPlugins.h"

#include "CATClipRoot.h"
#include "CATClipValues.h"
#include "ICATParent.h"
#include "ICATParent.h"
#include "RootNodeController.h"
#include "CATNodeControl.h"
#include <CATAPI/IHub.h>

class RootNodeCtrlClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { return new RootNodeCtrl(loading); }
	const TCHAR *	ClassName() { return _T("CATRigRootNodeCtrl"); } //GetString(IDS_CL_ROOT_NODE_CTRL); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID		ClassID() { return ROOT_NODE_CTRL_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATRigRootNodeCtrl"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle

};

static RootNodeCtrlClassDesc RootNodeCtrlDesc;
ClassDesc2* GetRootNodeCtrlDesc() { return &RootNodeCtrlDesc; }

class RootNodeCtrlRolloutData : TimeChangeCallback
{
	HWND hDlg;
	RootNodeCtrl* ctrlRootNodeCtrl;
	ISpinnerControl *spnStartTime;
	ISpinnerControl *spnEndTime;

public:

	RootNodeCtrlRolloutData() : hDlg(NULL), spnStartTime(NULL), spnEndTime(NULL)
	{
		ctrlRootNodeCtrl = NULL;
	}

	HWND GetHWnd() { return hDlg; }

	void TimeChanged(TimeValue)
	{
		if (ctrlRootNodeCtrl->TestFlag(RNFLAG_RESTONGROUND))
			SET_CHECKED(hDlg, IDC_CHK_RESTONGROUND, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_RESTONGROUND, FALSE);

		if (ctrlRootNodeCtrl->TestFlag(RNFLAG_ALLOWKEYFRAMING))
			SET_CHECKED(hDlg, IDC_CHK_ALLOWKEYFRAMING, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_ALLOWKEYFRAMING, FALSE);

		if (ctrlRootNodeCtrl->TestFlag(RNFLAG_LINEARMOTION))
			SET_CHECKED(hDlg, IDC_CHK_LINEARMOTION, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_LINEARMOTION, FALSE);

		if (ctrlRootNodeCtrl->TestFlag(RNFLAG_STATIC_ORIENTATION))
			SET_CHECKED(hDlg, IDC_CHK_STATIC_ORIENTATION, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_STATIC_ORIENTATION, FALSE);

		Interval range = ctrlRootNodeCtrl->clip_range;
		spnStartTime->SetValue((float)range.Start() / (float)GetTicksPerFrame(), FALSE);
		spnEndTime->SetValue((float)range.End() / (float)GetTicksPerFrame(), FALSE);
	}

	BOOL InitControls(HWND hWnd, RootNodeCtrl* RootNodeCtrl)
	{
		hDlg = hWnd;

		ctrlRootNodeCtrl = RootNodeCtrl;

		Interval range = ctrlRootNodeCtrl->clip_range;
		spnStartTime = SetupFloatSpinner(hDlg, IDC_SPIN_START_TIME, IDC_EDIT_START_TIME, -1000, 1000, (float)range.Start() / (float)GetTicksPerFrame());
		spnEndTime = SetupFloatSpinner(hDlg, IDC_SPIN_END_TIME, IDC_EDIT_END_TIME, -1000, 1000, (float)range.End() / (float)GetTicksPerFrame());
		spnStartTime->SetAutoScale();
		spnEndTime->SetAutoScale();

		TimeChanged(GetCOREInterface()->GetTime());
		return TRUE;
	}

	void ReleaseControls(HWND hWnd)
	{
		UNUSED_PARAM(hWnd);
		DbgAssert(hDlg == hWnd);

		SAFE_RELEASE_SPIN(spnStartTime);
		SAFE_RELEASE_SPIN(spnEndTime);

		hDlg = NULL;
		ctrlRootNodeCtrl = NULL;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (RootNodeCtrl*)lParam);
			GetCOREInterface()->RegisterTimeChangeCallback(this);
			break;
		case WM_DESTROY:
			ReleaseControls(hWnd);
			GetCOREInterface()->UnRegisterTimeChangeCallback(this);
			break;

		case WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				switch (LOWORD(wParam)) {
				case IDC_CHK_RESTONGROUND:		ctrlRootNodeCtrl->SetFlag(RNFLAG_RESTONGROUND, IS_CHECKED(hWnd, IDC_CHK_RESTONGROUND));				break;
				case IDC_CHK_ALLOWKEYFRAMING:	ctrlRootNodeCtrl->SetFlag(RNFLAG_ALLOWKEYFRAMING, IS_CHECKED(hWnd, IDC_CHK_ALLOWKEYFRAMING));		break;
				case IDC_CHK_LINEARMOTION:		ctrlRootNodeCtrl->SetFlag(RNFLAG_LINEARMOTION, IS_CHECKED(hWnd, IDC_CHK_LINEARMOTION));				break;
				case IDC_CHK_STATIC_ORIENTATION:ctrlRootNodeCtrl->SetFlag(RNFLAG_STATIC_ORIENTATION, IS_CHECKED(hWnd, IDC_CHK_STATIC_ORIENTATION));	break;
				}
				break;
			}
			break;
		case CC_SPINNER_CHANGE:
		{
			Interval range = ctrlRootNodeCtrl->clip_range;
			switch (LOWORD(wParam)) {
			case IDC_SPIN_START_TIME:	range.SetStart((TimeValue)(spnStartTime->GetFVal() * (float)GetTicksPerFrame()));	break;
			case IDC_SPIN_END_TIME:		range.SetEnd((TimeValue)(spnEndTime->GetFVal() * (float)GetTicksPerFrame()));	break;
			}
			ctrlRootNodeCtrl->clip_range = range;
			ctrlRootNodeCtrl->Update();
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			break;
		}
		default:
			return FALSE;
		}
		return TRUE;
	}
};

static RootNodeCtrlRolloutData staticRootNodeCtrlRollout;

static INT_PTR CALLBACK RootNodeCtrlRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticRootNodeCtrlRollout.DlgProc(hWnd, message, wParam, lParam);
};

RootNodeCtrl::RootNodeCtrl(BOOL loading)
{
	UNREFERENCED_PARAMETER(loading);
	layerTrans = NULL;
	catparenttrans = NULL;
	tmTransform.IdentityMatrix();
	bEvaluatingTM = FALSE;

	// Initialise to a value that may be usefull.
	clip_range.SetStart(0);
	clip_range.SetEnd(20 * 160);

	flags = 0;
}

RootNodeCtrl::~RootNodeCtrl() {
}

RefTargetHandle RootNodeCtrl::Clone(RemapDir& remap)
{
	// make a new datamusher object to be the clone
	RootNodeCtrl *newRootNodeCtrl = new RootNodeCtrl(FALSE);

	newRootNodeCtrl->flags = flags;

	ReplaceReference(LAYERTRANS, remap.CloneRef(layerTrans));

	remap.PatchPointer((RefTargetHandle*)&newRootNodeCtrl->catparenttrans, catparenttrans);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newRootNodeCtrl, remap);

	// now return the new object.
	return newRootNodeCtrl;
}

class SetTMRestore : public RestoreObj {
public:
	RootNodeCtrl	*ctrl;
	Matrix3		tmundo, tmredo;
	SetTMRestore(RootNodeCtrl *c) {
		ctrl = c;
		ctrl->SetAFlag(A_HELD);
		tmundo = ctrl->tmTransform;
	}
	void Restore(int isUndo) {
		if (isUndo) {
		}
		tmredo = ctrl->tmTransform;
		ctrl->tmTransform = tmundo;
	}
	void Redo() {
		ctrl->tmTransform = tmredo;
	}
	int Size() { return 3; }
	//	void EndHold() { ctrl->ClearAFlag(A_HELD); }
};

void RootNodeCtrl::MouseCycleStarted(TimeValue)
{

}
void RootNodeCtrl::MouseCycleCompleted(TimeValue)
{

}

void ApproxCharacterTransform(ICATParentTrans *catparenttrans, Matrix3 &tmTransform, TimeValue t, ULONG flags)
{
	CATNodeControl* roothub = catparenttrans->GetRootHub();
	if (!roothub) return;

	//	roothub->GetNode()->GetNodeTM(t);
	INodeTab nodes;
	catparenttrans->GetSystemNodes(nodes, kSNCFileSave);
	for (int i = 0; i < nodes.Count(); i++) {
		CATNodeControl *catnodecontrol = (CATNodeControl*)nodes[i]->GetTMController()->GetInterface(I_CATNODECONTROL);
		if (catnodecontrol) {
			nodes[i]->GetNodeTM(t);
		}
	}

	tmTransform = catparenttrans->ApproxCharacterTransform(t);
	Point3 pos = tmTransform.GetTrans();

	RotateMatrixToAlignWithVector(tmTransform, P3_UP_VEC, catparenttrans->GetLengthAxis());

	if (flags&RNFLAG_RESTONGROUND) {
		pos[Z] = 0.0f;
	}
	else {
		int countednodes = 0;
		catparenttrans->GetSystemNodes(nodes, kSNCFileSave);
		for (int i = 0; i < nodes.Count(); i++) {
			CATNodeControl *catnodecontrol = (CATNodeControl*)nodes[i]->GetTMController()->GetInterface(I_CATNODECONTROL);
			if (catnodecontrol != NULL)
			{
				if (catnodecontrol->GetLayerTrans() != NULL && catnodecontrol->GetLayerTrans()->TestFlag(CLIP_FLAG_HAS_TRANSFORM))
				{
					nodes[i]->GetNodeTM(t);
					float z = catnodecontrol->GetNodeTM(t).GetTrans()[Z];
					if (countednodes == 0 || z < pos[Z]) pos[Z] = z;

					countednodes++;
				}
			}
		}
	}
	tmTransform.SetTrans(pos);

}

void RootNodeCtrl::Update()
{
	INode* node = FindReferencingClass<INode>(this);
	if (node)
		node->InvalidateTM();
}

class RootNodeRestore : public RestoreObj {
public:
	RootNodeCtrl	*rootnode;
	RootNodeRestore(RootNodeCtrl *rn) {
		rootnode = rn;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			rootnode->Update();
		}
	}
	void Redo() {
		rootnode->Update();
	}
	int Size() { return 2; }
};

void RootNodeCtrl::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	SetXFormPacket *ptr = (SetXFormPacket*)val;

	if (catparenttrans) {

		if (TestFlag(RNFLAG_ALLOWKEYFRAMING)) {

			CalcRootNodeTransform(t);
			ptr->tmParent = tmTransform;

			if (layerTrans && ptr->command != XFORM_SCALE && catparenttrans->GetCATMode() != SETUPMODE)
				layerTrans->SetValue(t, val, commit, method);

		}
		else {

			switch (ptr->command)
			{
			case XFORM_MOVE:
				break;
			case XFORM_ROTATE:
				ptr->localOrigin = FALSE;
				break;
			}

			INodeTab nodes;
			catparenttrans->GetSystemNodes(nodes, kSNCClone);

			// Move any nodes that are root nodes(CLIP_FLAG_HAS_TRANSFORM on layerTrans)
			for (int i = 0; i < nodes.Count(); i++) {
				Control* ctrl = nodes[i]->GetTMController();
				if (ctrl->GetInterface(I_CATNODECONTROL)) {
					CATNodeControl* catnodecontrol = (CATNodeControl*)ctrl->GetInterface(I_CATNODECONTROL);
					CATClipMatrix3* layerTrans = catnodecontrol->GetLayerTrans();
					if (layerTrans && layerTrans->TestFlag(CLIP_FLAG_HAS_TRANSFORM)) {
						SetXFormPacket ptr2 = *ptr;
						//ptr2.tmParent = catnodecontrol->GetParentTM(t);
						ctrl->SetValue(t, (void*)&ptr2, commit, method);
					}
				}
			}

			if (theHold.Holding()) {
				theHold.Put(new RootNodeRestore(this));
			}
			Update();
		}
	}
}

void RootNodeCtrl::CalcRootNodeTransform(TimeValue t)
{
	if (TestFlag(RNFLAG_LINEARMOTION)) {
		Matrix3 tm1, tm2;
		ApproxCharacterTransform(catparenttrans, tm1, clip_range.Start(), flags);
		ApproxCharacterTransform(catparenttrans, tm2, clip_range.End(), flags);
		ApproxCharacterTransform(catparenttrans, tmTransform, t, flags);

		if (!TestFlag(RNFLAG_STATIC_ORIENTATION)) {
			if (clip_range.End() > clip_range.Start()) {
				// with the static rotation flag turned on, w3e use the rotation at the start of the sequence only
				// This if for sequences where an animated root causes motion when the motion was intended simply
				float blend = (float)(t - clip_range.Start()) / (float)(clip_range.End() - clip_range.Start());
				blend = clamp(blend, 0.0f, 1.0f);
				BlendMat(tm1, tm2, blend);
			}
		}
		if (clip_range.End() > clip_range.Start()) {
			// Project the position of the character onto the vector between the start and end of the clip.
			// This removes and side to side motion yet retains acceleration.
			Point3 movementdir = Normalize(tm2.GetTrans() - tm1.GetTrans());
			float movementdist = DotProd(tmTransform.GetTrans() - tm1.GetTrans(), movementdir);
			tm1.SetTrans(tm1.GetTrans() + (movementdist * movementdir));
			// The orientation of the root node is the slerped rotation between the start and the end.
		}
		tmTransform = tm1;
	}
	else {
		Matrix3 tm1;
		if (TestFlag(RNFLAG_STATIC_ORIENTATION)) {
			ApproxCharacterTransform(catparenttrans, tm1, clip_range.Start(), flags);
		}
		ApproxCharacterTransform(catparenttrans, tmTransform, t, flags);

		if (TestFlag(RNFLAG_STATIC_ORIENTATION)) {
			tm1.SetTrans(tmTransform.GetTrans());
			tmTransform = tm1;
		}
	}
}

void RootNodeCtrl::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod)
{
	// We ignore the Transform node parent because we don't want the character behaving
	// any differently when the transform nodes are created and not created, or linked to ther objects.
	// All the transform information should be contained in the prs controller
	DbgAssert("This doesn't work now.  Fix it.");

	if (catparenttrans && !bEvaluatingTM) {
		bEvaluatingTM = TRUE;

		CalcRootNodeTransform(t);

		if (layerTrans) layerTrans->GetValue(t, (void*)&tmTransform, valid, CTRL_RELATIVE);

		valid.SetInstant(t);
		*(Matrix3*)val = tmTransform;

		bEvaluatingTM = FALSE;
	}
}

void RootNodeCtrl::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	if (catparenttrans) {
		GetCOREInterface()->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_ROOT_NODE), &RootNodeCtrlRolloutProc, GetString(IDS_ROOT_NODE_TRANSFORM), (LPARAM)this);
	}

	if (layerTrans) layerTrans->BeginEditParams(ip, flags, prev);

}

void RootNodeCtrl::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);
	if (layerTrans) layerTrans->EndEditParams(ip, END_EDIT_REMOVEUI); // next);

	if (catparenttrans) {
		GetCOREInterface()->DeleteRollupPage(staticRootNodeCtrlRollout.GetHWnd());
	}
}

RefResult RootNodeCtrl::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_TARGET_DELETED:
		if (hTarg == layerTrans) layerTrans = NULL;
		break;
	}

	return REF_SUCCEED;
}

// Create a whole structure for CAT
INode* RootNodeCtrl::Initialise(ICATParentTrans* catparenttrans)
{
	this->catparenttrans = (ICATParentTrans*)catparenttrans;

	Interface* ip = GetCOREInterface();
	Object* obj = (Object*)CreateInstance(GEOMOBJECT_CLASS_ID, CATBONE_CLASS_ID);
	ICATObject* iobj = (ICATObject*)obj->GetInterface(I_CATOBJECT);
	DbgAssert(iobj);
	iobj->SetTransformController(this);
	iobj->SetLengthAxis(catparenttrans->GetLengthAxis());
	iobj->SetX(1.0f);
	iobj->SetY(1.0f);
	iobj->SetZ(1.0f);
	iobj->SetCATUnits(catparenttrans->GetCATUnits());
	//	iobj->SetLengthAxis(catparenttrans->GetLengthAxis());

	INode *node = ip->CreateObjectNode(obj);
	node->SetTMController(this);
	node->SetWireColor(catparenttrans->GetNode()->GetWireColor());
	node->SetName(catparenttrans->GetCATName() + GetString(IDS_TRANSFORM));

	flags |= RNFLAG_RESTONGROUND;

	float x = 200.0, y = 150.0, z = 1.0;
	Mesh mesh;
	mesh.setNumVerts(16);
	mesh.setNumFaces(22);
	mesh.setVert(0, -0.179756f * x, 0.233797f * y, -0.000003f * z);
	mesh.setVert(1, -0.074777f * x, 0.233797f * y, -0.000002f * z);
	mesh.setVert(2, -0.074777f * x, -0.374074f * y, 0.000002f * z);
	mesh.setVert(3, 0.000019f * x, -0.374075f * y, 0.000003f * z);
	mesh.setVert(4, 0.074815f * x, -0.374075f * y, 0.000003f * z);
	mesh.setVert(5, 0.074815f * x, 0.233797f * y, -0.000001f * z);
	mesh.setVert(6, 0.179794f * x, 0.233796f * y, -0.000001f * z);
	mesh.setVert(7, 0.000019f * x, 0.547312f * y, -0.000004f * z);
	mesh.setVert(8, -0.136709f * x, 0.258735f * y, 0.935184f * z);
	mesh.setVert(9, -0.049839f * x, 0.258735f * y, 0.935184f * z);
	mesh.setVert(10, -0.049839f * x, -0.349136f * y, 0.935189f * z);
	mesh.setVert(11, 0.000019f * x, -0.349136f * y, 0.935189f * z);
	mesh.setVert(12, 0.049876f * x, -0.349136f * y, 0.935189f * z);
	mesh.setVert(13, 0.049877f * x, 0.258735f * y, 0.935185f * z);
	mesh.setVert(14, 0.136747f * x, 0.258735f * y, 0.935185f * z);
	mesh.setVert(15, 0.000019f * x, 0.497179f * y, 0.935183f * z);
	MakeFace(mesh.faces[0], 1, 9, 8, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[1], 8, 0, 1, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[2], 2, 10, 9, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[3], 9, 1, 2, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[4], 3, 11, 10, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[5], 10, 2, 3, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[6], 4, 12, 11, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[7], 11, 3, 4, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[8], 5, 13, 12, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[9], 12, 4, 5, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[10], 6, 14, 13, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[11], 13, 5, 6, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[12], 7, 15, 14, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[13], 14, 6, 7, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[14], 0, 8, 15, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[15], 15, 7, 0, 1, 2, 0, 0, 2);
	MakeFace(mesh.faces[16], 13, 14, 15, 1, 2, 0, 0, 0);
	MakeFace(mesh.faces[17], 15, 8, 9, 1, 2, 0, 0, 0);
	MakeFace(mesh.faces[18], 13, 15, 9, 0, 0, 0, 0, 0);
	MakeFace(mesh.faces[19], 9, 10, 11, 1, 2, 0, 0, 0);
	MakeFace(mesh.faces[20], 9, 11, 12, 0, 2, 0, 0, 0);
	MakeFace(mesh.faces[21], 13, 9, 12, 0, 0, 4, 0, 0);

	iobj->SetCATMesh(&mesh);

	if (catparenttrans->GetRootHub()) {
		INode *currrootnode = catparenttrans->GetRootHub()->GetNode();
		// Look up through the hierarchy to rind the real root
		while (!currrootnode->GetParentNode()->IsRootNode()) currrootnode = currrootnode->GetParentNode();
		node->AttachChild(currrootnode, FALSE);
	}

	node->InvalidateTM();

	CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), NULL, catparenttrans, FALSE);
	layerController->SetFlag(CLIPFLAG_APPLYTRANSFORMS);
	ReplaceReference(LAYERTRANS, layerController);

	return node;
}

class SetRNFlagRestore : public RestoreObj {
public:
	RootNodeCtrl	*ctrl;
	DWORD		flagsundo, flagsredo;
	SetRNFlagRestore(RootNodeCtrl *c) : flagsredo(0L) {
		ctrl = c;
		flagsundo = ctrl->flags;
	}
	void Restore(int isUndo) {
		if (isUndo) {
			flagsredo = ctrl->flags;
			if (staticRootNodeCtrlRollout.GetHWnd())staticRootNodeCtrlRollout.TimeChanged(GetCOREInterface()->GetTime());
		}
		ctrl->flags = flagsundo;
	}
	void Redo() {
		ctrl->flags = flagsredo;
		if (staticRootNodeCtrlRollout.GetHWnd())staticRootNodeCtrlRollout.TimeChanged(GetCOREInterface()->GetTime());
	}
	int Size() { return 3; }
	void EndHold() {}
};

void RootNodeCtrl::SetFlag(ULONG f, BOOL on, BOOL enableundo) {
	BOOL newundo = FALSE;
	if (enableundo && !theHold.Holding()) { theHold.Begin(); newundo = TRUE; }
	if (enableundo && theHold.Holding()) theHold.Put(new SetRNFlagRestore(this));
	if (on)	flags |= f;
	else	flags &= ~f;
	if (enableundo && theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_ROOTNODESETTING));
		Update();
	}
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class RootNodeCtrlPLCB : public PostLoadCallback {
protected:
	RootNodeCtrl *rootnodectrl;
	ICATParentTrans *catparenttrans;

public:
	RootNodeCtrlPLCB(RootNodeCtrl *pOwner) : catparenttrans(NULL) { rootnodectrl = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(rootnodectrl);
		if (rootnodectrl)
		{
			catparenttrans = rootnodectrl->GetCATParentTrans();
			DbgAssert(catparenttrans);
			if (catparenttrans)
				return catparenttrans->GetFileSaveVersion();
		}
		return 0;
	}

	int Priority() { return 5; }

	void proc(ILoad *) {
		if (GetFileSaveVersion() < CAT3_VERSION_3210) {
			if (rootnodectrl && rootnodectrl->GetCATParentTrans())
			{
				CATClipValue* layerController = CreateClipValueController(GetCATClipMatrix3Desc(), NULL, rootnodectrl->GetCATParentTrans(), FALSE);
				rootnodectrl->ReplaceReference(0, layerController);
				layerController->SetFlag(CLIPFLAG_APPLYTRANSFORMS);
			}
		}

		delete this;
	}
};

#define		LAYERTRANSFORM_CATPARENT		6
#define		LAYERTRANSFORM_FLAGS			8
#define		LAYERTRANSFORM_CLIPRANGE		10

IOResult RootNodeCtrl::Save(ISave *isave)
{
	ULONG id;
	DWORD nb;//, r

	isave->BeginChunk(LAYERTRANSFORM_CATPARENT);
	id = isave->GetRefID(catparenttrans);
	isave->Write(&id, sizeof(ULONG), &nb);
	isave->EndChunk();

	isave->BeginChunk(LAYERTRANSFORM_FLAGS);
	isave->Write(&flags, sizeof(ULONG), &nb);
	isave->EndChunk();

	isave->BeginChunk(LAYERTRANSFORM_CLIPRANGE);
	isave->Write(&clip_range, sizeof(Interval), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult RootNodeCtrl::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L;
	ULONG id = 0L;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
		case LAYERTRANSFORM_CATPARENT:
			res = iload->Read(&id, sizeof(ULONG), &nb);
			if (res == IO_OK && id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&catparenttrans);
			break;
		case LAYERTRANSFORM_FLAGS:
			res = iload->Read(&flags, sizeof(ULONG), &nb);
			break;
		case LAYERTRANSFORM_CLIPRANGE:
			res = iload->Read(&clip_range, sizeof(Interval), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new RootNodeCtrlPLCB(this));

	return IO_OK;
}

BOOL RootNodeCtrl::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(idRootBone);

	if (layerTrans) {
		save->BeginGroup(idController);
		layerTrans->SaveClip(save, flags, timerange, layerindex);
		save->EndGroup();
	}
	save->EndGroup();
	return TRUE;
}

BOOL RootNodeCtrl::LoadClip(CATRigReader *load, Interval timerange, int layerindex, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	while (load->ok() && !done && ok) {

		// An error occurred in the clause, and we've
		// ended up with the next valid one.  If we're
		// not still in the correct group, return.
		if (!load->NextClause())
			if (load->CurGroupID() != idRootBone) return FALSE;

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idController:
			{
				if (layerTrans) layerTrans->LoadClip(load, timerange, layerindex, dScale, flags);
				break;
			};
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

// this will only propogate the messages
void RootNodeCtrl::CATMessage(TimeValue t, UINT msg, int data) {

	if (layerTrans) layerTrans->CATMessage(t, msg, data);
}

void RootNodeCtrl::AddLayerControllers(Tab <Control*>	 &layerctrls)
{
	Control *ctrl = layerTrans;;
	if (layerTrans) layerctrls.Append(1, &ctrl);
}
