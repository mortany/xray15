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

#include "../CATControls/CATRigPresets.h"

/////////////////////////////////////////////////////
// ICATObject Interface Class

#define CATOBJECTFLAG_USECUSTOMMESH		(1<<1)
#define CATOBJECTFLAG_CUSTOMMESHAVAIL	(1<<2)
#define CATOBJECTFLAG_LENGTHAXIS_X		(1<<3)

#define I_CATOBJECT		Interface_ID(0x72ba022d, 0x2ab9515a)

class ICATObject : public FPMixinInterface
{

public:

	FPInterfaceDesc* GetDesc(){ return GetDescByID(I_CATOBJECT); };

	virtual Object* AsObject() = 0;
	virtual float GetX() = 0;
	virtual float GetY() = 0;
	virtual float GetZ() = 0;
	virtual void SetX(float val) = 0;
	virtual void SetY(float val) = 0;
	virtual void SetZ(float val) = 0;
	virtual Point3	GetBoneDim() = 0;
	virtual void	SetBoneDim(Point3 val) = 0;

	virtual void SetLengthAxis(int axis) = 0;
	virtual int  GetLengthAxis()=0;

	// soon I don't want the objects to store CATUnits at all
	// I just want then to ask thier transform controller for CATUnits when they need it
	virtual float GetCATUnits() = 0;
	virtual void SetCATUnits(float val) = 0;

	virtual Control* GetTransformController() = 0;
	virtual void	 SetTransformController(Control* ctrl) = 0;

	virtual BOOL SaveRig(CATRigWriter *save) = 0;
	virtual BOOL LoadRig(CATRigReader *load) = 0;

//	virtual BOOL CanCollapseCATObject(INode* node) = 0;
	virtual BOOL CopyMeshFromNode(INode* node) = 0;
	virtual BOOL CustomMeshAvailable() = 0;
	virtual BOOL UsingCustomMesh() = 0;
	virtual void SetUseCustomMesh(BOOL b) = 0;

	virtual void Update() = 0;

	virtual BOOL PasteRig(ICATObject* pasteobj, DWORD flags, float scalefactor){
		UNREFERENCED_PARAMETER(flags);
		SetX(pasteobj->GetX() * scalefactor);
		SetY(pasteobj->GetY() * scalefactor);
		SetZ(pasteobj->GetZ() * scalefactor);
		return TRUE;
	}

	virtual void DisplayObject(TimeValue t, INode* inode, ViewExp *vpt, int flags, Color color, Box3 &bbox)=0;

	virtual void SetCATMesh(Mesh *catmesh)=0;

};

#define I_HUBOBJECT		Interface_ID(0x72ba123d, 0x2ab9515a)

class IHubObject : public ICATObject
{
public:

	virtual float GetPivotPosY() = 0;
	virtual float GetPivotPosZ() = 0;
	virtual void GetPivotPosY(float val) = 0;
	virtual void GetPivotPosZ(float val) = 0;
};
