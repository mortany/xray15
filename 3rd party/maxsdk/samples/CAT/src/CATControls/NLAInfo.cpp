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

#include "CATPlugins.h"
#include "NLAInfo.h"
#include "CATClipRoot.h"
#include "LayerTransform.h"

 // Rig Structure
#include "ICATParent.h"
#include "CATNodeControl.h"

static bool nla_info_InterfaceAdded = false;
class NLAInfoClassDesc : public INLAInfoClassDesc {
private:

public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE)
	{
		NLAInfo* nlainfo = new NLAInfo(loading);
		if (!nla_info_InterfaceAdded)
		{
			AddInterface(nlainfo->GetDescByID(I_LAYERINFO_FP));
			nla_info_InterfaceAdded = true;
		}
		return nlainfo;
	}

	const TCHAR *	ClassName() { return GetString(IDS_CL_NLAINFO); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return NLAINFO_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("NLAInfo"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

static NLAInfoClassDesc NLAInfoDesc;

INLAInfoClassDesc* GetNLAInfoDesc() {
	return &NLAInfoDesc;
}

// Elwood: Mocap layer is now officially defunct.  I need to patch the NLAInfo
// class, and Mocap has been deprecated from CAT for years.  It sounds
// like a good time to remove the class completely.  However there may
// still be files that contain a reference to this class, so we simply
// substitute a regular NLA info if any file requests a Mocap layer.
#define MOCAPLAYERINFO_CLASS_ID Class_ID(0x7280234e, 0x43e3351)
class DeprecatedMocapClassDesc : public NLAInfoClassDesc
{
	void *			Create(BOOL loading = FALSE)
	{
		DbgAssert(false && _M("Warning: loading Mocap Layer"));
		NLAInfo* pInfo = reinterpret_cast<NLAInfo*>(NLAInfoClassDesc::Create(loading));
		pInfo->SetMethod(LAYER_ABSOLUTE);
		return pInfo;
	}
	Class_ID		ClassID() { return MOCAPLAYERINFO_CLASS_ID; }
};
INLAInfoClassDesc* GetMocapLayerInfoDesc()
{
	static DeprecatedMocapClassDesc info;
	return &info;
}

static FPInterfaceDesc nlainfo_FPinterface(
	I_LAYERINFO_FP, _T("LayerInfoFPInterface"), 0, NULL, FP_MIXIN,

	NLAInfo::fnGetTransformNode, _T("GetTransformNode"), 0, TYPE_INODE, 0, 0,

	properties,

	NLAInfo::propGetName, NLAInfo::propSetName, _T("LayerName"), 0, TYPE_TSTR_BV,
	NLAInfo::propGetColour, NLAInfo::propSetColour, _T("LayerColour"), 0, TYPE_COLOR_BV,
	NLAInfo::propGetLayerType, FP_NO_FUNCTION, _T("LayerType"), 0, TYPE_TSTR_BV,
	NLAInfo::propGetEnabled, NLAInfo::propSetEnabled, _T("LayerEnabled"), 0, TYPE_BOOL,
	NLAInfo::propGetTransformNodeOn, NLAInfo::propSetTransformNodeOn, _T("TransformNodeOn"), 0, TYPE_BOOL,
	NLAInfo::propGetRemoveDisplacementOn, NLAInfo::propSetRemoveDisplacementOn, _T("RemoveDisplacement"), 0, TYPE_BOOL,
	NLAInfo::propGetLayerIndex, FP_NO_FUNCTION, _T("LayerIndex"), 0, TYPE_INDEX,
	NLAInfo::propGetCATParent, FP_NO_FUNCTION, _T("CATParent"), 0, TYPE_INODE,

	p_end
);

FPInterfaceDesc* NLAInfo::GetDescByID(Interface_ID id) {
	if (id == I_LAYERINFO_FP) return &nlainfo_FPinterface;
	return &nullInterface;
}

void NLAInfo::Init() {
	strName = GetString(IDS_NEWLYR);		// The name of the layer
	dwFlags = 0;;					// Flags describing the properties of this layer
	dwColour = RGB(32 + (rand() % 224), 32 + (rand() % 224), 32 + (rand() % 224));				// The colour of the layer

	weight = NULL;					// weights for this layer
	timewarp = NULL;				// weight view - shows weights hierarchy for each layer
	transform = NULL;
	iTransform = NEVER;
	tmTransform = Matrix3(1);
	tmInvOriginal = Matrix3(1);
	transform_node = NULL;

	nParentLayerCache = 0;			// This is managed by NLAInfo

	iTransform = NEVER;
	root_hub_displacement_valid = NEVER;
	ort_in = ort_out = ORT_CONSTANT;

	dwFileVersion = 0;

	// default new ghosts to being 'on'
	dwFlags |= LAYER_DISPLAY_GHOST;

	bbox.Init();
}

NLAInfo::NLAInfo(BOOL loading) : index(0) {

	Init();
	if (!loading)
	{
		Control* weightCtrl = NewDefaultFloatController();
		{ // Scope Suspend
			HoldSuspend suspend;
			// default our weight to being 100%
			float on = 1.0f;
			weightCtrl->SetValue(0, (void*)&on, 1, CTRL_ABSOLUTE);
		}
		ReplaceReference(REF_WEIGHT, weightCtrl);
	}
}

void NLAInfo::Initialise(CATClipRoot* root, const TSTR& name, ClipLayerMethod method, int index)
{
	{ // Scope Suspend
		HoldSuspend suspend;

		SetNLARoot(root);
		this->index = index;
		this->SetName(name);
		this->SetMethod(method);

		loop_range = GetCOREInterface()->GetAnimRange();
	}

	// only absolute layers can have a ghost transform
	if (CanTransform()) {
		ReplaceReference(NLAInfo::REF_TRANSFORM, NewDefaultMatrix3Controller());
	}

}
void NLAInfo::PostLayerCreateCallback() {

	CATClipRoot* root = GetNLARoot();
	if (root != NULL && CanTransform()) {
		// clear the inherit scale flags
		// this is to make stretchy mode a lot easier to work with
		int flags = INHERIT_ALL;
		flags &= ~(INHERIT_SCL_X | INHERIT_SCL_Y | INHERIT_SCL_Z);
		TimeValue t = GetCOREInterface()->GetTime();

		ICATParentTrans* pCATParent = GetCATParentTrans();
		if (pCATParent != NULL)
		{
			pCATParent->CATMessage(t, CLIP_LAYER_SET_INHERITANCE_FLAGS, flags);

			{ // Scope suspensions
				AnimateSuspend suspendAnim(TRUE, TRUE, FALSE); // Suspend SetKeys etc
				HoldSuspend suspendHold;

				// Call all the 'Post Layer Assign Callbacks'
				pCATParent->CATMessage(t, CLIP_LAYER_CALL_PLACB);

				// disable this layer to prevent it from affecting the
				// character transform, and key it with the current pose
				EnableLayer(FALSE);
				// In case this flag has been left on
				root->ClearFlag(CLIP_FLAG_COLLAPSINGLAYERS);
				pCATParent->CATMessage(t, CAT_KEYFREEFORM);
				EnableLayer(TRUE);			// re-enable the layer
			}
		}
	}
}

void NLAInfo::SetNLARoot(CATClipRoot* pNewRoot)
{
	m_root.SetRef(pNewRoot);
}

CATClipRoot* NLAInfo::GetNLARoot()
{
	return static_cast<CATClipRoot*>(m_root.GetRef());
}

NLAInfo::~NLAInfo()
{
	DeleteAllRefs();
}

class PostPatchNLAInfoClone : public PostPatchProc
{
private:
	NLAInfo* mpClonedOwner;
	RefTargetHandle mpOrigRoot;

public:

	PostPatchNLAInfoClone(NLAInfo* pClonedOwner, CATClipRoot* pOrigRoot)
		: mpClonedOwner(pClonedOwner)
		, mpOrigRoot(pOrigRoot)
	{
		DbgAssert(pClonedOwner != NULL);
	}

	~PostPatchNLAInfoClone() {};

	// The proc needs to
	int Proc(RemapDir& remap)
	{
		if (mpClonedOwner != NULL)
		{
			RefTargetHandle newClonedOwner = NULL;
			remap.PatchPointer(&newClonedOwner, mpOrigRoot);
			if (newClonedOwner == NULL) // If the root was not cloned, then preserve original NLA root
				newClonedOwner = mpOrigRoot;
			mpClonedOwner->SetNLARoot(static_cast<CATClipRoot*>(newClonedOwner));
		}
		return TRUE;
	}
};

void  NLAInfo::CloneNLAInfo(NLAInfo* newNLAInfo, RemapDir &remap)
{
	if (weight) newNLAInfo->ReplaceReference(REF_WEIGHT, remap.CloneRef(weight));
	if (timewarp) newNLAInfo->ReplaceReference(REF_TIMING, remap.CloneRef(timewarp));
	if (transform) newNLAInfo->ReplaceReference(REF_TRANSFORM, remap.CloneRef(transform));
	// Do not clone the node, it is unique per-character.

	newNLAInfo->strName = strName;			// The name of the layer
	newNLAInfo->dwFlags = dwFlags;			// Flags describing the properties of this layer
	newNLAInfo->dwColour = dwColour;			// The colour of the layer
	newNLAInfo->index = index;

	newNLAInfo->range = range;				// when using manual ranges we use this Interval
	newNLAInfo->ort_in = ort_in;
	newNLAInfo->ort_out = ort_out;

	// Reset caches on the cloned NLAInfo
	newNLAInfo->iTransform = NEVER;
	newNLAInfo->tmTransform = Matrix3(1),
		newNLAInfo->tmInvOriginal = Matrix3(1);

	newNLAInfo->nParentLayerCache = nParentLayerCache;	// This is managed by NLAInfo
	PostPatchNLAInfoClone* patchRootProc = new PostPatchNLAInfoClone(newNLAInfo, GetNLARoot());
	remap.AddPostPatchProc(patchRootProc, true);
}

RefTargetHandle NLAInfo::Clone(RemapDir& remap)
{
	NLAInfo *newNLAInfo = new NLAInfo(TRUE);

	CloneNLAInfo(newNLAInfo, remap);

	BaseClone(this, newNLAInfo, remap);

	// now return the new object.
	return newNLAInfo;
}

ReferenceTarget* NLAInfo::GetReference(int i)
{
	switch (i)
	{
	case REF_WEIGHT:			return weight;
	case REF_TIMING:			return timewarp;
	case REF_TRANSFORM:			return transform;
	case REF_TRANSFORMNODE:		return g_bSavingCAT3Rig ? NULL : transform_node;
	default:					return NULL;
	}
}

void NLAInfo::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i)
	{
	case REF_WEIGHT:		weight = (Control*)rtarg;		break;
	case REF_TIMING:		timewarp = (Control*)rtarg;		break;
	case REF_TRANSFORM:		transform = (Control*)rtarg;	break;
	case REF_TRANSFORMNODE:	transform_node = (INode*)rtarg;	break;
	}
}

