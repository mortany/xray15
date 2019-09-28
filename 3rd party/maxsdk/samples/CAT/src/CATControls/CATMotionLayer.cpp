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
#include "CATMotionLayer.h"
#include "CATClipRoot.h"
#include "CATHierarchyBranch2.h"
#include "../CATAPI.h"

static bool catmotionlayer_InterfaceAdded = false;
class CATMotionLayerClassDesc : public INLAInfoClassDesc {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE)
	{
		UNREFERENCED_PARAMETER(loading);
		CATMotionLayer* catmotionlayer = new CATMotionLayer(loading);
		if (!catmotionlayer_InterfaceAdded)
		{
			AddInterface(catmotionlayer->GetDescByID(I_LAYERINFO_FP));
			AddInterface(GetCATHierarchyRootDesc()->GetInterface(CATHIERARCHYROOT_INTERFACE));
			catmotionlayer_InterfaceAdded = true;
		}
		return catmotionlayer;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONLAYER); }
	SClass_ID		SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONLAYER_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionLayer"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionLayerClassDesc CATMotionLayerDesc;
INLAInfoClassDesc* GetCATMotionLayerDesc() { return &CATMotionLayerDesc; }

CATMotionLayer::CATMotionLayer(BOOL loading)
	: NLAInfo(loading)
{

	cathierarchyroot = NULL;

	//	if(!loading){
	//		ReplaceReference(REF_WEIGHT, NewDefaultFloatController());
	//	}
}

CATMotionLayer::~CATMotionLayer()
{
	DeleteAllRefs();
}

RefTargetHandle CATMotionLayer::GetReference(int i)
{
	if (i < NLAInfo::NumRefs())
		return NLAInfo::GetReference(i);
	i -= NLAInfo::NumRefs();

	if (i == 0) return cathierarchyroot;
	return NULL;
}

void CATMotionLayer::SetReference(int i, RefTargetHandle rtarg)
{
	if (i < NLAInfo::NumRefs())
		NLAInfo::SetReference(i, rtarg);
	i -= NLAInfo::NumRefs();

	if (i == 0) cathierarchyroot = (CATHierarchyRoot*)rtarg;
}

Animatable* CATMotionLayer::SubAnim(int i)
{
	if (i < NLAInfo::NumSubs()) return NLAInfo::SubAnim(i);
	//	i -= NLAInfo::NumSubs();

	switch (i)
	{
	case REF_CATHIERARHYROOT:	return cathierarchyroot;
	default:					return NULL;
	}
}

TSTR CATMotionLayer::SubAnimName(int i)
{
	if (i < NLAInfo::NumSubs()) return NLAInfo::SubAnimName(i);
	//	i -= NLAInfo::NumSubs();

	switch (i)
	{
	case REF_CATHIERARHYROOT:	return GetString(IDS_CATHIERARCHYROOT);
	default:					return _T("");
	}
}

Interval CATMotionLayer::GetTimeRange(DWORD flags) {

	return cathierarchyroot ? cathierarchyroot->GetCATMotionRange() : NLAInfo::GetTimeRange(flags);
};

void CATMotionLayer::MapKeys(TimeMap *map, DWORD flags) {

	if (!cathierarchyroot) NLAInfo::MapKeys(map, flags);

	Interval range = cathierarchyroot->GetCATMotionRange();
	range.Set(map->map(range.Start()), map->map(range.End()));

	DisableRefMsgs();
	cathierarchyroot->SetStartTime((float)(range.Start() / GetTicksPerFrame()));
	cathierarchyroot->SetEndTime((float)(range.End() / GetTicksPerFrame()));
	EnableRefMsgs();
	cathierarchyroot->IUpdateDistCovered();
};
void CATMotionLayer::EditTimeRange(Interval range, DWORD flags)
{
	UNREFERENCED_PARAMETER(flags);
	if (!cathierarchyroot) return;
	DisableRefMsgs();
	cathierarchyroot->SetStartTime((float)range.Start());
	cathierarchyroot->SetEndTime((float)range.End());
	EnableRefMsgs();
	cathierarchyroot->IUpdateDistCovered();
};

