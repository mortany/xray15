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
		Class descriptions for derived value types in the CAT Clip
		Hierarchy.  These are:

			Float
			Rotation
			Position
			Point3
			Scale

	Found an interesting problem with Catification.  When we update
	the progress bar, it spawns a massive system update through
	Node::UpdateTM().  If a CAT object node was flagged as fore-
	ground, GetValue() would be called.  This called GetValue on
	its associated CATClipValue which, if there was a rotation
	controller attached, would call ::SynchAngAxisVal().  This
	iterates through every keyframe in the controller, most of
	which are not initialised yet!  With the default flag of
	TFLAG_TCBQUAT_NOWINDUP set, we eventually call the function
	::MakeAngLessThan180() on an uninitialised key.  Often this
	was passed an enormous angle.  The loop in this function keeps
	subtracting TWOPI from the angle until it's less than TWOPI.
	But TWOPI is smaller than the smallest significand of the
	angle, thus subtraction has no effect and the loop never ends.

	This has led to the creation of the flag CLIP_FLAG_DISABLE_LAYERS
	owned by the root.  If it is set, GetValue() must do nothing.
 **********************************************************************/

#include "CATPlugins.h"

#include <CATAPI/CATClassID.h>
#include "CATClipRoot.h"
#include "CATClipValues.h"
#include "CATClipWeights.h"
#include "CATControl.h"
#include "RootNodeController.h"

#include "HDPivotTrans.h"
#include "HIPivotTrans.h"

 // Max includes
#include "decomp.h"

//keeps track of whether an FP interface desc has been added to the CATClipMatrix3 ClassDesc
static bool catclipFloatInterfacesAdded = false;

class CATClipFloatClassDesc : public ClassDesc2 {
public:
	int IsPublic() { return FALSE; }
	void *Create(BOOL loading = FALSE) {
		CATClipFloat* ctrl = new CATClipFloat(loading);
		if (!catclipFloatInterfacesAdded) {
			// here we add the clip operations to the CATClipMatrix3 ClassDesc
			AddInterface(GetCATClipRootDesc()->GetInterface(LAYERROOT_INTERFACE_FP));
			AddInterface(ctrl->GetDescByID(I_LAYERCONTROL_FP));
			AddInterface(ctrl->GetDescByID(I_LAYERFLOATCONTROL_FP));
			catclipFloatInterfacesAdded = true;
		}
		return ctrl;
	}

	const TCHAR *ClassName() { return GetString(IDS_CL_CATCLIPFLOAT); }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	Class_ID ClassID() { return CATCLIPFLOAT_CLASS_ID; }
	const TCHAR *Category() { return GetString(IDS_CATEGORY); }

	const TCHAR *InternalName() { return _T("CATClipFloat"); }
	HINSTANCE HInstance() { return hInstance; }
};

//keeps track of whether an FP interface desc has been added to the CATClipMatrix3 ClassDesc
static bool catclipMatrix3InterfacesAdded = false;

class CATClipMatrix3ClassDesc : public ClassDesc2 {
public:
	int IsPublic() { return FALSE; }
	void *Create(BOOL loading = FALSE) {
		CATClipMatrix3* ctrl = new CATClipMatrix3(loading);
		if (!catclipMatrix3InterfacesAdded) {
			// here we add the clip operations to the CATClipMatrix3 ClassDesc
			AddInterface(GetCATClipRootDesc()->GetInterface(LAYERROOT_INTERFACE_FP));
			AddInterface(ctrl->GetDescByID(I_LAYERCONTROL_FP));
			AddInterface(ctrl->GetDescByID(I_LAYERMATRIX3CONTROL_FP));
			catclipMatrix3InterfacesAdded = true;
		}
		return ctrl;
	}

	const TCHAR *ClassName() { return GetString(IDS_CL_CATCLIPMATRIX3); }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	Class_ID ClassID() { return CATCLIPMATRIX3_CLASS_ID; }
	const TCHAR *Category() { return GetString(IDS_CATEGORY); }

	const TCHAR *InternalName() { return _T("CATClipMatrix3"); }
	HINSTANCE HInstance() { return hInstance; }
};

static CATClipFloatClassDesc CATClipFloatDesc;
static CATClipMatrix3ClassDesc CATClipMatrix3Desc;

ClassDesc2* GetCATClipFloatDesc() { return &CATClipFloatDesc; }
ClassDesc2* GetCATClipMatrix3Desc() { return &CATClipMatrix3Desc; }

////////////////////////////////////////////////////////////////////////////////
// DUMMY VALUES
////////////////////////////////////////////////////////////////////////////////
//
// We store lots of Clip Values in the CAT Clip Hierarchy.  When we look at
// these values in track view we're seeing the dummies.  Internally we use
// references to the actual value controllers.  The purpose of these dummies
// is to hide keyframe data and stuff that would otherwise show up.
//
#define CATCLIPDUMMYFLOAT_CLASS_ID			Class_ID(0x540b5e81, 0x1aab617d)
#define CATCLIPDUMMYMATRIX3_CLASS_ID		Class_ID(0x5b816fb8, 0x36394f38)

class CATClipDummyValue : public Control
{
public:
	CATClipDummyValue() {}
	void DeleteThis() {}

	void Copy(Control*) {}

	int NumSubs() { return 0; }
	int NumRefs() { return 0; }

	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) { return REF_SUCCEED; }

	void GetValue(TimeValue, void *, Interval &, GetSetMethod) {}
	void SetValue(TimeValue, void*, int, GetSetMethod) {}

	BOOL IsKeyable() { return FALSE; }
	BOOL IsLeaf() { return TRUE; }
	BOOL CanCopyAnim() { return FALSE; }
	BOOL CanAssignController(int /*subAnim*/) { return FALSE; }
	BOOL IsReplaceable() { return FALSE; };
};

class CATClipDummyFloat : public CATClipDummyValue {
public:
	CATClipDummyFloat() {}
	Class_ID ClassID() { return CATCLIPDUMMYFLOAT_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATDummyFloat"); }
};

class CATClipDummyMatrix3 : public CATClipDummyValue {
public:
	CATClipDummyMatrix3() {}
	Class_ID ClassID() { return CATCLIPDUMMYMATRIX3_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("CATDummyMatrix3"); }
};

static CATClipDummyFloat dummyFloat;
static CATClipDummyMatrix3 dummyMatrix3;

Control *GetCATClipDummyFloat() { return &dummyFloat; }
Control *GetCATClipDummyMatrix3() { return &dummyMatrix3; }

/////////////////////////////////////////////////////////////////////////
/************************************************************************/
/* CATClipFloat                                                       */
/************************************************************************/

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc layer_float_FPinterface(
	I_LAYERFLOATCONTROL_FP, _T("ILayerFloatFPInterface"), 0, NULL, FP_MIXIN,

	properties,

	CATClipFloat::propGetSetupVal, CATClipFloat::propSetSetupVal, _T("SetupVal"), 0, TYPE_FLOAT,

	p_end
);

FPInterfaceDesc* CATClipFloat::GetDescByID(Interface_ID id) {
	if (id == I_LAYERFLOATCONTROL_FP) return &layer_float_FPinterface;
	return CATClipValue::GetDescByID(id);
}

class CATClipFloatRestore : public RestoreObj {
public:
	CATClipValue *cont;
	float dUndo, dRedo;

	CATClipFloatRestore(CATClipValue *c) : dRedo(0.0f) {
		cont = c;
		cont->GetSetupVal((void*)&dUndo);
	}

	void Restore(int isUndo) {
		if (isUndo) cont->GetSetupVal((void*)&dRedo);
		cont->SetSetupVal((void*)&dUndo);
		cont->NotifyDependents(FOREVER, 0, REFMSG_CHANGE);
	}

	void Redo() {
		cont->SetSetupVal((void*)&dRedo);
		cont->NotifyDependents(FOREVER, 0, REFMSG_CHANGE);
	}

	int Size() { return 1; }
	void EndHold() { cont->ClearAFlag(A_HELD); }
};

void CATClipFloat::HoldTrack()
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		theHold.Put(new CATClipFloatRestore(this));
		SetAFlag(A_HELD);
	}
}

////////////////////////////////////////////////////////////////////////////////
// CATClipFloat::SetValue()
//

void CATClipFloat::SetValue(TimeValue t, void* val, int commit, GetSetMethod method) {
	SetValue(t, (void*)&val, val, commit, method);
}
void CATClipFloat::SetValue(TimeValue t, const void *valParent, void* val, int commit, GetSetMethod method, LayerRange range, BOOL bWeighted)
{
	int iCATMode = GetCATMode();

	if (iCATMode == SETUPMODE &&
		!TestFlag(CLIP_FLAG_KEYFREEFORM) &&
		!GetRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS)
		)
	{

		// Save state for undo
		HoldTrack();

		if (method == CTRL_ABSOLUTE)
			setupVal = *(float*)val;
		else setupVal += *(float*)val;

		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		return;
	}

	int nSelected = GetSelectedIndex();
	if (nSelected == -1)
		return;

	if (!TestFlag(CLIP_FLAG_KEYFREEFORM) && !GetRoot()->LayerHasContribution(nSelected, t, range, GetWeightsCtrl()))
		return;

	clipvalueValid = NEVER;

	// Get the value of all layers before and all layers after this one.
	float dBefore = 0.0f;
	float dAfter = 0.0f;
	Interval valid = FOREVER;

	// Get values before - this is the parent value
	GetValue(t, valParent, (void*)&dBefore, valid, method, LayerRange(range.nFirst, nSelected - 1), bWeighted);

	// Get values after, (TODO: Fix this - I doubt it actually works...)
	if (!TestFlag(CLIP_FLAG_KEYFREEFORM))
		GetValue(t, valParent, (void*)&dAfter, valid, method, LayerRange(nSelected + 1, range.nLast), bWeighted);

	//  Do not try and weight 1 setvalue when none of the others handle it!
	//	if (method == CTRL_RELATIVE && bWeighted) {
	//		float dClipWeight = GetRoot()->GetLayerWeight(nSelected, t);
	//		*(float*)val *= GetRoot()->GetLayerWeight(nSelected, t);
	//		*(float*)val = -dBefore * (1.0f-dClipWeight) + *(float*)val - dAfter;
	//	} else {

	if (method == CTRL_ABSOLUTE) {
		if (GetSelectedNLAInfo()->ApplyAbsolute()) {
			*(float*)val -= dAfter;
		}
		else {
			*(float*)val -= dBefore + dAfter;
		}
	}

	Control* pTarget = GetLayer(nSelected);
	DbgAssert(pTarget != NULL);
	if (pTarget != NULL)
		pTarget->SetValue(t, val, commit, method);
}

