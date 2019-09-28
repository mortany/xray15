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

#include "FnPub/IBoneGroupManagerFP.h"
#include "CATControl.h"

class DigitSegTrans;

#define DIGITDATA_CLASS_ID	Class_ID(0x67e7064d, 0x28271d66)

class DigitData : public CATControl, public IBoneGroupManagerFP {
private:
	friend class DigitDataPLCB;

	Point3 p3PalmScale;

public:
	class PalmTrans2* pre_clone_palm;

	float FFweight;
	float BAweight;
	float FKWeight;
	float BendAngle;
	Point3 LimbScale;
	Matrix3 tmIKTarget;

	// Reference enumerations
	enum { PBLOCK_REF };

	enum {
		PB_DIGITID_DEPRECATED,
		PB_DIGITCOLOUR,
		PB_DIGITNAME,
		PB_DIGITWIDTH,
		PB_DIGITDEPTH,
		PB_ROOTPOS,
		PB_PALM,
		PB_NUMBONES,
		PB_SEGTRANSTAB,
		PB_SYMDIGIT,
		PB_CATWEIGHT
	};

	// Parameter block
	IParamBlock2	*pblock;	//ref 0

	//Constructor/Destructor
	DigitData(BOOL loading = FALSE);
	~DigitData();

	//From Animatable
	Class_ID ClassID() { return DIGITDATA_CLASS_ID; }
	SClass_ID SuperClassID();
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_DIGITDATA); }

	RefTargetHandle Clone(RemapDir &remap);
	void PostCloneManager();
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int) { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int) { return pblock; }

	// TODO: Maintain the number or references here
	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int) { return pblock; }
private:
	virtual void SetReference(int, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; }							// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }		// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	// this guy never gets shown
	BOOL BypassTreeView() { return true; };

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idDigit; }
	CATGroup* GetGroup();

	int NumLayerControllers() { return 0; };
	CATClipValue* GetLayerController(int) { return NULL; };
	int		NumChildCATControls() { return GetNumBones(); };
	CATControl*	GetChildCATControl(int i);
	virtual CATControl*	GetParentCATControl();

	void DestructAllCATMotionLayers();

	TSTR	GetBoneAddress();

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);
	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);
	void Initialise(PalmTrans2* palm, int id, BOOL loading);
	void SetLengthAxis(int axis);

	void UpdateUI();

	void	SetName(TSTR newname, BOOL quiet = FALSE) {
		if (newname != GetName()) {
			if (quiet) DisableRefMsgs();
			pblock->SetValue(PB_DIGITNAME, 0, newname);
			if (quiet) EnableRefMsgs();
		};
	}
	TSTR	GetName() { return  pblock->GetStr(PB_DIGITNAME); }

	void CATMessage(TimeValue t, UINT msg, int data = -1);

	//void ScaleSelectedObjectSize(SetXFormPacket* ptr);

	BOOL LoadRig(CATRigReader *load);
	BOOL SaveRig(CATRigWriter *save);

	BOOL LoadClip(CATRigReader *load, Interval range, int layerindex, float dScale, int flags);
	BOOL SaveClip(CATRigWriter *save, int flags, Interval timerange, int layerindex);

	//////////////////////////////////////////////////////////////////////////
	// Class IDigit
	PalmTrans2*		GetPalm();
	void			SetPalm(PalmTrans2 *p);

	DigitSegTrans*	GetBone(int BoneID);;

	void	SetDigitID(int id) { SetBoneID(id); }
	int		GetDigitID() { return GetBoneID(); }

	void	SetRootPos(Point3 val, GetSetMethod method = CTRL_ABSOLUTE);
	Point3	GetRootPos() { return pblock->GetPoint3(PB_ROOTPOS); };

	void	SetBoneLengthsTab();
	void	TotalChildLengths();

	void	CreateDigitBones();
	float 	GetDigitLength();
	void	SetDigitLength(float dNewDigitLength);
	float	CalcDefaultDigitLength();

	void 	SetDigitWidth(float width) { pblock->SetValue(PB_DIGITWIDTH, 0, width); };
	float 	GetDigitWidth() { return pblock->GetFloat(PB_DIGITWIDTH); }
	void 	SetDigitDepth(float depth) { pblock->SetValue(PB_DIGITDEPTH, 0, depth); };
	float 	GetDigitDepth() { return pblock->GetFloat(PB_DIGITDEPTH); }

	void	PoseLikeMe(TimeValue t, int iCATMode, DigitData* digit);

	//////////////////////////////////////////////////////////////////////////
	// IBoneGroupManager
	void	SetNumBones(int numsegs) { pblock->SetValue(PB_NUMBONES, 0, numsegs); };
	int		GetNumBones() const { return pblock->Count(PB_SEGTRANSTAB); };
	CatAPI::INodeControl* GetBoneINodeControl(int BoneID);;

};