void CATMotionLayer::Initialise(CATClipRoot* root, const TSTR& name, ClipLayerMethod method, int index) {
	NLAInfo::Initialise(root, name, method, index);

	SetFlag(LAYER_CATMOTION);

	// Build the first bit of the Hierarchy
	CATHierarchyRoot *cathierarchy = (CATHierarchyRoot*)CreateInstance(CTRL_FLOAT_CLASS_ID, CATHIERARCHYROOT_CLASS_ID);
	ReplaceReference(CATMotionLayer::REF_CATHIERARHYROOT, cathierarchy);
};

class SetupCATMotionLayerStartRestore : public RestoreObj {
public:
	SetupCATMotionLayerStartRestore() { DisableRefMsgs(); }
	void Restore(int isUndo) { UNREFERENCED_PARAMETER(isUndo); EnableRefMsgs(); }
	void Redo() {}
	void EndHold() {	}
	int Size() { return 1; }
};

class SetupCATMotionLayerEndRestore : public RestoreObj {
public:
	SetupCATMotionLayerEndRestore() { EnableRefMsgs(); }
	void Restore(int isUndo) { UNREFERENCED_PARAMETER(isUndo); DisableRefMsgs(); }
	void Redo() {}
	void EndHold() {	}
	int Size() { return 1; }
};

void CATMotionLayer::PostLayerCreateCallback()
{
	SuspendAnimate();
	AnimateOff();
	if (theHold.Holding())
		theHold.Put(new SetupCATMotionLayerStartRestore());

	CATClipRoot* root = GetNLARoot();
	if (root == NULL)
		return;

	ICATParentTrans* pCATParentTrans = root->GetCATParentTrans();
	if (pCATParentTrans == NULL)
		return;

	// Make sure our layer contains setup pose values by default
	CATMode catmode = pCATParentTrans->GetCATMode();
	if (catmode != SETUPMODE)
	{
		pCATParentTrans->SetCATMode(SETUPMODE);
		pCATParentTrans->UpdateCharacter();
	}

	//////////////////////////////////////////////////////////////////////////
	EnableLayer(FALSE);			// disable this layer, and key it with the current pose
	// Key the current layer with the Setup pose.
	TimeValue t = GetCOREInterface()->GetTime();
	pCATParentTrans->CATMessage(t, CAT_KEYFREEFORM);
	EnableLayer(TRUE);			// re-enable the layer
	///////////////////////////////////////////////////////////////////////////

	// Let the layer configure its self
	// the magic happens here.
	// ensure all rollouts are closed

	root->SelectLayer(-1);
	if (cathierarchyroot) cathierarchyroot->Initialise(root->AsControl(), this, index);

	if (catmode != SETUPMODE)
		pCATParentTrans->SetCATMode(catmode);

	root->SelectLayer(index);
	ResumeAnimate();

	if (theHold.Holding())
		theHold.Put(new SetupCATMotionLayerEndRestore());
};

void CATMotionLayer::PreLayerRemoveCallback() {
	NLAInfo::PreLayerRemoveCallback();

	if (cathierarchyroot) {
		cathierarchyroot->IDestroyCATWindow();
		cathierarchyroot->SetPathNode((INode*)NULL);
		cathierarchyroot->IRemoveFootPrints(FALSE);
	}
};

void CATMotionLayer::AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt)
{
	NLAInfo::AddSystemNodes(nodes, ctxt);
	if (cathierarchyroot) {
		cathierarchyroot->AddSystemNodes(nodes, ctxt);
	}
}

RefTargetHandle CATMotionLayer::Clone(RemapDir& remap)
{
	CATMotionLayer* newCATMotionLayer = new CATMotionLayer(TRUE);
	remap.AddEntry(this, newCATMotionLayer);

	CloneNLAInfo(newCATMotionLayer, remap);

	newCATMotionLayer->ReplaceReference(REF_CATHIERARHYROOT, remap.CloneRef(cathierarchyroot));

	BaseClone(this, newCATMotionLayer, remap);

	return newCATMotionLayer;
};