/////////////////////////////////////////////////////////////////////////
/************************************************************************/
/* CATClipMatrix3                                                       */
/************************************************************************/

/**********************************************************************
 * Function publishing interface descriptor...
 */
static FPInterfaceDesc layer_matrix3_FPinterface(
	I_LAYERMATRIX3CONTROL_FP, _T("ILayerMatrix3FPInterface"), 0, NULL, FP_MIXIN,

	properties,

	ILayerMatrix3ControlFP::propGetSetupMatrix, ILayerMatrix3ControlFP::propSetSetupMatrix, _T("SetupVal"), 0, TYPE_MATRIX3_BV,

	p_end
);

FPInterfaceDesc* CATClipMatrix3::GetDescByID(Interface_ID id) {
	if (id == I_LAYERMATRIX3CONTROL_FP) return &layer_matrix3_FPinterface;
	return CATClipValue::GetDescByID(id);
}

/////////////////////////////////////////////////////////////////////////

void CATClipMatrix3::CommitValue(TimeValue t) {
	if (GetCATMode() == SETUPMODE) {
		mTempSetupMatrix = mSetupMatrix;
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}
	Control* pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		pSelectedCtrl->CommitValue(t);
}
void CATClipMatrix3::RestoreValue(TimeValue t) {
	if (GetCATMode() == SETUPMODE) {
		mSetupMatrix = mTempSetupMatrix;
	}
	Control* pSelectedCtrl = GetSelectedLayer();
	if (pSelectedCtrl != NULL)
		pSelectedCtrl->RestoreValue(t);
}

Control *CATClipMatrix3::GetPositionController() {
	Control* pSelectedCtrl = GetSelectedLayer();
	return (pSelectedCtrl == NULL || GetCATMode() == SETUPMODE) ? NULL : pSelectedCtrl->GetPositionController();
}
Control *CATClipMatrix3::GetRotationController() {
	Control* pSelectedCtrl = GetSelectedLayer();
	return (pSelectedCtrl == NULL || GetCATMode() == SETUPMODE) ? NULL : pSelectedCtrl->GetRotationController();
}
Control *CATClipMatrix3::GetScaleController() {
	Control* pSelectedCtrl = GetSelectedLayer();
	return (pSelectedCtrl == NULL || GetCATMode() == SETUPMODE) ? NULL : pSelectedCtrl->GetScaleController();
}
BOOL CATClipMatrix3::SetPositionController(Control *c) {
	Control* pSelectedCtrl = GetSelectedLayer();
	return (pSelectedCtrl == NULL || GetCATMode() == SETUPMODE) ? FALSE : pSelectedCtrl->SetPositionController(c);
}
BOOL CATClipMatrix3::SetRotationController(Control *c) {
	Control* pSelectedCtrl = GetSelectedLayer();
	return (pSelectedCtrl == NULL || GetCATMode() == SETUPMODE) ? FALSE : pSelectedCtrl->SetRotationController(c);
}
BOOL CATClipMatrix3::SetScaleController(Control *c) {
	Control* pSelectedCtrl = GetSelectedLayer();
	return (pSelectedCtrl == NULL || GetCATMode() == SETUPMODE) ? FALSE : pSelectedCtrl->SetScaleController(c);
}

#define CLIPMATRIX3_VALUE_CHUNK			1
#define CLIPMATRIX3_SETUPVAL_CHUNK		2

////////////////////////////////////////////////////////////////////////////////
// Standard Max SetValue()

class CATClipMatrix3Restore : public RestoreObj {
public:
	CATClipMatrix3 *cont;
	Matrix3 tmUndo, tmRedo;

	CATClipMatrix3Restore(CATClipMatrix3 *c)
		: cont(c)
	{
		cont->GetSetupVal(&tmUndo);
	}

	void Restore(int isUndo)
	{
		if (isUndo)
			cont->GetSetupVal(&tmRedo);
		cont->SetSetupVal(&tmUndo);
	}
	void Redo() { cont->SetSetupVal(&tmRedo); }

	int Size() { return sizeof(this); }
	void EndHold() { cont->ClearAFlag(A_HELD); }
};

void CATClipMatrix3::HoldTrack()
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		theHold.Put(new CATClipMatrix3Restore(this));
		SetAFlag(A_HELD);
	}
}
// if the P, R, or S values are locked, then we need to mek sure the
// Layers still match up wioth setuppose. This is basicly for if someone edits
// the psoiton of a finger or collarbone after they have started animation.
// any non-animated layers get
void CATClipMatrix3::EnforcePRSLocks() {
	// TODO:  This function attempts to handle the case
	// when someone modifies a setup pose after creating layers.
	// It will reset the layers to the setup positoin/rotation
	// if locks are enforced.  This is a bad idea, because...
	// 1: They may have already posed the layer, this would destroy that
	// 2: Its inconsistent with what happens to animated layers.

/*	if(!catparenttrans || nLayers==0) return;
	Matrix3 tmSetup(1);
	ApplySetupVal(GetCOREInterface()->GetTime(), tmSetup, Point3(1,1,1), FOREVER);

	Tab<TimeValue> keytimes;

	if(GetCATControl()->TestCCFlag(CNCFLAG_LOCK_LOCAL_POS)){
		for(int i=0;i<nLayers;i++){
			if(tabLayers[i] && GetRoot()->GetLayer(i)->ApplyAbsolute() && tabLayers[i]->GetPositionController()){
				tabLayers[i]->GetPositionController()->GetKeyTimes(keytimes, FOREVER, KEYAT_POSITION);
				if(keytimes.Count() <= 1){
					tabLayers[i]->GetPositionController()->SetValue(0, (void*)&tmSetup.GetTrans(), 1, CTRL_ABSOLUTE);
				}
			}
		}
	};

	if(GetCATControl()->TestCCFlag(CNCFLAG_LOCK_LOCAL_ROT)){
		for(int i=0;i<nLayers;i++){
			if(tabLayers[i] && GetRoot()->GetLayer(i)->ApplyAbsolute() && tabLayers[i]->GetRotationController()){
				tabLayers[i]->GetRotationController()->GetKeyTimes(keytimes, FOREVER, KEYAT_POSITION);
				if(keytimes.Count() <= 1){
					Quat qt(tmSetup);
					tabLayers[i]->GetRotationController()->SetValue(0, (void*)&qt, 1, CTRL_ABSOLUTE);
				}
			}
		}
	};
	if(GetCATControl()->TestCCFlag(CNCFLAG_LOCK_LOCAL_SCL)){

	};
	*/
}

Matrix3 GetSetupMatrixToSet(const Matrix3& tmParent, const SetXFormPacket& val, BOOL adjustOffset)
{
	Matrix3 tmSetup = val.tmAxis * Inverse(val.tmParent);
	if (adjustOffset)
	{
		// Keep the position from val.tmParent, but reset the orientation.
		Matrix3 tmSetupRelToParent = tmParent;
		tmSetupRelToParent.SetTrans(val.tmParent.GetTrans());
		tmSetupRelToParent = val.tmAxis * Inverse(tmSetupRelToParent);
		//Point3 p3Rel
		tmSetup.SetTrans(tmSetupRelToParent.GetTrans());
	}
	return tmSetup;
}

