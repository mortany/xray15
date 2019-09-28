/**********************************************************************
 *<
	FILE: gridhelp.h

	DESCRIPTION:  Defines a Grid Helper Object Class

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#pragma once

#include "iparamm2.h"
#include <ReferenceSaveManager.h>

#ifndef MAX_FLOAT
#define MAX_FLOAT ((float)1.0e20)
#endif

class GridHelpObjCreateCallBack;

#define PBLOCK_REF_NO	 0

#define BMIN_LENGTH		float(0)
#define BMAX_LENGTH		MAX_FLOAT
#define BMIN_WIDTH		float(0)
#define BMAX_WIDTH		MAX_FLOAT
#define BMIN_GRID			float(0)
#define BMAX_GRID			MAX_FLOAT

#define BDEF_DIM			float(0)
#define GRIDHELP_DFLT_SPACING	10.0f

class GridHelpObject : public ConstObject {
private:
	// Object parameters		
	IParamBlock2 *pblock;
	Interval ivalid;

	// local (non-virtual) methods
	void UpdateMesh(TimeValue t);
	void GetBBox(TimeValue t, Matrix3 &tm, Box3& box);
	int Select(TimeValue t, INode *inode, GraphicsWindow *gw, Material *ma, HitRegion *hr, int abortOnHit);
	void FixConstructionTM(Matrix3 &tm, ViewExp *vpt);
	void SetGrid(TimeValue t, float grid);
	float GetGrid(TimeValue t, Interval& valid = Interval(0, 0));
	void InvalidateGrid() { ivalid.SetEmpty(); }

public:
	GridHelpObject();

	//  inherited virtual methods:

	// Animatable methods
	void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return Class_ID(GRIDHELP_CLASS_ID, 0); }
	void GetClassName(TSTR& s) override { s = TSTR(GetString(IDS_DB_GRIDHELPER)); }
	LRESULT CALLBACK TrackViewWinProc(HWND hwnd, UINT message,
		WPARAM wParam, LPARAM lParam) override {
		return(0);
	}
	int NumSubs() override { return 1; }
	Animatable* SubAnim(int i) override;
	TSTR SubAnimName(int i) override;
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i) override { return (i == 0) ? pblock : nullptr; }
	IParamBlock2* GetParamBlockByID(short id) override { return (id == GRIDHELP_PARAMBLOCK_ID) ? pblock : nullptr; }
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;

	// From refmaker
	bool SpecifySaveReferences(ReferenceSaveManager& referenceSaveManager) override;
	IOResult Save(ISave *isave) override;
	IOResult Load(ILoad *iload) override;
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate) override;
	int NumRefs() override { return 1; }
	RefTargetHandle GetReference(int i) override { return (i == 0) ? pblock : nullptr; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) override { if (i == 0) pblock = (IParamBlock2*)rtarg; }
public:

	// From reftarg
	RefTargetHandle Clone(RemapDir& remap) override;

	// From BaseObject
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) override;
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box) override;
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box) override;
	const TCHAR* GetObjectName() override { return GetString(IDS_DB_GRID_OBJECT); }

	// From Object
	ObjectState Eval(TimeValue time) override;
	void InitNodeName(TSTR& s) override { s = TSTR(GetString(IDS_DB_GRID_NODENAME)); }
	int UsesWireColor() override { return 1; }
	Interval ObjectValidity(TimeValue t) override;
	int CanConvertToType(Class_ID obtype) override;
	Object* ConvertToType(TimeValue t, Class_ID obtype) override;
	int IntersectRay(TimeValue t, Ray& r, float& at, Point3& norm) override;

	// From ConstObject
	void GetConstructionTM(TimeValue t, INode* inode, ViewExp *vpt, Matrix3 &tm) override;	// Get the transform for this view
	void SetConstructionPlane(int p, BOOL notify = TRUE) override;
	int  GetConstructionPlane(void) override;
	Point3 GetSnaps(TimeValue t) override;		// Get snaps
	void SetSnaps(TimeValue t, Point3 p) override;
	Point3 GetExtents(TimeValue t) override;
	void SetExtents(TimeValue t, Point3 p) override;
};