BOOL NLAInfo::AssignController(Animatable *control, int subAnim)
{
	if (subAnim >= 0 && subAnim < NumSubs()) {
		ReplaceReference(subAnim, (RefTargetHandle)control);
	}
	NotifyDependents(FOREVER, 0, REFMSG_CONTROLREF_CHANGE, TREE_VIEW_CLASS_ID, FALSE);
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	return TRUE;
}

Animatable* NLAInfo::SubAnim(int i)
{
	switch (i)
	{
	case REF_WEIGHT:	return weight;
	case REF_TIMING:	return timewarp;
	case REF_TRANSFORM:	return transform;
	default:			return NULL;
	}
}

TSTR NLAInfo::SubAnimName(int i)
{
	switch (i)
	{
	case REF_WEIGHT:	return GetString(IDS_WEIGHTS);
	case REF_TIMING:	return GetString(IDS_TIMEWARP);
	case REF_TRANSFORM:	return GetString(IDS_TRANSFORM);
	default:			return _T("");
	}
}

ParamDimension* NLAInfo::GetParamDimension(int i) {
	switch (i)
	{
	case REF_WEIGHT:	return stdPercentDim;
	case REF_TIMING:	return stdTimeDim;
	case REF_TRANSFORM:	return defaultDim;
	default:			return defaultDim;
	}
}