////////////////////////////////////////////////////////////////////////////////
// CATClipMatrix3::SetValue()
void CATClipMatrix3::SetValue(TimeValue t, void* val, int commit, GetSetMethod method) {
	Point3 p3ParentScale(P3_IDENTITY_SCALE);
	SetXFormPacket* ptr = (SetXFormPacket*)val;
	SetValue(t, ptr->tmParent, p3ParentScale, *ptr, commit, method);
}
void CATClipMatrix3::SetValue(TimeValue t, const Matrix3& tmParent, const Point3& p3ParentScale, SetXFormPacket& val, int commit, GetSetMethod method, LayerRange range, BOOL bWeighted)
{
	int iCATMode = GetCATMode();

	if (iCATMode == SETUPMODE &&
		!TestFlag(CLIP_FLAG_KEYFREEFORM) &&
		!GetRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS))
	{

		// the logic behind this is that a whole character is selected
		// and the user is moving the character while in setupmode. Because the
		// hub is not a child of the CATParent it is recieving setvalues. Because
		// the root hub actually inherits off the CATParent while in Setupmode
		// so this setvalue should not occur.
		ICATParentTrans* pParentTrans = GetCATParentTrans();
		if (pParentTrans == NULL || pParentTrans->GetNode() == NULL || pParentTrans->GetNode()->Selected())
			return;

		if (val.command == XFORM_SCALE) return;

		///////////////////////////////////////////////////////////
		// If we use a controller to manage the setup pose
		if (ctrlSetup && TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER))
		{
			if (iCATMode == SETUPMODE && TestFlag(CLIP_FLAG_HAS_TRANSFORM))
				val.tmParent = pParentTrans->GettmCATParent(t);

			if ((val.command == XFORM_SET) && TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT))
			{
				Matrix3 tmSetup = GetSetupMatrixToSet(tmParent, val, TRUE);
				val.tmAxis = tmSetup * val.tmParent;
			}

			ctrlSetup->SetValue(t, &val, commit, method);
			return;
		}
		///////////////////////////////////////////////////////////

		// Save state for undo
		HoldTrack();

		mTempSetupMatrix = mSetupMatrix;
		Matrix3 newSetupMatrix = mSetupMatrix;

		float dCATUnits = GetCATUnits();
		Matrix3 tmCATParent = pParentTrans->GettmCATParent(t);
		Point3 catparentpos = tmCATParent.GetTrans();

		switch (val.command)
		{
		case XFORM_MOVE:
		{
			if (TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT))
			{
				// Our relative XFORM_MOVE needs to happen relative to the
				// the original parent.  Pretty complicated...
				val.tmParent = tmParent;
			}
			if (method == CTRL_RELATIVE)
			{
				Point3 pos = VectorTransform(val.tmAxis*Inverse(val.tmParent), val.p);
				pos /= dCATUnits;
				newSetupMatrix.Translate(pos);
			}
			else
			{
				val.p = (val.p * Inverse(val.tmParent)) / dCATUnits;;
				newSetupMatrix.SetTrans(val.p);
			}
			break;
		}
		case XFORM_ROTATE:
		{

			val.aa.axis = Normalize(VectorTransform(val.tmAxis*Inverse(val.tmParent), val.aa.axis));

			Point3 pos = newSetupMatrix.GetTrans();
			RotateMatrix(newSetupMatrix, val.aa);
			newSetupMatrix.SetTrans(pos);
			break;
		}
		case XFORM_SET:
		{
			// Set our offset
			newSetupMatrix = GetSetupMatrixToSet(tmParent, val, TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT));
			newSetupMatrix.SetTrans(newSetupMatrix.GetTrans() / dCATUnits);
			break;
		}
		}

		// Set the new transform
		SetSetupVal(&newSetupMatrix);

		// if the P, R, or S values are locked, then we need to mek sure the
		// Layers still match up wioth setuppose. This is basicly for if someone edits
		// the psoiton of a finger or collarbone after they have started animation.
		EnforcePRSLocks();

		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		return;
	}

	int nSelected = GetSelectedIndex();
	Control* pSelectedCtrl = GetLayer(nSelected);
	if (pSelectedCtrl == NULL)
		return;

	if (!TestFlag(CLIP_FLAG_KEYFREEFORM) && !GetRoot()->LayerHasContribution(nSelected, t, range, GetWeightsCtrl())) return;

	clipvalueValid = NEVER;
	NLAInfo *layerinfo = GetSelectedNLAInfo();
	ClipLayerMethod layermethod = layerinfo->GetMethod();

	// Get the value of all layers before and all layers after this one.
	Interval valid = FOREVER;
	Matrix3 tmBefore(1), tmAfter(1), tmCurrent(1);

	// apply the time warp curve to our time value.
	TimeValue warped_t = layerinfo->GetTime(t);

	Matrix3 tmValParent = val.tmParent;

	////////////////////////////////////////////////////////////////////
	// If our parent has been scaled at all, we need to remove it
	// from our positional offset without distorting our rotation.
	if ((p3ParentScale != P3_IDENTITY_SCALE) && TestFlag(CLIP_FLAG_INHERIT_SCL))
	{
		Matrix3 preLayer;
		if (layerinfo->ApplyAbsolute())
		{
			Interval iv;
			preLayer = CalculateAbsLayerParent(layerinfo, t, iv, val.tmParent);
		}
		else
		{
			preLayer = val.tmParent;
		}

		Point3 localPos = (val.tmAxis * Inverse(preLayer)).GetTrans();
		localPos = localPos * Inverse(ScaleMatrix(p3ParentScale)) * preLayer;
		val.tmAxis.SetTrans(localPos);
	}

	switch (layermethod) {
	case LAYER_RELATIVE:
		// if the setting CLIP_FLAG_RELATIVE_TO_SETUPPOSE is on, then this GetValue shoud get that as well.
		GetValue(t, (void*)&tmValParent, (void*)&val.tmParent, valid, CTRL_RELATIVE, LayerRange(range.nFirst, nSelected - 1), bWeighted);
		break;
	case LAYER_RELATIVE_WORLD:
		switch (val.command)
		{
		case XFORM_MOVE:
		case XFORM_ROTATE:
			val.tmParent = Matrix3(1);
			break;
		case XFORM_SET:
			if (method == CTRL_ABSOLUTE)
			{
				GetValue(t, (void*)&val.tmParent, (void*)&val.tmParent, valid, CTRL_RELATIVE, LayerRange(range.nFirst, nSelected - 1), bWeighted);
				Point3 pos = val.tmAxis.GetTrans() - val.tmParent.GetTrans();
				val.tmAxis = Inverse(val.tmParent) * val.tmAxis;
				val.tmAxis.SetTrans(pos);
				val.tmParent = Matrix3(1);
			}
		}
		break;
	case LAYER_ABSOLUTE:
	case LAYER_CATMOTION:
		if (!TestFlag(CLIP_FLAG_HAS_TRANSFORM))
		{
			DWORD flags = pSelectedCtrl->GetInheritanceFlags();
			if (flags&INHERIT_ROT_MASK && pSelectedCtrl->InheritsParentTransform())
			{
				Matrix3 tmLayerTransform = layerinfo->GetTransform(t, valid);
				Point3 pos = val.tmParent.GetTrans();
				val.tmParent = val.tmParent * tmLayerTransform;
				val.tmParent.SetTrans(pos);
			}
		}

		// If we do NOT inherit position, but we DO inherit rotation, ignore rotation on position SetValues
		DbgAssert(layerinfo->ApplyAbsolute());
		if (!TestFlag(CLIP_FLAG_INHERIT_POS) && TestFlag(CLIP_FLAG_INHERIT_ROT))
		{
			if (val.command == XFORM_MOVE)
			{
				// If we are doing a move, just reset the parent to
				// the layer transform, as that is what position is
				// evaluated against
				val.tmParent = layerinfo->GetTransform(warped_t, valid);
			}
			else if (val.command == XFORM_SET)
			{
				// If we 'root' node, then our parent is relative to LTG
				Matrix3 tmLTGParent = layerinfo->GetTransform(warped_t, valid);
				if (CLIP_FLAG_HAS_TRANSFORM)
				{
					val.tmParent = val.tmParent * tmLTGParent;
				}

				// Now, we need to keep this rotation as calculated.
				// However, the effect of the parents rotation needs
				// to be negated for position part of TM.  Recalculate
				// tmAxis.Trans to evaluate to the correct position,
				// but relative to tmParent

				// This is the final pos we want
				Point3 p3CorrectPos = (val.tmAxis * Inverse(tmLTGParent)).GetTrans();
				// This is the final pos relative to tmParent
				Point3 tmCorrectRelToParent = val.tmParent.PointTransform(p3CorrectPos);
				// This -should- be decomposed to the correct position in whatever finally recieves the SetValue
				val.tmAxis.SetTrans(tmCorrectRelToParent);
			}
			else // For rotation/scale, we can ignore the special case
			{
				// Find the same parent we would use in the GetValue
				Interval iv;
				val.tmParent = CalculateAbsLayerParent(layerinfo, t, iv, val.tmParent);
			}
		}
		else if (TestFlag(CLIP_FLAG_INHERIT_POS) && !TestFlag(CLIP_FLAG_INHERIT_ROT))
		{
			// We inherit position, but not rotation.
			if (val.command == XFORM_MOVE)
			{
				val.tmParent = tmParent;
			}
			else
			{
				// Find the same parent we would use in the GetValue
				Interval iv;
				val.tmParent = CalculateAbsLayerParent(layerinfo, t, iv, val.tmParent);

				if (val.command == XFORM_SET)
				{
					// If we are set, we need to evaluate pos vs tmParent
					Matrix3 tmPosOnlyParent = tmParent;
					// This is why this code is problematic.  We have to keep the rotation from the
					// original parent, to calculate our position correctly.  However, we can't
					// use the position from the original parent, because our CATController may
					// have modified it.
					tmPosOnlyParent.SetTrans(tmValParent.GetTrans());
					// This is the final pos we want set on the position controller
					Point3 p3CorrectPos = (val.tmAxis * Inverse(tmPosOnlyParent)).GetTrans();
					// So make the final position relative to tmParent
					Point3 tmCorrectRelToParent = val.tmParent.PointTransform(p3CorrectPos);
					// This -should- be decomposed to the correct position in whatever finally recieves the SetValue
					val.tmAxis.SetTrans(tmCorrectRelToParent);
				}
			}
		}
		else
		{
			// Find the same parent we would use in the GetValue
			Interval iv;
			val.tmParent = CalculateAbsLayerParent(layerinfo, t, iv, val.tmParent);
		}

		break;
	}

	// Now pass the corrected value to the selected layer.
	pSelectedCtrl->SetValue(warped_t, &val, commit, method);
}

