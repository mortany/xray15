//
// Copyright 2015 Autodesk, Inc.  All rights reserved. 
//
// This computer source code and related instructions and comments are the
// unpublished confidential and proprietary information of Autodesk, Inc. and
// are protected under applicable copyright and trade secret law.  They may
// not be disclosed to, copied or used by any third party without the prior
// written consent of Autodesk, Inc.
//


#pragma once
#include "mouseman.h"
#include "cmdmode.h"
#include "objmode.h"
#include "MeshTopoData.h"

class IObjParam;
class UnwrapMod;

//spline map command mode IDs
#define CID_SPLINEMAP_ALIGN CID_USER + 203
#define CID_SPLINEMAP_ADDCROSS_SECTION CID_USER + 204
#define CID_SPLINEMAP_ALIGNSECTION CID_USER + 205

/******************************************************
Command mode and mouse proc to deal with Spline Map
Aligning cross sections to faces.   The user goes into the 
command mode selects a Face and all the selected cross sections
will be aligned to it
*******************************************************/
class SplineMapAlignFaceMouseProc : public MouseCallBack {
private:
	UnwrapMod *mod;
	IObjParam *iObjParams;
	IPoint2 om;
public:
	SplineMapAlignFaceMouseProc(UnwrapMod* bmod, IObjParam *i) { mod=bmod; iObjParams=i; }
	int proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m );
};

class SplineMapAlignFaceMode : public CommandMode {
private:
	ChangeFGObject fgProc;
	SplineMapAlignFaceMouseProc eproc;
	UnwrapMod *mod;

public:
	SplineMapAlignFaceMode(UnwrapMod* bmod, IObjParam *i);// :

	int Class() { return MODIFY_COMMAND; }
	int ID() { return CID_SPLINEMAP_ALIGN; }
	MouseCallBack *MouseProc(int *numPoints) { *numPoints=1; return &eproc; }
	ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
	BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
	void EnterMode();
	void ExitMode();
};

/******************************************************
Command mode and mouse proc to deal with Spline Map
Aligning cross sections to another cross section.   The user goes into the 
command mode selects a cross section and all the selected cross sections
will be aligned to it
*******************************************************/
class SplineMapAlignSectionMouseProc : public MouseCallBack {
private:
	UnwrapMod *mod;
	IObjParam *iObjParams;
	IPoint2 om;
public:
	SplineMapAlignSectionMouseProc(UnwrapMod* bmod, IObjParam *i) { mod=bmod; iObjParams=i; }
	int proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m );
};

class SplineMapAlignSectionMode : public CommandMode {
private:
	ChangeFGObject fgProc;
	SplineMapAlignSectionMouseProc eproc;
	UnwrapMod *mod;

public:
	SplineMapAlignSectionMode(UnwrapMod* bmod, IObjParam *i);// :

	int Class() { return MODIFY_COMMAND; }
	int ID() { return CID_SPLINEMAP_ALIGNSECTION; }
	MouseCallBack *MouseProc(int *numPoints) { *numPoints=1; return &eproc; }
	ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
	BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
	void EnterMode();
	void ExitMode();
};


/******************************************************
Command mode and mouse proc to deal with Spline Map
Add cross sections to a spline.   The user goes into the 
command mode hits a spline and the cross section is inserted
at that point
*******************************************************/
class SplineMapAddCrossSectionMouseProc : public MouseCallBack {
private:
	UnwrapMod *mod;
	IObjParam *iObjParams;
	IPoint2 om;
public:
	SplineMapAddCrossSectionMouseProc(UnwrapMod* bmod, IObjParam *i) { mod=bmod; iObjParams=i; }
	int proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m );
};

class SplineMapAddCrossSectionMode : public CommandMode {
private:
	ChangeFGObject fgProc;
	SplineMapAddCrossSectionMouseProc eproc;
	UnwrapMod *mod;

public:
	SplineMapAddCrossSectionMode(UnwrapMod* bmod, IObjParam *i);// :
	//                        fgProc(bmod), eproc(bmod,i) {mod=bmod;}

	int Class() { return MODIFY_COMMAND; }
	int ID() { return CID_SPLINEMAP_ADDCROSS_SECTION; }
	MouseCallBack *MouseProc(int *numPoints) { *numPoints=1; return &eproc; }
	ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
	BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
	void EnterMode();
	void ExitMode();
};