RefResult NLAInfo::NotifyRefChanged(const Interval&, RefTargetHandle hTarget,
	PartID&, RefMessage message, BOOL) {
	switch (message)
	{
	case REFMSG_CHANGE:
	{
		CATClipRoot* root = GetNLARoot();
		if (root != NULL)
		{
			ICATParentTrans* pCATParentTrans = root->GetCATParentTrans();
			if (pCATParentTrans != NULL)
			{
				if (hTarget == weight)
				{
					// When adding a layer, we do the weights controller first, and then the timewarp.
					// During an Undo
					TimeValue t = GetCOREInterface()->GetTime();
					pCATParentTrans->CATMessage(t, CLIP_WEIGHTS_CHANGED, index);
					if (!theHold.RestoreOrRedoing())
						GetCOREInterface()->RedrawViews(t);
					UpdateUI();
					break;
				}
				if (hTarget == timewarp)
				{
					int isUndoing = 0;
					if ((!theHold.RestoreOrRedoing() || (theHold.Restoring(isUndoing) && isUndoing) || theHold.Redoing()))
						pCATParentTrans->UpdateCharacter();
					UpdateUI();
				}

				if (hTarget == transform)
				{
					iTransform = NEVER;
					pCATParentTrans->UpdateCharacter();
				}
			}
		}
	}
	break;
	case REFMSG_TARGET_DELETED:
		if (hTarget == weight) { weight = NULL;			break; }
		if (hTarget == timewarp) { timewarp = NULL;		break; }
		if (hTarget == transform) { transform = NULL;		break; }
		if (hTarget == transform_node) { transform_node = NULL;	break; }
		break;
	case REFMSG_TEST_DEPENDENCY:

		break;

	}
	return REF_SUCCEED;
};

void NLAInfo::GetValueLocalTime(TimeValue, void *val, Interval &, GetSetMethod) {
	*(float*)val = 0.0f;
};

void NLAInfo::SetValueLocalTime(TimeValue, void *, int, GetSetMethod) {
};

///////////////////////////////////////////////////////////////////
// TimeRange editing stuff
//
BOOL NLAInfo::SupportTimeOperations()
{
	return TRUE;
	//	return timewarp->SupportTimeOperations();
}

void AddControllerTimeRange(Control* ctrl, DWORD flags, Interval &range) {
	Interval iv;
	iv = ctrl->GetTimeRange(flags);
	if (!iv.Empty()) {
		range += iv.Start();
		range += iv.End();
	}
}

Interval NLAInfo::GetTimeRange(DWORD flags) {

	if (TestFlag(LAYER_RANGEUNLOCKED) || CATEvaluationLock::IsEvaluationLocked())
		return range;

	CATClipRoot* root = GetNLARoot();
	if (root == NULL)
		return FOREVER;

	range = root->GetLayerTimeRange(index, TIMERANGE_ALL | TIMERANGE_CHILDANIMS);

	if (weight)		AddControllerTimeRange(weight, flags, range);
	if (transform)	AddControllerTimeRange(transform, flags, range);
	if (root)		AddControllerTimeRange(root, flags, range);

	return range;
};

int NLAInfo::GetKeyTimes(Tab<TimeValue> &times, Interval range, DWORD flags)
{
	return weight->GetKeyTimes(times, range, flags);
}

void NLAInfo::EditTimeRange(Interval range, DWORD flags)
{
	// CHECK - This function does not initialize the undo system, but
	// others in this file do so - why the difference?
	if (flags&EDITRANGE_LINKTOKEYS) {
		ClearFlag(LAYER_RANGEUNLOCKED);
	}
	else {
		SetFlag(LAYER_RANGEUNLOCKED);
		HoldData(this->range);
		this->range = range;
	}
	if (weight)		weight->EditTimeRange(range, flags);
	if (transform)	transform->EditTimeRange(range, flags);
	CATClipRoot* root = GetNLARoot();
	if (root)		root->EditLayerTimeRange(index, range, flags);
};