void CATClipMatrix3::GetScale(TimeValue t, Point3 &val, Interval& valid)
{
	// TODO: Would be nice to optimize this.
	Matrix3 tmValue(1);
	GetValue(t, &tmValue, valid, CTRL_ABSOLUTE);
	val.x = tmValue.GetRow(X).Length();
	val.y = tmValue.GetRow(Y).Length();
	val.z = tmValue.GetRow(Z).Length();
}

void CATClipMatrix3::SetScale(TimeValue t, Point3 scale, Point3 parentscale, GetSetMethod method)
{
	int iCATMode = GetCATMode();
	if (iCATMode == SETUPMODE &&
		!TestFlag(CLIP_FLAG_KEYFREEFORM) &&
		!GetRoot()->TestFlag(CLIP_FLAG_COLLAPSINGLAYERS)) {
		return;
	}

	int nSelected = GetSelectedIndex();
	Control* pSelectedCtrl = GetLayer(nSelected);
	if (pSelectedCtrl == NULL)
		return;

	if (!TestFlag(CLIP_FLAG_KEYFREEFORM) && !GetRoot()->LayerHasContribution(nSelected, t, LAYER_RANGE_ALL, GetWeightsCtrl()))
		return;

	NLAInfo *layerinfo = GetSelectedNLAInfo();
	// Get the value of all layers before and all layers after this one.
	Interval valid;

	if (!layerinfo->ApplyAbsolute()) {
		// We don't care about parent transformations here, as parent scale is always removed anyway
		Matrix3 tm(1);
		Point3 p3Scale;
		GetTransformation(t, tm, tm, valid, parentscale, p3Scale, LayerRange(0, nSelected - 1));
	}

	// subtract the effect off of our parent scale
	ApplyInheritance(t, parentscale, nSelected);
	scale = scale / parentscale;

	if (pSelectedCtrl->GetScaleController()) {
		ScaleValue sv(scale);
		pSelectedCtrl->GetScaleController()->SetValue(t, (void*)&sv, 1, method);
	}
	else {
		SetXFormPacket xform(scale, 1);
		pSelectedCtrl->SetValue(t, (void*)&xform, 1, method);
	}
}

BOOL CATClipMatrix3::PasteRig(CATClipValue* pasteLayers, DWORD flags, float scalefactor)
{
	if (ClassID() != pasteLayers->ClassID()) return FALSE;
	CATClipMatrix3* pasteCATClipMatrix3 = (CATClipMatrix3*)pasteLayers;

	SetFlags(pasteCATClipMatrix3->GetFlags());

	Matrix3 tmSetup;
	pasteCATClipMatrix3->GetSetupVal((void*)&tmSetup);
	if (flags&PASTERIGFLAG_MIRROR)
	{
		ICATParentTrans* pParentTrans = GetCATParentTrans();
		if (pParentTrans != NULL && pParentTrans->GetLengthAxis() == Z)
			MirrorMatrix(tmSetup, kXAxis);
		else
			MirrorMatrix(tmSetup, kZAxis);
	}
	tmSetup.SetTrans(tmSetup.GetTrans() * scalefactor);
	SetSetupVal((void*)&tmSetup);
	return TRUE;
}

// Note: This method is realted to the Gimbal gizmo in Max. Look at the method
// CATClipMatrix3::GetRotationContoller for a bigger note on what the problem is.
bool CATClipMatrix3::GetLocalTMComponents(TimeValue t, TMComponentsArg& cmpts, Matrix3Indirect& parentMatrix, const Matrix3& /*tmOrigParent*/)
{
	return Control::GetLocalTMComponents(t, cmpts, parentMatrix);

	//if(!(cmpts.position||cmpts.rotation||cmpts.scale)) return true;
	//// We can't use the parent that has been passed to us,
	//// because in many cases it isn't the right matrix.
	//// like in the case of segments > 0 on segmented bones.
	//Matrix3 tmParent = parentMatrix();
	//Interval valid = FOREVER;

	//if(GetRoot()->GetCATMode() == SETUPMODE || TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE)){
	//	if(GetRoot()->GetCATMode() != SETUPMODE && TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE)){
	//		Point3 localscale(1, 1, 1);
	//		ApplySetupVal(t, tmOrigParent, tmParent, localscale, valid);
	//	}else{
	//		if(ctrlSetup && TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)){
	//			return ctrlSetup->GetLocalTMComponents(t, cmpts, parentMatrix);
	//		}

	//		Matrix3Indirect newParentMatrix(tmParent);
	//		return Control::GetLocalTMComponents(t, cmpts, newParentMatrix);
	//	}
	//}

	//int nSelected = GetSelectedIndex();
	//if (nSelected == -1)
	//	return false;

	//Control* pSelectedCtrl = GetLayer(nSelected);
	//if (pSelectedCtrl == NULL)
	//	return false;
	//
	//NLAInfo *layerinfo = GetSelectedNLAInfo();
	//// Get the value of all layers before and all layers after this one.
	//float dClipWeight = layerinfo->GetWeight(t, valid);
	//// apply the time warp curve to our time value.
	//TimeValue warped_t = layerinfo->GetTime(t);
	//
	//Matrix3 tmValParent = tmParent;

	//switch(layerinfo->GetMethod()){
	//case LAYER_RELATIVE:
	//	GetValue(t, (void*)&tmValParent, (void*)&tmParent, valid, CTRL_RELATIVE, LayerRange(0,nSelected - 1));
	//	break;
	//case LAYER_RELATIVE_WORLD:
	//	tmParent = Matrix3(1);
	//	break;
	//case LAYER_ABSOLUTE:
	//	if (TestFlag(CLIP_FLAG_HAS_TRANSFORM)){
	//		Matrix3 tmLayerTransform = layerinfo->GetTransform(warped_t, valid);
	//		tmParent = tmParent * tmLayerTransform;
	//	}else{
	//		DWORD flags = GetLayer(nSelected)->GetInheritanceFlags();
	//		if (flags&INHERIT_ROT_MASK && pSelectedCtrl->InheritsParentTransform()) {
	//			Matrix3 tmLayerTransform = layerinfo->GetTransform(warped_t, valid);
	//			Point3 pos = tmParent.GetTrans();
	//			tmParent = tmParent * tmLayerTransform;
	//			tmParent.SetTrans(pos);
	//		}
	//	}
	//	break;
	//case LAYER_MOCAP:
	//	break;
	//}

	//Matrix3Indirect newParentMatrix(tmParent);
	//return pSelectedCtrl->GetLocalTMComponents(t, cmpts, newParentMatrix);
}

void CATClipMatrix3::ApplyInheritance(TimeValue /*t*/, Point3 &psc, int layerid) //, Control *ctrl)
{
	if (layerid == -1) layerid = GetSelectedIndex();
	DWORD flags = GetLayer(layerid)->GetInheritanceFlags();
	if (flags&INHERIT_ANY_MASK) {
		//	if (flags&INHERIT_SCL_MASK) {
		//		for (int i=0; i<3; i++) {
		//			if (flags&(1<<(i+6))) {
		//				psc[i] = 1.0f;
		//			}
		//		}
		//	}
		if (flags&INHERIT_SCL_X) psc[0] = 1.0f;
		if (flags&INHERIT_SCL_Y) psc[1] = 1.0f;
		if (flags&INHERIT_SCL_Z) psc[2] = 1.0f;
	}
}

//void CATClipMatrix3::ApplyInheritance(TimeValue t, Point3 &psc, int layerid) //, Control *ctrl)
//Matrix3 CATClipMatrix3::ApplyInheritance(TimeValue t,const Matrix3 &ptm,Control *pos, Point3 cpos,BOOL usecpos)

// Note: this function is copied from ctrlimp.cpp in the Max Debug source
void CATClipMatrix3::ApplyInheritance(TimeValue t, Matrix3 &ptm, int layerid, DWORD flags)
{
	if (layerid == -1) layerid = GetSelectedIndex();
	Control *pos = GetLayer(layerid)->GetPositionController();
	//	DWORD flags = GetLayer(layerid)->GetInheritanceFlags();
	if (flags&INHERIT_ANY_MASK) {
		Matrix3 tm = ptm;

		if (flags&INHERIT_POS_MASK) {
			Point3 trans = tm.GetTrans();
			for (int i = 0; i < 3; i++) {
				if (flags&(1 << i)) {
					trans[i] = 0.0f;
				}
			}
			tm.SetTrans(trans);
		}

		AffineParts parts;
		if (flags&INHERIT_ROT_MASK || flags&INHERIT_SCL_MASK) {
			decomp_affine(tm, &parts);
		}

		if (flags&INHERIT_ROT_MASK) {
			float ang[3];
			QuatToEuler(parts.q, ang);
			for (int i = 0; i < 3; i++) {
				if (flags&(1 << (i + 3))) {
					ang[i] = 0.0f;
				}
			}
			EulerToQuat(ang, parts.q);
		}
		if (flags&INHERIT_SCL_MASK) {
			for (int i = 0; i < 3; i++) {
				if (flags&(1 << (i + 6))) {
					parts.k[i] = 1.0f;
				}
			}
		}
		if (flags&INHERIT_ROT_MASK || flags&INHERIT_SCL_MASK) {
			// Phils note.
			// what we are doing here is related to inheriting position off the parent
			// but not the rotation. the problem is, is that the :ApplyInheritcane method
			// gets called before the position controller gets called.
			// This means that what we are doing here is calculating a tmParent that,
			// once the pos gets applied(in world space), the result is that it looksd like it gets
			// applied in local space.
			// I think this is a rather stupid solution myself and I would have simply
			// had 3 different ApplyInheritance methods, one ofr pos rot, and scl.

			// Find where the position would have been...
			Point3 pdelta = Point3(0.0f, 0.0f, 0.0f);
			Interval valid;
			/*	if (usecpos) pdelta = cpos;
				else*/ pos->GetValue(t, &pdelta, valid, CTRL_ABSOLUTE);
			tm.PreTranslate(pdelta);
			Point3 dest = tm.GetTrans();

			// Put in whatever scale and rotation are left.
			tm.IdentityMatrix();
			PreRotateMatrix(tm, parts.q);
			//ScaleValue s(parts.k,parts.u);
			ScaleValue s(parts.k);
			ApplyScaling(tm, s);

			// If we're not inheriting position then we don't want the
			// translational effects of rotation or scale to apply.
			if (flags&INHERIT_POS_MASK) {
				for (int i = 0; i < 3; i++) {
					if (flags&(1 << i)) {
						dest[i] = 0.0f;
						pdelta[i] = 0.0f;
					}
				}
			}

			// Set the position to the position we would have been at if
			// we inherited averything and then subtract the delta so
			// when the position controller adds it in it will cancel.
			tm.SetTrans(dest);
			tm.PreTranslate(-pdelta);
		}
		ptm = tm;
	}
}

