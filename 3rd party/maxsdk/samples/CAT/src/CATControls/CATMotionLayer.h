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

#include "NLAInfo.h"
#include "CATHierarchyRoot.h"

#define CATMOTIONLAYER_CLASS_ID Class_ID(0x4ceb45bd, 0xb9c062e)

extern INLAInfoClassDesc* GetCATMotionLayerDesc();

class CATMotionLayer : public NLAInfo {

public:

	CATHierarchyRoot* cathierarchyroot;

	// Reference enumerations
	enum {
		REF_CATHIERARHYROOT = NUM_REFS,
		CATMOTIONLAYER_NUM_REFS
	};

	// Construction
	CATMotionLayer(BOOL loading = FALSE);
	virtual ~CATMotionLayer();

	//////////////////////////////////////////////////////////////////////////
	//From Animatable
	Class_ID ClassID() { return CATMOTIONLAYER_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_CATMOTIONLAYER); }

	int NumSubs() { return CATMOTIONLAYER_NUM_REFS; };
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);

	// TODO: Maintain the number or references here
	int NumRefs() { return CATMOTIONLAYER_NUM_REFS; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	//////////////////////////////////////////////////////////////////////////
	// From Class NLAInfo

//	BOOL SupportTimeOperations();
	virtual Interval GetTimeRange(DWORD flags);
	virtual void EditTimeRange(Interval range, DWORD flags);
	virtual void MapKeys(TimeMap *map, DWORD flags);
	//	int GetKeyTimes(Tab<TimeValue> &times,Interval range,DWORD flags);

	virtual		void Initialise(CATClipRoot* root, const TSTR& name, ClipLayerMethod method, int index);
	virtual		void PostLayerCreateCallback();
	virtual		void PreLayerRemoveCallback();

	virtual			ClipLayerMethod GetMethod() const { return LAYER_CATMOTION; }
	virtual TSTR	GetLayerType() { return _T("CATMotion"); } // used by script // SA: This appears to be used in IO so I won't globalize

	virtual BOOL	ApplyAbsolute() { return TRUE; }
	virtual BOOL	CanTransform() { return FALSE; }

	virtual		INLAInfoClassDesc* GetClassDesc() { return GetCATMotionLayerDesc(); };

	RefTargetHandle CATMotionLayer::Clone(RemapDir& remap);

	virtual void AddSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	virtual		BOOL	SaveClip(CATRigWriter *save, int flags, Interval timerange);
	virtual		BOOL	LoadClip(CATRigReader *load, Interval range, float dScale, int flags);

	BaseInterface* GetInterface(Interface_ID id) {
		if (id == CATHIERARCHYROOT_INTERFACE) {
			if (cathierarchyroot)
				return cathierarchyroot->GetInterface(CATHIERARCHYROOT_INTERFACE);
			return NULL;
		}
		else return NLAInfo::GetInterface(id);
	}

	BOOL PostPaste_CleanupLayer();
};
