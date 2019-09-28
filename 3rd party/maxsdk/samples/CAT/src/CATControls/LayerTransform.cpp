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

#include "CATClipRoot.h"
#include "CATClipValues.h"
#include "NLAInfo.h"
#include "CATNodeControl.h"
#include "ICATParent.h"
#include "LayerTransform.h"
#include "../DataRestoreObj.h"

class LayerTransformClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { return new LayerTransform(loading); }
	const TCHAR *	ClassName() { return _T("LayerTransform"); } //GetString(IDS_CL_LAYER_TRANSFORM); }
	SClass_ID		SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID		ClassID() { return LAYER_TRANSFORM_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("LayerTransform"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle

};

static LayerTransformClassDesc LayerTransformDesc;
ClassDesc2* GetLayerTransformDesc() { return &LayerTransformDesc; }

class LayerTransformRolloutData : TimeChangeCallback
{
	HWND hDlg;
	LayerTransform* ctrlLayerTransform;
	ISpinnerControl *spnStartTime;
	ISpinnerControl *spnEndTime;

public:

	LayerTransformRolloutData() : hDlg(NULL), spnStartTime(NULL), spnEndTime(NULL)
	{
		ctrlLayerTransform = NULL;
	}

	HWND GetHWnd() { return hDlg; }

	void TimeChanged(TimeValue)
	{
		if (ctrlLayerTransform->GetDisplayGhost())
			SET_CHECKED(hDlg, IDC_CHK_DISPLAY_GHOST, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_DISPLAY_GHOST, FALSE);

		if (ctrlLayerTransform->layerinfo->TestFlag(LAYER_MAKE_LOOPABLE))
			SET_CHECKED(hDlg, IDC_CHK_MAKE_LOOPABLE, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_MAKE_LOOPABLE, FALSE);

		if (ctrlLayerTransform->layerinfo->IGetRemoveDisplacementOn())
			SET_CHECKED(hDlg, IDC_CHK_REMOVE_DISPLACEMENT, TRUE);
		else SET_CHECKED(hDlg, IDC_CHK_REMOVE_DISPLACEMENT, FALSE);

		Interval range = ctrlLayerTransform->layerinfo->GetLoopRange();

		spnStartTime->SetValue((float)range.Start() / (float)GetTicksPerFrame(), FALSE);
		spnEndTime->SetValue((float)range.End() / (float)GetTicksPerFrame(), FALSE);

	}

	BOOL InitControls(HWND hWnd, LayerTransform* LayerTransform)
	{
		hDlg = hWnd;

		ctrlLayerTransform = LayerTransform;

		// Initialise the INI files so we can read button text and tooltips
		CatDotIni catCfg;
		Interval range = ctrlLayerTransform->layerinfo->GetLoopRange();
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
		ctrlLayerTransform = NULL;
	}

	INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG:
			InitControls(hWnd, (LayerTransform*)lParam);
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
				case IDC_CHK_DISPLAY_GHOST:
					ctrlLayerTransform->SetDisplayGhost(IS_CHECKED(hWnd, IDC_CHK_DISPLAY_GHOST));
					GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
					break;
					/*		case IDC_CHK_MAKE_LOOPABLE:
								ctrlLayerTransform->layerinfo->SetFlag(LAYER_MAKE_LOOPABLE, IS_CHECKED(hWnd, IDC_CHK_MAKE_LOOPABLE));
								if(IS_CHECKED(hWnd, IDC_CHK_MAKE_LOOPABLE)) ctrlLayerTransform->layerinfo->iTransform = NEVER;
								ctrlLayerTransform->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
								ctrlLayerTransform->layerinfo->GetCATParentTrans()->UpdateCharacter();
								GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
								break;
					*/		case IDC_CHK_REMOVE_DISPLACEMENT:
						ctrlLayerTransform->layerinfo->ISetRemoveDisplacementOn(IS_CHECKED(hWnd, IDC_CHK_REMOVE_DISPLACEMENT));
						GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
						break;

				}
				break;
			}
			break;
		case CC_SPINNER_CHANGE:
		{
			Interval range = ctrlLayerTransform->layerinfo->GetLoopRange();
			switch (LOWORD(wParam)) {
			case IDC_SPIN_START_TIME:	range.SetStart((TimeValue)(spnStartTime->GetFVal() * (float)GetTicksPerFrame()));	break;
			case IDC_SPIN_END_TIME:		range.SetEnd((TimeValue)(spnEndTime->GetFVal() * (float)GetTicksPerFrame()));	break;
			}
			ctrlLayerTransform->layerinfo->SetLoopRange(range);
			ctrlLayerTransform->layerinfo->GetCATParentTrans()->UpdateCharacter();
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			break;
		}
		default:
			return FALSE;
		}
		return TRUE;
	}
};