Control* CATClipMatrix3::NewWhateverIAmController() {
	Control* prs = CreatePRSControl();

	// In some weird cases, new PRS controller default to garbage values.
	// In the case from Blue fang Games, the beak on their penguin kept disappearing.
	SetXFormPacket ptr(Matrix3(1));
	prs->SetValue(0, (void*)&ptr);

	// We would like the default scale controller for CAT to be the ScaleXYZ controller
	// this is because this scale controller doesn't store a skew value like the normal default one
	Control *scale = (Control*)CreateInstance(CTRL_SCALE_CLASS_ID, ISCALE_CONTROL_CLASS_ID);
	ScaleValue sv(P3_IDENTITY_SCALE);
	scale->SetValue(0, (void*)&sv);
	prs->SetScaleController(scale);

	return prs;
}

//////////////////////////////////////////////////////////////////////////
//

IOResult CATClipMatrix3::SaveSetupval(ISave *isave)
{
	DWORD nb = 0;
	IOResult res = isave->Write(&mSetupMatrix, sizeof Matrix3, &nb);
	DbgAssert(res == IO_OK && nb == sizeof(mSetupMatrix));
	return res;
}

IOResult CATClipMatrix3::LoadSetupval(ILoad *iload)
{
	DWORD nb;
	IOResult res = iload->Read(&mSetupMatrix, sizeof(Matrix3), &nb);
	DbgAssert(res == IO_OK && nb == sizeof(mSetupMatrix));

	// Note: Orthogonalize will also remove scaling
	mSetupMatrix.Orthogonalize();
	return res;
}

BOOL CATClipMatrix3::SaveRig(CATRigWriter *save)
{
	save->BeginGroup(idController);

	if (ctrlSetup && TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
		save->BeginGroup(idSetupCtrl);
		Interval range = ctrlSetup->GetTimeRange(TIMERANGE_ALL);
		save->WriteController(ctrlSetup, 0, range);
		save->EndGroup();
	}
	else {
		save->Write(idSetupTM, mSetupMatrix);
	}
	save->Write(idFlags, flags);

	save->EndGroup();
	return TRUE;
}

BOOL CATClipMatrix3::LoadRig(CATRigReader *load)
{
	BOOL done = FALSE;
	BOOL ok = TRUE;

	while (load->ok() && !done && ok)
	{
		if (!load->NextClause())
		{
			// An error occurred in the clause, and we've
			// ended up with the next valid one.  If we're
			// not still in the correct group, return.
			if (load->CurGroupID() != idController) return FALSE;
		}

		// Now check the clause ID and act accordingly.
		switch (load->CurClauseID())
		{
		case rigBeginGroup:
			switch (load->CurIdentifier())
			{
			case idSetupCtrl:
				ok = load->ReadController(ctrlSetup, GetCOREInterface()->GetAnimRange(), 1.0f, 0);
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
			case idFlags:		load->GetValue(flags);		break;
			case idSetupTM:
			{
				Matrix3 tmSetup(1);
				load->GetValue(tmSetup);
				SetSetupVal(&tmSetup);
				break;
			}
			default:			load->AssertOutOfPlace();	break;
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

	// We prolly changed, so notify, yeah?
	NotifyDependents(FOREVER, REFMSG_CHANGE, PART_TM);

	ClearFlag(CLIP_FLAG_DISABLE_LAYERS);

	return ok && load->ok();
}

/************************************************************************/
/* Here we get the clip saver going                                     */
/************************************************************************/
void CATClipMatrix3::SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex)
{
	// For poses we use the result of the combined layer stack.
	if (!(flags&CLIPFLAG_CLIP))
		save->WritePose(timerange.Start(), this);
	else
	{
		Control* pLayerCtrl = GetLayer(layerindex);
		if (pLayerCtrl != NULL)
		{
			if (pLayerCtrl->ClassID() == CATMOTIONPLATFORM_CLASS_ID)
				flags |= CLIPFLAG_SKIP_NODE_TABLES;

			save->WriteController(pLayerCtrl, flags, timerange);
			// Clear the flag
			flags &= ~CLIPFLAG_SKIP_NODE_TABLES;
		}
	}
}

BOOL CATClipMatrix3::LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags)
{
	Interval iv;
	if ((flags&CLIPFLAG_ONLYWEIGHTED) && (GetWeight(range.Start(), layerindex, iv) == 0.0f))
	{
		load->SkipGroup();
		// even though we are skipping this group, it is not due to an error
		return TRUE;
	}

	if (TestFlag(CLIP_FLAG_HAS_TRANSFORM))	flags |= CLIPFLAG_WORLDSPACE;
	else									flags &= ~CLIPFLAG_WORLDSPACE;

	BOOL done = FALSE;
	BOOL ok = TRUE;
	load->SetLayerController(this);

	// Load a clip into the currently selected
	// layer, at the time set.
	Control *pLayerCtrl = GetLayer(layerindex);
	if (pLayerCtrl != NULL)
	{
		while (load->ok() && !done && ok) {
			if (!load->NextClause())
				return FALSE;

			// Now check the clause ID and act accordingly.
			switch (load->CurClauseID()) {
			case rigAssignment:
				// Here we find out what our base controller is
				switch (load->CurIdentifier()) {
				case idValClassIDs:
				{
					TokClassIDs classid;
					load->GetValue(classid);

					// we don't need to check anything for poses, just use the CATClipValues SetValue
					// since 1.3 Poses wouldn't be sotered for a particulatr layer. Instead the first
					// Tag in the CATClipValue would have been idValMatrix3, or idValFloat.
					// This line is here for Legacy reasons only
					if (!(flags&CLIPFLAG_CLIP)) {
						load->ReadController(this, range, dScale, flags);
						done = TRUE;
						break;
					}

					Class_ID newClassID(classid.usClassIDA, classid.usClassIDB);
					Control *pLoadingCtrl = pLayerCtrl;

					// are we loading an old file, onto a PRS, that used to be a rotation.
					if (load->GetVersion() < CAT_VERSION_1700 &&
						classid.usSuperClassID == CTRL_ROTATION_CLASS_ID)
					{// instead load the clip into the rotation controller
						pLoadingCtrl = pLoadingCtrl->GetRotationController();
					}

					if (pLoadingCtrl->ClassID() != newClassID)
					{
						Control *pNewLayer = (Control*)CreateInstance(classid.usSuperClassID, newClassID);
						assert(pNewLayer);
						pNewLayer->Copy(pLoadingCtrl);
						if (pLayerCtrl == pLoadingCtrl)	//
							ReplaceReference(layerindex, pNewLayer);
						else
							pLayerCtrl->SetRotationController(pNewLayer);
						pLayerCtrl = pNewLayer;
					}

					if (!((SuperClassID() == CTRL_MATRIX3_CLASS_ID) || (SuperClassID() == CTRL_POSITION_CLASS_ID)))
						flags = flags&~CLIPFLAG_SCALE_DATA;
					// If loading a clip, dont modify it. Set it directly into the layer.
					ok = load->ReadController(pLayerCtrl, range, dScale, flags);
					done = TRUE;
					break;
				}
				case idFlags: {
					DWORD inheritflags = 0;
					load->GetValue(inheritflags);
					// for some retarded reason, the flags are inverted when SETTING the inheritance flags
					// when 'Getting' a clear bit means it DOES inherit,
					// while 'Setting' a clear bit means it DOESN'T.... logical aye...
					inheritflags = ~inheritflags;
					pLayerCtrl->SetInheritanceFlags(inheritflags, FALSE);
					break;
				}
								  // maybe we are a pose, in which case we really don't need a classID
				case idValPoint: {
					// Loading Scale
					Point3 scale(1, 1, 1);
					if (load->GetValue(scale)) {
						SetScale(range.Start(), scale, Point3(1, 1, 1), CTRL_ABSOLUTE);
					}
					break;
				}
				case idValQuat:
				case idValMatrix3:
					// GetValuePose now just processes the pose data and returns it.
					// this new method is for putting it into the controller.
					// For Clips we always want the value plugged straight into the correct layer
					// ignoring things like the weighting system
					if (flags&CLIPFLAG_CLIP)
						load->ReadPoseIntoController(pLayerCtrl, range.Start(), dScale, flags);
					else load->ReadPoseIntoController(this, range.Start(), dScale, flags);
					break;
				}
				break;
			case rigEndGroup:
				done = TRUE;
				break;
			case rigAbort:
			case rigEnd:
				return FALSE;
			}
		}
	}
	load->SetLayerController(NULL);
	return ok;
}

void CATClipMatrix3::CATMessage(TimeValue t, UINT msg, int data)
{
	CATClipValue::CATMessage(t, msg, data);
	switch (msg) {
	case CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER:
	{
		Control* pLayerCtrl = GetLayer(data);
		if (pLayerCtrl)
		{
			Matrix3 val;
			if (TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE))
				val.IdentityMatrix();
			else {
				GetSetupVal((void*)&val);
				val.SetTrans(val.GetTrans()*GetCATUnits());
			}
			SetXFormPacket xform(val);
			pLayerCtrl->SetValue(t, (void*)&xform, 1, CTRL_ABSOLUTE);
		}
	}
	break;
	case CLIP_LAYER_SET_INHERITANCE_FLAGS:
	{
		Control* pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			pSelectedCtrl->SetInheritanceFlags(data, FALSE);
	}
	break;
	case CLIP_LAYER_SETPOS_CLASS:
	{
		Control* pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			pSelectedCtrl->SetPositionController((Control*)CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(data, 0)));
	}
	break;
	case CLIP_LAYER_SETROT_CLASS:
	{
		Control* pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			pSelectedCtrl->SetRotationController((Control*)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(data, 0)));
	}
	break;
	case CLIP_LAYER_SETSCL_CLASS:
	{
		Control* pSelectedCtrl = GetSelectedLayer();
		if (pSelectedCtrl != NULL)
			pSelectedCtrl->SetScaleController((Control*)CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(data, 0)));
	}
	break;
	}
}