void NLAInfo::MapKeys(TimeMap *map, DWORD flags) {

	if (TestFlag(LAYER_RANGEUNLOCKED)) {
		range.Set(map->map(range.Start()), map->map(range.End()));
	}
	if (weight)		weight->MapKeys(map, flags);
	if (transform)	transform->MapKeys(map, flags);

	if (timewarp) {
		timewarp->MapKeys(map, flags);
		IKeyControl *ikc = GetKeyControlInterface(timewarp);
		if (ikc) {
			// When mapping timeworp keys, we must move them up and down
			// at the same time as moving thme side to side
			int num = ikc->GetNumKeys();
			switch (timewarp->ClassID().PartA())
			{
			case HYBRIDINTERP_FLOAT_CLASS_ID: {
				IBezFloatKey	key;
				for (int i = 0; i < num; i++) {
					ikc->GetKey(i, &key);
					key.val = (float)map->map((TimeValue)key.val);
					ikc->SetKey(i, &key);
				}
				break;
			}
			case TCBINTERP_FLOAT_CLASS_ID: {
				ITCBFloatKey	key;
				for (int i = 0; i < num; i++) {
					ikc->GetKey(i, &key);
					key.val = (float)map->map((TimeValue)key.val);
					ikc->SetKey(i, &key);
				}
				break;
			}
			case LININTERP_FLOAT_CLASS_ID: {
				ILinFloatKey	key;
				for (int i = 0; i < num; i++) {
					ikc->GetKey(i, &key);
					key.val = (float)map->map((TimeValue)key.val);
					ikc->SetKey(i, &key);
				}
				break;
			}
			}
		}
	}

	CATClipRoot* root = GetNLARoot();
	if (root) root->MapLayerKeys(index, map, flags);
};
///////////////////////////////////////////////////////////////////

int  NLAInfo::GetORT(int type)
{
	if (type == ORT_BEFORE && TestFlag(LAYER_MANUAL_ORT_IN))
		return ort_in;
	if (type == ORT_AFTER && TestFlag(LAYER_MANUAL_ORT_OUT))
		return ort_out;
	return ORT_CONSTANT;
}

void NLAInfo::SetORT(int ort, int type)
{
	if (type == ORT_BEFORE) {
		SetFlag(LAYER_MANUAL_ORT_IN);
		HoldData(ort_in);
		ort_in = ort;
	}
	else if (type == ORT_AFTER) {
		SetFlag(LAYER_MANUAL_ORT_OUT);
		HoldData(ort_out);
		ort_out = ort;
	}
	if (weight)		weight->SetORT(ort, type);
	if (transform)	transform->SetORT(ort, type);
	CATClipRoot* root = GetNLARoot();
	if (root)		root->SetLayerORT(index, ort, type);
}

void NLAInfo::GetWorldBoundBox(TimeValue, INode *, ViewExp*, Box3& box)
{
	box += bbox;
}

//////////////////////////////////////////////////
// NLAInfor Undos
enum {
	NLAINFO_UNDO_FLAGS,
	NLAINFO_UNDO_NAME,
	NLAINFO_UNDO_COLOUR,
	NLAINFO_UNDO_CREATETRANSFORMNODE,
	NLAINFO_UNDO_EDITRANGES
};

///////////////////////////////////////////////////////////////////
// NLAInfor Methods

Matrix3 NLAInfo::GetTransform(TimeValue t, Interval &valid)
{
	if (!CanTransform()) return Matrix3(1);
	if (!transform || iTransform.InInterval(t)) {
		valid &= iTransform;
		return tmTransform;
	}

	iTransform = FOREVER;
	tmTransform = Matrix3(1);

	Control *position = transform->GetPositionController();
	Control *rotation = transform->GetRotationController();
	//	Control *scale    = transform->GetScaleController();
	if (position && rotation)
	{
		position->GetValue(t, (void*)&tmTransform, iTransform, CTRL_RELATIVE);
		rotation->GetValue(t, (void*)&tmTransform, iTransform, CTRL_RELATIVE);
	}
	else
	{
		transform->GetValue(t, (void*)&tmTransform, iTransform, CTRL_RELATIVE);
		tmTransform.NoScale();
	}

	if (TestFlag(LAYER_REMOVE_DISPLACEMENT)) {

		CATNodeControl *root_hub = GetCATParentTrans()->GetRootHub();

		if (root_hub) {
			Interface *ip = GetCOREInterface();
			// We cannot use the Gettime method because that only works during interactive playback.
		//	TimeValue world_t = ip->GetTime();
			TimeValue world_t = t;

			HoldSuspend suspend;
			DisableRefMsgs();
			BOOL scene_redraw_disabled = ip->IsSceneRedrawDisabled();
			if (!scene_redraw_disabled) ip->DisableSceneRedraw();

			ClearFlag(LAYER_REMOVE_DISPLACEMENT, FALSE);
			int solo = GetNLARoot()->GetSoloLayer();
			GetNLARoot()->SoloLayer(GetIndex());

			iTransform = NEVER;
			Matrix3 tm1 = root_hub->GetNodeTM(loop_range.Start());
			iTransform = NEVER;
			Matrix3 tm2 = root_hub->GetNodeTM(loop_range.End());
			Point3 vec = (tm1.GetTrans() - tm2.GetTrans());

			float ratio = (float)(world_t - loop_range.Start()) / (float)(loop_range.End() - loop_range.Start());
			ratio = (ratio > 1.0f) ? 1.0f : (ratio < 0.0f ? 0.0f : ratio);
			vec *= ratio;

			tmTransform.SetTrans(tmTransform.GetTrans() + vec);

			if (!scene_redraw_disabled) ip->EnableSceneRedraw();

			SetFlag(LAYER_REMOVE_DISPLACEMENT, TRUE, FALSE);
			GetNLARoot()->SoloLayer(solo);

			root_hub->GetNode()->InvalidateTM();
			EnableRefMsgs();
		}
		iTransform.SetInstant(t);
	}

	valid &= iTransform;

	return tmTransform;
}
void NLAInfo::SetTransform(TimeValue t, Matrix3 tm) {
	if (transform) {
		SetXFormPacket ptr(tm);
		transform->SetValue(t, (void*)&ptr);
	}
}

