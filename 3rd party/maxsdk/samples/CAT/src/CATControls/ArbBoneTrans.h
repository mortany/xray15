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
//
//	Static IDs and Strings for the PalmTrans2 class.
//

// Instead of a locally defined ClassID
#include "CATNodeControl.h"

#define ARBBONETRANS_CLASS_ID		Class_ID(0xdab5c9d, 0x5db56555)

class LimbData2;
class CATClipMatrix3;

class ArbBoneTrans : public CATNodeControl, public CATGroup
{
private:

	// We store a group pointer.  Currently, all this is is a
	// simple cache of whatever the group pointer for our creator
	// was.  However, in future this will be extended so
	// arb bones can create new groups.  This will allows for
	// local weights to be applied to custom rigging groups here.
	// ArbBoneTrans will need to derive from CATGroup to achieve this
	CATControl		*mpGroup;
	DWORD			dwColour;

	enum REFLIST {
		LAYERTRANS,
		WEIGHTS,
		NUMREFS
	};		// our REFERENCE LIST

public:
	//
	// constructors.
	//
	ArbBoneTrans(BOOL loading = FALSE);
	~ArbBoneTrans();

	//
	// from class Animatable:
	//
	Class_ID ClassID() { return ARBBONETRANS_CLASS_ID; }
	void GetClassName(TSTR& s) { s = _T("ArbBone"); }
	int NumSubs() { return NUMREFS; }
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	//////////////////////////////////////////////////////////////////////////
	// ReferenceMaker
	int NumRefs() { return NUMREFS; };
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL);

	// Overwrite the CATNodeControl GetInterface
	// implementation to prevent this class from
	// automatically deleting it's owning CATControl
	void* GetInterface(ULONG id);

	//
	// from class Control:
	//
	int IsKeyable() { return FALSE; };
	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);

	/**********************************************************************
	 * Loading and saving....
	 */

	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//
	// These 2 methods (from animatable) deal with copy/paste.
	//
	virtual BOOL CanCopyAnim() { return TRUE; }				// can we be copied and pasted?

	//////////////////////////////////////////////////////////////////////////
	//
	void Initialise(CATGroup* group, CATNodeControl* ctrlParent, int boneid, BOOL loading, bool bCreateNode = true);

	//////////////////////////////////////////////////////////////////////////
	// CATcontrol
	// If we do not have an external group, we become our own group
	CATGroup* GetGroup() { return mpGroup ? mpGroup->GetGroup() : this; };
	void SetGroup(CATGroup* pGroup) { mpGroup = (pGroup != NULL) ? pGroup->AsCATControl() : NULL; }

	int NumLayerControllers();
	CATClipValue* GetLayerController(int i);

	//////////////////////////////////////////////////////////////////////////
	// CATNodeControl
	USHORT GetRigID() { return idArbBone; }
	CATControl*	GetParentCATControl() { return GetParentCATNodeControl(true); };
	void		ClearParentCATControl() { SetParentCATNodeControl(NULL); }

	// Arb bones implement an almost identical implementation
	// to CATNodeControl.  The only difference is ArbBone does
	// not call the virtual version of ApplyChildOffset on
	// BoneSegTrans, because BoneSegTrans resets the inherited
	// matrix for CATbones (so children do not inherit Twist).
	// ArbBoneTrans is one class that _does_ want to inherit twist though.
	CATNodeControl* FindParentCATNodeControl();

	void CalcParentTransform(TimeValue t, Matrix3& tmParent, Point3& p3ParentScale);

	virtual Color		GetBonesRigColour() { return Color(dwColour); };
	virtual void		SetBonesRigColour(Color clr) {
		dwColour = asRGB(clr);
		UpdateColour(GetCOREInterface()->GetTime());
	};

	virtual	void GetSystemNodes(INodeTab &nodes, SysNodeContext ctxt);

	BOOL PasteRig(CATControl* pastectrl, DWORD flags, float scalefactor);

	//////////////////////////////////////////////////////////////////////////
	// CATGroup functions

	virtual class CATControl* AsCATControl() { return this; };

	virtual Color GetGroupColour();

	virtual void SetGroupColour(Color clr);
};