void CATClipFloat::CATMessage(TimeValue t, UINT msg, int data)
{
	CATClipValue::CATMessage(t, msg, data);
	switch (msg) {
	case CLIP_LAYER_DUMP_SETUP_POSE_TO_LAYER:
	{
		Control* pLayerCtrl = GetLayer(data);
		if (pLayerCtrl)
		{
			float val;
			GetSetupVal((void*)&val);
			pLayerCtrl->SetValue(t, (void*)&val, 1, CTRL_ABSOLUTE);
		}
	}
	break;
	}
}
////////////////////////////////////////////////////////////////////////

void CATClipMatrix3::CreateSetupController(BOOL tf, Matrix3 tmParent)
{
	BOOL newundo = FALSE;
	if (!theHold.Holding()) { theHold.Begin(); newundo = TRUE; }

	int nLayers = GetNumLayers();
	if (tf) {
		if (!ctrlSetup) {

			// Make a new PRS controllerand put it into the Setup slot
			ReplaceReference(nLayers, (Control*)CreateInstance(CTRL_MATRIX3_CLASS_ID, Class_ID(PRS_CONTROL_CLASS_ID, 0)));
			assert(ctrlSetup);
			Matrix3 tmSetup = mSetupMatrix;
			// the setup controller stores the actual pos offset including CATUnits
			tmSetup.SetTrans(tmSetup.GetTrans()*GetCATUnits());
			SetXFormPacket ptr(tmSetup);
			ctrlSetup->SetValue(0, (void*)&ptr, 1, CTRL_ABSOLUTE);

			//TODO :
		/*	if(!TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_CATPARENT)){
				ctrlSetup->SetPositionController( (Control*)CreateInstance( CTRL_POSITION_CLASS_ID, Class_ID(POSITION_CONSTRAINT_CLASS_ID, 0)));
			}
			if(!TestFlag(CLIP_FLAG_SETUP_INHERITS_ROT_FROM_CATPARENT)){
				ctrlSetup->SetRotationController( (Control*)CreateInstance( CTRL_ROTATION_CLASS_ID, Class_ID(ORIENTATION_CONSTRAINT_CLASS_ID, 0)));

				if(TestFlag(CLIP_FLAG_SETUP_FLIPROT_FROM_CATPARENT)){
					ctrlSetup->SetRotationController( (Control*)CreateInstance( CTRL_ROTATION_CLASS_ID, Class_ID(ROTLIST_CONTROL_CLASS_ID, 0)));
					Control *eulerRot =  (Control*)CreateInstance( CTRL_ROTATION_CLASS_ID, Class_ID(EULER_CONTROL_CLASS_ID, 0));
				}
			}
		*/
			SetFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER);
			if (ipClip && GetCATMode() == SETUPMODE)
				BeginEditLayers(GetSelectedIndex(), ipClip, flagsBegin, GetCATMode());
		}
	}
	else {
		if (ctrlSetup)
		{
			if (ipClip && GetCATMode() == SETUPMODE)
				EndEditLayers(GetSelectedIndex(), ipClip, flagsBegin, GetCATMode());
			assert(ctrlSetup);
			Matrix3 tmSetup = tmParent;
			Interval iv;
			ctrlSetup->GetValue(GetCOREInterface()->GetTime(), (void*)&tmSetup, iv, CTRL_RELATIVE);
			tmSetup = tmSetup *Inverse(tmParent);
			SetSetupVal(&tmSetup);
			DeleteReference(nLayers);

			ClearFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER);
		}
	}

	if (theHold.Holding() && newundo) {
		theHold.Accept(GetString(IDS_HLD_LYRSETUPCNTRL));
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}
	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED, NOTIFY_ALL, FALSE);
}

//void CATClipMatrix3::ApplySetupVal(void* val) {
void CATClipMatrix3::ApplySetupVal(TimeValue t, const Matrix3& tmParent, Matrix3& tmWorld, Point3 &localscale, Interval &valid)
{
	Matrix3 tmSetup(1);
	Point3 p3SetupPos(0, 0, 0);
	if (ctrlSetup)
	{
		Matrix3 tmApplied = tmWorld;
		ctrlSetup->GetValue(t, &tmApplied, valid, CTRL_RELATIVE);
		tmSetup = tmApplied * Inverse(tmWorld);

		// Don't apply CATUnits to the setupcontroller position
		// If the user is using a procedural controller, like pos constraint, the this simply breaks everything
		p3SetupPos = tmSetup.GetTrans();

		if (GetCATMode() != SETUPMODE && TestFlag(CLIP_FLAG_RELATIVE_TO_SETUPPOSE))
		{
			/////////////////////////////////////////////////////////////////////
			// Here we are getting the scale separately and removing it from the matrix
			// this allows us to reapply the scales in local space in the CATNodeControl,
			// And we can pass the scales separately to our children
			Control* scale_control = ctrlSetup->GetScaleController();
			if (scale_control) {
				ScaleValue temp;
				scale_control->GetValue(t, (void*)&temp, valid, CTRL_ABSOLUTE);
				// This value is this layers contribution to the scale of this bone.
				if (temp.s != P3_IDENTITY_SCALE) {
					tmWorld = Inverse(ScaleMatrix(temp.s)) * tmWorld;
				}
				// add the scale to the total scale
				localscale = temp.s * localscale;
			}
			else {
				// we must break the matrix into its componenets
				AffineParts parts;
				decomp_affine(tmWorld, &parts);
				localscale = parts.k;

				// Now build a new non-scaled matrix
				tmWorld.IdentityMatrix();
				tmWorld.SetRotate(parts.q);
				tmWorld.SetTrans(parts.t);
			}
		}
	}
	else
	{
		tmSetup = mSetupMatrix;
		p3SetupPos = tmSetup.GetTrans() * GetCATUnits();
	}

	// Our parent matrix has our setup position baked in?
	if (TestFlag(CLIP_FLAG_SETUP_INHERITS_POS_FROM_PARENT))
	{
		// Apply our setup position from our parent, and set it to world.
		tmWorld.SetTrans(tmWorld.GetTrans() + tmParent.VectorTransform(p3SetupPos));
		// Now remove the translation from tmSetup.
		tmSetup.NoTrans();
	}
	else
		tmSetup.SetTrans(p3SetupPos);

	tmWorld = tmSetup * tmWorld;

	// We don't set the validity intervals here.
	// We may be using SetupValue in conjunction with animation,
	// so setting the validity will overwrite the inherited one.
}

void CATClipMatrix3::SetSetupVal(void* val)
{
	DbgAssert(val != NULL);
	if (val == NULL)
		return;

	// Ensure that there is no scale in our setup mtx.
	// Note: Orthogonalize is actually ortho-normalize
	Matrix3* setupVal = (Matrix3*)val;
	setupVal->Orthogonalize();

	if (ctrlSetup && TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
		SetXFormPacket ptr(*setupVal);
		ctrlSetup->SetValue(GetCOREInterface()->GetTime(), (void*)&ptr, 1, CTRL_ABSOLUTE);
	}
	else {
		HoldTrack();
		mSetupMatrix = *setupVal;
		EnforcePRSLocks();
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}
};

void CATClipMatrix3::GetSetupVal(void* val)
{
	if (ctrlSetup && TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) {
		// this method should always return the local offset of the setup controller.
		Interval iv;
		Point3 p3Scale(1, 1, 1);
		Matrix3* tmSetup = (Matrix3*)val;
		tmSetup->IdentityMatrix();
		ApplySetupVal(GetCOREInterface()->GetTime(), Matrix3(1), *tmSetup, p3Scale, iv);
	}
	else {
		*(Matrix3*)val = mSetupMatrix;
	}
};
Matrix3	CATClipMatrix3::GetSetupMatrix() {
	Matrix3 tm(1); GetSetupVal((void*)&tm);
	if (!ctrlSetup) tm.SetTrans(tm.GetTrans() * GetCATUnits());
	return tm;
};
void	CATClipMatrix3::SetSetupMatrix(Matrix3 tm) {
	if (!ctrlSetup) tm.SetTrans(tm.GetTrans() / GetCATUnits());
	SetSetupVal((void*)&tm);
};

