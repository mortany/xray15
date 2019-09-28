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

#include "Simpobj.h"

#include "ICATObject.h"

/////////////////////////////////////////////////////
// CATObject
// Superclass

class CATObjectRestore;
class CATObject : public SimpleObject2, public ICATObject {

protected:
	//	Tab<Point3> tabVerticies;		// Vertex List
	//	Tab<Face> tabFaces;				// Face List
	DWORD		flags;
	IObjParam	*ip;				//Access to the interface
	Mesh		catmesh;
	DWORD		dwFileSaveVersion;
	INode		*node;

public:
	friend class CATObjectRestore;
	friend class CATObjectPostLoadCallback;

	void Init();

	/////////////////////////////////////////////////////////////////////////
	// From Class SimpleObject
	SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }

	// Loading/Saving
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	/////////////////////////////////////////////////////////////////////////
	// From Object

	CreateMouseCallBack* GetCreateMouseCallBack();

	BOOL HasUVW() { return FALSE; }
	void SetGenUVW(BOOL sw) { UNREFERENCED_PARAMETER(sw); };

	int CanConvertToType(Class_ID obtype);
	Object* ConvertToType(TimeValue t, Class_ID obtype);
	void GetCollapseTypes(Tab<Class_ID> &clist, Tab<TSTR*> &nlist) { UNREFERENCED_PARAMETER(clist); UNREFERENCED_PARAMETER(nlist); };

	int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);
	ObjectState Eval(TimeValue) { return ObjectState(this); };
	Interval ObjectValidity(TimeValue) { return FOREVER; }

	void BuildMesh(TimeValue t);

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	/////////////////////////////////////////////////////////////////////////
	// Class ICATObject

	Object* AsObject() { return this; };

	void Update() {
		ivalid = NEVER;
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

	BOOL SaveMesh(CATRigWriter *save);
	BOOL LoadMesh(CATRigReader *load);

	BOOL SaveRig(CATRigWriter *save);
	BOOL LoadRig(CATRigReader *load);

	BOOL CopyMeshFromNode(INode* node);
	BOOL CustomMeshAvailable() { return (flags&CATOBJECTFLAG_CUSTOMMESHAVAIL); }
	BOOL UsingCustomMesh() { return (flags&CATOBJECTFLAG_USECUSTOMMESH); }
	void SetUseCustomMesh(BOOL b) {
		if (CustomMeshAvailable()) {
			if (b) flags |= CATOBJECTFLAG_USECUSTOMMESH;
			else flags &= ~CATOBJECTFLAG_USECUSTOMMESH;
			Update();
			GetCOREInterface()->RedrawViews(GetCurrentTime());
		}
	}
	virtual Control* GetTransformController();

	virtual BOOL PasteRig(ICATObject* pasteobj, DWORD flags, float scalefactor);
	virtual void NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index);

	virtual Point3	GetBoneDim() { return Point3(GetX(), GetY(), GetZ()); };
	virtual void	SetBoneDim(Point3 val) { SetX(val.x); SetY(val.y); SetZ(val.z); };

	virtual void SetLengthAxis(int axis);//{ if(axis==X) flags|=CATOBJECTFLAG_LENGTHAXIS_X; else flags&=~CATOBJECTFLAG_LENGTHAXIS_X; }
	virtual int  GetLengthAxis() { return (flags&CATOBJECTFLAG_LENGTHAXIS_X ? X : Z); }

	virtual void DisplayObject(TimeValue t, INode* inode, ViewExp *vpt, int flags, Color color, Box3 &bbox);

	void SetCATMesh(Mesh *catmesh) { this->catmesh = *catmesh; flags |= CATOBJECTFLAG_CUSTOMMESHAVAIL; flags |= CATOBJECTFLAG_USECUSTOMMESH; }

	//////////////////////////////////////////////////////////////////////////
	// Function publishing
	BaseInterface* GetInterface(Interface_ID id)
	{
		if (id == I_CATOBJECT)
			return (ICATObject*)this;

		BaseInterface* pInterface = BaseInterface::GetInterface(id);
		if (pInterface != NULL)
		{
			return pInterface;
		}
		// return the GetInterface() of its super class
		return SimpleObject2::GetInterface(id);
	}

	FPInterfaceDesc* GetDescByID(Interface_ID id);

	enum {
		fnCopyMeshFromNode,
		fnPasteRig,
		propGetTMController,
		propGetUseCustomMesh,
		propSetUseCustomMesh
	};
	void IPasteRig(Object* pasteobj, BOOL bMirrorData) {
		ICATObject *iobj = (ICATObject*)pasteobj->GetInterface(I_CATOBJECT);
		DWORD flags = bMirrorData ? PASTERIGFLAG_MIRROR : 0;
		if (iobj) PasteRig(iobj, flags, 1.0f);
	}

	BEGIN_FUNCTION_MAP
		VFN_1(fnCopyMeshFromNode, CopyMeshFromNode, TYPE_INODE);
		VFN_2(fnPasteRig, IPasteRig, TYPE_OBJECT, TYPE_BOOL);
		RO_PROP_FN(propGetTMController, GetTransformController, TYPE_CONTROL);
		PROP_FNS(propGetUseCustomMesh, UsingCustomMesh, propSetUseCustomMesh, SetUseCustomMesh, TYPE_BOOL);
	END_FUNCTION_MAP

};

