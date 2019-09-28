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

// Transform Controller for the feet

#include "CatPlugins.h"
#include <CATAPI/CATClassID.h>

 // Rig Structure
#include "../CATObjects/ICATObject.h"
#include "ICATParent.h"
#include "FootTrans2.h"
#include "LimbData2.h"
#include "Hub.h"
#include "BoneData.h"
#include "../CATObjects/IKTargetObject.h"

// CATMotion
#include "CATHierarchy.h"
#include "CATMotionPlatform.h"
#include "CATMotionLimb.h"

// Layer System
#include "CATClipValues.h"
#include "HDPivotTrans.h"
#include "HIPivotTrans.h"

//
//	FootTrans2ClassDesc
//
//	This gives the MAX information about our class
//	before it has to actually implement it.
//
class FootTrans2ClassDesc : public CATNodeControlClassDesc
{
public:
	CATControl*		DoCreate(BOOL loading = FALSE)
	{
		FootTrans2 *foot = new FootTrans2(loading);
		return foot;
	}
	const TCHAR *	ClassName() { return GetString(IDS_CL_FOOTTRANS2); }
	Class_ID		ClassID() { return FOOTTRANS2_CLASS_ID; }
	const TCHAR*	InternalName() { return _T("FootTrans"); }			// returns fixed parsable name (scripter-visible name)
};

// our global instance of our classdesc class.
static FootTrans2ClassDesc FootTrans2Desc;
ClassDesc2* GetFootTrans2Desc() { return &FootTrans2Desc; }

//	FootTrans2  Implementation.
//

static ParamBlockDesc2 FootTrans2_t_param_blk(FootTrans2::PBLOCK_REF, _T("FootTrans2params"), 0, &FootTrans2Desc,
	P_AUTO_CONSTRUCT, FootTrans2::PBLOCK_REF,

	// params
	FootTrans2::PB_CATPARENT_DEPRECATED, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	FootTrans2::PB_LIMBDATA, _T("Limb"), TYPE_REFTARG, P_NO_REF, IDS_LIMBDATA,
		p_end,
	FootTrans2::PB_PALMTRANS, _T(""), TYPE_REFTARG, P_NO_REF, 0,
		p_end,
	FootTrans2::PB_OBJPLATFORM, _T(""), TYPE_REFTARG, 0, 0,
		p_end,
	FootTrans2::PB_NODEPLATFORM, _T(""), TYPE_INODE, P_NO_REF, 0,
		p_end,
	FootTrans2::PB_TMSETUP, _T(""), TYPE_MATRIX3, 0, 0,
		p_end,
	////////////////////////////////////////////////////////////////////////////////////////////
	FootTrans2::PB_LAYERPIVOTPOS, _T(""), TYPE_POINT3, P_ANIMATABLE, 0,
		p_end,
	FootTrans2::PB_LAYERPIVOTROT, _T(""), TYPE_POINT3, P_ANIMATABLE, 0,
		p_end,
	FootTrans2::PB_STEPSHAPE, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_end,
	FootTrans2::PB_STEPSLIDE, _T(""), TYPE_FLOAT, P_ANIMATABLE, 0,
		p_accessor, &ZeroToOneParamAccessor,
		p_default, 1.0f,
		p_end,
	FootTrans2::PB_CAT_PRINT_TM, _T(""), TYPE_MATRIX3_TAB, 0, 0, 0,
		p_default, Matrix3(1),
		p_end,
	FootTrans2::PB_CAT_PRINT_INV_TM, _T(""), TYPE_MATRIX3_TAB, 0, 0, 0,
		p_default, Matrix3(1),
		p_end,
	FootTrans2::PB_PRINT_NODES, _T(""), TYPE_INODE_TAB, 0, 0, 0,
		p_end,
	FootTrans2::PB_SCALE, _T(""), TYPE_POINT3, P_ANIMATABLE, 0,
		p_default, Point3(1, 1, 1),
		p_end,
	p_end
);

FootTrans2::FootTrans2(BOOL loading)
{
	pblock = NULL;
	ctrlStepMask = NULL;

	mLayerTrans = NULL;

	FootTrans2Desc.MakeAutoParamBlocks(this);

	if (!loading)
	{
	}
}

void FootTrans2::PostCloneNode()
{
}