class PickSplineNode : 
	public PickModeCallback,
	public PickNodeCallback {
public:				
	UnwrapMod *mod;
	PickSplineNode() {mod=NULL;}
	BOOL HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags);		
	BOOL Pick(IObjParam *ip,ViewExp *vpt);		
	void EnterMode(IObjParam *ip);
	void ExitMode(IObjParam *ip);		
	BOOL Filter(INode *node);
	PickNodeCallback *GetFilter() {return this;}
	BOOL RightClick(IObjParam *ip,ViewExp *vpt) {return TRUE;}
	HCURSOR GetDefCursor(IObjParam *ip);
	HCURSOR GetHitCursor(IObjParam *ip);
	IPoint2 mLastHitPoint;
};



class SelectMode : public MouseCallBack {
public:
	UnwrapMod *mod;
	BOOL region, toggle, subtract;
	IPoint2 om, lm;
	SelectMode(UnwrapMod *m) : region(0), toggle(0), subtract(0) {mod=m;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	virtual int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)=0;
	virtual HCURSOR GetXFormCur()=0;
};

class PaintSelectMode : public MouseCallBack {
public:
	UnwrapMod *mod;
	BOOL  subtract,toggle;
	BOOL first;
	IPoint2 om, lm;
	PaintSelectMode(UnwrapMod *m) : toggle(0), subtract(0) , first(0) {mod=m;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
};

//This is just a class to store the hit IDs since we now need to track subobject ID
//and which local data also
class TVHitData
{
public:
	TVHitData() : mLocalDataID(-1), mID(-1) {}
	int mLocalDataID;
	int mID;
};

// a paint brush to move the vertices within the brush based on the brush settings
class PaintMoveMode : public MouseCallBack
{
public:
	Tab<TVHitData> hits;
	Tab<float> hitsInflu;
	UnwrapMod *mod;
	BOOL  subtract,toggle;
	BOOL first, bDirty, bHolding, bResetCursor, bSoftSelEnabled;
	IPoint2 om, lastPt2, cursorPt2;
	bool mInRelax, userStartRelax;
	int currentMode;
	PaintMoveMode(UnwrapMod *m) : toggle(0), subtract(0) , first(0), mod(m), bDirty(TRUE), bHolding(FALSE), bResetCursor(FALSE), bSoftSelEnabled(FALSE), mInRelax(false) {}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
protected:
	void ResetCursorPos(HWND hWnd, IPoint2& m);
	void StartHolding();
	void EndHolding(bool bCancel);
	void StartPaintMove();
	void EndPaintMove();
	void UpdateHitData(HWND hwnd, IPoint2 m);
	void ClearHitData();
	void DrawGizmo(HWND hWnd, IPoint2 m);
	bool AdjustBrushRadius(HWND hWnd, int flags, IPoint2& m);
	bool TestStartRelax(HWND hWnd, int flags, IPoint2& m);
	bool TestEndRelax(HWND hWnd);
	virtual bool IsRelaxButtonPressed(int flags);
};

// relax brush, use old relax algorithms on chosen vertices
class RelaxMoveMode : public PaintMoveMode
{
public:
	RelaxMoveMode(UnwrapMod *m, int type) : PaintMoveMode(m) {}
	//int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m); // no need to change
	
	virtual bool IsRelaxButtonPressed(int flags);
};

class MoveMode : public SelectMode {
public:				
	MoveMode(UnwrapMod *m) : SelectMode(m) {}
	int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};
class RotateMode : public SelectMode {
public:				
	RotateMode(UnwrapMod *m) : SelectMode(m) {}
	int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};
class ScaleMode : public SelectMode {
public:				
	ScaleMode(UnwrapMod *m) : SelectMode(m) {}
	int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};
class WeldMode : public SelectMode {
public:				
	WeldMode(UnwrapMod *m) : SelectMode(m) {}
	int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};

class PanMode : public MouseCallBack {
public:
	UnwrapMod *mod;
	IPoint2 om;
	float oxscroll, oyscroll;
	BOOL reset;
	PanMode(UnwrapMod *m) : oxscroll(0.0f), oyscroll(0.0f) {mod=m;reset=FALSE;}
	void ResetInitialParams();
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur(); 

};

class ZoomMode : public MouseCallBack {
public:
	UnwrapMod *mod;
	IPoint2 om;
	float ozoom;
	float oxscroll, oyscroll;
	ZoomMode(UnwrapMod *m) : oxscroll(0.0f), oyscroll(0.0f), ozoom(0.0f) {mod=m;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};

class ZoomRegMode : public MouseCallBack {
public:
	UnwrapMod *mod;
	IPoint2 om, lm;		
	ZoomRegMode(UnwrapMod *m) {mod=m;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};

class FreeFormMode : public SelectMode  {
public:
	UnwrapMod *mod;
	IPoint2 om, lm;		
	BOOL dragging;
	FreeFormMode(UnwrapMod *m)  : SelectMode(m) {mod = m;dragging=FALSE;}
	int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};

class SketchIndexData
{
public:
	MeshTopoData *mLD;
	int mIndex;
};

class SketchMode : public MouseCallBack {
public:				
	UnwrapMod *mod;
	Tab<SketchIndexData> indexList;