////////////////////////////////////////////////////////////////////////////////
// CATClipMatrix3::GetValue()
//

Matrix3 CATClipMatrix3::CalculateAbsLayerParent(NLAInfo * info, TimeValue warped_t, Interval& valid, const Matrix3 &tmValue)
{
	DbgAssert(info->ApplyAbsolute());
	Matrix3 tmLayerTrans(1);

	if (info->CanTransform())
	{
		if (TestFlag(CLIP_FLAG_HAS_TRANSFORM)) // In this case, we always inherit from layer trans (tmValue should be Identity?)
		{
			Matrix3 tmLayerTransform = info->GetTransform(warped_t, valid);
			tmLayerTrans = tmValue * tmLayerTransform;
		}
		// Else, if there is anything we do NOT inherit...
		else if (!(TestFlag(CLIP_FLAG_INHERIT_POS) && TestFlag(CLIP_FLAG_INHERIT_ROT) && TestFlag(CLIP_FLAG_INHERIT_SCL)))
		{
			// We inherit part of the LTG's transform, but not all.
			// In this case, we need to remove the parts of the
			// transform that we still inherit from our parent
			Matrix3 tmLTGParent = info->GetTransform(warped_t, valid);

			// Apply Transform
			tmLayerTrans = tmValue * tmLTGParent;

			// If we inherit pos from our parent, we don't inherit from LTG
			// NOTE - This is not the complete solution, see below pLayer->GetValue
			if (TestFlag(CLIP_FLAG_INHERIT_POS))
				tmLayerTrans.SetTrans(tmValue.GetTrans());

			// If we inherit rotation, remove the LTG's part.
			if (TestFlag(CLIP_FLAG_INHERIT_ROT)) // If we inherit rot from our parent, we don't inherit from LTG
			{
				// Transfer rotation (ignore scale, its already been removed)
				tmLayerTrans.SetRow(X, tmValue.GetRow(X));
				tmLayerTrans.SetRow(Y, tmValue.GetRow(Y));
				tmLayerTrans.SetRow(Z, tmValue.GetRow(Z));
			}
		}
		else // Inherit from our parent like normal
			tmLayerTrans = tmValue;
	}
	else
		tmLayerTrans = tmValue;
	return tmLayerTrans;
}

void CATClipMatrix3::GetValue(TimeValue t, void* val, Interval& valid, GetSetMethod method) {
	GetValue(t, val, val, valid, method);
}
void CATClipMatrix3::GetValue(TimeValue t, const void* valParent, void *val, Interval& valid, GetSetMethod /*method*/, LayerRange range, BOOL bWeighted)
{
	// tmParent is not really being used anymore. It was originally intended
	// to include the CATMoiont information, but we now put that into the
	// layer system.
	Point3 p3Scale(1, 1, 1);
	Matrix3* tmValue = (Matrix3*)val;
	Matrix3* tmParent = (Matrix3*)valParent;

	// Extract parent scale.  I dont think this is all that necessary (not sure
	// when precisely this fn is called) but no harm in implementing it correctly.
	Point3* p3ParentRows = reinterpret_cast<Point3*>(tmParent->GetAddr());
	Point3 p3ParentScale;
	p3ParentScale.x = p3ParentRows[0].LengthUnify();
	p3ParentScale.y = p3ParentRows[1].LengthUnify();
	p3ParentScale.z = p3ParentRows[2].LengthUnify();
	GetTransformation(t, *tmParent, *tmValue, valid, p3ParentScale, p3Scale, range, bWeighted);

	tmValue->Scale(p3Scale, TRUE);
};

void CATClipMatrix3::GetTransformation(TimeValue t, const Matrix3& tmOrigParent, Matrix3 &tmValue, Interval& valid, const Point3& p3ParentScale, Point3 &p3LocalScale, LayerRange range/*=LAYER_RANGE_ALL*/, BOOL bWeighted/*=TRUE*/)
{
	if (!GetRoot() || CATEvaluationLock::IsEvaluationLocked()) return;
	if (TestFlag(CLIP_FLAG_DISABLE_LAYERS)) return;

	int nLayers = GetNumLayers();
	int nSelected = GetSelectedIndex();
	range.Limit(0, nLayers - 1);

	// We now create a new local value(tm) to hold the transformation matrix while we calculate it.
	// If 'Remove Displacement' were turned on, then the rig would jump.
	// This is because 'val' is a reference of the INode cache. When we call GetNodeTM in the
	// NLAInfo::GetTransform method, it would cause the cache to be changed, and when the
	// stack unfolded, the val in this method was changed, once the call to GetTransform returned.
	// Discovered Siggraph06 by phil.
	Matrix3 tmAppliedValue = tmValue;
	Matrix3 tmLayerTrans(1), tmLayerTransform(1);
	Point3 p3LayerScale;

	float dClipWeight = 0.0f;
	BOOL bKeyFreeform = TestFlag(CLIP_FLAG_KEYFREEFORM);

	// The bWeighted variable also overrides layer solo and ignore.  In future
	// it will be replaced with some flags to determine which types of layer
	// are weighted, and whether ignore and solo are in effect or not.  This
	// will give the user more control over next generation (catV2) ghosts.
	int nSoloLayer;
	int nFirstActiveLayer;
	BOOL bApplyWeight = bWeighted;

	if (bWeighted) {
		nSoloLayer = GetRoot()->GetSoloLayer();
		nFirstActiveLayer = GetFirstActiveAbsLayer(t, range);
		if (nSoloLayer >= 0) nFirstActiveLayer = nSoloLayer;
	}
	else {
		nSoloLayer = -1;
		nFirstActiveLayer = range.nFirst;
	}

	TimeValue warped_t;

	// This is the guts...
	for (int i = max(range.nFirst, nFirstActiveLayer); i <= range.nLast; i++) {
		NLAInfo *info = GetRoot()->GetLayer(i);
		Control* pLayerCtrl = GetLayer(i);
		if (!info || !pLayerCtrl)
			continue;
		if (bWeighted) {
			bApplyWeight = TRUE;
			if (nSoloLayer == i) bApplyWeight = FALSE;
			else if (bKeyFreeform && nSelected == i) continue;
			else if (nSoloLayer >= 0 && nSoloLayer != info->GetParentLayer()) continue;
			else if (!info->LayerEnabled()) continue;
			else {
				if (weights)
					dClipWeight = weights->GetWeight(t, i, valid);
				else {
					dClipWeight = info->GetWeight(t, valid);
				}
			}
		}

		// apply the time warp curve to our time value.
		warped_t = info->GetTime(t);

		if ((dClipWeight > 0.0f) || !bApplyWeight)
		{
			BOOL applyabsolute = info->ApplyAbsolute();
			ClipLayerMethod layermethod = info->GetMethod();

			if (applyabsolute)
			{
				// ApplyAbsolute Layers inherit transforms and scale off the parent bone..
				tmLayerTrans = CalculateAbsLayerParent(info, warped_t, valid, tmValue);

				// ApplyAbsolute Layers inherit scale off the parent bone..
				p3LayerScale = p3ParentScale;

			}
			else {
				// ApplyRelative Layers inherit transform and scale off the previous layer...
				tmLayerTrans = tmAppliedValue;
				p3LayerScale = p3LocalScale;
			}

			// Get the actual value for this layer
			Matrix3 prelayer = tmLayerTrans;
			if (layermethod == LAYER_RELATIVE_WORLD)
			{
				// Layers that are relative to the world do not use the parent matrix
				Matrix3 tmRelWorld = Matrix3(1);
				pLayerCtrl->GetValue(warped_t, (void*)&tmRelWorld, valid, CTRL_RELATIVE);

				// now post-transform the transform (apply relative to world)
				Point3 p3Trans = tmLayerTrans.GetTrans() + tmRelWorld.GetTrans();
				tmRelWorld.NoTrans();
				tmLayerTrans = tmLayerTrans * tmRelWorld;
				tmLayerTrans.SetTrans(p3Trans);
			}
			else
			{
				pLayerCtrl->GetValue(warped_t, (void*)&tmLayerTrans, valid, CTRL_RELATIVE);

				// Inheritance games...  If we inherit rotation (but not position)
				// from our parent, then we want to apply our position directly
				// in our LayerTransform space.  Currently, it also includes the rotation
				// of our parent, which means when our parent rotates, we move (it seems like
				// we inherit rotation of the parent.
				// This is not strictly speaking mathematically correct, but seems more natural
				if (info->CanTransform())
				{
					if (!TestFlag(CLIP_FLAG_INHERIT_POS) && TestFlag(CLIP_FLAG_INHERIT_ROT))
					{
						Matrix3 tmLTGParent = info->GetTransform(warped_t, valid);
						pLayerCtrl->GetValue(warped_t, (void*)&tmLTGParent, valid, CTRL_RELATIVE);
						tmLayerTrans.SetTrans(tmLTGParent.GetTrans());
					}
					else if (TestFlag(CLIP_FLAG_INHERIT_POS) && !TestFlag(CLIP_FLAG_INHERIT_ROT))
					{
						Matrix3 tmCorrectPos = tmOrigParent;
						pLayerCtrl->GetValue(warped_t, (void*)&tmCorrectPos, valid, CTRL_RELATIVE);
						tmLayerTrans.SetTrans(tmCorrectPos.GetTrans());
					}
				}
			}

			// Now apply the layers offset transformation
			if (info->CanTransform())
			{
				tmLayerTransform = info->GetTransform(warped_t, valid);
				DWORD flags = pLayerCtrl->GetInheritanceFlags();
				if (flags&INHERIT_ROT_MASK && pLayerCtrl->InheritsParentTransform()) {
					// If all the rotation flags have been cleared, then
					// The transform nodes effect will be removed unless
					// we add it one after the layer has evaluated.
					Point3 pos = tmLayerTrans.GetTrans();
					tmLayerTrans = tmLayerTrans * tmLayerTransform;
					tmLayerTrans.SetTrans(pos);
				}
			}

			////////////////////////////////////////////////////////////////////
			// at this point p3LayerScale still holds our parents scale.
			// if our parent has been scaled at all, we need to apply this to our
			// positional offset without distorting our rotations, or causing a non-uniform matrix
			if (p3LayerScale != P3_IDENTITY_SCALE) {
				// find our local positional offset
				Point3 localpos = (tmLayerTrans * Inverse(prelayer)).GetTrans();
				localpos = localpos * (ScaleMatrix(p3ParentScale) * prelayer); // TODO: this line could be more efficient
				tmLayerTrans.SetTrans(localpos);
			}

			////////////////////////////////////////////////////////////////////
			// Now see if we are inheriting the scale from our parent node/previous layer.
			ApplyInheritance(warped_t, p3LayerScale, i);

			/////////////////////////////////////////////////////////////////////
			// Here we are getting the scale separately and removing it from the matrix
			// this allows us to reapply the scales in local space in the CATNodeControl,
			// And we can pass the scales separately to our children
			Control* scale_control = pLayerCtrl->GetScaleController();
			if (scale_control) {
				ScaleValue temp;
				scale_control->GetValue(warped_t, (void*)&temp, valid, CTRL_ABSOLUTE);
				// The value is this layers contribution to the scale of this bone.
				if (temp.s != P3_IDENTITY_SCALE) {
					// subtract the scale right back off the transform
					if (layermethod == LAYER_RELATIVE_WORLD) {
						// Preserve the translation.
						Point3 pos = tmLayerTrans.GetTrans();
						tmLayerTrans = tmLayerTrans * Inverse(ScaleMatrix(temp.s));
						tmLayerTrans.SetTrans(pos);

						// Put this local scale value into wold space
						temp.s -= P3_IDENTITY_SCALE;
						temp.s = Inverse(tmLayerTrans).VectorTransform(temp.s);
						temp.s.x = fabs(temp.s.x);
						temp.s.y = fabs(temp.s.y);
						temp.s.z = fabs(temp.s.z);
						temp.s += P3_IDENTITY_SCALE;
					}
					else {
						tmLayerTrans = Inverse(ScaleMatrix(temp.s)) * tmLayerTrans;
					}
					// add the scale to the total scale
					p3LayerScale = temp.s * p3LayerScale;
				}
			}
			else {
				// we must break the matrix into its componenets
				AffineParts parts;
				decomp_affine(tmLayerTrans, &parts);
				p3LayerScale = parts.k;

				// Now build a new non-scaled matrix
				tmLayerTrans.IdentityMatrix();
				tmLayerTrans.SetRotate(parts.q);
				tmLayerTrans.SetTrans(parts.t);
			}
			/////////////////////////////////////////////////////////////////////
			// TODO: I am sure the following section is a bit broken. Trys scaling a
			// layer transform that isn't aligned tothe world axes
			if (info->CanTransform() && TestFlag(CLIP_FLAG_HAS_TRANSFORM))
			{
				Point3 oldPos = tmLayerTransform.GetTrans();
				Point3 p3Scale = info->GetScale(warped_t, valid);
				if (p3Scale != P3_IDENTITY_SCALE) tmLayerTrans.SetTrans(oldPos + ((tmLayerTrans.GetTrans() - oldPos) * p3Scale));
			}

			if (bApplyWeight) {
				// blend the Transform
				BlendMat(tmAppliedValue, tmLayerTrans, dClipWeight);
				// build up our local scale value
				p3LocalScale = p3LocalScale + ((p3LayerScale - p3LocalScale) * dClipWeight);
			}
			else {
				tmAppliedValue = tmLayerTrans;
				p3LocalScale = p3LayerScale;
			}
			if (warped_t != t) { valid.SetInstant(t); }
		}

		// apply the time warp curve to our time value.
//		valid.SetStart(GetRoot()->GetTime(valid.Start(), i));
//		valid.SetEnd(GetRoot()->GetTime(valid.End(), i));

	}

	////////////////////////////////////////
	// for some reason, the LinkConstraint returns validity intervals on NEVER.
	// this causes ans infinite loop if the character has a skin attached
	// (See Gregor Weib - Link Constraint Problem)
	// This forces the controller to return an interval that at least includes
	// the current time.
	if (!valid.InInterval(t))
		valid.SetInstant(t);

	//if(absvalue) tm = tm * Inverse(inparent);

	// save the value and interval so we can use them
	tmValue = tmAppliedValue;
	validVal = tmValue;
	validScaleVal.SetRow(X, p3LocalScale);
	clipvalueValid = valid;
}