Point3 NLAInfo::GetScale(TimeValue t, Interval &valid) {
	Point3 p3Scale;
	if (!transform) return P3_IDENTITY_SCALE;
	Control *scale = transform->GetScaleController();
	if (scale)
	{
		ScaleValue svScale;
		scale->GetValue(t, (void*)&svScale, valid, CTRL_ABSOLUTE);
		p3Scale = svScale.s;
	}
	else
	{
		Matrix3 val(1);
		transform->GetValue(t, (void*)&val, valid, CTRL_RELATIVE);
		p3Scale[0] = Length(val.GetRow(0));
		p3Scale[1] = Length(val.GetRow(1));
		p3Scale[2] = Length(val.GetRow(2));
	}
	return p3Scale;
}

void NLAInfo::MakeLoopable(TimeValue t, INode* node, Matrix3 &tm, BOOL edit_pos, BOOL edit_rot)
{
	UNREFERENCED_PARAMETER(edit_rot);
	Interface *ip = GetCOREInterface();
	if (!node->GetParentNode()->IsRootNode()) return;

	CATNodeControl *root_hub = GetCATParentTrans()->GetRootHub();
	DbgAssert(root_hub);

	theHold.Suspend();
	DisableRefMsgs();
	BOOL scene_redraw_disabled = ip->IsSceneRedrawDisabled();
	if (!scene_redraw_disabled) ip->DisableSceneRedraw();

	ClearFlag(LAYER_MAKE_LOOPABLE);
	int solo = GetNLARoot()->GetSoloLayer();
	GetNLARoot()->SoloLayer(GetIndex());

	Matrix3 tm1, tm2;

	if (!root_hub_displacement_valid.InInterval(t)) {
		tm1 = root_hub->GetNode()->GetNodeTM(loop_range.Start());
		tm2 = root_hub->GetNode()->GetNodeTM(loop_range.End());
		root_hub_displacement = (tm1.GetTrans() - tm2.GetTrans());
		root_hub_displacement_valid.SetInstant(t);
	}

	if (node != root_hub->GetNode()) {
		tm1 = node->GetNodeTM(loop_range.Start());
		tm2 = node->GetNodeTM(loop_range.End());
	}

	float ratio;
	if (t > loop_range.End())
		ratio = 1.0f;
	else if (t < loop_range.Start())
		ratio = 0.0f;
	else {
		ratio = (float)(ip->GetTime() - loop_range.Start()) / (float)(loop_range.End() - loop_range.Start());
	}

	if (edit_pos && node->GetParentNode()->IsRootNode()) {
		if (node != root_hub->GetNode()) {
			Point3 vec = (tm1.GetTrans() - tm2.GetTrans());
			vec -= root_hub_displacement;
			vec -= (vec*0.5f);
			tm.SetTrans(tm.GetTrans() + (vec*((ratio - 0.5f)*2.0f)));
		}
	}
	/*	if(edit_rot){
			Quat qt(tm2 * Inverse(tm1));
			Slerp(qt, Quat(0.0f, 0.0f, 1.0f, 0.0f), 0.5f);
			ratio -= 0.5f;
			ratio *= 2.0f;
			if(ratio<0.0f ){
				ratio *= -1.0f;
				qt = Inverse(qt);
			}
			Slerp(qt, Quat(0.0f, 0.0f, 1.0f, 0.0f), (ratio > 1.0f) ? 1.0f : (ratio < 0.0f ? 0.0f : ratio));

			Point3 pos = tm.GetTrans();
			Matrix3 rot; qt.MakeMatrix(rot);
			tm = tm * rot;
			tm.SetTrans(pos);
		}
	*/
	if (!scene_redraw_disabled) ip->EnableSceneRedraw();

	// put things back the way they were
	GetNLARoot()->SoloLayer(solo);
	SetFlag(LAYER_MAKE_LOOPABLE);
	EnableRefMsgs();
	theHold.Resume();

	node->InvalidateTM();
	root_hub->GetNode()->InvalidateTM();
}

// this method creates a transform node to be use to move th layer around with
INode* NLAInfo::CreateTransformNode()
{
	if (!CanTransform()) return NULL;
	LayerTransform* layertransform = (LayerTransform*)CreateInstance(CTRL_MATRIX3_CLASS_ID, LAYER_TRANSFORM_CLASS_ID);
	INode* node = layertransform->Initialise(this, transform);

	ReplaceReference(REF_TRANSFORMNODE, node);
	iTransform = NEVER;
	UpdateUI();
	return node;
}

void NLAInfo::DestroyTransformNode()
{
	if (transform_node) {
		transform_node->Delete(GetCOREInterface()->GetTime(), FALSE);
		//	DeleteReference(REF_TRANSFORMNODE);
		transform_node = NULL;
	}
}

ICATParentTrans*	NLAInfo::GetCATParentTrans()
{
	CATClipRoot* root = GetNLARoot();
	if (root != NULL)
		return root->GetCATParentTrans();
	return NULL;
};
INode*				NLAInfo::IGetCATParent()
{
	ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent != NULL)
		return pCATParent->GetNode();
	return NULL;
};

void NLAInfo::SetName(TSTR str) {
	// the Edit box on the UI is constantly sending messages to tell us to rename the layer
	if (str != strName)
	{
		// register an Undo
		HoldActions hold(IDS_HLD_LYRNAME);
		HoldData(strName, this);
		strName = str;

		UpdateUI();
	}
};

void	NLAInfo::SetFlag(DWORD flag, BOOL on/*=TRUE*/, BOOL enable_undo/*=TRUE*/) {
	// register an Undo
	if (enable_undo)
	{
		HoldActions hold(IDS_HLD_LYRFLAG);
		HoldData(dwFlags, this);
		if (on) dwFlags |= flag; else dwFlags &= ~flag;
	}
	else
		if (on) dwFlags |= flag; else dwFlags &= ~flag;
}