RefTargetHandle FootTrans2::Clone(RemapDir &remap)
{
	// make a new FootTrans2 object to be the clone
	// call true for loading so the new FootTrans2 doesn't
	// make new default subcontrollers.
	FootTrans2 *newFootTrans2 = new FootTrans2(TRUE);

	newFootTrans2->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));
	if (mLayerTrans)			newFootTrans2->ReplaceReference(LAYERTRANS, remap.CloneRef(mLayerTrans));

	// For some very weird reason, the clone is handling patching the LimbData pointer
	// I have no idea it doesn't work on the FootTrans, yet seems to work for every
	// other bone in the body, but I don't care because (a) the LimbData is a required clone
	// when cloning the foot, so if we are being cloned it will get cloned
	// soon regardless, and (b) it fixes the crash... (it really shouldnt be necessary though)
	// Symptoms - Feet not moving with body when shift-cloning, and crash on cancel clone.
	LimbData2* newLimbData = static_cast<LimbData2*>(remap.CloneRef(GetLimbData()));
	newFootTrans2->SetLimbData(newLimbData);

	// CATNondeControl can handle cloning the arbitrary bones and things
	CloneCATNodeControl(newFootTrans2, remap);

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newFootTrans2, remap);

	// now return the new object.
	return newFootTrans2;
}

FootTrans2::~FootTrans2()
{
	DeleteAllRefs();
}

void FootTrans2::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		// we're copying from a class of our type, so
		// also copy the references of the objects it uses
		// as well.
		FootTrans2 *newctrl = (FootTrans2*)from;

		if (newctrl->mLayerTrans)	ReplaceReference(LAYERTRANS, newctrl->mLayerTrans);
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

RefTargetHandle FootTrans2::GetReference(int i)
{
	switch (i)
	{
	case PBLOCK_REF:	return pblock;
	case LAYERTRANS:	return mLayerTrans;
	case STEPMASK:		return ctrlStepMask;
	default:			return NULL;
	}
}

void FootTrans2::SetReference(int i, RefTargetHandle rtarg)	// RefTargetHandle is just a ReferenceTarget*
{
	switch (i)
	{
	case PBLOCK_REF:		pblock = (IParamBlock2*)rtarg;			break;
	case LAYERTRANS:		mLayerTrans = (CATClipMatrix3*)rtarg;	break;
	case STEPMASK:			ctrlStepMask = (Control*)rtarg;			break;
	}

}

Animatable* FootTrans2::SubAnim(int i)
{
	switch (i)
	{
	case SUBTRANS:	return mLayerTrans;
	default:		return NULL;
	}
}

TSTR FootTrans2::SubAnimName(int i)
{
	switch (i)
	{
	case SUBTRANS:	return GetString(IDS_LAYERTRANS);
	default:		return _T("");
	}
}

RefResult FootTrans2::NotifyRefChanged(const Interval&, RefTargetHandle hTarg, PartID&, RefMessage msg, BOOL)
{
	//	if(footprint)
	//	{
	//		// in the middle of a footprint calculation, there is no message we wish to pass on
	//		return REF_STOP;
	//	}
	switch (msg)
	{
		/*	case REFMSG_CONTROLREF_CHANGE:
				{
					if(hTarg == ctrlStepGraph && ctrlStepGraph->ClassID() == STEPGRAPH_CLASS_ID)
					{
						Control *newStepEase = ctrlStepGraph->GetParamBlock(0)->GetControllerByID(StepGraph::pb_StepEase);
						stepease_control = newStepEase;
					}
				}*/
	case REFMSG_CHANGE:
	{
		if (pblock == hTarg)
		{
			ParamID lastNotify;
			int paramIndex;
			lastNotify = pblock->LastNotifyParamID(paramIndex);
			switch (lastNotify) {
			case PB_LAYERPIVOTPOS:
			case PB_LAYERPIVOTROT:
				//					if(theHold.RestoreOrRedoing() && ip)
				//						FootTransParamCallBack.TimeChanged(GetCOREInterface()->GetTime());
				break;
			}
		}
	}
	break;
	case REFMSG_TARGET_DELETED:
		// one of our referenced objects is deleting.  hTarg points
		// to the deleting object.
		if (pblock == hTarg)				pblock = NULL;
		else if (mLayerTrans == hTarg)		mLayerTrans = NULL;

		break;
	}
	return REF_SUCCEED;
}