static LayerTransformRolloutData staticLayerTransformRollout;

static INT_PTR CALLBACK LayerTransformRolloutProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return staticLayerTransformRollout.DlgProc(hWnd, message, wParam, lParam);
};

LayerTransform::LayerTransform(BOOL loading) : bodypart(NULL)
, startT(0)
, layerindex(0)
, flags(0)
{
	UNREFERENCED_PARAMETER(loading);
	prs = NULL;
	layerinfo = NULL;
	catparenttrans = NULL;
	tmOffset.IdentityMatrix();
	tmTransform.IdentityMatrix();
	ghostTimeRange = FOREVER;
	transformsloaded = FALSE;
}

LayerTransform::~LayerTransform() {
	DeleteAllRefs();
}

RefTargetHandle LayerTransform::Clone(RemapDir& remap)
{
	DbgAssert(false); // This cannot be cloned!
	// make a new datamusher object to be the clone
	LayerTransform *newLayerTransform = new LayerTransform(FALSE);

	ReplaceReference(PRS, remap.CloneRef(prs));

	// these pointers need to be reset somewhere,
	// but if we NULL them, at least it won't crash after cloning with the gizmo on
	// Actually - it still does...
	catparenttrans = NULL;
	layerinfo = NULL;

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newLayerTransform, remap);

	// now return the new object.
	return newLayerTransform;
}

void LayerTransform::MouseCycleStarted(TimeValue)
{

}
void LayerTransform::MouseCycleCompleted(TimeValue)
{

}

void LayerTransform::SetValue(TimeValue t, void *val, int commit, GetSetMethod method)
{
	SetXFormPacket *ptr = (SetXFormPacket*)val;

	if (prs) {
		switch (ptr->command)
		{
		case XFORM_MOVE:
			break;
		case XFORM_ROTATE:
			ptr->localOrigin = FALSE;
			break;
		}
		prs->SetValue(t, val, commit, method);
	}
	else {
		//	if(theHold.Holding()) theHold.Restore();

		switch (ptr->command)
		{
		case XFORM_MOVE:
			if (method == CTRL_RELATIVE) {
				tmTransform.Translate(VectorTransform(ptr->tmAxis*Inverse(ptr->tmParent), ptr->p));
			}
			else {
				ptr->p = (ptr->p * Inverse(ptr->tmParent));
				tmTransform.SetTrans(ptr->p);
			}
			break;
		case XFORM_ROTATE: {
			ptr->aa.axis = Normalize(VectorTransform(ptr->tmAxis*Inverse(ptr->tmParent), ptr->aa.axis));

			Point3 pos = tmTransform.GetTrans();
			RotateMatrix(tmTransform, ptr->aa);
			tmTransform.SetTrans(pos);
			break;
		}
		case XFORM_SET:
			if (method == CTRL_ABSOLUTE) {
				tmTransform = ptr->tmAxis * Inverse(ptr->tmParent);
			}
			else {

			}
			break;
		}
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		// Reload the clip file with our new transform offset
		if (catparenttrans) {
			catparenttrans->LoadClip(file, layerindex, flags, startT, bodypart);
		}
	}
}

void LayerTransform::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod)
{
	// We ignore the Transform node parent because we don't want the character behaving
	// any differently when the transform noeds are created and not created, or linked to ther objects.
	// All the transform information should be contained in the prs controller
//	Matrix3 tmTransform(1);
//	if(prs) prs->GetValue(t, (void*)&tmTransform, valid, CTRL_RELATIVE);
	if (prs) {
		if (layerinfo) {
			tmTransform = layerinfo->GetTransform(t, valid);
		}
		*(Matrix3*)val = tmOffset * tmTransform;
	}
	else {
		*(Matrix3*)val = tmTransform;
	}

	if (layerinfo && GetDisplayGhost()) {
		valid.SetInstant(t);
	}
}