void	NLAInfo::ClearFlag(DWORD flag, BOOL enable_undo/*=TRUE*/) {
	SetFlag(flag, FALSE, enable_undo);
}

void NLAInfo::EnableLayer(BOOL bEnable)
{
	HoldActions hold(bEnable ? IDS_HLD_LYRENABLE : IDS_HLD_LYRDISABLE);
	SetFlag(LAYER_DISABLE, !bEnable);

	// Automaticly hide transform nodes for nodes that are disabled
	// Gregor Weiss asked for this
	if (transform_node)
		transform_node->Hide(!bEnable);

	UpdateUI();

	ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent != NULL)
	{
		if (pCATParent->GetEffectiveColourMode() != COLOURMODE_CLASSIC)
			pCATParent->UpdateColours(FALSE, TRUE);
		// We must update the character. This method is called frequently during Batch Export
		pCATParent->UpdateCharacter();
	}
}

void NLAInfo::SetColour(Color clr) {
	DWORD newColor = clr.toRGB();
	if (newColor == dwColour)
		return;

	// register an Undo
	HoldActions hold(IDS_HLD_LYRCOLOR);
	HoldData(dwColour, this);
	dwColour = newColor;

	// Trigger bones to update their colors
	ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent)
		pCATParent->UpdateColours();

	UpdateUI();
}

void NLAInfo::SetWeight(TimeValue t, float val) {
	if (weight)
	{
		HoldActions hold(IDS_HLD_LYRWGT);
		weight->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
	}
}

void NLAInfo::CreateTimeController()
{
	if (timewarp == NULL)
	{
		Control* timingCtrl = (Control*)CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(LININTERP_FLOAT_CLASS_ID, 0));

		// In case we are already holding, do not create undo items for the init actions.
		{
			HoldSuspend suspend;

			// here we create a straight line that will not distort time at all by default
			IKeyControl* pTimingKeys = GetKeyControlInterface(timingCtrl);
			DbgAssert(pTimingKeys != NULL);
			if (pTimingKeys != NULL)
			{
				ILinFloatKey aKey;
				Interval animationrange = GetCOREInterface()->GetAnimRange();
				TimeValue starttime = animationrange.Start();
				TimeValue endtime = animationrange.End();

				// In case of a 0-length animation, default to
				// 100 frames difference.  It really doesn't matter
				// the actual values as long as they are equivalent
				if (starttime == endtime)
					endtime = starttime + (GetTicksPerFrame() * 100);

				aKey.time = aKey.val = starttime;
				pTimingKeys->AppendKey(&aKey);

				aKey.time = aKey.val = endtime;
				pTimingKeys->AppendKey(&aKey);
			}

			timingCtrl->SetORT(ORT_LINEAR, ORT_BEFORE);
			timingCtrl->SetORT(ORT_LINEAR, ORT_AFTER);
		}

		// if we are not holding, we want to be able to undo this!
		HoldActions(IDS_TT_CREATETIMEWARPS);
		ReplaceReference(REF_TIMING, timingCtrl);
	}
}

TimeValue	NLAInfo::GetTime(TimeValue t) { //, Interval &valid){
	float val;
	if (timewarp) {
		// if somebody has deleted all the keys on the whole layer
		// they may have deleted the timewarp keys, so if there are no keys than return t
		IKeyControl *ikeys = GetKeyControlInterface(timewarp);
		if (ikeys && (ikeys->GetNumKeys() == 0 || ikeys->GetNumKeys() == 1)) return (TimeValue)t;
		Interval valid = FOREVER;
		timewarp->GetValue(t, (void*)&val, valid, CTRL_ABSOLUTE);
		return (TimeValue)val;
	}
	return t;
}

void NLAInfo::SetTime(TimeValue t, float val)
{
	HoldActions hold(IDS_HLD_LYRTIME);
	if (!timewarp)
		CreateTimeController();
	DbgAssert(timewarp != NULL);
	if (timewarp != NULL)
		timewarp->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
}

void NLAInfo::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	switch (ctxt)
	{
	case kSNCDelete:
	case kSNCFileSave:
	case kSNCFileMerge:
		if (transform_node != NULL)
			nodes.Append(1, &transform_node);
	case kSNCClone:
		// do NOT clone the LTG.
		break;
	default:
		DbgAssert("UnHandled case here");
	}
}

// Backwards compatibility
//

class NLAInfoPLCB : public PostLoadCallback {
protected:
	NLAInfo *ctrl;
	CATClipRoot* mpRoot;
public:
	NLAInfoPLCB(NLAInfo *pOwner)
		: ctrl(pOwner)
		, mpRoot(NULL)
	{ }

	DWORD GetFileSaveVersion() {
		if (ctrl->dwFileVersion < CAT_VERSION_2300) return CAT_VERSION_2200;
		return ctrl->dwFileVersion;
	}

	// We need this PLCB to be called before the ClipHierarchy
	// gets destroyed by the CATClip Roots PLCB.
	int Priority() { return 3; }

	void RecordRootBackpatch(ILoad* pLoader, int refId)
	{
		if (pLoader != NULL)
			pLoader->RecordBackpatch(refId, reinterpret_cast<void**>(&mpRoot));
	}

	void proc(ILoad *iload) {

		// First, restore our root.  Our root must be loaded by now.
		if (ctrl != NULL)
			ctrl->SetNLARoot(mpRoot);

		if (GetFileSaveVersion() < CAT_VERSION_2300) {
			ctrl->SetLoopRange(GetCOREInterface()->GetAnimRange());
			iload->SetObsolete();
		}

		// To clean up, delete the PLCB.  This class must
		// obviously have been created by calling 'new'.
		delete this;
	}
};