void FootTrans2::CATMessage(TimeValue t, UINT msg, int data) //, void* CATData)
{
	CATNodeControl::CATMessage(t, msg, data);

	/*	switch (msg)
		{
		case CAT_CATUNITS_CHANGED:
			UpdateCATUnits();
		case CAT_UPDATE:
			if(GetNode()) GetNode()->InvalidateTreeTM();
			break;
		case CAT_COLOUR_CHANGED:
			UpdateColour(t);
			break;
		case CAT_VISIBILITY:
			UpdateVisibility();
			break;
		case CAT_NAMECHANGE:
			UpdateName();
			break;
		case CAT_SET_LENGTH_AXIS:
			SetLengthAxis(data);
			break;
		case CAT_REFRESH_OBJECTS_LENGTHAXIS:
			ICATObject *iobj = GetICATObject();
			if(iobj && catparenttrans)iobj->SetLengthAxis(catparenttrans->GetLengthAxis());
			break;
		}

		if(layerTrans)		layerTrans->CATMessage(t, msg, data);
	//	if(layerPivotPos)	layerPivotPos->CATMessage(t, msg, data);
	//	if(layerPivotRot)	layerPivotRot->CATMessage(t, msg, data);
	*/

	// Note: Special case hack.
	// We want to apply a special conroller to the foot to handle
	// pivot positioning. After the layer has been
	if (msg == CLIP_LAYER_INSERT)
	{
		if (mLayerTrans->GetLayerMethod(data) == LAYER_ABSOLUTE)
		{
			Control* pivottrans = (Control*)CreateInstance(CTRL_MATRIX3_CLASS_ID, HD_PIVOTTRANS_CLASS_ID);
			DbgAssert(pivottrans);
			mLayerTrans->SetLayer(data, pivottrans);
		}
	}

	//	CATControl::CATMessage(t, msg, data);
}

// Limb Creation/Maintenance etc.
BOOL FootTrans2::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idPlatform);
	save->Write(idFlags, ccflags);

	if (GetLayerTrans()) GetLayerTrans()->SaveRig(save);

	ICATObject* iobj = GetICATObject();
	if (iobj) iobj->SaveRig(save);

	save->EndGroup();
	return TRUE;
}

BOOL FootTrans2::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;
	float val;

	ICATObject* iobj = (ICATObject*)GetObject()->GetInterface(I_CATOBJECT);
	DbgAssert(iobj);
	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idPlatform) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idObjectParams:	if (iobj) iobj->LoadRig(load);									break;
			case idController:		DbgAssert(GetLayerTrans()); GetLayerTrans()->LoadRig(load);		break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idSetupTM: {
				Matrix3 tmSetup = Matrix3(1);
				load->GetValue(tmSetup);
				mLayerTrans->SetSetupVal((void*)&tmSetup);
			}
			break;
			case idFlags:		load->GetValue(ccflags);											break;
			case idWidth:	load->GetValue(val);	iobj->SetX(val);		break;
			case idLength:	load->GetValue(val);	iobj->SetY(val);		break;
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
		if (load->GetVersion() < CAT3_VERSION_2707)
		{
			HoldSuspend hs;
			SetCCFlag(CNCFLAG_INHERIT_ANIM_ROT);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_POS);
			SetCCFlag(CNCFLAG_INHERIT_ANIM_SCL);
		}
	}
	return ok && load->ok();
}

// Limb Creation/Maintenance etc.
BOOL FootTrans2::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	save->BeginGroup(idIKTargetValues);

	// if we are saving out a pose then we can
	// just save out the world space matrix for this bone
	if (!(flags&CLIPFLAG_CLIP)) {
		Matrix3 tm = GetNode()->GetNodeTM(timerange.Start());
		save->Write(idValMatrix3, tm);
	}
	else {
		if (mLayerTrans) {
			save->BeginGroup(idController);
			mLayerTrans->SaveClip(save, flags, timerange, layerindex);
			save->EndGroup();
		}
	}

	// call our special saveclip function to save out al our arb bones
	SaveClipArbBones(save, flags, timerange, layerindex);
	SaveClipExtraControllers(save, flags, timerange, layerindex);

	save->EndGroup();
	return TRUE;
}

