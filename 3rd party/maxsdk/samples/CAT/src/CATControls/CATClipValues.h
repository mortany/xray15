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

#pragma once

#include "CATClipValue.h"
#include "resource.h"

extern Control *GetCATClipDummyFloat();
extern Control *GetCATClipDummyMatrix3();

// stolen from ctrlimp.cpp
#define INHERIT_POS_MASK	7
#define INHERIT_ROT_MASK	(7<<3)
#define INHERIT_SCL_MASK	(7<<6)
#define INHERIT_ANY_MASK	(INHERIT_POS_MASK|INHERIT_ROT_MASK|INHERIT_SCL_MASK)

//////////////////////////////////////////////////////////////////////
// CATClipFloat
//
class CATClipFloat : public CATClipValue, public ILayerFloatControlFP
{
public:
	// Function publishing stuff
//		virtual FPInterfaceDesc* GetDescByID(Interface_ID id);
//		virtual	FPInterfaceDesc* GetDesc();

protected:
	float validVal;
	float setupVal;

	virtual Control* ControlOrDummy(Control* ctrl) { UNREFERENCED_PARAMETER(ctrl); return GetCATClipDummyFloat(); }
	virtual Control* NewWhateverIAmController() { return NewDefaultFloatController(); }
	virtual ClassDesc2* GetClassDesc() { return GetCATClipFloatDesc(); };

public:
	// Construction
	CATClipFloat(BOOL loading = FALSE) : CATClipValue(loading) { ResetSetupVal(); };

	// access to the setup values
	void SetSetupVal(void* val) { HoldTrack(); setupVal = *(float*)val; NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); };
	void GetSetupVal(void* val) { *(float*)val = setupVal; };
	void ResetSetupVal() { setupVal = 0.0f; };
	// TODO:
	virtual void CreateSetupController(BOOL tf) { UNREFERENCED_PARAMETER(tf); };

	void HoldTrack();

	virtual IOResult SaveSetupval(ISave *isave) { DWORD nb; return isave->Write(&setupVal, sizeof(float), &nb); };
	virtual IOResult LoadSetupval(ILoad *iload) { DWORD nb; return iload->Read(&setupVal, sizeof(float), &nb); };

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method = CTRL_ABSOLUTE);
	void SetValue(TimeValue t, void *val, int commit = 1, GetSetMethod method = CTRL_ABSOLUTE);

	void GetValue(TimeValue t, const void *valParent, void* val, Interval& valid, GetSetMethod method = CTRL_ABSOLUTE, LayerRange range = LAYER_RANGE_ALL, BOOL bWeighted = TRUE);
	void SetValue(TimeValue t, const void *valParent, void* val, int commit = 1, GetSetMethod method = CTRL_ABSOLUTE, LayerRange range = LAYER_RANGE_ALL, BOOL bWeighted = TRUE);

	virtual void CATMessage(TimeValue t, UINT msg, int data);

	// Class stuff
	Class_ID ClassID() { return CATCLIPFLOAT_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATCLIPFLOAT); }
	void DeleteThis() { delete this; }

	////////////////////////////////////////////////////
	// Published Functions
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_LAYERFLOATCONTROL_FP) return (ILayerFloatControlFP*)this;
		return CATClipValue::GetInterface(id);
	}

	// ILayerFloatControlFP
	virtual float		GetSetupVal() { float v; GetSetupVal((void*)&v); return v; };
	virtual void		SetSetupVal(float v) { SetSetupVal((void*)&v); };
};

//////////////////////////////////////////////////////////////////////
// CATClipMatrix3
//
class CATClipMatrix3 : public CATClipValue, public ILayerMatrix3ControlFP
{
private:
	// A table of nodes the same size as the table of layers
	// Each layer can have a different parent
	Tab<INode*> tabParentNodes;		// our layer parent nodes
	Tab<USHORT*> tabFlags;			// our layer flags

	//Matrix3 tmParent, inparent;

protected:
	Matrix3 validVal;
	Matrix3 validScaleVal;
	Matrix3 mSetupMatrix;
	Matrix3 mTempSetupMatrix;
	TimeValue setvaluetime;

	virtual Control* ControlOrDummy(Control* ctrl) { UNREFERENCED_PARAMETER(ctrl); return GetCATClipDummyMatrix3(); }
	virtual Control* NewWhateverIAmController();// { return CreatePRSControl(); }
	virtual ClassDesc2* GetClassDesc() { return GetCATClipMatrix3Desc(); };

public:
	// Construction
	CATClipMatrix3(BOOL loading = FALSE) : CATClipValue(loading) { ResetSetupVal(); }

	BOOL LayerHasKeys(int layer) {
		if (layer == -1) return FALSE;
		return (GetLayer(layer)->GetPositionController()->NumKeys() > 0 ||
			GetLayer(layer)->GetRotationController()->NumKeys() > 0);
	}

	// access to the setup values
	void SetSetupVal(void* val);//{ HoldTrack(); setupVal = *(Matrix3*)val; EnforcePRSLocks(); NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE); };
	void GetSetupVal(void* val);//{ *(Matrix3*)val = setupVal; };
	void ResetSetupVal() { mSetupMatrix = Matrix3(1); };
	//	void ApplySetupVal(void* val);
	void ApplySetupVal(TimeValue t, const Matrix3& tmParent, Matrix3 &val, Point3 &localscale, Interval &valid);
	void CreateSetupController(BOOL tf, Matrix3 tmParent);
	void CreateSetupController(BOOL tf) { CreateSetupController(tf, Matrix3(1)); }
	BOOL IsUsingSetupController() { return ((ctrlSetup != NULL) && TestFlag(CLIP_FLAG_SETUPPOSE_CONTROLLER)) ? TRUE : FALSE; }