void LayerTransform::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	if (layerinfo) {
		GetCOREInterface()->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_TRANSFORM_NODE), &LayerTransformRolloutProc, GetString(IDS_LYR_TRANSFORM), (LPARAM)this);

		layerinfo->BeginEditParams(ip, flags, prev);
	}

	if (prs) prs->BeginEditParams(ip, flags, prev);
}

void LayerTransform::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	UNREFERENCED_PARAMETER(flags); UNREFERENCED_PARAMETER(next);
	if (prs) prs->EndEditParams(ip, END_EDIT_REMOVEUI); // next);

	if (layerinfo) {
		layerinfo->EndEditParams(ip, END_EDIT_REMOVEUI);

		GetCOREInterface()->DeleteRollupPage(staticLayerTransformRollout.GetHWnd());
	}
}

RefResult LayerTransform::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	switch (msg)
	{
	case REFMSG_TARGET_DELETED:
		if (hTarg == prs) prs = NULL;
		break;
	}

	return REF_SUCCEED;
}

INode* LayerTransform::CreateINode(TSTR name, float size, Color clr)
{
	Object *obj = (Object*)CreateInstance(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));
	// pointobj_params and pointobj_size have been helpfully defined in istdplug.h
	// so we default the footprints to the right size
	IParamBlock2 *objpblock = ((Animatable*)obj)->GetParamBlock(pointobj_params);
	objpblock->SetValue(pointobj_size, 0, size);
	objpblock->SetValue(pointobj_centermarker, 0, FALSE);
	objpblock->SetValue(pointobj_axistripod, 0, TRUE);
	objpblock->SetValue(pointobj_cross, 0, FALSE);
	objpblock->SetValue(pointobj_box, 0, TRUE);
	objpblock->SetValue(pointobj_screensize, 0, FALSE);
	objpblock->SetValue(pointobj_drawontop, 0, TRUE);

	INode *node = GetCOREInterface()->CreateObjectNode(obj);
	node->SetWireColor(clr.toRGB());//asRGB(layerinfo->GetColour()));
	node->SetName(name);
	node->SetTMController(this);

	return node;
}

void LayerTransform::ReInitialize(NLAInfo* layerinfo)
{
	this->layerinfo = (NLAInfo*)layerinfo;
	this->catparenttrans = layerinfo->GetCATParentTrans();
}

// Create a whole structure for CAT
INode* LayerTransform::Initialise(NLAInfo* layerinfo, Control *prs)
{
	this->layerinfo = (NLAInfo*)layerinfo;
	this->catparenttrans = layerinfo->GetCATParentTrans();
	CATClipRoot* root = static_cast<CATClipRoot*>(catparenttrans->GetLayerRoot());

	TimeValue t = GetCOREInterface()->GetTime();;

	Object *obj = (Object*)CreateInstance(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));
	// pointobj_params and pointobj_size have been helpfully defined in istdplug.h
	// so we default the footprints to the right size
	IParamBlock2 *objpblock = ((Animatable*)obj)->GetParamBlock(pointobj_params);
	objpblock->SetValue(pointobj_size, 0, 20.0f*catparenttrans->GetCATUnits());
	objpblock->SetValue(pointobj_centermarker, 0, FALSE);
	objpblock->SetValue(pointobj_axistripod, 0, TRUE);
	objpblock->SetValue(pointobj_cross, 0, FALSE);
	objpblock->SetValue(pointobj_box, 0, TRUE);
	objpblock->SetValue(pointobj_screensize, 0, FALSE);
	objpblock->SetValue(pointobj_drawontop, 0, TRUE);

	INode *node = GetCOREInterface()->CreateObjectNode(obj);
	node->SetWireColor(layerinfo->GetColour().toRGB());//asRGB(layerinfo->GetColour()));
	node->SetName(layerinfo->GetName() + GetString(IDS_TRANSFORM));
	node->SetTMController(this);

	Interval iv = FOREVER;
	if (GetKeyState(VK_CONTROL) < 0) {
		if (GetCOREInterface()->GetSelNodeCount() == 1) {
			INode *selnode = GetCOREInterface()->GetSelNode(0);
			DbgAssert(selnode);
			Matrix3 tmRigTM = selnode->GetNodeTM(t);

			Matrix3 tmTransform(1);
			prs->GetValue(t, (void*)&tmTransform, iv, CTRL_RELATIVE);

			tmOffset = tmRigTM * Inverse(tmTransform);
		}
	}
	else if (GetKeyState(VK_MENU) < 0) {
		tmOffset.IdentityMatrix();
	}
	else {
		// Give the Transform Gizmo an aproximate position directly beneath the character
		theHold.Suspend();
		// To make sure this is the only layer taht effects the position of the gizmo
		int solo = root->GetSoloLayer();
		CATMode catmode = catparenttrans->GetCATMode();
		catparenttrans->SetCATMode(NORMAL);
		root->SoloLayer(layerinfo->GetIndex());

		catparenttrans->CATMessage(t, CAT_INVALIDATE_TM);
		Matrix3 tmRigTM = catparenttrans->ApproxCharacterTransform(t);

		root->SoloLayer(solo);
		catparenttrans->SetCATMode(catmode);
		catparenttrans->CATMessage(t, CAT_INVALIDATE_TM);
		theHold.Resume();

		Matrix3 tmTransform(1);
		prs->GetValue(t, (void*)&tmTransform, iv, CTRL_RELATIVE);

		tmOffset = tmRigTM * Inverse(tmTransform);
	}

	node->InvalidateTM();
	ReplaceReference(PRS, prs);

	return node;
}