//**********************************************************************//
/* Here we get the clip saver going                                     */
//**********************************************************************//
BOOL CATMotionLayer::SaveClip(CATRigWriter *save, int flags, Interval timerange)
{
	if (!(flags&CLIPFLAG_CLIP)) {
		return TRUE;
	}
	else {
		save->BeginGroup(idCATMotionLayer);

		NLAInfo::SaveClip(save, flags, timerange);

		int startime = cathierarchyroot->GetStartTime();
		save->Write(idRangeStart, startime);

		int endtime = cathierarchyroot->GetEndTime();
		save->Write(idRangeEnd, endtime);

		int flags = cathierarchyroot->pblock->GetInt(CATHierarchyRoot::PB_CATMOTION_FLAGS);
		save->Write(idFlags, flags);

		int walkmode = cathierarchyroot->GetWalkMode();
		save->Write(idWalkMode, walkmode);

		INode *node = cathierarchyroot->GetPathNode();
		if (node) {
			save->WriteNode(node);
		}

		int numlayers = cathierarchyroot->GetNumLayers();
		save->Write(idNumLayers, numlayers);
		for (int i = 0; i < numlayers; i++) {
			TSTR layername = cathierarchyroot->GetLayerName(i);
			save->Write(idLayerName, layername);

			save->BeginGroup(idWeights);
			save->WriteController(cathierarchyroot->GetWeights()->GetLayer(i), flags, timerange);
			save->EndGroup();
		}

		save->BeginGroup(idGlobals);
		save->WriteController(cathierarchyroot->GetGlobalsBranch(), flags, timerange);
		save->EndGroup();

		save->BeginGroup(idLimbPhases);
		save->WriteController(cathierarchyroot->GetLimbPhasesBranch(), flags, timerange);
		save->EndGroup();

		int activelayer = cathierarchyroot->GetActiveLayer();
		save->Write(idSelectedLayer, activelayer);

		save->EndGroup();
	}
	return TRUE;
}

// This
BOOL CATMotionLayer::LoadClip(CATRigReader *load, Interval range, float dScale, int flags)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	int loadedlayers = 0;
	int numlayers = -1;
	int activelayer;
	TSTR layername;
	int rangestart, rangeend;
	int catmotionflags, walkmode;

	flags &= ~CLIPFLAG_APPLYTRANSFORMS;
	flags &= ~CLIPFLAG_MIRROR;
	flags &= ~CLIPFLAG_SCALE_DATA;

	DisableRefMsgs();

	while (load->ok() && !done && ok) {
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idCATMotionLayer) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID()) {
		case rigBeginGroup:
			switch (load->CurIdentifier()) {
			case idNLAInfo:
				NLAInfo::LoadClip(load, range, dScale, flags);
				break;
			case idWeights:
				load->ReadSubAnim(cathierarchyroot->GetWeights(), loadedlayers, range, dScale, flags);
				loadedlayers++;
				break;
			case idGlobals:
				load->ReadSubAnim(cathierarchyroot, cathierarchyroot->SubAnimIndex(cathierarchyroot->GetGlobalsBranch()), range, dScale, flags);
				break;
			case idLimbPhases: {
				load->ReadSubAnim(cathierarchyroot, cathierarchyroot->SubAnimIndex(cathierarchyroot->GetLimbPhasesBranch()), range, dScale, flags);
				break;
			}
									 break;
			}
		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idRangeStart:
				load->GetValue(rangestart);
				cathierarchyroot->SetStartTime(((float)rangestart) / ((float)GetTicksPerFrame()));
				break;
			case idRangeEnd:
				load->GetValue(rangeend);
				cathierarchyroot->SetEndTime(((float)rangeend) / ((float)GetTicksPerFrame()));
				break;
			case idFlags:
				load->GetValue(catmotionflags);
				cathierarchyroot->pblock->SetValue(CATHierarchyRoot::PB_CATMOTION_FLAGS, 0, catmotionflags);
				break;
			case idWalkMode:
				load->GetValue(walkmode);
				cathierarchyroot->pblock->SetValue(CATHierarchyRoot::PB_WALKMODE, 0, walkmode);
				break;
			case idNumLayers:
				load->GetValue(numlayers);
				for (int i = 1; i < numlayers; i++) {
					cathierarchyroot->AddLayer();
				}
				break;
			case idLayerName:
				load->GetValue(layername);
				if (loadedlayers < numlayers) {
					cathierarchyroot->SetLayerName(loadedlayers, layername);
				}
				break;
			case idSelectedLayer:
				load->GetValue(activelayer);
				cathierarchyroot->SetActiveLayer(activelayer);
				break;
			case idNode:
			case idNodeName: {
				INode *node = load->GetINode();
				if (!node) break;
				cathierarchyroot->SetPathNode(node);
				break;
			}
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

	EnableRefMsgs();
	cathierarchyroot->IUpdateDistCovered();

	return ok && load->ok();
}

BOOL CATMotionLayer::PostPaste_CleanupLayer()
{
	cathierarchyroot->CleanupTree();
	return TRUE;
}