////////////////////////////////////////////////////////////////////////////////
// CATClipFloat::GetValue()
//

void CATClipFloat::GetValue(TimeValue t, void* val, Interval& valid, GetSetMethod method) {
	GetValue(t, (void*)&val, val, valid, method);
}

void CATClipFloat::GetValue(TimeValue t, const void *, void* val, Interval& valid, GetSetMethod method, LayerRange range, BOOL bWeighted)
{
	CATClipRoot* pRoot = GetRoot();
	DbgAssert(pRoot != NULL);
	if (pRoot == NULL)
	{
		*(float*)val = 0.0f;
		return;
	}

	if (GetCATMode() == SETUPMODE) {
		// Return the setup value
		*(float*)val = setupVal;
		valid.SetInfinite();
		return;
	}

	//	use the validity intervals to save us time
/*	if(clipvalueValid.InInterval(t)&&(methodValid == method))
	{
		// Return the cached value
		*(float*)val = validVal;
		return;
	}
*/
//	valid.SetInstant(t);

	float dClipFloat = 0.0f;

	if (method == CTRL_ABSOLUTE)
		*(float*)val = 0.0f;

	range.Limit(0, GetNumLayers() - 1);

	float dClipWeight = 0.0f;

	// The bWeighted variable also overrides layer solo and ignore.  In future
	// it will be replaced with some flags to determine which types of layer
	// are weighted, and whether ignore and solo are in effect or not.  This
	// will give the user more control over next generation (catV2) ghosts.
	int nSoloLayer = -1;
	int nFirstActiveLayer;
	BOOL bApplyWeight = bWeighted;
	int nSelected = GetSelectedIndex();
	BOOL bKeyFreeform = TestFlag(CLIP_FLAG_KEYFREEFORM);

	if (bWeighted) {
		if (pRoot != NULL)
			nSoloLayer = pRoot->GetSoloLayer();
		nFirstActiveLayer = GetFirstActiveAbsLayer(t, range);
		if (nSoloLayer >= 0) nFirstActiveLayer = nSoloLayer;
	}
	else {
		nSoloLayer = -1;
		nFirstActiveLayer = range.nFirst;
	}
	TimeValue warped_t;

	// This is the guts...
	for (int i = max(range.nFirst, nFirstActiveLayer); i <= range.nLast; i++) {
		NLAInfo *layerinfo = GetRoot()->GetLayer(i);
		Control *pLayerCtrl = GetLayer(i);
		if (!layerinfo || !pLayerCtrl) continue;

		// apply the time warp curve to our time value.
		warped_t = layerinfo->GetTime(t);

		if (bWeighted) {
			bApplyWeight = TRUE;
			if (nSoloLayer == i) bApplyWeight = FALSE;
			else if (bKeyFreeform && nSelected == i) continue;
			else if (nSoloLayer >= 0 && nSoloLayer != layerinfo->GetParentLayer()) continue;
			else if (!layerinfo->LayerEnabled()) continue;
			else {
				if (GetWeightsCtrl())
					dClipWeight = GetWeightsCtrl()->GetWeight(warped_t, i, valid);
				else {
					dClipWeight = layerinfo->GetWeight(warped_t, valid);
				}
			}
		}

		if ((dClipWeight > 0.0f) || !bApplyWeight)
		{
			pLayerCtrl->GetValue(warped_t, (void*)&dClipFloat, valid, CTRL_ABSOLUTE);

			switch (layerinfo->GetMethod()) {
			case LAYER_RELATIVE_WORLD:
			case LAYER_RELATIVE:
				if (bApplyWeight) *(float*)val += (dClipFloat * dClipWeight);
				else *(float*)val += dClipFloat;
				break;

			case LAYER_CATMOTION:
			case LAYER_ABSOLUTE:
				if (bApplyWeight) *(float*)val = *(float*)val + ((dClipFloat - *(float*)val) * dClipWeight);
				else *(float*)val = *(float*)val + dClipFloat;
				break;

			case LAYER_IGNORE:
				continue;
			}
		}

		// We may need to overwrite the validity set earlier, if our time has been warped.
		// TODO: Fix this to calculate the actual validity interval
		if (warped_t != t) valid.SetInstant(t);
	}

	// save the value and interval so we can use them
	validVal = *(float*)val;
	clipvalueValid = valid;
}