	Tab<Point3*> pointList;
	Tab<Point3> tempPointList;
	Tab<IPoint2> ipointList;
	int lastPoint;
	IPoint2 prevPoint,currentPoint;	
	int pointCount;
	int drawPointCount;
	int oldMode;
	int oldSubObjectMode;
	SketchMode(UnwrapMod *m) : lastPoint(0), oldMode(0), oldSubObjectMode(0), 
		drawPointCount(0), pointCount(0), inPickLineMode(0)
	{mod = m;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();

	void GetIndexListFromSelection();
	void Apply(HWND hWnd, IPoint2 m);
	BOOL inPickLineMode;
};

//PELT
class PeltStraightenMode : public SelectMode {
public:				
	PeltStraightenMode(UnwrapMod *m) : SelectMode(m) {}
	int subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
	HCURSOR GetXFormCur();
};

class RightMouseMode : public MouseCallBack {
public:
	UnwrapMod *mod;		
	RightMouseMode(UnwrapMod *m) {mod=m;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
};


class MiddleMouseMode : public MouseCallBack {
public:
	UnwrapMod *mod;		
	IPoint2 om;
	float ozoom;
	float oxscroll, oyscroll;
	BOOL inDrag;
	BOOL reset;
	void ResetInitialParams();
	MiddleMouseMode(UnwrapMod *m) : oxscroll(0.0f), oyscroll(0.0f), ozoom(0.0f) {mod=m;inDrag = FALSE;reset=FALSE;}
	int proc(HWND hWnd, int msg, int point, int flags, IPoint2 m);
};

class UnwrapSubModSelectionProcessor : public SubModSelectionProcessor {
protected:
	bool HasOverrideDoubleClickProc() const;
	void OverrideDoubleClickProc(ViewExp* vpt, int flags);

public:
	UnwrapSubModSelectionProcessor(TransformModBox *mc, BaseObject* o, IObjParam* i) 
		: SubModSelectionProcessor(mc,o,i){}
};

class UnwrapSelectModBoxCMode : public CommandMode {
private:
	ChangeFGObject fgProc;
	UnwrapSubModSelectionProcessor mouseProc;
	SelectModBox transProc;
	IObjParam &ip;

public:
	UnwrapSelectModBoxCMode( BaseObject* o, IObjParam* i )
		: fgProc((ReferenceTarget*)o)
		, transProc(o,i)
		, mouseProc(NULL/*&transProc*/,o,i)
		, ip(*i)
	{}

	int Class() { return SELECT_COMMAND; }
	int ID() { return CID_SUBOBJSELECT; }
	MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &mouseProc; }
	ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
	BOOL ChangeFG( CommandMode* oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
	void EnterMode() { ip.SetToolButtonState(SELECT_BUTTON,TRUE); }
	void ExitMode() { ip.SetToolButtonState(SELECT_BUTTON,FALSE); }
};


class PointToPointMouseProc : public MouseCallBack {
private:
	UnwrapMod *mod;
	IObjParam *iObjParams;
	bool enableEdgeSelection;

public:
	PointToPointMouseProc(UnwrapMod* bmod, IObjParam *i)
		: mod(bmod)
		, iObjParams(i)
		, enableEdgeSelection(true)
	{}

	int proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m );

private:
	static void GetPointsFromGeomHit(MeshTopoData *ld, const GeomHit &hit, Tab<int> &points);

	static void SetCursorByHit(const MouseProcHitTestInfo &hitInfo);

	static void ResizeEdgesArrary(BitArray &edges, const MouseProcHitTestInfo &hitInfo);

	static bool RemoveValFromTab(int val, const Tab<int> &inTab, Tab<int> &outTab);

	static void KeepShortestSeam(const Tab<int> &edges, int &minLen, Tab<int> &outputEdges);

	void GetRemovableEdges(MeshTopoData *ld, BitArray &removableEdges);

	void SetSeam(const MouseProcHitTestInfo &hitInfo, IPoint2 m, int procFlags);

	void PreviewSeam(const MouseProcHitTestInfo &hitInfo, IPoint2 m, int procFlags);