BOOL FootTrans2::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	if (!(flags&CLIPFLAG_LOADFEET))
	{
		load->SkipGroup();
		return TRUE;
	}

	BOOL done = FALSE;
	BOOL ok = TRUE;

	INode* node = GetNode();
	INode *parentNode = node->GetParentNode();
	if (parentNode->IsRootNode())
		flags |= CLIPFLAG_WORLDSPACE;
	else flags &= ~CLIPFLAG_WORLDSPACE;

	// In the old days we didn't have an IKTarget controller,
	// We loaded data straight into the LayerTrans
	if (load->GetVersion() < CAT_VERSION_1210 && load->CurIdentifier() == idIKTargetValues)
	{
		mLayerTrans->LoadClip(load, range, layerindex, dScale, flags);
		return TRUE;
	}

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idPlatform && load->CurGroupID() != idIKTargetValues)
				return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idController: {
				int newflags = flags;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)) newflags |= CLIPFLAG_SKIPPOS;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)) newflags |= CLIPFLAG_SKIPROT;
				if (TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)) newflags |= CLIPFLAG_SKIPSCL;

				mLayerTrans->LoadClip(load, range, layerindex, dScale, newflags);
				break;
			}
			case idArbBones:
				LoadClipArbBones(load, range, layerindex, dScale, flags);
				break;
			case idExtraControllers:
				LoadClipExtraControllers(load, range, layerindex, dScale, flags);
				break;
			default:
				load->AssertOutOfPlace();
				load->SkipGroup();
				break;
			}
			break;

		case rigAssignment:
			switch (load->CurIdentifier())
			{
			case idValMatrix3: {
				flags |= CLIPFLAG_WORLDSPACE;
				Matrix3 val;
				// this method will do all the processing of the pose for us
				if (load->GetValuePose(flags, SuperClassID(), (void*)&val)) {
					node = GetNode();
					if (node) node->SetNodeTM(range.Start(), val);
				}
				break;
			}
			default:
				load->AssertOutOfPlace();
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

// Create a whole structure for CAT
INode* FootTrans2::Initialise(Control* ctrlLMData, BOOL loading)
{
	LimbData2* pLimbData = (LimbData2*)ctrlLMData;
	DbgAssert(pLimbData != NULL);
	if (pLimbData == NULL)
		return NULL;

	SetLimbData(pLimbData);

	CATClipValue* layerIKTargetTM = CreateClipValueController(GetCATClipMatrix3Desc(), pLimbData->GetWeights(), GetCATParentTrans(), FALSE);
	ReplaceReference(LAYERTRANS, layerIKTargetTM);

	Interface* ip = GetCOREInterface();
	IKTargetObject* obj = (IKTargetObject*)CreateInstance(HELPER_CLASS_ID, IKTARGET_OBJECT_CLASS_ID);
	obj->SetCross(FALSE);
	obj->SetCATUnits(GetCATUnits());

	INode* pNode = CreateNode(obj);
	pNode->SetWireColor(asRGB(pLimbData->GetGroupColour()));

	pblock->SetValue(PB_NODEPLATFORM, 0, pNode);

	if (!loading) {
		// Configure a default value
		Matrix3 tmCATParent = GetCATParentTrans()->GettmCATParent(ip->GetTime());
		Matrix3 tmSetup = tmCATParent;
		tmSetup.SetTrans(pLimbData->GetHub()->GetNodeTM(0).GetTrans());
		tmSetup = tmSetup * Inverse(tmCATParent);
		tmSetup.SetTrans(tmSetup.GetTrans() / GetCATUnits());

		if (GetLengthAxis() == Z) {
			tmSetup.SetTrans(tmSetup.GetTrans() * Point3(1.0f, 1.0f, 0.0f));
			tmSetup.PreTranslate(Point3(20.0f * pLimbData->GetLMR(), 0.0f, 0.0f));
		}
		else {
			tmSetup.SetTrans(tmSetup.GetTrans() * Point3(0.0f, 1.0f, 1.0f));
			tmSetup.PreTranslate(Point3(0.0f, 0.0f, -20.0f * pLimbData->GetLMR()));
		}

		layerIKTargetTM->SetSetupVal((void*)&tmSetup);
	}

	return pNode;
}

// user Properties.
// these will be used by the runtime libraries
void	FootTrans2::UpdateUserProps() {

	CATNodeControl::UpdateUserProps();

	// Generate a unique identifier for the limb.
	// we will borrow the node handle from the 1st bone in the limb
	ULONG limbid = GetLimbData()->GetBoneData(0)->GetNode()->GetHandle();
	GetNode()->SetUserPropInt(_T("CATProp_LimbIKTarget"), limbid);
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//

#define FOOTPRINTS_ON_CHUNK			0x00001
#define FOOTTRANS_CATNODECONTROL	0x00100
#define FOOTTRANS_LIMBDATA			20

IOResult FootTrans2::Save(ISave *isave)
{
	isave->BeginChunk(FOOTTRANS_CATNODECONTROL);
	CATNodeControl::Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult FootTrans2::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case FOOTTRANS_CATNODECONTROL:
			res = CATNodeControl::Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	return IO_OK;
}

void FootTrans2::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	flagsbegin = flags;
	ipbegin = ip;

	if (flagsbegin&BEGIN_EDIT_LINKINFO) {
		CATNodeControl::BeginEditParams(ip, flags, prev);
	}
	else if (flagsbegin&BEGIN_EDIT_MOTION) {
		if (mLayerTrans)
			mLayerTrans->BeginEditParams(ip, flags, prev);
		LimbData2* pLimb = GetLimbData();
		if (pLimb != NULL)
			pLimb->BeginEditParams(ip, flags, prev);
	}
	else if (flags == 0) {

	}
}

void FootTrans2::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	if (flagsbegin&BEGIN_EDIT_LINKINFO) {
		CATNodeControl::EndEditParams(ip, flags, next);
	}
	else if (flagsbegin&BEGIN_EDIT_MOTION) {
		LimbData2* pLimb = GetLimbData();
		if (pLimb != NULL)
			pLimb->EndEditParams(ip, END_EDIT_REMOVEUI);
		if (mLayerTrans) mLayerTrans->EndEditParams(ip, flags, next);
	}
	ipbegin = NULL;
}