INode*	LayerTransform::BuildNode()
{
	// Create an invisible point helper
	Object *obj = (Object*)CreateInstance(HELPER_CLASS_ID, Class_ID(POINTHELP_CLASS_ID, 0));

	IParamBlock2 *objpblock = ((Animatable*)obj)->GetParamBlock(pointobj_params);
	objpblock->SetValue(pointobj_size, 0, 0.0f);
	objpblock->SetValue(pointobj_centermarker, 0, FALSE);
	objpblock->SetValue(pointobj_axistripod, 0, FALSE);
	objpblock->SetValue(pointobj_cross, 0, FALSE);
	objpblock->SetValue(pointobj_box, 0, FALSE);
	objpblock->SetValue(pointobj_screensize, 0, FALSE);
	objpblock->SetValue(pointobj_drawontop, 0, FALSE);

	INode *node = GetCOREInterface()->CreateObjectNode(obj);
	node->SetTMController(this);

	return node;
}

INode* LayerTransform::Initialise(ICATParentTrans* catparenttrans, TSTR name, TSTR file, int layerindex, int flags, TimeValue startT, CATControl* bodypart)
{
	this->catparenttrans = catparenttrans;

	this->file = file;
	this->layerindex = layerindex;
	this->flags = flags;
	this->startT = startT;
	this->bodypart = bodypart;

	return NULL;//node;
}

void LayerTransform::SetTransforms(Matrix3 tmOffset, Matrix3 tmTransform)
{
	HoldData(this->tmOffset);
	HoldData(this->tmTransform);

	this->tmOffset = tmOffset;
	this->tmTransform = tmTransform;
	transformsloaded = TRUE;
};

BOOL LayerTransform::GetDisplayGhost() { return layerinfo->TestFlag(LAYER_DISPLAY_GHOST); }
void LayerTransform::SetDisplayGhost(BOOL b) { layerinfo->SetFlag(LAYER_DISPLAY_GHOST, b);; NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }

int LayerTransform::Display(TimeValue t, INode*, ViewExp *vpt, int flags)
{
	if (layerinfo) {
		if (layerinfo->TestFlag(LAYER_DISPLAY_GHOST)) {
			HoldSuspend hs;

			if (!catparenttrans) {
				catparenttrans = layerinfo->GetCATParentTrans();
				DbgAssert(catparenttrans != NULL);
			}
			CATClipRoot* root = static_cast<CATClipRoot*>(catparenttrans->GetLayerRoot());

			int solo = root->GetSoloLayer();
			CATMode catmode = catparenttrans->GetCATMode();
			catparenttrans->SetCATMode(NORMAL);
			root->SoloLayer(layerinfo->GetIndex());

			// now force the character to display the current layer
			catparenttrans->CATMessage(t, CAT_INVALIDATE_TM);

			bbox.Init();
			CATControl *hub = catparenttrans->GetRootHub();
			if (hub)	hub->DisplayLayer(t, vpt, flags, bbox);

			// put things back the way they were
			root->SoloLayer(solo);
			catparenttrans->SetCATMode(catmode);
			catparenttrans->CATMessage(t, CAT_INVALIDATE_TM);
		}
	}
	else
	{
		// We will just be dsplaying the ghost of the ;
		if (ghostTimeRange.InInterval(t)) return 1;
		if (t < ghostTimeRange.Start()) t = ghostTimeRange.Start();
		if (t > ghostTimeRange.End()) t = ghostTimeRange.End();

		// now force the character to dispaly the current layer
		catparenttrans->CATMessage(t, CAT_INVALIDATE_TM);

		bbox.Init();

		if (bodypart) {
			bodypart->DisplayLayer(t, vpt, flags, bbox);
		}
		else {
			CATControl *hub = catparenttrans->GetRootHub();
			if (hub)	hub->DisplayLayer(t, vpt, flags, bbox);
		}

		catparenttrans->CATMessage(t, CAT_INVALIDATE_TM);
	}
	return 1;
}
void LayerTransform::GetWorldBoundBox(TimeValue, INode *, ViewExp*, Box3& box)
{
	//	if(layerinfo) layerinfo->GetWorldBoundBox(t, inode, vpt, box);
	box += bbox;
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//
class LayerTransformPLCB : public PostLoadCallback {
protected:
	LayerTransform *bone;
	ICATParentTrans *catparenttrans;

public:
	LayerTransformPLCB(LayerTransform *pOwner) : catparenttrans(NULL) { bone = pOwner; }

	DWORD GetFileSaveVersion() {
		DbgAssert(bone);
		NLAInfo* layerinfo = bone->GetLayerInfo();
		DbgAssert(layerinfo);
		catparenttrans = layerinfo->GetCATParentTrans();
		DbgAssert(catparenttrans);
		return catparenttrans->GetFileSaveVersion();
	}

	int Priority() { return 5; }

	void proc(ILoad *) {
		delete this;
	}
};

#define		LAYERTRANSFORM_NLAINFO			0
#define		LAYERTRANSFORM_OFFSETTM			1
//#define		LAYERTRANSFORM_DISPLAY_GHOST	2
#define		LAYERTRANSFORM_CATPARENT		6

IOResult LayerTransform::Save(ISave *isave)
{
	ULONG id;
	DWORD nb;//, r

	isave->BeginChunk(LAYERTRANSFORM_NLAINFO);
	id = isave->GetRefID(layerinfo);
	isave->Write(&id, sizeof(ULONG), &nb);
	isave->EndChunk();

	isave->BeginChunk(LAYERTRANSFORM_OFFSETTM);
	isave->Write(&tmOffset, sizeof(Matrix3), &nb);
	isave->EndChunk();

	isave->BeginChunk(LAYERTRANSFORM_CATPARENT);
	id = isave->GetRefID(catparenttrans);
	isave->Write(&id, sizeof(ULONG), &nb);
	isave->EndChunk();

	//	isave->BeginChunk(LAYERTRANSFORM_DISPLAY_GHOST);
	//	isave->Write(&display_ghost, sizeof(BOOL), &nb);
	//	isave->EndChunk();

	return IO_OK;
}

IOResult LayerTransform::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb;
	ULONG id = 0L;

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case LAYERTRANSFORM_NLAINFO:
			res = iload->Read(&id, sizeof(ULONG), &nb);
			if (res == IO_OK && id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&layerinfo);
			break;
		case LAYERTRANSFORM_OFFSETTM:
			res = iload->Read(&tmOffset, sizeof(Matrix3), &nb);
			break;
		case LAYERTRANSFORM_CATPARENT:
			res = iload->Read(&id, sizeof(ULONG), &nb);
			if (res == IO_OK && id != 0xffffffff)
				iload->RecordBackpatch(id, (void**)&catparenttrans);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(new LayerTransformPLCB(this));

	return IO_OK;
}