// Load/Save stuff
enum {
	NLAINFO_FILEVERSION_CHUNK,
	NLAINFO_NAME_CHUNK,
	NLAINFO_FLAGS_CHUNK,
	NLAINFO_COLOUR_CHUNK,
	NLAINFO_ROOT_CHUNK,
	NLAINFO_INDEX_CHUNK,
	NLAINFO_MANUAL_RANGE_CHUNK,
	NLAINFO_ORT_IN_CHUNK,
	NLAINFO_ORT_OUT_CHUNK,
	NLAINFO_WALKONSPOT_RANGE_CHUNK,
};

IOResult NLAInfo::Save(ISave *isave)
{
	DWORD nb, refID;
	DWORD dwFileVersion = CAT_VERSION_CURRENT;

	// Save out our file version
	isave->BeginChunk(NLAINFO_FILEVERSION_CHUNK);
	isave->Write(&dwFileVersion, sizeof(DWORD), &nb);
	isave->EndChunk();

	// ClipLayerInfo::strName -- Layer name
	isave->BeginChunk(NLAINFO_NAME_CHUNK);
	isave->WriteCString(strName.data());
	isave->EndChunk();

	// our layer index in the Layer stack on CATClipRoot
	isave->BeginChunk(NLAINFO_INDEX_CHUNK);
	isave->Write(&index, sizeof(int), &nb);
	isave->EndChunk();

	// ClipLayerInfo::dwFlags -- Layer flags
	isave->BeginChunk(NLAINFO_FLAGS_CHUNK);
	isave->Write(&dwFlags, sizeof(DWORD), &nb);
	isave->EndChunk();

	// ClipLayerInfo::dwLayerColour -- Ghost colour
	isave->BeginChunk(NLAINFO_COLOUR_CHUNK);
	isave->Write(&dwColour, sizeof(COLORREF), &nb);
	isave->EndChunk();

	// Handle to mister CATParent
	CATClipRoot* root = GetNLARoot();
	DbgAssert(root != NULL);
	if (root != NULL)
	{
		refID = isave->GetRefID((void*)root);
		isave->BeginChunk(NLAINFO_ROOT_CHUNK);
		isave->Write(&refID, sizeof(DWORD), &nb);
		isave->EndChunk();
	}
	isave->BeginChunk(NLAINFO_MANUAL_RANGE_CHUNK);
	isave->Write(&range, sizeof(Interval), &nb);
	isave->EndChunk();

	isave->BeginChunk(NLAINFO_ORT_IN_CHUNK);
	isave->Write(&ort_in, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(NLAINFO_ORT_OUT_CHUNK);
	isave->Write(&ort_out, sizeof(int), &nb);
	isave->EndChunk();

	isave->BeginChunk(NLAINFO_WALKONSPOT_RANGE_CHUNK);
	isave->Write(&loop_range, sizeof(Interval), &nb);
	isave->EndChunk();

	return IO_OK;
}

IOResult NLAInfo::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	DWORD nb = 0L, refID = 0L;
	NLAInfoPLCB* pPostLoad = new NLAInfoPLCB(this);

	while (IO_OK == (res = iload->OpenChunk())) {
		switch (iload->CurChunkID()) {

		case NLAINFO_FILEVERSION_CHUNK:
			res = iload->Read(&dwFileVersion, sizeof(DWORD), &nb);
			break;
		case NLAINFO_NAME_CHUNK:
			TCHAR *strBuf;
			res = iload->ReadCStringChunk(&strBuf);
			if (res == IO_OK) strName = TSTR(strBuf);
			break;
		case NLAINFO_INDEX_CHUNK:
			res = iload->Read(&index, sizeof(int), &nb);
			break;
		case NLAINFO_COLOUR_CHUNK:
			res = iload->Read(&dwColour, sizeof(COLORREF), &nb);
			break;
		case NLAINFO_FLAGS_CHUNK:
			res = iload->Read(&dwFlags, sizeof(DWORD), &nb);
			break;
		case NLAINFO_ROOT_CHUNK:
			res = iload->Read(&refID, sizeof(int), &nb);
			if (res == IO_OK && refID != (DWORD)-1)
				pPostLoad->RecordRootBackpatch(iload, refID);
			break;
		case NLAINFO_MANUAL_RANGE_CHUNK:
			res = iload->Read(&range, sizeof(Interval), &nb);
			break;
		case NLAINFO_ORT_IN_CHUNK:
			res = iload->Read(&ort_in, sizeof(int), &nb);
			break;
		case NLAINFO_ORT_OUT_CHUNK:
			res = iload->Read(&ort_out, sizeof(int), &nb);
			break;
		case NLAINFO_WALKONSPOT_RANGE_CHUNK:
			res = iload->Read(&loop_range, sizeof(Interval), &nb);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	// Register post-load callbacks to make necessary adjustments
	// for files saved with older versions of CAT.  These callbacks
	// must of course be registered in an appropriate order.
	iload->RegisterPostLoadCallback(pPostLoad);

	return IO_OK;
}

//**********************************************************************//
/* Here we get the clip saver going                                     */
//**********************************************************************//
BOOL NLAInfo::SaveClip(CATRigWriter *save, int flags, Interval timerange)
{
	if (!(flags&CLIPFLAG_CLIP)) {
		return TRUE;
		//		save->WritePose(timerange.Start(), this);
	}
	else {
		save->BeginGroup(idNLAInfo);

		save->Write(idTimeRange, loop_range);

		LayerInfo layerinfo(dwFlags, dwColour, strName);
		save->Write(idValLayerInfo, layerinfo);

		save->BeginGroup(idWeights);
		save->WriteController(weight, flags, timerange);
		save->EndGroup();

		save->BeginGroup(idTimeWarp);
		save->WriteController(timewarp, flags | CLIPFLAG_TIMEWARP_KEYFRAMES, FOREVER);
		save->EndGroup();

		save->BeginGroup(idController);
		save->WriteController(transform, flags, timerange);
		save->EndGroup();

		save->EndGroup();
	}
	return TRUE;
}

// This
BOOL NLAInfo::LoadClip(CATRigReader *load, Interval range, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idNLAInfo) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idWeights: {
				DbgAssert(weight);
				// we don't need to process the values comming in here at all
				int newflags = flags;
				newflags &= ~CLIPFLAG_APPLYTRANSFORMS;
				newflags &= ~CLIPFLAG_MIRROR;
				newflags &= ~CLIPFLAG_SCALE_DATA;
				load->ReadSubAnim(this, REF_WEIGHT, range, dScale, newflags);
				ICATParentTrans* pCATParent = GetCATParentTrans();
				if (pCATParent != NULL)
					pCATParent->CATMessage(range.Start(), CLIP_WEIGHTS_CHANGED, index);
				break;
			}
			case idTimeWarp: {
				if (timewarp == NULL)
					CreateTimeController();
				DbgAssert(timewarp);

				// we don't need to process the values comming in here at all
				int newflags = flags;
				newflags &= ~CLIPFLAG_APPLYTRANSFORMS;
				newflags &= ~CLIPFLAG_MIRROR;
				newflags &= ~CLIPFLAG_SCALE_DATA;
				newflags |= CLIPFLAG_TIMEWARP_KEYFRAMES;

				timewarp->DeleteKeys(TRACK_DOALL + TRACK_RIGHTTOLEFT);
				load->ReadSubAnim(this, REF_TIMING, range, dScale, newflags);

				// For a timewarp controller, the our
				// key value (out time) is relative to the
				// key time (in time).  So needs to be
				// offset as well!

				break;
			}
			case idController:
				// If loading a clip, dont modify it. Set it directly into the layer.
				// If we are applying transforms, to the loaded keyframes, or mirring our animation, then we do this
				// in global space, and the keyframe values that are loaded are modified,
				// We don't want our transfomr node being modified also.
				if (transform && !(flags&CLIPFLAG_MIRROR || flags&CLIPFLAG_APPLYTRANSFORMS)) {
					load->ReadSubAnim(this, REF_TRANSFORM, range, dScale, flags);
				}
				else load->SkipGroup();
				break;
			}
			break;
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idValLayerInfo: {
				LayerInfo layerinfo;
				load->GetValue(layerinfo);
				dwFlags = layerinfo.dwFlags;
				dwColour = layerinfo.dwLayerColour;
				strName = layerinfo.strName;

				if (GetMethod() == LAYER_ABSOLUTE) {
					BOOL bIsAnimating = Animating();
					if (bIsAnimating) AnimateOff();
					// Make the Transform controler that wasn't created by default
					ReplaceReference(REF_TRANSFORM, NewDefaultMatrix3Controller());
					ICATParentTrans* pCATParent = GetCATParentTrans();
					if (pCATParent != NULL)
						pCATParent->CATMessage(range.Start(), CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER, index);
					if (bIsAnimating) AnimateOn();
				}
				break;
			}
			case idTimeRange:
				load->GetValue(loop_range);
				break;
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

// This Code was taken from CATClipValues Load Clip Function. This is because all the layer weights
// used to be stored in one big CATClipValue. Now each NLAInfo has its own weights controller
//
BOOL NLAInfo::LoadPreCAT2Weights(CATRigReader *load, Interval range, float dScale, int flags)
{
	// we don't need to process the values comming in here at all
	flags = flags&~CLIPFLAG_APPLYTRANSFORMS;
	flags = flags&~CLIPFLAG_MIRROR;
	flags = flags&~CLIPFLAG_SCALE_DATA;

	if (load->NextClause())
	{
		// Here we find out what our base controller is
		switch (load->CurIdentifier()) {
		case idValClassIDs:
		{
			TokClassIDs classid;
			load->GetValue(classid);

			Class_ID newClassID(classid.usClassIDA, classid.usClassIDB);
			Control *oldLayer = weight;

			if (!oldLayer || oldLayer->ClassID() != newClassID)
			{
				Control *newLayer = (Control*)CreateInstance(classid.usSuperClassID, newClassID);
				DbgAssert(newLayer);
				if (oldLayer) newLayer->Copy(oldLayer);
				ReplaceReference(REF_WEIGHT, newLayer);
				oldLayer = newLayer;
			}

			// If loading a clip, dont modify it. Set it directly into the layer.
			return load->ReadController(oldLayer, range, dScale, flags);
		}
		// maybe we are a pose, in which case we really don't need a classID
		case idValFloat:
			load->ReadPoseIntoController(this, range.Start(), dScale, flags);
			load->NextClause();
			break;
		}

	}
	return FALSE;

}

//////////////////////////////////////////////////////////////////////////
// Undo system callbacks

void NLAInfo::UpdateUI()
{
	CATClipRoot* pClipRoot = GetNLARoot();
	if (pClipRoot)
		pClipRoot->UpdateUI();
}

void NLAInfo::OnRestoreDataChanged(COLORREF val)
{
	// Trigger bones to update their colors
	// Setting the wire color is not an undoable
	// operation in Max normally, so we manually
	// trigger the update here.
	ICATParentTrans* pCATParent = GetCATParentTrans();
	if (pCATParent)
	{
		pCATParent->UpdateCharacter();
		pCATParent->UpdateColours();
	}
	UpdateUI();
}
