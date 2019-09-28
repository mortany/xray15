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

#include "CatPlugins.h"

 // Rig Structure
#include "LimbData2.h"

// CATMotion
#include "CATHierarchyRoot.h"

#include "CATMotionLimb.h"
#include "CATMotionRot.h"
#include "CATNodeControl.h"

class CATMotionRotClassDesc :public ClassDesc2 {
public:
	int 			IsPublic() { return FALSE; }
	void *			Create(BOOL loading = FALSE) { UNREFERENCED_PARAMETER(loading); return new CATMotionRot(); }
	const TCHAR *	ClassName() { return GetString(IDS_CL_CATMOTIONROT); }
	SClass_ID		SuperClassID() { return CTRL_ROTATION_CLASS_ID; }
	Class_ID		ClassID() { return CATMOTIONROT_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("CATMotionRot"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle

};

static CATMotionRotClassDesc CATMotionRotDesc;
ClassDesc2* GetCATMotionRotDesc() { return &CATMotionRotDesc; }

static ParamBlockDesc2 weightshift_param_blk(CATMotionRot::PBLOCK_REF, _T("CATMotionRot params"), 0, &CATMotionRotDesc,
	P_AUTO_CONSTRUCT, CATMotionRot::PBLOCK_REF,
	CATMotionRot::PB_LIMBDATA, _T("LimbData"), TYPE_REFTARG, P_NO_REF, IDS_CL_CATMOTIONLIMB,
		p_end,
	CATMotionRot::PB_P3CATOFFSET, _T("CATMotionOffset"), TYPE_POINT3, P_ANIMATABLE, IDS_OFFSET,
		p_end,
	CATMotionRot::PB_P3CATMOTION, _T("CATMotion"), TYPE_POINT3, P_ANIMATABLE, IDS_CATMOTION,
		p_end,
	CATMotionRot::PB_FLAGS, _T(""), TYPE_INT, 0, 0,
		p_end,
	CATMotionRot::PB_TMOFFSET, _T("OffsetTM"), TYPE_MATRIX3, 0, 0,
		p_end,
	p_end
);

CATMotionRot::CATMotionRot() {
	pblock = NULL;
	CATMotionRotDesc.MakeAutoParamBlocks(this);

	ctrlCATOffset = NULL;
	ctrlCATRotation = NULL;
	flags = 0;
}

CATMotionRot::~CATMotionRot() {
	DeleteAllRefs();
}

void CATMotionRot::Initialise(CATMotionLimb* catmotionlimb, CATNodeControl* bone, Control* p3CATOffset, Control* p3CATRotation, int flags)
{
	pblock->SetValue(PB_LIMBDATA, 0, catmotionlimb, FALSE);
	//	pblock->SetValue(PB_BONE, 0, catnodecontrol, FALSE);
	Matrix3 tmOffset = bone->GetSetupMatrix();
	tmOffset.NoTrans();
	pblock->SetValue(PB_TMOFFSET, 0, tmOffset, FALSE);
	if (p3CATOffset)		pblock->SetControllerByID(PB_P3CATOFFSET, 0, p3CATOffset, FALSE);
	if (p3CATRotation)	pblock->SetControllerByID(PB_P3CATMOTION, 0, p3CATRotation, FALSE);

	pblock->SetValue(CATMotionRot::PB_FLAGS, 0, flags);
	this->flags = flags;
}

RefTargetHandle CATMotionRot::Clone(RemapDir& remap)
{
	// make a new CATMotionRot object to be the clone
	CATMotionRot *newCATMotionRot = new CATMotionRot();
	remap.AddEntry(this, newCATMotionRot);

	newCATMotionRot->ReplaceReference(PBLOCK_REF, CloneParamBlock(pblock, remap));

	// This base clone method allows our parent objects
	// to clone their private data.  (something we cannot do from here)
	BaseClone(this, newCATMotionRot, remap);

	// now return the new object.
	return newCATMotionRot;
}

void CATMotionRot::Copy(Control *from)
{
	if (from->ClassID() == ClassID())
	{
		CATMotionRot *newctrl = (CATMotionRot*)from;
		// a copy will construct its own pblock to keep the pblock-to-owner 1-to-1.
		ReplaceReference(PBLOCK_REF, CloneRefHierarchy(newctrl->pblock));
	}

	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);		// tell our dependents that we've changed.  (this is the default usage for this method)
}

void CATMotionRot::InitControls()
{
	flags = pblock->GetInt(PB_FLAGS);
}

void CATMotionRot::SetValue(TimeValue t, void *val, int, GetSetMethod) {

	AngAxis ax = *(AngAxis*)val;
	Matrix3 tmOffset = pblock->GetMatrix3(PB_TMOFFSET);
	RotateMatrix(tmOffset, ax);
	tmOffset.NoTrans();
	pblock->SetValue(PB_TMOFFSET, t, tmOffset);
};

//-------------------------------------------------------------
void CATMotionRot::GetValue(TimeValue t, void * val, Interval&valid, GetSetMethod method)
{
	CATMotionLimb* limb = (CATMotionLimb*)pblock->GetReferenceTarget(PB_LIMBDATA);
	if (limb == NULL)
		return;

	flags = pblock->GetInt(PB_FLAGS);

	if (method == CTRL_RELATIVE)
	{
		if (flags&CATROT_INHERITORIENT)
		{
			Point3 pos = (*(Matrix3*)val).GetTrans();

			CATHierarchyRoot* pRoot = limb->GetCATMotionRoot();
			if (pRoot != NULL)
				pRoot->GettmPath(t, *(Matrix3*)val, valid, limb->GetPathOffset(), FALSE);
			// Make sure the matrix points down.
			(*(Matrix3*)val).PreRotateY(PI);
			(*(Matrix3*)val).SetTrans(pos);

		}

		Matrix3 tmOffset = pblock->GetMatrix3(PB_TMOFFSET);
		tmOffset.NoTrans();
		*(Matrix3*)val = tmOffset * *(Matrix3*)val;
	}

	Point3 p3CATMotion(Point3::Origin);
	Point3 p3Offset(Point3::Origin);

	pblock->GetValue(PB_P3CATOFFSET, t, p3Offset, valid);
	pblock->GetValue(PB_P3CATMOTION, t, p3CATMotion, valid);

	p3CATMotion += p3Offset;

	// ST 03-02-04 We can't limit our values effectively before this point.
	// which means it's possible to effect values which will create an infinite
	// loop if the values are so large that subtracting off pi has no effect. (see quat::set)
	for (int i = 0; i < 3; i++)
		p3CATMotion[i] = max(min(p3CATMotion[i], 3600), -3600);

	if (limb) {
		float lmr = (float)limb->GetLMR();
		int lengthaxis = Z;
		ICATParentTrans* pParent = limb->GetCATParentTrans();
		if (pParent != NULL)
			lengthaxis = pParent->GetLengthAxis();

		if (lengthaxis == Z) {
			p3CATMotion[Y] *= lmr;
			p3CATMotion[Z] *= lmr;
		}
		else {
			p3CATMotion[Y] *= lmr;
			p3CATMotion[X] *= lmr;
		}
	}

	if ((flags&CATROT_USEORIENT) && limb && method == CTRL_RELATIVE)
	{
		Point3 p3Pos = (*(Matrix3*)val).GetTrans();

		Matrix3 tmOrient;
		limb->GetCATMotionRoot()->GettmPath(t, tmOrient, valid, limb->GetPathOffset(), FALSE);

		AngAxis ax;
		ax = AngAxis(tmOrient.GetRow(X), DegToRad(p3CATMotion.x));
		RotateMatrix(*(Matrix3*)val, ax);
		ax = AngAxis(tmOrient.GetRow(Y), DegToRad(p3CATMotion.y));
		RotateMatrix(*(Matrix3*)val, ax);
		ax = AngAxis(tmOrient.GetRow(Z), DegToRad(p3CATMotion.z));
		RotateMatrix(*(Matrix3*)val, ax);

		(*(Matrix3*)val).SetTrans(p3Pos);
	}
	else
	{
		float CATEuler[] = { DegToRad(p3CATMotion.x), DegToRad(p3CATMotion.y), DegToRad(p3CATMotion.z) };
		Quat qtCATRot;
		EulerToQuat(CATEuler, qtCATRot);

		if (method == CTRL_ABSOLUTE) *(Quat*)val = qtCATRot;
		else
			PreRotateMatrix(*(Matrix3*)val, qtCATRot);
	}
}

/*
int CATMotionRot::NumSubs()
{
	int nSubs = 0;
	if(pblock->GetController(PB_P3CATOFFSET))
		nSubs++;
	if(pblock->GetController(PB_P3CATOFFSET))
		nSubs++;
	return nSubs;
}
*/
TSTR CATMotionRot::SubAnimName(int i)
{
	switch (i)
	{
	case 0:			return GetString(IDS_PARAMS);
	case 1:			return GetString(IDS_CL_CATMOTIONLIMB);
	default:		return _T("");
	}
}
Animatable* CATMotionRot::SubAnim(int i)
{
	switch (i)
	{
	case 0:			return pblock;
	case 1:			return pblock->GetReferenceTarget(CATMotionRot::PB_LIMBDATA);
	default:		return NULL;
	}
}

void CATMotionRot::RefDeleted()
{
	return ReferenceTarget::RefDeleted();
}

//////////////////////////////////////////////////////////////////////
// Backwards compatibility
//

IOResult CATMotionRot::Save(ISave *)
{
	return IO_OK;
}

IOResult CATMotionRot::Load(ILoad *iload)
{
	return IO_OK;
}

