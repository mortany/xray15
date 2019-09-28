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

#include "CATControl.h"
#include "CATGroup.h"
#include "FnPub/IBoneGroupManagerFP.h"
#include "FnPub/ITailFP.h"

class TailTrans;
class Hub;

/**********************************************************************
 * ILimb: Published functions for the Limb As a whole
 */

class TailData2 : public CATControl,
	public ITailFP,
	public IBoneGroupManagerFP,
	public CATGroup
{
private:
	BOOL bNumSegmentsChanging;

	int flagsBegin;
	IParamBlock2	*pblock;	//ref 0

public:
	enum EnumRefs {
		PBLOCK_REF,
		WEIGHTS,
		NUMREFS
	};

	enum {
		PB_HUB,
		PB_TAILID_DEPRECATED,
		PB_NUMBONES,
		PB_TAILTRANS_TAB,
		PB_TAIL_STIFFNESS,			// unused
		PB_NAME,
		PB_COLOUR,
		PB_LENGTH,			// unused
		PB_SIZE,
		PB_TAPER,
		PB_HEIGHT
	};

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	// Loading/Saving
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	// Rig loading/saving
	BOOL LoadRig(CATRigReader* load);
	BOOL SaveRig(CATRigWriter* save);

	BOOL SaveClip(CATRigWriter* save, int flags, Interval timerange, int layerindex);
	BOOL LoadClip(CATRigReader* load, Interval range, int layerindex, float dScale, int flags);

	//From Animatable
	Class_ID ClassID() { return TAILDATA2_CLASS_ID; }
	SClass_ID SuperClassID() { return REF_TARGET_CLASS_ID; };
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_TAILDATA); }

	RefTargetHandle Clone(RemapDir &remap);
	void PostCloneManager();

	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return NUMREFS; }
	TSTR SubAnimName(int i);
	Animatable* SubAnim(int i);

	// TODO: Maintain the number or references here
	int NumRefs() { return NUMREFS; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }
	//Constructor/Destructor

	TailData2(BOOL loading = FALSE);
	~TailData2();

	///////////////////////////////////////////////////
	// CATGroup
	CATControl* AsCATControl() { return this; }

	Color GetGroupColour() { return pblock->GetColor(PB_COLOUR); };
	void  SetGroupColour(Color clr) { pblock->SetValue(PB_COLOUR, 0, clr); };

	///////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idTail; }

	CATGroup* GetGroup() { return this; }

	int NumLayerControllers();
	CATClipValue* GetLayerController(int i);

	int		NumChildCATControls();
	CATControl*	GetChildCATControl(int i);
	CATControl*	GetParentCATControl();

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	TSTR GetName() { return pblock->GetStr(PB_NAME); };
	void SetName(TSTR newname, BOOL quiet = FALSE) {
		if (newname != GetName()) {
			if (quiet) DisableRefMsgs();
			pblock->SetValue(PB_NAME, 0, newname);
			if (quiet) EnableRefMsgs();
		};
	};//){ if(newname!=GetName()) pblock->SetValue(PB_NAME, 0, newname);};

	TSTR GetBoneAddress();

	void UpdateUI();

	void DestructAllCATMotionLayers();

	//////////////////////////////////////////////////////////////////////////
	// From IBoneGroupManager
	virtual void SetNumBones(int newNumBones) { pblock->SetValue(PB_NUMBONES, 0, newNumBones); };
	virtual int  GetNumBones() const { return pblock->Count(PB_TAILTRANS_TAB); };
	virtual CatAPI::INodeControl*	GetBoneINodeControl(int index);

	CATNodeControl*	GetBone(int index);
	//////////////////////////////////////////////////////////////////////////
	// Tail Data Methods
	void Initialise(Hub *hub, int id, BOOL loading, int numbones);

	void SetHub(Hub* h) { pblock->SetValue(PB_HUB, 0, (ReferenceTarget*)h); }
	Hub* GetHub() { return (Hub*)pblock->GetReferenceTarget(PB_HUB); }
	CatAPI::IHub* GetIHub();

	virtual int		GetTailID() { return GetBoneID(); }
	virtual	void	SetTailID(int id) { SetBoneID(id); }

	virtual void CreateTailBones(BOOL loading = FALSE);

	virtual float GetTailStiffness(int index);

	virtual void ScaleTailSize(TimeValue t, const Point3& p3Scale);

	float GetTailLength();
	void  SetTailLength(float val);

	//TSTR GetTailName(BOOL GlobalName = FALSE){UNREFERENCED_PARAMETER(GlobalName); return pblock->GetStr(PB_NAME); };
	//void SetTailName(TSTR newname){ pblock->SetValue(PB_NAME, 0, newname); };

	void UpdateObjDim();

	void UpdateTailColours(TimeValue t);
	void RenameTail();

	///////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	// Published Functions
	//
	BaseInterface* GetInterface(Interface_ID id);
	virtual FPInterfaceDesc* GetDescByID(Interface_ID id);
};