int FootTrans2::Display(TimeValue t, INode*, ViewExp *vpt, int flags)
{
	Interval iv;
	LimbData2* pLimb = GetLimbData();
	if (pLimb == NULL)
		return 0;

	if (pLimb && pLimb->TestFlag(LIMBFLAG_DISPAY_FK_LIMB_IN_IK) && pLimb->GetIKFKRatio(t, iv) < 1.0f)
	{
		HoldSuspend hs;
		int data = -1;

		pLimb->SetFlag(LIMBFLAG_LOCKED_FK);
		pLimb->CATMessage(CAT_INVALIDATE_TM, data);

		// now force the character to dispaly the current layer
		Box3 bbox;
		pLimb->DisplayLayer(t, vpt, flags, bbox);

		// put things back the way they were
		pLimb->ClearFlag(LIMBFLAG_LOCKED_FK);
		pLimb->CATMessage(CAT_INVALIDATE_TM, data);
	}
	return 1;
}

TSTR FootTrans2::GetRigName()
{
	LimbData2 *pLimb = GetLimbData();
	ICATParentTrans* pCATParentTrans = GetCATParentTrans();
	DbgAssert(pCATParentTrans && pLimb);
	TSTR catName = _T("<<unknown>>");
	TSTR limbName = _T("<<unknown>>");
	if (pCATParentTrans)
		catName = pCATParentTrans->GetCATName();
	if (pLimb)
		limbName = pLimb->GetName();
	return  catName + limbName + GetName();
}

BOOL FootTrans2::PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor)
{
	if (pastectrl->ClassID() != ClassID()) return FALSE;

	FootTrans2* pasteiktarget = (FootTrans2*)pastectrl;

	Matrix3 tmSetup(1);

	if (pasteiktarget->GetLimbData()->GetHub() != GetLimbData()->GetHub()) {
		TimeValue t = GetCOREInterface()->GetTime();
		Matrix3 tmHubOffset = pasteiktarget->GetNodeTM(t) * Inverse(pasteiktarget->GetLimbData()->GetHub()->GetNodeTM(t));
		tmSetup = (tmHubOffset * GetLimbData()->GetHub()->GetNodeTM(t)) * Inverse(GetCATParentTrans()->GettmCATParent(t));

		Point3 v(1.0f, 1.0f, 0.0f);
		ModVec(v, GetLengthAxis());
		tmSetup.SetTrans((tmSetup.GetTrans() * v) / GetCATUnits());

	}
	else {
		// If we are pasting a limb from one s
		if (pasteiktarget->GetLimbData()->GetLMR() != GetLimbData()->GetLMR())
			pasteiktarget->GetLayerTrans()->GetSetupVal((void*)&tmSetup);
		else mLayerTrans->GetSetupVal((void*)&tmSetup);
	}

	if (flags&PASTERIGFLAG_MIRROR) {
		if (GetLengthAxis() == Z)	MirrorMatrix(tmSetup, kXAxis);
		else								MirrorMatrix(tmSetup, kZAxis);
	}

	mLayerTrans->SetSetupVal((void*)&tmSetup);
	mLayerTrans->SetFlags(pasteiktarget->GetLayerTrans()->GetFlags());

	ICATObject* iobj = GetICATObject();
	ICATObject* pasteiobj = pasteiktarget->GetICATObject();
	if (iobj && pasteiobj)	iobj->PasteRig(pasteiobj, flags, scalefactor);

	return TRUE;
}

LimbData2* FootTrans2::GetLimbData()
{
	return static_cast<LimbData2*>(pblock->GetReferenceTarget(PB_LIMBDATA));
}

void FootTrans2::SetLimbData(LimbData2* l)
{
	pblock->SetValue(PB_LIMBDATA, 0, l);
}

CATGroup* FootTrans2::GetGroup()
{
	LimbData2* pLimbData = GetLimbData();
	if (pLimbData != NULL)
		return pLimbData->GetGroup();
	return NULL;
}

CATControl* FootTrans2::GetParentCATControl()
{
	return GetLimbData();
}
