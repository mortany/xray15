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

#include "CATNodeControl.h"
#include "../CATObjects/ICATObject.h"

#define COLLARBONETRANS_CLASS_ID		Class_ID(0x68c00456, 0x455634fe)

class LimbData2;
class CollarBoneTrans : public CATNodeControl {
private:
	friend class CollarbonePLCB;

	// Scripted object pb enumeration
	enum {
		COLOBJ_PB_COLLARBONETRANS,
		COLOBJ_PB_CATUNITS,
		COLOBJ_PB_BONELENGTH,
		COLOBJ_PB_BONESIZE
	};

public:

	enum { pb_idParams };

	enum {
		PB_LIMBDATA,
		PB_OBJCOLLARBONE_PB,
		PB_NODECOLLARBONE,
		PB_TMSETUP,
		PB_SCALE,
		PB_NAME
	};

	IParamBlock2	*pblock;

	enum PARAMLIST {
		REF_PBLOCK,
		REF_LAYERTRANS,
		NUMPARAMS
	};		// our REFERENCE LIST
	enum SUBLIST {
		SUBROTATION,
		SUBSCALE,
		NUMSUBS
	};

public:
	//
	// constructors.
	//
	CollarBoneTrans(BOOL loading = FALSE);
	~CollarBoneTrans();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return COLLARBONETRANS_CLASS_ID; }
	SClass_ID SuperClassID() { return CTRL_MATRIX3_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CL_COLLARBONE); }
	int NumSubs() { return NUMSUBS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev = NULL);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next = NULL);

	//
	// from class ReferenceMaker?
	// Do I need to do anything to this??
	// Later a node reference will be added to the
	// controller as well as the subanims
	int NumRefs() { return NUMPARAMS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);
	void CATMessage(TimeValue t, UINT msg, int data = -1);

	//
	// from class Control:
	//
	//void GetValue(TimeValue t, void *val, Interval &valid, GetSetMethod method);
	//void SetValue(TimeValue t, void *val, int commit, GetSetMethod method);

	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	/**********************************************************************
	 * Loading and saving....
	 */
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	BOOL LoadRig(CATRigReader *load);
	//		BOOL SaveRig(CATRigWriter *save);

			// Parameter block access
	int NumParamBlocks() { return 1; }						// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int) { return pblock; }	// return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?
	// this is a translation tool that lets max go through our
	// subanims and get their appropriate ref number.  Most of the time it will just
	// translate direcly to our subNum but it may not.  (if we reference an object that ain't a subanim.)
	int SubNumToRefNum(int subNum) { return subNum; };

	//////////////////////////////////////////////////////////////////////////
	// CATControl
	USHORT GetRigID() { return idCollarbone; }

	CATGroup* GetGroup();;

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl

	virtual void Initialise(CATControl* owner, BOOL loading);
	CATNodeControl* FindParentCATNodeControl();

	CATNodeControl* GetChildCATNodeControl(int i);
	int NumChildCATNodeControls() { return 2; };

	TSTR GetName() { return pblock->GetStr(PB_NAME); };
	void SetName(TSTR newname, BOOL quiet = FALSE) {
		if (newname != GetName()) {
			if (quiet) DisableRefMsgs();
			pblock->SetValue(PB_NAME, 0, newname);
			if (quiet) EnableRefMsgs();
		};
	};
	TSTR GetRigName();

	void SetLengthAxis(int axis);

	void UpdateUserProps();

	// Our children inherit rotation from the hub, not us
	virtual void ApplyChildOffset(TimeValue t, Matrix3& tmChild) const;

	// Our parent needs to be rotated to be consistent with old CAT
	void CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	//////////////////////////////////////////////////////////////////////////
	// ICollarbone
	CATControl* GetParentCATControl();
	LimbData2* GetLimbData();
	void	SetLimbData(LimbData2* l);;
};