	void ReplaceSavedHit(const MouseProcHitTestInfo &hitInfo, IPoint2 m);

	void _SetOrPreviewSeam(const MouseProcHitTestInfo &hitInfo, IPoint2 m, int procFlags, bool isSet=true);

	void BuildSeamEdges(const MouseProcHitTestInfo &hitInfo, int procFlags, Tab<int> &seamEdges);

	void _BuildSeamEdges(MeshTopoData *ld, const GeomHit &hitPrevious, const GeomHit &hitCurrent, bool isRemoval, Tab<int> &outputEdges);

	void SetSeamEdges(const Tab<int> &seamEdges, const MouseProcHitTestInfo &hitInfo, Point3 previousAnchor, int procFlags);

	void PreviewSeamEdges(const Tab<int> &seamEdges, const MouseProcHitTestInfo &hitInfo);

	void SetAnchorAndRetrievePrevious(const MouseProcHitTestInfo &hitInfo, Point3 &previousAnchor);

	void HitTestSubObject(IPoint2 m, ViewExp& vpt, MouseProcHitTestInfo &hitInfo);

	void RecordMacroPeltSelectedSeams(const MouseProcHitTestInfo &hitInfo);

	void ClearPreviewAndRedraw();

	bool ClearPreview();

	void DrawAnchorLine(IPoint2 m, ViewExp &vpt, HWND &cancelHWND, IPoint2 *cancelPoints);

	bool IsOperatingOpenEdges();
	bool IsOperatingSeams();
};

class PeltPointToPointMode : public CommandMode {
private:
	ChangeFGObject fgProc;
	PointToPointMouseProc eproc;
	UnwrapMod* mod;

public:
	PeltPointToPointMode(UnwrapMod* bmod, IObjParam *i);

	  int Class() { return MODIFY_COMMAND; }
	  int ID() { return CID_PELTEDGEMODE; }
	  MouseCallBack *MouseProc(int *numPoints) { *numPoints=1; return &eproc; }
	  ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
	  BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
	  void EnterMode();
	  void ExitMode();
};

#define CID_TWEAKMODE CID_USER + 205

class TweakMouseProc : public MouseCallBack {
private:
	UnwrapMod *mod;
	IObjParam *iObjParams;

	MeshTopoData *mHitLD;
	int mHitLDIndex;
	int mHitFace;
	int mHitVert;
	int mHitTVVert;
	Point3 mBary;
	Point3 mHitUVW[3];
	Point3 mSourceUVW;
	Point3 mHitP[3];
	Point3 mFinalUVW;
	
	MeshTopoData * HitTest(ViewExp& vpt, IPoint2 m, int &hitVert);
	void ComputeNewUVW(ViewExp& vpt, IPoint2 m);
	void AverageUVW();
public:
	TweakMouseProc(UnwrapMod* bmod, IObjParam *i) 
	{ 
		mod=bmod; 
		iObjParams=i; 
		mHitLD = NULL;
		mHitFace = -1;
		mHitVert = -1;
		mHitTVVert = -1;
		mHitLDIndex = -1;
		mBary = Point3(0.0f,0.0f,0.0f);
		mHitUVW[0] = Point3(0.0f,0.0f,0.0f);
		mHitUVW[1] = Point3(0.0f,0.0f,0.0f);
		mHitUVW[2] = Point3(0.0f,0.0f,0.0f);

		mHitP[0] = Point3(0.0f,0.0f,0.0f);
		mHitP[1] = Point3(0.0f,0.0f,0.0f);
		mHitP[2] = Point3(0.0f,0.0f,0.0f);

		mSourceUVW  = Point3(0.0f,0.0f,0.0f);
		mFinalUVW  = Point3(0.0f,0.0f,0.0f);

	}
	int proc( 
		HWND hwnd, 
		int msg, 
		int point, 
		int flags, 
		IPoint2 m );
};

class TweakMode : public CommandMode {
private:
	ChangeFGObject fgProc;
	TweakMouseProc eproc;
	UnwrapMod* mod;

public:
	TweakMode(UnwrapMod* bmod, IObjParam *i);

	  int Class() { return MODIFY_COMMAND; }
	  int ID() { return CID_TWEAKMODE; }
	  MouseCallBack *MouseProc(int *numPoints) { *numPoints=2; return &eproc; }
	  ChangeForegroundCallback *ChangeFGProc() { return &fgProc; }
	  BOOL ChangeFG( CommandMode *oldMode ) { return oldMode->ChangeFGProc() != &fgProc; }
	  void EnterMode();
	  void ExitMode();
};
