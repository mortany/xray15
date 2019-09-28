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

//////////////////////////////////////////////////////////////////////
// CATClipWeights
//
class CATClipWeights : public CATClipValue, public ILayerWeightsControlFP
{
private:

protected:
	Tab<float>		tabCacheValues;
	Tab<Interval>	tabCacheIntervals;

	virtual Control* ControlOrDummy(Control* ctrl) { return ctrl; }
	virtual Control* NewWhateverIAmController();// { return NewDefaultFloatController(); }
	virtual ClassDesc2* GetClassDesc() { return GetCATClipWeightsDesc(); };

	// this is used during interative manipulation of the value, whic
	virtual void HoldTrack() {};

public:
	virtual void ResizeList(int n, BOOL loading = FALSE);

	void InvalidateCache(int index = -1);

	virtual CATClipWeights* GetWeightsCtrl();
	virtual float GetWeight(TimeValue t, int index, Interval& valid);
	//		virtual float GetCATMotionWeight(TimeValue t, Interval *valid=NULL);

			// used by colour modes
	virtual Color GetActiveLayerColour(TimeValue t);
	virtual Color GetBlendedLayerColour(TimeValue t);

	virtual void CATMessage(TimeValue t, UINT msg, int data);

	void SetSetupVal(void* val) { UNREFERENCED_PARAMETER(val); };
	void GetSetupVal(void* val) { UNREFERENCED_PARAMETER(val); };
	void ResetSetupVal() {};
	void CreateSetupController(BOOL tf) { UNREFERENCED_PARAMETER(tf); };

	IOResult SaveSetupval(ISave *) { return IO_OK; };
	IOResult LoadSetupval(ILoad *) { return IO_OK; };

public:
	// Construction
	CATClipWeights(BOOL loading = FALSE);

	ParamDimension* GetParamDimension(int) { return stdPercentDim; }

	// Class stuff
	Class_ID ClassID() { return CATCLIPWEIGHTS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_FLOAT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATCLIPWEIGHTS); }
	void DeleteThis() { delete this; }

	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);
	//	Animatable* SubAnim(int i);

	virtual Interval GetTimeRange(DWORD flags) { return Control::GetTimeRange(flags); }

	virtual CATClipWeights* ParentWeights();

	// This is only used by the PLCB for backwards compatiblity
//	void SetWeightController(CATClipThing* dad);

	////////////////////////////////////////////////////
	// Published Functions
	FPInterfaceDesc* GetDescByID(Interface_ID id);

	virtual BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_LAYERWEIGHTSCONTROL_FP) return (ILayerWeightsControlFP*)this;
		return CATClipValue::GetInterface(id);
	}

	// ILayerWeightsControlFP
	virtual float		GetCombinedWeight(int index, TimeValue t);//{ return GetWeight(t, index, FOREVER); };
	virtual Control*	GetParentWeightController() { return ParentWeights(); };

	virtual Control*	GetLayerController(int index) { return tabLayers[index]; };

};