	void EnforcePRSLocks();

	virtual BOOL PasteRig(CATClipValue* pasteLayers, DWORD flags, float scalefactor);

	void HoldTrack();

	virtual IOResult SaveSetupval(ISave *isave);
	virtual IOResult LoadSetupval(ILoad *iload);

	BOOL SaveRig(CATRigWriter *save);
	BOOL LoadRig(CATRigReader *load);

	virtual void SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);
	virtual BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);

	virtual void CATMessage(TimeValue t, UINT msg, int data);

	Matrix3 CalculateAbsLayerParent(NLAInfo * info, TimeValue warped_t, Interval& valid, const Matrix3 &tmValue);
	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method = CTRL_ABSOLUTE);
	void SetValue(TimeValue t, void *val, int commit = 1, GetSetMethod method = CTRL_ABSOLUTE);

	void GetValue(TimeValue t, const void *valParent, void *val, Interval& valid, GetSetMethod method = CTRL_ABSOLUTE, LayerRange range = LAYER_RANGE_ALL, BOOL bWeighted = TRUE);
	void SetValue(TimeValue t, const Matrix3 &valParent, const Point3& p3ParentScale, SetXFormPacket &val, int commit = 1, GetSetMethod method = CTRL_ABSOLUTE, LayerRange range = LAYER_RANGE_ALL, BOOL bWeighted = TRUE);

	//	void GetValue(TimeValue t, void *val, void *localscale, Interval& valid, GetSetMethod method=CTRL_ABSOLUTE, LayerRange range=LAYER_RANGE_ALL, BOOL bWeighted=TRUE);
	void GetTransformation(TimeValue t, const Matrix3& tmOrigParent, Matrix3 &tmValue, Interval& valid, const Point3& p3ParentScale, Point3 &p3LocalScale, LayerRange range = LAYER_RANGE_ALL, BOOL bWeighted = TRUE);
	// TODO: do this function
	void GetScale(TimeValue t, Point3 &val, Interval& valid);
	void SetScale(TimeValue t, Point3 val, Point3 parentscale, GetSetMethod method);

	DWORD	GetInheritanceFlags() { Control* pCtrl = GetSelectedLayer(); return (pCtrl != NULL) ? pCtrl->GetInheritanceFlags() : INHERIT_ALL; };
	BOOL	SetInheritanceFlags(DWORD f, BOOL keepPos) { Control* pCtrl = GetSelectedLayer(); return (pCtrl != NULL) ? pCtrl->SetInheritanceFlags(f, keepPos) : FALSE; }
	void	ApplyInheritance(TimeValue t, Point3 &psc, int layerid = -1);
	void	ApplyInheritance(TimeValue t, Matrix3 &ptm, int layerid, DWORD flags);

	bool GetLocalTMComponents(TimeValue t, TMComponentsArg& cmpts, Matrix3Indirect& parentMatrix, const Matrix3& tmOrigParent);

	// Class stuff
	Class_ID ClassID() { return CATCLIPMATRIX3_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATCLIPMATRIX3); }
	void DeleteThis() { delete this; }

	void CommitValue(TimeValue t);
	void RestoreValue(TimeValue t);

	// GB 07-Nov-2003: Make key filters on the Time Slider work correctly
	Control *GetPositionController(); //{ return (nSelected==-1 || daddy->Root()->GetCATParent()->GetCATMode()==SETUPMODE) ? NULL : GetLayer(nSelected)->GetPositionController(); }
	Control *GetRotationController(); // { return (nSelected==-1 || daddy->Root()->GetCATParent()->GetCATMode()==SETUPMODE) ? NULL : GetLayer(nSelected)->GetRotationController(); }
	Control *GetScaleController(); // { return (nSelected==-1 || daddy->Root()->GetCATParent()->GetCATMode()==SETUPMODE) ? NULL : GetLayer(nSelected)->GetScaleController(); }
	BOOL SetPositionController(Control *c); // { return (nSelected==-1 || daddy->Root()->GetCATParent()->GetCATMode()==SETUPMODE) ? FALSE : GetLayer(nSelected)->SetPositionController(c); }
	BOOL SetRotationController(Control *c); // { return (nSelected==-1 || daddy->Root()->GetCATParent()->GetCATMode()==SETUPMODE) ? FALSE : GetLayer(nSelected)->SetRotationController(c); }
	BOOL SetScaleController(Control *c); // { return (nSelected==-1 || daddy->Root()->GetCATParent()->GetCATMode()==SETUPMODE) ? FALSE : GetLayer(nSelected)->SetScaleController(c); }

	// These get called by the superclass
	/* SA 10/09 - these are commented out of the superclass
	virtual void ValueSet(void *dest, void *src, float weight=1.0f) {}
	virtual void ValueAdd(void *dest, void *src, float weight=1.0f) {}
	virtual void ValueInterp(void *dest, void *src, float weight=1.0f) {}
	virtual void ValueTransform(void *val, const Matrix3& trans) {}
	virtual void ValueMakeRelative(void *val, void *absVal) {}
	virtual void ValueInverse(void *val) {}
	*/

	////////////////////////////////////////////////////
	// Published Functions
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_LAYERMATRIX3CONTROL_FP) return (ILayerMatrix3ControlFP*)this;
		return CATClipValue::GetInterface(id);
	}

	// ILayerMatrix3ControlFP
	virtual Matrix3		GetSetupMatrix();//{ Matrix3 tm(1); GetSetupVal((void*)&tm); return tm;	};
	virtual void		SetSetupMatrix(Matrix3 tm);//{ SetSetupVal((void*)&tm);			};

};
