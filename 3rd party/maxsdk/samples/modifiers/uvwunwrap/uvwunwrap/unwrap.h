/*

Copyright 2010 Autodesk, Inc.  All rights reserved. 

Use of this software is subject to the terms of the Autodesk license agreement provided at 
the time of installation or download, or which otherwise accompanies this software in either 
electronic or hard copy form. 

*/

//**************************************************************************/
// DESCRIPTION: Unwrap classes
// AUTHOR: Peter Watje
// DATE: 2006/10/07 
//***************************************************************************/

#pragma once 

#define _CRT_SECURE_NO_WARNINGS

#include "mods.h"
#include "iparamm.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "meshadj.h"
#include "decomp.h"

#include "gport.h"
#include "bmmlib.h"
#include "macrorec.h"
#include "notify.h"

#include "stdmat.h"
#include "MaxIcon.h"

#include "iunwrap.h"
#include "iunwrapMax8.h"
#include "TVData.h"
//PELT
#include "PeltData.h"
#include "undo.h"
#include <Util/StaticAssert.h>

#include "nodedisp.h"

#include <maxscript/maxscript.h>
#include <maxscript/util/listener.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#include "imacroscript.h"
#include "MeshTopoData.h"
#include "ToolRegularMap.h"
#include "ToolLSCM.h"
#include "ToolGrouping.h"
#include "UnwrapCustomToolBars.h"
#include "UnwrapCustomUI.h"
#include "UnwrapSideBarUI.h"
#include "UnwrapModifierPanelUI.h"

#include <ICUIMouseConfigManager.h>
#include "unwrapMouseProc.h"

#include "Painter2D.h"
#include "Painter2DLegacy.h"
#include "ClusterClass.h"
#include "IUnwrapInternal.h"

#include <shellapi.h>

#define UNWRAP_NAME		GetString(IDS_RB_UNWRAPMOD)

//#define DEBUGMODE	1
#define ScriptPrint (the_listener->edit_stream->printf)

#define  TVOBJECTMODE	0
#define  TVVERTMODE		1
#define  TVEDGEMODE		2
#define  TVFACEMODE		3

#define NOMAP			0
#define PLANARMAP		1
#define CYLINDRICALMAP	2
#define SPHERICALMAP	3
#define BOXMAP			4
#define PELTMAP			5
#define SPLINEMAP		6
#define UNFOLDMAP		7
#define LSCMMAP			8

#define	PBLOCK_REF		95

class UnwrapMod;

#define			LINECOLORID			0x368408e0
#define			SELCOLORID			0x368408e1
#define			OPENEDGECOLORID		0x368408e2
#define			HANDLECOLORID		0x368408e3
#define			FREEFORMCOLORID		0x368408e4
#define			SHAREDCOLORID		0x368408e5
#define			BACKGROUNDCOLORID	0x368408e6
//new
#define			GRIDCOLORID			0x368408e7
#define			GEOMEDGECOLORID		0x368408e8
#define			GEOMVERTCOLORID		0x368408e9
#define			PELTSEAMCOLORID		0x368408f0
#define			EDGEDISTORTIONCOLORID		0x368408f1
#define			EDGEDISTORTIONGOALCOLORID		0x368408f2

#define			CHECKERACOLORID					0x368408f3
#define			CHECKERBCOLORID					0x368408f4
#define			PEELCOLORID						0x368408f5

#define CBS		(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL)

#define SAME_SIGNS(A, B) (((A>=0.0f) && (B>0.0f)) || ((A<0.0f) && (B<0.0f)))

#define  maxmin(x1, x2, min) (x1 >= x2 ? (min = x2, x1) : (min = x1, x2))

// Rightclick menu UI stuff
#define FACE4_CHUNK		0x0310
#define VEC4_CHUNK		0x0311

#define TEXTURECHECKERINDEX -1

#define FREEMODEMOVERECTANGLE 0.25

#define CHECKERTILINGDEFAULT 1.0f

#define MODIFIERPANELSECTION	_T("ModifierPanel")
#define VIEWTOOLSECTION			_T("ViewTool")
#define PREFERENCESSECTION		_T("Preferences")
#define MAPSECTION				_T("Map")
#define PEELSECTION				_T("Peel")
#define PACKSECTION				_T("Pack")
#define RELAXSECTION			_T("Relax")
#define BRUSHSECTION			_T("Brush")
#define STITCHSECTION			_T("Stitch")
#define SKETCHSECTION			_T("Sketch")
#define RENDERTEMPLATESECTION	_T("RenderTemplate")

//PELT
extern INT_PTR CALLBACK UnwrapMapRollupWndProc(	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern INT_PTR CALLBACK UnwrapFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//stitch dlg proc
extern INT_PTR CALLBACK UnwrapStitchFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//flatten dlg proc
extern INT_PTR CALLBACK UnwrapFlattenFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//unfold dlg proc
extern INT_PTR CALLBACK UnwrapUnfoldFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//unfold dlg proc
extern INT_PTR CALLBACK UnwrapNormalFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//unfold dlg proc
extern INT_PTR CALLBACK UnwrapPackFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//sketch dlg proc
extern INT_PTR CALLBACK UnwrapSketchFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//relax dlg proc
extern INT_PTR CALLBACK UnwrapRelaxFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//spline map dialog proc
extern INT_PTR CALLBACK UnwrapSplineMapFloaterDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//snap setting proc
extern INT_PTR CALLBACK UnwrapSnapSettingDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
enum
{
	unwrap_recursive_pack = 0,
	unwrap_linear_pack,
	unwrap_non_convex_pack,
};

enum 
{
	unwrap_params,
};

enum
{
	unwrap_renderuv_params
};

enum 
{ 
	unwrap_texmaplist,
	unwrap_checkmtl,
	unwrap_originalmtl, //do not use anymore
	unwrap_texmapidlist,

	unwrap_gridsnap,
	unwrap_vertexsnap,
	unwrap_edgesnap,

	unwrap_showimagealpha,

	unwrap_renderuv_width,
	unwrap_renderuv_height,

	unwrap_renderuv_edgecolor,
	unwrap_renderuv_edgealpha,
	unwrap_renderuv_visible,
	unwrap_renderuv_invisible,
	unwrap_renderuv_seamedges,
	unwrap_renderuv_seamcolor,
	unwrap_renderuv_showframebuffer,
	unwrap_renderuv_force2sided,
	unwrap_renderuv_fillmode,
	unwrap_renderuv_fillcolor,
	unwrap_renderuv_fillalpha,

	unwrap_renderuv_overlap,
	unwrap_renderuv_overlapcolor,

	unwrap_qmap_preview,unwrap_qmap_align,	
	unwrap_originalmtl_list,

	//spline map properties
	unwrap_splinemap_node,				// spline node
	unwrap_splinemap_manualseams,		// whether to use the projection or pelt seams to find the map borders
	unwrap_splinemap_projectiontype,	// 0 circular projection 1 planar projection
	unwrap_splinemap_display,			// turns on/off the spline map display
	unwrap_splinemap_uscale,			// the spline map tiling and offset options
	unwrap_splinemap_vscale,
	unwrap_splinemap_uoffset,
	unwrap_splinemap_voffset,
	/******* NOTE these next 2 entries no longer exist and should not be used *******/
	/******* they are just stubs to allow loading older files *******/
	unwrap_splinemap_selectSplinesMode_DONOTUSE,			// the spline select filter
	unwrap_splinemap_selectCrossSectionsMode_DONOTUSE,	// the cross section filter
	unwrap_localDistorion,
	unwrap_splinemap_iterations,	// this is an iteration value used to iterate down the closest sample
									//the higher this value the more accurate spline mapping will be but it will
									//slower
	unwrap_splinemap_resample_count,	// this is how many cross sections to use when the user resamples a spline
	unwrap_splinemap_method,	// this is how many cross sections to use when the user resamples a spline
	unwrap_toolbar_visible,

	/******* NOTE these next 2 entries no longer exist and should not be used *******/
	/******* they are just stubs to allow loading older files *******/
	unwrap_floaters_DEPRECATED, unwrap_floaters_visible_DEPRECATED,

	unwrap_buttonpanel_visible,
	unwrap_buttonpanel_width,

	unwrap_buttonpanel_height1,
	unwrap_buttonpanel_height2,

	unwrap_weldonlyshared,

	unwrap_group_name, unwrap_group_density,
	unwrap_group_display,

	unwrap_autopin, unwrap_filterpin,

	unwrap_flattenby,
	unwrap_peel_autoedit,
	unwrap_texturecheckermtl
	
};

enum eFreeFormMoveAxis
{
	eMoveX,
	eMoveY,
	eMoveBoth
};

//this call back is used to override the default mechanism for zoom to to selected
//since we do not actually change the objects sub selection it always defaults
//to the whole object.  We want to change that
class UnwrapNodeDisplayCallback : public NodeDisplayCallbackEx
{
	void StartDisplay(TimeValue t, ViewExp * /*vpt*/, int flags) {}
	void EndDisplay(TimeValue t, ViewExp * /*vpt*/, int flags) {}
	bool Display(TimeValue t, ViewExp * /*vpt*/, int flags, INode *node,Object *pObj) {return false; }
	bool SuspendObjectDisplay(TimeValue t, INode *node) { return false;}
	bool HitTest(TimeValue t, INode *node, int type, int crossing, int flags, IPoint2 *p, ViewExp* /*vpt*/,Object *pObj){return FALSE;}
	void Activate() {}
	void Deactivate() {}
	bool SuspendObjectDisplay(TimeValue t, ViewExp * /*vpt*/,  INode *node,Object *pObj ) {return false; }

	//only method we want to override since we want to change the bounding box
	void AddNodeCallbackBox(TimeValue t, INode *node, ViewExp *vpt, Box3& box,Object *pObj);


	TSTR GetName() const { return _T(""); }
	
public:
	Box3 mBounds;
	UnwrapMod *mod;
};


//just a older class we need to keep around to load older alpha files
class SplineCrossSectionOldV1
{
public:
	bool	mIsSelected;			//whether the cross section is selected
	float	mU;						//the position of the cross section along the spline
	float	mTwist;					//the twist around the spline
	Point2	mScale;					//the scale of the cross section
	Point3  mP;						//the point in spline space of the cross section
	Point3  mTangent;				//the tangent of the spline at this cross section, the length is value to the next cross section
	Point3  mTangentNormalized;		//the normalized tangent
	Matrix3 mTM;					//Transform for the cross section
	Matrix3 mITM;					//Inverse Transform for the cross section
};

/******************************************************
This is our cross section class.  A cross section can 
circular or planar and is used to determine the mapping
projection
*******************************************************/
class SplineCrossSection
{
public:
	SplineCrossSection();

	//Displays the current cross section
	void Display(GraphicsWindow *gw);
	//returns the larges X/Y scale value of the cross section
	float GetLargestScale();
	//just dumps the cross section info to the debug window
	void	Dump();

//this interpolates between this current section and the passed in section.  It uses the sub components to do the interoplation
//ie the quat, pos, and scale
	SplineCrossSection	Interp(SplineCrossSection next, float u);
//this interpolates between this current section and the passed in section.  It uses transformation matrix to interpolate
	SplineCrossSection	InterpOnlyMatrix(SplineCrossSection next, float u);
	

	// Warning - instances of this class are saved as a binary blob to scene file
	// Adding/removing members will break file i/o

	bool	mIsSelected;			//whether the cross section is selected
	Quat	mQuat;					//this is the orientation of our cross section
	Point3	mOffset;				//this is offset from the spline, xy is local space of the sample, z is along the spline 
	Point2	mScale;					//the scale of the cross section
	Point3  mP;						//the point in spline space of the cross section
	Point3  mTangent;				//the tangent of the spline at this cross section, the length is value to the next cross section
	Point3  mTangentNormalized;		//the normalized tangent

	//given an upvec this computes the the Transform for this cross section
	void	ComputeTransform(Point3 upVec);
	Matrix3 mTM;					//Transform for the cross section
	Matrix3 mITM;					//Inverse Transform for the cross section

	//these are the initial transform and inverse transform of the crossselection
	Matrix3 mBaseTM;
	Matrix3 mIBaseTM;
};

// compile-time validates size of SplineCrossSection class to ensure its size doesn't change since instances are 
// saved as a binary blob to scene file. Changes in size will break file i/o
class SplineCrossSectionSizeValidator {
	SplineCrossSectionSizeValidator() {}  // not creatable
	MaxSDK::Util::StaticAssert< (sizeof(SplineCrossSection) == sizeof(bool)+sizeof(Quat)+4*sizeof(Point3)+sizeof(Point2)+4*sizeof(Matrix3)+3) > validator; // 3 bytes padding
};

/******************************************************
This is the sampled down version of our spline into segments
We compute a cross section for each sample
*******************************************************/ 
class SubSplineBoundsInfo
{
public:
	bool    mVisisted;					//not used yet
	Box3	mBounds;					//the bounding box for this sample
	SplineCrossSection mCrossSection;	//the cross section for this sample, this is computed from the user defined cross sections
};

/******************************************************
This acceleration structure where we groups samples together
to look them up faster
*******************************************************/ 
class SegmentBoundingBox
{
public:
	Box3	mBounds;		//the bounds of the samples
	int	 	mStart;			//the start sample
	int		mEnd;			//the end sample
};

//these are our projection types
enum SplineMapProjectionTypes {kPlanar,kCircular} ;

/******************************************************
This is our spline element info.  The spline element
contains our user defined cross sections, the spline
cache, and acceleration data
*******************************************************/ 
class SplineElementData
{
public:

	SplineElementData();
	SplineElementData( SplineElementData *splineData);

	~SplineElementData();

	//whether the spline is closed or not
	bool	IsClosed();
	void	SetClosed(bool closed);

	//this display color of the spline
	Point3	GetColor();

	//gets/sets the num of curves, typically a curve is a segment between the knots
	//but this is determined by the actual shape
	int		GetNumCurves();
	void	SetNumCurves(int num);

	//get/set the number of cross sections for this spline
	int		GetNumberOfCrossSections();
	void	SetNumberOfCrossSections(int count);

	//get/set for the spline select state
	void				Select(bool sel);
	bool				IsSelected();

	//get/set for the cross section select state
	void				CrossSectionSelect(int crossSectionIndex,bool sel);
	bool				CrossSectionIsSelected(int crossSectionIndex);

	//allows access to the user defined cross sections
	SplineCrossSection	GetCrossSection(int crossIndex);
	SplineCrossSection*	GetCrossSectionPtr(int crossIndex);

	//allows access to the interpted cross section data
	SplineCrossSection	GetCrossSection(float u);

	//allows access to the interpted cross section on derived data
	SplineCrossSection	GetSubCrossSection(float u);

	//allows you to set the cross section info
	void				SetCrossSection(int index, Point3 p, Point2 scale, Quat q);

	//deletes a cross section
	void				DeleteCrossSection(int crossSectionIndex);
	//insert the cross section at the U 
	void				InsertCrossSection(float u);

	//after the moving a cross section this needs to be called to resort the list
	void				SortCrossSections();

	//when ever a cross section or spine is changed this needs to be called
	//this recomputes all our interpolated spline data
	void				UpdateCrossSectionData();

	//this just clears all the bounding box info
	void				ResetBoundingBox();	

	//these are our spline bounding boxes functions to help speed up look ups
	//this sets the bouding box info
	void				SetBoundingBox(Box3 bounds);
	//returns the bounding box info
	Box3				GetBoundingBox();

	//these are the acceleration bounding boxes functions so we can do look up fasters
	void				SetNumberOfSubBoundingBoxes(int ct);
	int					GetNumberOfSubBoundingBoxes();
	void				SetSubBoundingBoxes(int index, Box3 bounds, float u, SplineCrossSection crossSection);
	SubSplineBoundsInfo	GetSubBoundingBoxes(int index);
	
	//this function determines which samples contains the point
	//	p the point to check in spline space
	//	hits the samples that contain the point
	//	scale this is a scale value to enlarge the sample bounding box by
	void				Contains( Point3 p,Tab<int> &hits, float scale);

	//this will check whether a point is within a specific sample point
	//	p the point to check
	//	subSamples the number
	//	uvw the returning UV projection
	//	d the distnace from spline
	//	hitPoint the hit point on the spline
	//	projType whether to do a circular or planar projection
	//	scale extra scale factor to apply
	bool				ProjectPoint( Point3 p,  int subSample,  Point3 &uvw, float &d, Point3 &hitPoint, SplineMapProjectionTypes projectionType, float scale, int maxIterations);

	//display methods
	void				DisplayCrossSections(GraphicsWindow *gw, int crossSectionIndex,SplineMapProjectionTypes projType );
	void				DisplaySpline(GraphicsWindow *gw, Matrix3 tm,SplineMapProjectionTypes projType );
	void				Display(GraphicsWindow *gw, Matrix3 tm,SplineMapProjectionTypes projType );

	//hittest methods
	void				HitTest(GraphicsWindow *gw, Matrix3 tm,SplineMapProjectionTypes projType, BOOL selectCrossSection );
	BOOL				HitTestSpline(GraphicsWindow *gw, HitRegion *hr,Matrix3 tm, float &u, SplineCrossSection &crossSection);

	//spline cache info
	void				SetLineCache(PolyLine *line);
	float				GetSplineLength();
	void				SetSplineLength(float value);

	BitArray mClosestSubs;

	Point3				UpVec() { return mUpVec; };

	//this is a toggle on how to build the cross section.  If FALSE this method will find the closest
	//point on the spline and extract a tm by interpolating the cross section below and above it.  The 
	//problem with this method is that cross section can intersect which cause confusion.  If this is TRUE
	//we use the 4 polylines that make the edge of of the envelope.  Cross sections are formed by using the
	//edges instead of the center spline.  Since we remove cross overs on the edges we remove the intersection 
	//problem but need more sections to get a good value since we have to interp between transforms instead of 
	//the components
	BOOL				mAdvanceSplines;
	//this is used for planar maps to determine which initial direction to align them to. If this is set they 
	//are align up if not they are rotated 90 to be aligned to the side.
	BOOL				mPlanarMapUp;
//this computes whether we want teh sections to be aligned up or to the sides
//planarMapUp is whether the majority of faces point up or to the side
	void				ComputePlanarUp(BOOL planarMapUp);
private:	

	//just helper draw function to draw a bouding box
	void	DrawBox(GraphicsWindow *gw, Box3 bounds);

	bool					mIsClosed;			//whether this spline is closed
	bool					mIsSelected;		//wheher this spline is selected
	Point3					mColor;				//the color for this spline element

	//this is a list of cross sections for this spline.  There must be a least 2 one
	// at 0.0 and at 1.0
	Tab<SplineCrossSection>	mCrossSectionData;

	//this is the bounding box for this spline in spline space 
	Box3					mBounds;
	//this is the up vector for the initial orientation of the Z UP for the spline cross section
	Point3					mUpVec;

	//this is an acceleration structure so we can look up spline samples.  It contains the bounds
	//for the each curve segment knot to knot typically.
	Tab<SegmentBoundingBox>     mSegmentBounds;

	//this is our sampled down spline, we sample down to line segments since we can have
	//different types of spline type.  This also contains all our interpolated cross section info
	Tab<SubSplineBoundsInfo>	mSubBounds;

	int						mCurves;		//number of curves in the spline

	PolyLine				*mLine;			//out polyline cache

	//this is a list of poly lines that form the vorder of the envelope.  We have 4 polylines per user defined crosssection which are +x, +y, -x, -y
	Tab<PolyLine*>			mBorderLines;

	float					mSplineLength;	//the length of the polyline

	//this gets the center point of specific segement at the U value.  Instead of using the center polyline the border lines are used
	Point3					GetAdvanceCenter(int seg, float u);
	//this gets the tangent and x/y scale  of specific segement at the U value.  Instead of using the center polyline the border lines are used
	Point3					GetAdvanceTangent(int seg, float u, Point3 &xVec, Point3 &yVec);
};
/************************************************************************
THIS is a container class that contains all our spline elements
************************************************************************/
class SplineData
{
public:
	//these are just enums for our current command mode
	enum {kNoMode,kAddMode, kAlignFaceMode, kAlignSectionMode};
	SplineData();
	~SplineData();

//display methods
	void	Display(GraphicsWindow *gw, MeshTopoData *ld, Matrix3 tm, Tab<Point3> &faceCenters,SplineMapProjectionTypes projType );
//hit test methods
	//this does a general hit test of splines and cross sections dependant on the filters
	//	vpt is just the active viewport
	//	node is the node that owns the unwrap modifier this is needed since we can instance the modifier
	//	mc is the mod context so that the hit test can be logged against it
	//  tm is the nodes local to world space tm
	//	hr is the hit region to test agains
	//	flags are the hittesting sel flags
	//	projtype is the spine map projection type circular or planar
	//	selectCross is the filter to determine whether cross sections or splines are hit tested
	int		HitTest(ViewExp *vpt, INode *node, ModContext *mc, Matrix3 tm, HitRegion hr, int flags, SplineMapProjectionTypes projType, BOOL selectCrossSection );
	//this just hit tests the spline and returns if the spline is hit
	//	gw is the graphics window of the active viewport
	//	hr is the hit region to check against, only the first hit will be returned
	//	splineIndex is the index of the spline that was hit
	//	u is the percent along the path that was hit
	BOOL	HitTestSpline(GraphicsWindow *gw, HitRegion *hr,int &splineIndex, float &u);
	//this hittests all the visible crosssections
	//	gw is the graphics window of the active viewport
	//	hr is the hit region to check against, only the first hit will be returned
	//	projType is the projection type that the spline is set to either kCirular or kPlanar
	//	hitSplines a tab containing the hit splines
	//  hitCrossSection a tab containing the hit cross sections which will be the same size as hitSplines
	BOOL	HitTestCrossSection(GraphicsWindow *gw, HitRegion hr, SplineMapProjectionTypes projType,  Tab<int> &hitSplines, Tab<int> &hitCrossSections);

	//Returns the world space bounds of the spline + cross Sections
	Box3	GetWorldsBounds();

	//this adds a node to the system and rebuild everything
	bool	AddNode(INode *node);
	//this just updates the existing info with the current node
	bool	UpdateNode(INode *node);
	
	//returns the numnber of spline elements 
	int		NumberOfSplines();
	//returns the spline length of an element
	float	SplineLength(int splineIndex);
	//returns whether a spline is closed or not
	bool	SplineClosed(int splineIndex);

	//returns the number of user defined cross sections for a  specific spline
	int		NumberOfCrossSections(int splineIndex);
	
	//returns a user defined cross section
	SplineCrossSection*	GetCrossSection(int splineIndex, int crossSectionIndex);

	//aligns a cross section to a vector.  The vector should be in world space
	void	AlignCrossSection(int splineIndex, int crossSectionIndex,Point3 vec);

	//spline selection property
	void	SelectSpline(int splineIndex,bool sel);
	bool	IsSplineSelected(int splineIndex);

	//returns a tab of all the selected splines
	void	GetSelectedSplines(Tab<int> &selSplines);

	//cross section select properties
	void	CrossSectionSelect(int splineIndex,int crossSectionIndex,bool sel);
	bool	CrossSectionIsSelected(int splineIndex,int crossSectionIndex);


	//returns a tab of all the selected splines/cross sections
	void	GetSelectedCrossSections(Tab<int> &selSplines, Tab<int> &selCrossSections);

	//this inserts a new cross section at the specified point.  The new cross section
	//is interpolated based on where it is inserted
	void	InsertCrossSection(int splineIndex, float u);

	//delete a specific cross section
	void	DeleteCrossSection(int splineIndex, int crossSectionIndex);

	//delete the selected cross section
	void	DeleteSelectedCrossSection();

	//moves all the selected cross sections along the spline and x/y local space
	//note the first and last cross sections cannot be moved
	//also you cannot move a cross section past another one
	void	MoveSelectedCrossSections(Point3 v);

	//scales all the selected cross sections along the spline	
	void	ScaleSelectedCrossSections(Point2 v);

	//rotate all the selected cross sections in radians along the Z Axis
//	void	RotateSelectedCrossSections(float v);

	//rotate all the selected cross sections by a quat
	void	RotateSelectedCrossSections(Quat q);

	//this sorts the cross sections we need to keep the cross sections in order of their U values
	//this needs to be called when ever the u values of a cross section change
	void	SortCrossSections();
	
	//this recomputes all our derived data and needs to be called whenever a cross section changes
	void    RecomputeCrossSections();

	//this just returns the closest spline to a point
	int		FindClosestSpline(Point3 p);
	
	//this projects a point into spline space
	//	spline index the spline to check
	//	p the point in world space
	//	hitUVW the return param space point
	//	d the distance from the spline
	//	hitPoint the point on the spline
	//	projectionType the spline projection type circular or planar
	//	onlyInsideEnvelope if set this will only compute the value if the point lies inside the spline envelope
	bool	ProjectPoint(int splineIndex, Point3 p,  Point3 &hitUVW, float &d, Point3 &hitPoint, SplineMapProjectionTypes projectionType, bool onlyInsideEnvelope, int maxIterations);

	//this sets up all our command modes, should be called on beginEdit of the modifier
	void	CreateCommandModes(UnwrapMod *mod, IObjParam *i);
	//this tears down all our command modes and should be called on endEdit
	void	DestroyCommandModes();

	//these start/stop the various spline map command modes
	void	AlignCommandMode();
	void	AlignSectionCommandMode();
	void	AddCrossSectionCommandMode();

	//cross section copy buffer methods
	//returns if there is a copy buffer present
	BOOL	HasCopyBuffer();
	//copies the first selected cross section into the copy buffer
	void	Copy();
	//pastes the copy buffer into our selected cross sections
	void	Paste();
	//copies the specified cross section into the selected cross sections
	void	PasteToSelected(int splineIndex, int crossSectionIndex);

	//these are copy and paste functions for the entire cross sections
	//used for holding the cross section info
	void	CopyCrossSectionData(Tab<SplineElementData*> &data);
	void	PasteCrossSectionData(Tab<SplineElementData*> data);

	//this holds all our cross section data to the hold buffer
	void	HoldData();

	//this resample the cross sections dow;
	void	Resample(int sampleCount);

	//save and load methods
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload, int ver);

	//the HWND dialog property
	void SetHWND(HWND hWND);
	HWND GetHWND();

	//command mode related methods
	//which command mode if any active property
	int		CurrentMode();
	void	SetCurrentMode(int mode);
	//gets the specific command modes
	CommandMode *GetAlignSectionMode();
	CommandMode *GetAlignFaceMode();
	CommandMode *GetAddCrossSectionMode();

	//these get and set the active spline we only use on spline element of the spline
	//if there are multiple elements.  This tells us which spline to use
	void	SetActiveSpline(int index) {mActiveSpline = index;};
	int		GetActiveSpline() {return mActiveSpline;};

	//this determines whether to use the basic interpolation using the center polyline or to use the border poly lines.  If set to 0 the basic 
	//center line interpolation is used
	void	SetMappingType(int type);
	//this is used for planar maps to determine which initial direction to align them to. If this is set they 
	//are align up if not they are rotated 90 to be aligned to the side.
	void	SetPlanarMapDirection(BOOL up);

private:
	//frees all our data
	void FreeData();

	int		mCurrentMode;		//the current command mode
	HWND	mHWND;				//the HWND of the spline map dialog
	bool mSplineDataLoaded;		//just a load flag to know when we need to update stuff
	
	int mSampleRate;			//just how much to sample the spline down by
	
	//NOTE THE SPLINE IS IN WORLD SPACE
	Tab<SplineElementData*> mSplineElementData;	//the list of all our spline elements
	PolyShape				mShapeCache;		//the shape cache from the Node
	
//command modes
	SplineMapAlignFaceMode	*mAlignCommandMode;
	SplineMapAlignSectionMode	*mAlignSectionCommandMode;
	SplineMapAddCrossSectionMode	*mAddCrossSectionCommandMode;

	//the copy buffer info
	BOOL				mHasCopyBuffer;
	SplineCrossSection	mCopyBuffer;

	//this is which spline element is active since we only support one spline at a time now
	int			mActiveSpline;

	bool					mReset;			// this just a flag to tell us to reset the data from loading old data
	BOOL				mPlanarMapUp;     //this sets whether we want planar sections to point up or to the side
};

//Spline Map Hold records
class SplineMapHoldObject : public RestoreObj {
public:
	Tab<SplineElementData*> rdata;
	Tab<SplineElementData*> udata;
	SplineData *m;
	BOOL mRecomputeCrossSections;

	SplineMapHoldObject(SplineData *m, BOOL recomputeCrossSections = FALSE); 
	~SplineMapHoldObject();
	void Restore(int isUndo);
	void Redo();
	void EndHold();
	TSTR Description();
};

#define NumElements(array) (sizeof(array) / sizeof(array[0]))


//COPYPASTE STUFF
//this is the copy and paste buffer class
//it contains the face and vertex data to pasted
class CopyPasteBuffer
{
public:
	~CopyPasteBuffer();
	BOOL CanPaste();
	BOOL CanPasteInstance(UnwrapMod *mod);

	int iRotate;
	BitArray lastSel;
	Tab<Point3> tVertData;				//list of TV vertex data that is in the copy buffer

	Tab<UVW_TVFaceClass*> faceData;		//the list of face data for the copy buffer
	UnwrapMod *mod;						//pointer to the mod that did the last copy
	//we need this since we cannot instance paste across different modifiers

	//FIX we neeed to put some support code in here to know when an lmd or mod goes away
	MeshTopoData *lmd;
	int copyType;						// 0 = one face
	// 1 = whole object
	// 2 = a sub selection
};

class UwrapAction;
class UnwrapActionCallback;
class PeltPointToPointMode;
class TweakMode;

//this is just a class that holds all our thread info for relax
class RelaxThreadData
{
public:

	BOOL mStarted;			//this just stores the state of whether the thread is started or not
	UnwrapMod *mod;			//pointer to the modifier so the thread can call the relax method
	int mType;				//these are all our relax parameters
	BOOL mBoundary;
	BOOL mCorner;
	float mAmount;
	int mIterations;
	float mStretch;
	Tab<MeshTopoData*> mLocalData;
};

//this is just a class that holds all our thread info for relax
class PeltThreadData
{
public:

	BOOL mStarted;			//this just stores the state of whether the thread is started or not
	UnwrapMod *mod;			//pointer to the modifier so the thread can call the relax method
	Tab<MeshTopoData*> mLocalData;

};

enum DistortionOptions
{
	eUndefined,
	eAngleDistortion,
	eAreaDistortion,
};
//indicate the item offset in the list box.
//the offset is computed from the bottom.
#define AngleDistortionOffset 7
#define AreaDistortionOffset 6
//the 5 is assigned to one line
#define PickMapOffset 4
#define RemoveMapOffset 3
#define ResetMapOffset 2

#define CURRENTVERSION 95
//class UnwrapFaceAlignMode;
//5.1.01
class UnwrapMod : 
	public IUnwrapMod, TimeChangeCallback,IUnwrapMod2,IUnwrapMod3,IUnwrapMod4,IUnwrapMod5,IUnwrapMod6, 
	IMeshTopoDataChangeListener, IUnwrapInternal
{	
public:
	friend class Painter2D;
	friend class Painter2DLegacy;
//REFERENCES
	//this is the control for the mapping gizmo, even though we dont allow animation it is easier to keep 
	//a control reference to handle undo and etc
	Control *tmControl;
	// Parameter block
	IParamBlock2	*pblock;	//ref 95

	//we just have these to load in older files then delete them
	StdMat* checkerMat;				//legacy ref no longer used
	ReferenceTarget* originalMat; //legacy ref no longer used
	Texmap *map[90];			//legacy ref no longer used

	UnwrapMod();
	~UnwrapMod();

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s);
	virtual Class_ID ClassID() {return UNWRAP_CLASSID;}
	void BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev);
	void EndEditParams(IObjParam *ip,ULONG flags,Animatable *next);		
	const TCHAR *GetObjectName();
	CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 
	BOOL AssignController(Animatable *control,int subAnim);

	ChannelMask ChannelsUsed()  {return TEXMAP_CHANNEL|GEOM_CHANNEL|TOPO_CHANNEL|SELECT_CHANNEL|SUBSEL_TYPE_CHANNEL|VERTCOLOR_CHANNEL;}
	ChannelMask ChannelsChanged() {return TEXMAP_CHANNEL|VERTCOLOR_CHANNEL|SELECT_CHANNEL|TOPO_CHANNEL; }		
	Class_ID InputType() {return mapObjectClassID;}
	void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t);		
	Interval GetValidity(TimeValue t);		

	int NumRefs() {
		int ct = 0;
		ct += mUVWControls.cont.Count();
		return ct+11+100;
	}
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	int RemapRefOnLoad(int iref) ;

	int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock


	// From BaseObject
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flagst, ModContext *mc);
	unsigned long GetObjectDisplayRequirement() const;
	bool PerpareDisplay(
		const MaxSDK::Graphics::UpdateDisplayContext& displayContext);
	bool UpdatePerNodeItems(
		const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
		MaxSDK::Graphics::UpdateNodeContext& nodeContext,
		MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);

	void InitControl(TimeValue t);
	Matrix3 GetMapGizmoMatrix(TimeValue t);
	void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box, ModContext *mc);
	void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
	void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);

	void Move(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) ;
	void Rotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin);
	void Scale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin); 


	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc);
	void ActivateSubobjSel(int level, XFormModes& modes);
	void SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert=FALSE);
	void ClearSelection(int selLevel);
	void SelectAll(int selLevel);
	void InvertSelection(int selLevel);
	int NumSubs() {
		int ct = 0;
		ct += mUVWControls.cont.Count();

		return ct;
	}

	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);

	RefTargetHandle Clone(RemapDir& remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	//Named Selection set support
	BOOL SupportsNamedSubSels() {return TRUE;}
	void ActivateSubSelSet(TSTR &setName);
	void NewSetFromCurSel(TSTR &setName);
	void RemoveSubSelSet(TSTR &setName);
	void SetupNamedSelDropDown();
	int NumNamedSelSets();
	TSTR GetNamedSelSetName(int i);
	void SetNamedSelSetName(int i,TSTR &newName);
	void NewSetByOperator(TSTR &newName,Tab<int> &sets,int op);

	// Local methods for handling named selection sets
	int FindSet(TSTR &setName);		
	DWORD AddSet(TSTR &setName);
	void RemoveSet(TSTR &setName);
	void ClearSetNames();

	void LocalDataChanged();
	IOResult LoadNamedSelChunk(ILoad *iload);
	IOResult SaveLocalData(ISave *isave, LocalModData *ld);
	IOResult LoadLocalData(ILoad *iload, LocalModData **pld);
	void SetNumSelLabel();	

	// From TimeChangeCallback
	void TimeChanged(TimeValue t) {InvalidateView();}


	//NS: New SubObjType API
	int NumSubObjTypes();
	ISubObjType *GetSubObjType(int i);

	//watje 10-19-99 213458
	BOOL DependOnTopology(ModContext &mc) {return TRUE;}

	bool GetLivePeelModeEnabled();

	//published functions		 
	//Function Publishing method (Mixin Interface)
	//******************************
	BaseInterface* GetInterface(Interface_ID id) 
	{ 
		if (id == UNWRAP_INTERFACE) 
			return (IUnwrapMod*)this; 
		else if (id == UNWRAP_INTERFACE2) 
			return (IUnwrapMod2*)this; 
		else if (id == UNWRAP_INTERFACE3) //5.1.05
			return (IUnwrapMod3*)this; 
		else if (id == UNWRAP_INTERFACE4) //5.1.05
			return (IUnwrapMod4*)this; 
		else if (id == UNWRAP_INTERFACE5)
			return (IUnwrapMod5*)this; 
		else if (id == UNWRAP_INTERFACE6)
			return (IUnwrapMod6*)this; 

		else 
			return Modifier::GetInterface(id);
	} 

	void SelectHandles(int dir);
	void GetFaceSelectionFromMesh(ObjectState *os, ModContext &mc, TimeValue t);
	void GetFaceSelectionFromPatch(ObjectState *os, ModContext &mc, TimeValue t);
	void GetFaceSelectionFromMNMesh(ObjectState *os, ModContext &mc, TimeValue t);

	void ParseMaterials(Mtl *base, Tab<Texmap*> &tmaps, Tab<int> &matIDs);
	Mtl* GetMultiMaterials(Mtl *base);
	void ResetMaterialList();
	void AddToMaterialList(Texmap *map, int id);
	void DeleteFromMaterialList(int index);

	Texmap* GetActiveMap();
	Mtl* GetCheckerMap();
	Mtl* GetOriginalMap();
	void AddMaterial(MtlBase *mtl, BOOL update = TRUE);
	void UpdateMapListBox(); // fills out the map/texmap list box tha contains the active map displayed in the unwrap dialog

	void ApplyGizmo();
	void ComputeSelectedFaceData();
	void ApplyQMap();
	void fnQMap();
	int GetQMapAlign();
	void SetQMapAlign(int align);
	void SetQMapPreview(BOOL preview);
	BOOL GetQMapPreview();

	void fnSetQMapPreview(BOOL preview);
	BOOL fnGetQMapPreview();

	void fnSetQMapAlign(BOOL align);

	void fnSetShowMapSeams(BOOL show);
	BOOL fnGetShowMapSeams();

	void BuildAllFilters();
	void BuildMatIDList();
	void SetMatFilters();
	void UpdateMatFilters(); // update the mat id list and the current mat id filter.
	void UpdateEditorMatListBox(); // update the mat id list

	int GetAxis();

	// Floater win methods
	void SetupDlg(HWND hWnd);
	void SizeDlg();
	void DestroyDlg();
	void ResetEditorMatIDList(); // update the mat id list and set to "all"

	void fnPaintView();

	void fnCloseEditor();
	void fnCloseOptions();

	static void OnModSelChanged(void *param, NotifyInfo *info);

	static void RegisterClasses();
	Point2 UVWToScreen(Point3 pt, float xzoom, float yzoom,int w,int h);
	void ComputeZooms(HWND hWnd, float &xzoom, float &yzoom,int &width,int &height);
	void SetMode(int m, BOOL updateMenuBar = TRUE);
	void InvalidateView();

	//this hittest the teture subojbject in the dialog
	BOOL HitTest(Rect rect,Tab<TVHitData> &hits,BOOL selOnly,BOOL circleMode = FALSE);
	BOOL HitTestCircle(IPoint2& center, float radius, Tab<TVHitData>& hits);

	void DoubleClickSelect(Tab<TVHitData> &hits,BOOL toggle);
	void Select(Tab<TVHitData> &hits,BOOL subtract,BOOL all);
	void ClearSelect();
	void SyncSelect();

	void HoldPoints();
	void HoldPointsAndFaces();
	void HoldSelection();

	BOOL suspendNotify;
	void MovePoints(Point2 pt);
	void MovePaintPoints(Tab<TVHitData>& hits, Point2 pt, Tab<float>& hitsInflu);
	void RotatePoints(HWND h, float ang,bool bAngleSnap = true);
	void ScalePoints(HWND h, float scale, int direction);
	void MirrorPoints(/*HWND h,*/ int direction,BOOL hold = TRUE);
	void FlipPoints(int direction);
	void DetachEdgeVerts( BOOL hold = TRUE);

	void PickMap();
	void SetupImage();
	void GetUVWIndices(int &i1, int &i2);
	void PropDialog();

	void ZoomExtents();
	void Reset();
	void SetupChannelButtons();
	void SetupTypeins();
	void TypeInChanged(int which);
	void ChannelChanged(int which, float x);
	void SetVertexPosition(MeshTopoData *ld, TimeValue t, int which, Point3 pos, BOOL hold = TRUE, BOOL update = TRUE);

	void SnapPoint( Point3 &p);
	void BreakSelected();
	void WeldSelected(BOOL hold = TRUE, BOOL notify = FALSE);
	BOOL WeldPoints(HWND h, IPoint2 m);

	void HideSelected();
	void UnHideAll();

	void FreezeSelected();
	void UnFreezeAll();

	void ZoomSelected();

	void DeleteSelected();

	void ExpandSelection(MeshTopoData *ld, int dir);
	void ExpandSelection(int dir, BOOL rebuildCache = TRUE, BOOL hold = TRUE);

	void UpdatePaintInfluence(Tab<TVHitData>& hits, Point2 center, Tab<float>& hitsInflu);
	void RebuildDistCache();
	void ComputeFalloff(float &u, int ftype);

	void PackFull(bool isAutoPack=false, bool useSel = false);  // pack normalize
	void AutoEdgeToSeamAndLSCMResolve();
	void EraseOpenEdgesByDeSelection(HitRecord *hitRec);

	void LoadUVW(HWND hWnd);
	void SaveUVW(HWND hWnd);

	void	fnPlanarMap();
	void	fnSave();
	void	fnLoad();
	void	fnReset();
	void	fnEdit();
	void	fnSetMapChannel(int channel);
	int		fnGetMapChannel();

	void	fnSetProjectionType(int proj);
	int		fnGetProjectionType();

	void	fnSetVC(BOOL vc);
	BOOL	fnGetVC();

	void	fnMove();
	void	fnMoveH();
	void	fnMoveV();

	void	fnRotate();

	void	fnScale();
	void	fnScaleH();
	void	fnScaleV();

	void	fnMirrorH();
	void	fnMirrorV();

	void	fnExpandSelection();
	void	fnContractSelection();

	void	fnSetFalloffType(int falloff);
	int		fnGetFalloffType();
	void	fnSetFalloffSpace(int space);
	int		fnGetFalloffSpace();
	void	fnSetFalloffDist(float dist);
	float	fnGetFalloffDist();

	void	fnBreakSelected();
	void	fnWeld();
	void	fnWeldSelected();

	void	fnUpdatemap();
	void	fnDisplaymap(BOOL update);
	BOOL	fnIsMapDisplayed();

	void	fnSetUVSpace(int space);
	int		fnGetUVSpace();
	void	fnOptions();

	void	fnLock();
	void	fnHide();
	void	fnUnhide();

	void	fnFreeze();
	void	fnThaw();
	void	fnFilterSelected();

	void	fnPan();
	void	fnZoom();
	void	fnZoomRegion();
	void	fnFit();
	void	fnFitSelected();

	void	fnSnapToggle();
	void	fnSnapSettingDialog();

	int		fnGetCurrentMap();
	void	fnSetCurrentMap(int map);
	int		fnNumberMaps();

	Point3  lColor,sColor;
	Point3*	fnGetLineColor();
	void	fnSetLineColor(Point3 color);

	Point3*	fnGetSelColor();
	void	fnSetSelColor(Point3 color);

	Point3*	fnGetSelPreviewColor();
	void	fnSetSelPreviewColor(Point3 color);

	void	fnSetRenderWidth(int dist);
	int		fnGetRenderWidth();
	void	fnSetRenderHeight(int dist);
	int		fnGetRenderHeight();

	void	fnSetWeldThreshold(float dist);
	float	fnGetWeldThresold();

	void	fnSetUseBitmapRes(BOOL useBitmapRes);
	BOOL	fnGetUseBitmapRes();

	BOOL	fnGetConstantUpdate();
	void	fnSetConstantUpdate(BOOL constantUpdates);

	BOOL	fnGetShowSelectedVertices();
	void	fnSetShowSelectedVertices(BOOL show);

	BOOL	fnGetPixelCenterSnape();
	void	fnSetPixelCenterSnape(BOOL pixelCenter);

	BOOL	fnGetPixelCornerSnap();
	void	fnSetPixelCornerSnap(BOOL pixelCorner);

	int		fnGetMatID();
	void	fnSetMatID(int matid);
	int		fnNumberMatIDs();

	BitArray* fnGetSelectedVerts();
	void	fnSelectVerts(BitArray *sel);
	BOOL	fnIsVertexSelected(int index);

	void	fnMoveSelectedVertices(Point3 offset);
	void	fnRotateSelectedVertices(float angle);
	void	fnRotateSelectedVertices(float angle, Point3 axis);
	void	fnScaleSelectedVertices(float scale,int dir);
	void	fnScaleSelectedVertices(float scale,int dir,Point3 axis);

	Point3* fnGetVertexPosition(TimeValue t,  int index, INode *node);
	BitArray* fnGetSelectedPolygons(INode *node);
	int		fnNumberVertices(INode *node);
	void fnSelectPolygons(BitArray *sel, INode *node);
	BOOL fnIsPolygonSelected(int index, INode *node);
	int	fnNumberPolygons(INode *node);
	void fnMarkAsDead(int index, INode *node);
	int fnNumberPointsInFace(int index, INode *node);
	int fnGetVertexIndexFromFace(int index,int vertexIndex, INode *node);
	int fnGetHandleIndexFromFace(int index,int vertexIndex, INode *node);
	int fnGetInteriorIndexFromFace(int index,int vertexIndex, INode *node);
	int	fnGetVertexGIndexFromFace(int index,int vertexIndex, INode *node);
	int	fnGetHandleGIndexFromFace(int index,int vertexIndex, INode *node);
	int	fnGetInteriorGIndexFromFace(int index,int vertexIndex, INode *node);
	void fnAddPoint(Point3 pos, int fIndex,int ithV, BOOL sel, INode *node);
	void fnAddHandle(Point3 pos, int fIndex,int ithV, BOOL sel, INode *node);
	void fnAddInterior(Point3 pos, int fIndex,int ithV, BOOL sel, INode *node);

	void fnSetFaceVertexIndex(int fIndex,int ithV, int vIndex, INode *node);
	void fnSetFaceHandleIndex(int fIndex,int ithV, int vIndex, INode *node);
	void fnSetFaceInteriorIndex(int fIndex,int ithV, int vIndex, INode *node);

	void fnGetArea(BitArray *faceSelection, 
		float &uvArea, float &geomArea,
		INode *node);
	void fnGetBounds(BitArray *faceSelection, 
		float &x, float &y,
		float &width, float &height,
		INode *node);
	void fnSetSeamSelection(BitArray *sel, INode *node);
	BitArray* fnGetSeamSelection(INode *node);
	void fnSelectPolygonsUpdate(BitArray *sel, BOOL update, INode *node);
	void  fnSketchByNode(Tab<int> *indexList,Tab<Point3*> *positionList,INode *node);
	Point3*  fnGetNormal(int faceIndex, INode *node);
	void fnSetVertexPosition2(TimeValue t, int index, Point3 pos, BOOL hold, BOOL update, INode *node);
	void fnSetVertexPosition(TimeValue t, int index, Point3 pos, INode *node);	
	void fnSetVertexWeight(int index,float weight, INode *node);
	float fnGetVertexWeight(int index, INode *node);
	void fnModifyWeight(int index, BOOL modified, INode *node);
	BOOL fnIsWeightModified(int index, INode *node);
	BitArray* fnGetSelectedVerts(INode *node);
	void fnSelectVerts(BitArray *sel, INode *node);
	BOOL fnIsVertexSelected(int index, INode *node);
	BitArray* fnGetSelectedFaces(INode *node);
	void fnSelectFaces(BitArray *sel,INode *node);
	BOOL fnIsFaceSelected(int index, INode *node);
	
	BitArray* fnGetSelectedEdges(INode *node);
	void fnSelectEdges(BitArray *sel, INode *node);
	BOOL fnIsEdgeSelected(int index, INode *node);

	void fnSetGeomVertexSelection(BitArray *sel, INode *node);
	BitArray* fnGetGeomVertexSelection(INode *node);

	BitArray* fnGetGeomEdgeSelection(INode *node);
	void fnSetGeomEdgeSelection(BitArray *sel, INode *node);

	void GetArea(BitArray *faceSelection, 
		float &x, float &y,
		float &width, float &height,
		float &uvArea, float &geomArea,
		MeshTopoData *ld);

	Point3* fnGetVertexPosition(TimeValue t, int index);

	int		fnNumberVertices();

	void	fnMoveX(float p);
	void	fnMoveY(float p);
	void	fnMoveZ(float p);

	BitArray* fnGetSelectedPolygons();
	void	fnSelectPolygons(BitArray *sel);
	BOOL	fnIsPolygonSelected(int index);
	int		fnNumberPolygons();

	void	fnDetachEdgeVerts();

	void	fnFlipH();
	void	fnFlipV();

	BOOL	fnGetLockAspect();
	void	fnSetLockAspect(BOOL a);

	float	fnGetMapScale();
	void	fnSetMapScale(float sc);

	void	fnGetSelectionFromFace();
	void	fnForceUpdate(BOOL update);
	Box3	gizmoBounds;
	void	fnZoomToGizmo(BOOL all);

	void	fnSetVertexPosition(TimeValue t, int index, Point3 pos);
	void	fnMarkAsDead(int index);

	int		fnNumberPointsInFace(int index);
	int		fnGetVertexIndexFromFace(int index,int vertexIndex);
	int		fnGetHandleIndexFromFace(int index,int vertexIndex);
	int		fnGetInteriorIndexFromFace(int index,int vertexIndex);
	int		fnGetVertexGIndexFromFace(int index,int vertexIndex);
	int		fnGetHandleGIndexFromFace(int index,int vertexIndex);
	int		fnGetInteriorGIndexFromFace(int index,int vertexIndex);

	void	fnAddPoint(Point3 pos, int fIndex,int vIndex, BOOL sel);
	void	fnAddHandle(Point3 pos, int fIndex,int vIndex, BOOL sel);
	void	fnAddInterior(Point3 pos, int fIndex,int vIndex, BOOL sel);

	void	fnSetFaceVertexIndex(int fIndex,int ithV, int vIndex);
	void	fnSetFaceHandleIndex(int fIndex,int ithV, int vIndex);
	void	fnSetFaceInteriorIndex(int fIndex,int ithV, int vIndex);

	void	fnUpdateViews();

	void	fnGetFaceSelFromStack();
	/*************************************************************************************

	This function creates a bit array of all the faces that are within a normal and an angle threshold.

	MeshTopoData *md  - pointer to the local data of Unwrap
	BitArray &sel - where the selection of all faces within the threshold is stored
	Point3 norm - the normal that the faces are checked against
	float angle - the threshold of to apply against the norm
	Tab<Point3> &normList - list of normals of all the faces.  If the tab is empty it will be filled in

	**************************************************************************************/
	void	SelectFacesByNormals( MeshTopoData *md, BitArray &sel, Point3 norm, float angle,Tab<Point3> &normList);

	/*************************************************************************************

	This function creates a bit array of all contigous faces that are within a normal and an angle threshold.

	MeshTopoData *md  - pointer to the local data of Unwrap
	int seedFaceIndex, - this is the face that the function starts searching from
	BitArray &sel - where the selection of all faces within the threshold is stored
	Point3 norm - the normal of the seed face
	float angle - the threshold of to apply against the norm
	Tab<Point3> &normList - list of normals of all the faces.  If the tab is empty it will be filled in

	**************************************************************************************/
	void	SelectFacesByGroup( MeshTopoData *md,BitArray &sel,int seedFaceIndex, Point3 norm, float angle, BOOL relative, MeshTopoData::GroupBy groupBy, Tab<Point3> &normList,
		Tab<BorderClass> &borderData,
		AdjEdgeList *edges=NULL);

	void	FreeClusterList();
	/***************************************************************************************
	This builds clusters a cluster list based on angle threshold
	returns true if it was successful, false if the user aborted it
	****************************************************************************************/
	BOOL	BuildCluster( Tab<Point3> normalList, float threshold, BOOL connected, BOOL cleanUpStrayFaces, MeshTopoData::GroupBy groupBy);

	/***************************************************************************************
	This builds clusters a cluster list based on natural tv element cluster
	returns true if it was successful, false if the user aborted it
	****************************************************************************************/
	BOOL	BuildClusterFromTVVertexElement(MeshTopoData *ld);
	void	NormalizeCluster(float spacing = 0.002f);
	void	fnSelectPolygonsUpdate(BitArray *sel, BOOL update);
	void	fnSelectFacesByNormal(Point3 normal, float angleThreshold, BOOL update);
	void	fnSelectClusterByNormal(float angleThreshold, int seedIndex, BOOL relative, BOOL update);
	void	fnNormalMap( Tab<Point3*> *normaList,  float spacing, BOOL normalize, int layoutType, BOOL rotateClusters, BOOL alignWidth);
	void	fnNormalMapNoParams();
	void	fnNormalMapDialog();
	void	SetNormalDialogPos();
	void	SaveNormalDialogPos();

	void	fnFlattenMap(float angleThreshold, Tab<Point3*> *normaList,  float spacing, BOOL normalize, int layoutType, BOOL rotateClusters, BOOL alignWidth);

	void	fnFlattenMapByMatIDNoParams();
	void	fnFlattenMapByMatID(float angleThreshold, float spacing, BOOL normalize, int layoutType, BOOL rotateClusters, BOOL alignWidth);

	void	fnUnfoldSelectedPolygons(int unfoldMethod,BOOL normalize);
	void	fnUnfoldSelectedPolygonsDialog();
	void	fnUnfoldSelectedPolygonsNoParams();
	void	SetUnfoldDialogPos();
	void	SaveUnfoldDialogPos();

	Point3* fnGetNormal(int faceIndex);
	void fnSetSeedFace();
	// pack methods
	virtual void	CalculateClusterBounds(int i);

	BOOL	LayoutClusters(float spacing, BOOL rotateCluster, BOOL alignWidth, BOOL combineClusters);
	BOOL	LayoutClusters2(float spacing, BOOL rotateCluster, BOOL combineClusters);
	BOOL	LayoutClusters3(float spacing, BOOL rotateCluster, BOOL combineClusters);

	float	PlaceClusters2(float area);
	void	JoinCluster(ClusterClass *baseCluster, ClusterClass *joinCluster);
	void	JoinCluster(ClusterClass *baseCluster, ClusterClass *joinCluster, float x, float y);

	void	FlipSingleCluster(int i,float spacing);

	void	FlipClusters(BOOL flipW,float spacing);
	//returns the total area of the faces
	virtual BOOL	RotateClusters(float &area);
	virtual BOOL	CollapseClusters(float spacing, BOOL rotateClusters, BOOL onlyCenter);

	//COPYPASTE
	void	fnCopy();
	void	fnPaste(BOOL rotate);
	void	fnPasteInstance();
	void	CleanUpDeadVertices();

	void	fnSetDebugLevel(int level);

	void	fnStitchVerts(BOOL bAlign, float fBias);
	void	StitchVerts(BOOL bAlign, float fBias, BOOL bScale);

	void	fnSelectElement();

	void	fnShowVertexConnectionList();
	void	fnStitchVertsNoParams();
	void	fnStitchVertsDialog();
	void	SetStitchDialogPos();
	void	SaveStitchDialogPos();

	void	SelectElement(BOOL addSelection = TRUE);

	void	fnFlattenMapDialog();
	void	fnFlattenMapNoParams();
	void	SetFlattenDialogPos();
	void	SaveFlattenDialogPos();

	BOOL	fnGetTile();
	void	fnSetTile(BOOL tile);

	int		fnGetTileLimit();
	void	fnSetTileLimit(int lmit);

	float	fnGetTileContrast();
	void	fnSetTileContrast(float contrast);

	float	fnGetCheckerTiling() const;
	void	fnSetCheckerTiling(const float tiling);

	//action Table methods
	void DeActivateActionTable();
	void ActivateActionTable();
	BOOL WtIsChecked(int id);
	BOOL WtIsEnabled(int id);

	//this is where all actions route through now this includes WM_COMMAND messages, keyboard action items
	// id is the action item ID (loword when comes from a WM_COMMAND)
	// highWord is the high world when it comes from a WM_COMMAND
	// override not used anymore
	// emitMacroScript will cause a macro script to emit usually called when from WM_COMMAND and not a action item
	BOOL WtExecute(int id, int highWord = -1, BOOL override = TRUE, BOOL emitMacroScript = FALSE);

	BOOL	fnGetShowMap();
	void	fnSetShowMap(BOOL smap);
	void	fnShowMap();

	BOOL	fnGetShowMultiTile();
	void	fnSetShowMultiTile(BOOL smultiTile);
	void	fnShowMultiTile();

	BOOL	fnGetLimitSoftSel();
	void	fnSetLimitSoftSel(BOOL limit);

	int		fnGetLimitSoftSelRange();
	void	fnSetLimitSoftSelRange(int range);


	float	fnGetVertexWeight(int index);
	void	fnSetVertexWeight(int index,float weight);

	BOOL	fnIsWeightModified(int index);
	void	fnModifyWeight(int index, BOOL modified);

	BOOL	fnGetGeomElemMode();
	void	fnSetGeomElemMode(BOOL elem);
	void	SelectGeomElement(MeshTopoData* tmd, BOOL addSel = TRUE, BitArray *oFaces = NULL);
	//		void	SelectGeomElement(BOOL addSel = TRUE, BitArray *oFaces = NULL);

	BOOL	fnGetGeomPlanarMode();
	void	fnSetGeomPlanarMode(BOOL planar);

	void	SelectGeomFacesByAngle(MeshTopoData* tmd);
	float	fnGetGeomPlanarModeThreshold();
	void	fnSetGeomPlanarModeThreshold(float threshold);

	int		fnGetWindowX(bool removeUIScaling = false);
	int		fnGetWindowY(bool removeUIScaling = false);
	int		fnGetWindowW(bool removeUIScaling = false);
	int		fnGetWindowH(bool removeUIScaling = false);

	BOOL	fnGetBackFaceCull();
	void	fnSetBackFaceCull(BOOL backFaceCull);

	BOOL	fnGetOldSelMethod();
	void	fnSetOldSelMethod(BOOL oldSelMethod);

	void	fnSetSelectionMatID(int matID);
	void	fnSelectByMatID(int matID);

	void	fnSelectBySG(int sg);

	BOOL	fnGetTVElementMode();
	void	fnSetTVElementMode(BOOL mode);

	void	GeomExpandFaceSel(MeshTopoData* tmd);
	void	GeomContractFaceSel(MeshTopoData* tmd);
	void	fnGeomExpandFaceSel();
	void	fnGeomContractFaceSel();

	BOOL	fnGetAlwaysEdit();
	void	fnSetAlwaysEdit(BOOL always);

	BOOL	fnGetShowConnection();
	void	fnSetShowConnection(BOOL show);

	BOOL	fnGetFilteredSelected();
	void	fnSetFilteredSelected(BOOL filter);

	BOOL	fnGetLock();
	void	fnSetLock(BOOL snap);

	float	Pack(int method, float spacing, BOOL normalize, BOOL rotate, BOOL fillHoles, BOOL buildClusters = TRUE, BOOL useSelection = TRUE);
	float	fnPack( int method, float spacing, BOOL normalize, BOOL rotate, BOOL fillHoles);
	void	fnPackNoParams();
	void	fnPackDialog();
	void	SetPackDialogPos();
	void	SavePackDialogPos();

	int		fnGetTVSubMode();
	void	fnSetTVSubMode(int mode);

	BitArray* fnGetSelectedFaces();
	void	fnSelectFaces(BitArray *sel);
	BOOL	fnIsFaceSelected(int index);


	//these must be called in pairs
	//this transfer our current selection into vertex selection
	void	TransferSelectionStart();
	//this transfer our vertex selection into our curren selection
	void	TransferSelectionEnd(BOOL partial,BOOL recomputeSelection);

	inline BOOL LineIntersect(Point3 p1, Point3 p2, Point3 q1, Point3 q2);

	int		fnGetFillMode();
	void	fnSetFillMode(int mode);

	void	fnMoveSelected(Point3 offset);
	void	fnRotateSelected(float angle);
	void	fnRotateSelected(float angle, Point3 axis);
	void	fnScaleSelected(float scale,int dir);
	void	fnScaleSelected(float scale,int dir,Point3 axis);

	BitArray* fnGetSelectedEdges();
	void	fnSelectEdges(BitArray *sel);
	BOOL	fnIsEdgeSelected(int index);

	BOOL	fnGetDisplayOpenEdges();
	void	fnSetDisplayOpenEdges(BOOL openEdgeDisplay);

	Point3*	fnGetOpenEdgeColor();
	void	fnSetOpenEdgeColor(Point3 color);

	void	RebuildEdges(bool bAffectGeomEdges = true); // bAffectGeomEdges means that 3d edges are considered to rebuilt or not.

	BOOL	fnGetUVEdgeMode();
	void	fnSetUVEdgeMode(BOOL uvmode);

	void	SelectUVEdge(BOOL openEdgeSelect);	
	void	SelectOpenEdge();

	void	GrowSelectOpenEdge();
	void	ShrinkSelectOpenEdge();

	void	GrowUVLoop(BOOL selectOpenEdges);
	void	ShrinkUVLoop();

	void	GrowUVRing(bool doall);
	void	ShrinkUVRing();

	void	UVFaceLoop(Tab<TVHitData> &hits);
	void	UVEdgeRing(Tab<TVHitData> &hits);

	void	fnAlign(BOOL horizontal);
	void	fnSpace(BOOL horizontal);
	void    fnAlignLinear();

	//align = 0 align h/v
	//align = 1 space h/v
	//align = 2 space and align based on endpoints
	void	SpaceOrAlign(int align, BOOL horizontal);

	void    fnAlignElementToEdge();

	BOOL	fnGetOpenEdgeMode();
	void	fnSetOpenEdgeMode(BOOL uvmode);

	void	fnUVEdgeSelect();
	void	fnOpenEdgeSelect();

	void	fnVertToEdgeSelect(BOOL bPartialSelect = FALSE);
	void	fnVertToFaceSelect(BOOL bPartialSelect = FALSE);

	void	fnEdgeToVertSelect();
	void	fnEdgeToFaceSelect(BOOL bPartialSelect = FALSE);

	void	fnFaceToVertSelect();
	void	fnFaceToEdgeSelect();

	BOOL	fnGetDisplayHiddenEdges();
	void	fnSetDisplayHiddenEdges(BOOL hiddenEdgeDisplay);

	Point3*	fnGetHandleColor();
	void	fnSetHandleColor(Point3 color);

	BOOL	fnGetFreeFormMode();
	void	fnSetFreeFormMode(BOOL freeFormMode);

	Point3*	fnGetFreeFormColor();
	void	fnSetFreeFormColor(Point3 color);

	void ScalePointsXY(HWND h, float scaleX, float scaleY);
	void fnScaleSelectedXY(float scaleX,float scaleY,Point3 axis);

	void	fnSnapPivot(int pos);
	Point3*	fnGetPivotOffset();
	void	fnSetPivotOffset(Point3 color);
	Point3*	fnGetSelCenter();
	void	fnGetSelCenter(Point3 color);

	void	Sketch(Tab<SketchIndexData> *indexList, Tab<Point3*> *positionList, BOOL closed = FALSE);
	void	fnSketch(Tab<int> *indexList, Tab<Point3*> *positionList);
	void	fnSketchNoParams();
	void	fnSketchReverse();
	void	fnSketchSetFirst(int index);
	void	fnSketchDialog();
	void	SetSketchDialogPos();
	void	SaveSketchDialogPos();

	void	InitReverseSoftData();
	void	ApplyReverseSoftData();
	void	PolySelection(BOOL add = TRUE, BOOL bPreview = FALSE);

	int		fnGetHitSize(bool removeUIScaling = false);
	void	fnSetHitSize(int size, bool applyUIScaling = false);

	BOOL	fnGetResetPivotOnSel();
	void	fnSetResetPivotOnSel(BOOL reset);

	BOOL	fnGetPolyMode();
	void	fnSetPolyMode(BOOL pmode);
	void	fnPolySelect();
	void	fnPolySelect2(BOOL add = TRUE);

	BOOL	fnGetAllowSelectionInsideGizmo();
	void	fnSetAllowSelectionInsideGizmo(BOOL select);
	void	RebuildFreeFormData();


	void	GetCfgFilename( TCHAR *filename, size_t filenameSize);
	void	fnSetAsDefaults();
	void	fnLoadDefaults();

	void	fnSetAsDefaults(TSTR sectionName);
	void	fnLoadDefaults(TSTR sectionName);

	void	fnSetSharedColor(Point3 color);
	Point3*	fnGetSharedColor();

	BOOL	fnGetShowShared();
	void	fnSetShowShared(BOOL share);

	void	UpdateShowSharedTVEdges(MeshTopoData* pData);

	inline  void DrawEdge(HDC hdc, /*int a,int b,*/ int va,int vb, 
		IPoint2 pa, IPoint2 pb, IPoint2 pva, IPoint2 pvb);

	BOOL	fnGetSyncSelectionMode();
	void	fnSetSyncSelectionMode(BOOL sync);

	void	SyncTVFromGeomSelection(MeshTopoData *md);
	void	SyncGeomFromTVSelection(MeshTopoData *md);

	void	fnSyncTVSelection();
	void	SyncGeomSelection(bool bUpdateUI = true);
	void	fnSyncGeomSelection();

	Point3*	fnGetBackgroundColor();
	void	fnSetBackgroundColor(Point3 color);

	void	fnUpdateMenuBar();

	BOOL	fnGetBrightCenterTile();
	void	fnSetBrightCenterTile(BOOL bright);

	BOOL	fnGetBlendToBack();
	void	fnSetBlendToBack(BOOL blend);

	BOOL	fnGetFilterMap();
	void	fnSetFilterMap(BOOL filter);

	BOOL	fnGetPaintMode();
	void	fnSetPaintMode(BOOL paint);

	int		fnGetPaintSize(bool removeUIScaling = false);
	void	fnSetPaintSize(int size, bool applyUIScaling = false);

	void	fnIncPaintSize();
	void	fnDecPaintSize();

	int		fnGetTickSize(bool removeUIScaling = false);
	void	fnSetTickSize(int size, bool applyUIScaling = false);

	//new
	float	fnGetGridSize();
	void	fnSetGridSize(float size);

	BOOL	fnGetSnapToggle();
	void	fnSetSnapToggle(BOOL snap);

	BOOL	fnGetGridVisible();
	void	fnSetGridVisible(BOOL visible);

	Point3*	fnGetGridColor();
	void	fnSetGridColor(Point3 color);

	float	fnGetSnapStrength();
	void	fnSetSnapStrength(float str);

	BOOL	fnGetAutoMap();
	void	fnSetAutoMap(BOOL autoMap);

	float	fnGetFlattenAngle();
	void	fnSetFlattenAngle(float angle);

	float	fnGetFlattenSpacing();
	void	fnSetFlattenSpacing(float spacing);

	BOOL	fnGetFlattenNormalize();
	void	fnSetFlattenNormalize(BOOL normalize);

	BOOL	fnGetFlattenRotate();
	void	fnSetFlattenRotate(BOOL rotate);

	BOOL	fnGetFlattenFillHoles();
	void	fnSetFlattenFillHoles(BOOL fillHoles);

	BOOL	fnGetPackRescaleCluster();
	void	fnSetPackRescaleCluster(BOOL rescale);

	BOOL	fnGetPreventFlattening();
	void	fnSetPreventFlattening(BOOL preventFlattening);

	BOOL	fnGetEnableSoftSelection();
	void	fnSetEnableSoftSelection(BOOL enable);

	BOOL	fnGetApplyToWholeObject();
	void	fnSetApplyToWholeObject(BOOL wholeObject);

	void	fnSetVertexPosition2(TimeValue t, int index, Point3 pos, BOOL hold, BOOL update);
	void	fnRelax(int iteration, float str, BOOL lockEdges, BOOL matchArea);

	void	fnFit(int iteration, float str);

	BOOL	fnGetAutoBackground();
	void	fnSetAutoBackground(BOOL autoBackground);

	float	fnGetRelaxAmount();
	void	fnSetRelaxAmount(float amount);
	int		fnGetRelaxIter();
	void	fnSetRelaxIter(int iter);
	BOOL	fnGetRelaxBoundary();
	void	fnSetRelaxBoundary(BOOL boundary);
	BOOL	fnGetRelaxSaddle();
	void	fnSetRelaxSaddle(BOOL saddle);
	void	fnRelax2();
	void	fnRelax2Dialog();

	void	SetRelaxDialogPos();
	void	SaveRelaxDialogPos();


	void	SetPeltDialogPos();
	void	SavePeltDialogPos();

	BOOL	fnGetThickOpenEdges();
	void	fnSetThickOpenEdges(BOOL thick);

	BOOL	fnGetViewportOpenEdges();
	void	fnSetViewportOpenEdges(BOOL thick);

	void	RelaxVerts2(float relax, int iter, BOOL boundary, BOOL saddle, BOOL updateUI = TRUE);

	void	fnSelectInvertedFaces();

	BOOL	fnGetRelativeTypeInMode();
	void	fnSetRelativeTypeInMode(BOOL absolute);
	void    SetAbsoluteTypeInMode(BOOL absolute);

	void	fnStitchVerts2(BOOL bAlign, float fBias, BOOL bScale);

	void	fnAddMap(Texmap *map);

	void	fnGetArea(BitArray *faceSelection, 
		float &x, float &y,
		float &width, float &height,
		float &uvArea, float &geomArea);

	void	fnSetMax5Flatten(BOOL like5);

	void BailStart () 
	{
		GetAsyncKeyState (VK_ESCAPE);
	}

	void BuildVisibleFaces(GraphicsWindow *gw, MeshTopoData *ld, BitArray &sel, int flags);
	void HitGeomEdgeData(MeshTopoData *ld, Tab<UVWHitData> &hitEdges,GraphicsWindow *gw,  HitRegion hr);
	void HitGeomVertData(MeshTopoData *ld, Tab<UVWHitData> &hitVerts,GraphicsWindow *gw,  HitRegion hr);

	void fnPeltDialog();

	BOOL fnGetPeltEditSeamsMode();
	void fnSetPeltEditSeamsMode(BOOL mode);

	BitArray* fnGetSeamSelection();
	void fnSetSeamSelection(BitArray *sel);

	BOOL fnGetPeltPointToPointSeamsMode();
	void fnSetPeltPointToPointSeamsMode(BOOL mode);

	void fnPeltExpandSelectionToSeams();

	void fnPeltDialogResetRig();

	void fnPeltDialogSelectRig();
	void fnPeltDialogSelectPelt();

	void fnPeltDialogSnapRig();

	BOOL fnGetPeltDialogStraightenSeamsMode();
	void fnSetPeltDialogStraightenSeamsMode(BOOL mode);

	void fnPeltDialogMirrorRig();

	void fnPeltDialogRun();
	void fnPeltDialogRelax1();
	void fnPeltDialogRelax2();

	void fnPeltDialogStraighten(int a, int b);

	int fnGetPeltDialogFrames();
	void fnSetPeltDialogFrames(int frames);

	int fnGetPeltDialogSamples();
	void fnSetPeltDialogSamples(int samples);

	float fnGetPeltDialogRigStrength();
	void fnSetPeltDialogRigStrength(float strength);

	float fnGetPeltDialogStiffness();
	void fnSetPeltDialogStiffness(float stiffness);

	float fnGetPeltDialogDampening();
	void fnSetPeltDialogDampening(float dampening);

	float fnGetPeltDialogDecay();
	void fnSetPeltDialogDecay(float decay);

	float fnGetPeltDialogMirrorAxis();
	void fnSetPeltDialogMirrorAxis(float axis);

	void fnAlignAndFit(int axis);

	void EnableMapButtons(BOOL enable);
	void EnableAlignButtons(BOOL enable);

	int fnGetMapMode();
	void fnSetMapMode(int mode);

	void fnSetGizmoTM(Matrix3 tm);
	Matrix3* fnGetGizmoTM();

	void TransformHoldingStart(TimeValue t);
	void TransformHoldingFinish(TimeValue t);

	void fnGizmoFit();
	void fnGizmoCenter();
	void fnGizmoAlignToView();

	void	fnGeomExpandEdgeSel();
	void	fnGeomContractEdgeSel();

	void EnableSubSelectionUI(BOOL enable);
	void EnableFaceSelectionUI(BOOL enable);
	void EnableEdgeSelectionUI(BOOL enable);

	void fnGeomLoopSelect();
	void fnGeomRingSelect();

	void	fnGeomExpandVertexSel();
	void	fnGeomContractVertexSel();

	BitArray* fnGetGeomVertexSelection();
	void fnSetGeomVertexSelection(BitArray *sel);

	BitArray* fnGetGeomEdgeSelection();
	void fnSetGeomEdgeSelection(BitArray *sel);

	BOOL fnGetAlwayShowPeltSeams();
	void fnSetAlwayShowPeltSeams(BOOL show);

	void fnPeltSeamsToSel(BOOL replace);
	void fnPeltSelToSeams(BOOL replace);

	void fnSetNormalizeMap(BOOL normalize);
	BOOL fnGetNormalizeMap();

	void fnSetShowEdgeDistortion(BOOL show);
	BOOL fnGetShowEdgeDistortion();

	void fnSetLockSpringEdges(BOOL lock);
	BOOL fnGetLockSpringEdges();

	void GetOpenEdges(MeshTopoData *ld, Tab<int> &openEdges, Tab<int> &results);

	void fnSelectOverlap();
	void SelectOverlap();

	void fnGizmoReset();

	void fnSetEdgeDistortionScale(float scale);
	float fnGetEdgeDistortionScale();

	void fnRelaxByFaceAngle(int frames, float stretch, float str, BOOL lockOuterVerts,HWND status = NULL);
	void fnRelaxByEdgeAngle(int frames, float stretch,  float str, BOOL lockOuterVerts,HWND status = NULL);
	void fnRelaxBySprings(int frames, float stretch, BOOL lockOuterVerts);
	void fnRelaxBySpringsDialog();

	void fnRelaxByFaceAngleNoDialog(int frames, float stretch, float str, BOOL lockOuterVerts);
	void fnRelaxByEdgeAngleNoDialog(int frames, float stretch,  float str, BOOL lockOuterVerts);

	int fnGetRelaxBySpringIteration();
	void fnSetRelaxBySpringIteration(int iteration);

	float fnGetRelaxBySpringStretch();
	void fnSetRelaxBySpringStretch(float stretch);

	void SetRelaxBySpringDialogPos();
	void SaveRelaxBySpringDialogPos();

	BOOL fnGetRelaxBySpringVEdges();
	void fnSetRelaxBySpringVEdges(BOOL useVEdges);

	float fnGetRelaxStretch();
	void fnSetRelaxStretch(float stretch);

	int fnGetRelaxType();
	void fnSetRelaxType(int type);

	void ShowCheckerMaterial(BOOL show);

	void fnSetWindowXOffset(int offset, bool applyUIScaling = false);
	void fnSetWindowYOffset(int offset, bool applyUIScaling = false);

	void SetCheckerMapChannel();

	void fnFrameSelectedElement();
	void FrameSelectedElement();

	int		fnGetSG();
	void	fnSetSG(int sg);

	int		fnGetMatIDSelect();
	void	fnSetMatIDSelect(int matID);

	void fnSetShowCounter(BOOL show);
	BOOL fnGetShowCounter();

	void BuildSnapBuffer();
	void FreeSnapBuffer();

	BOOL GetGridSnap();
	void SetGridSnap(BOOL snap);

	BOOL GetVertexSnap();
	void SetVertexSnap(BOOL snap);

	BOOL GetEdgeSnap();
	void SetEdgeSnap(BOOL snap);

	BOOL GetShowImageAlpha();
	void SetShowImageAlpha(BOOL show);

	void fnRenderUVDialog() override;
	void fnRenderUV();
	void RenderUV(BitmapInfo bi);
	void RenderUV(int UTile, int VTile);
	void fnRenderUV(const TCHAR *name, int UTile, int VTile) override;
	// Helper function for Render UV templace
	// Append renderUVbi file name with UV tile suffix
	// Return the old file name
	TSTR AppendUVSuffixToBitmap( int UTile, int VTile );


	void GuessAspectRatio();

	BOOL fnIsMesh();

	Point3 SnapPoint(Point3 p, MeshTopoData *ld, int snapIndex);

	int GetMeshTopoDataCount();
	MeshTopoData *GetMeshTopoData(int index);
	MeshTopoData *GetMeshTopoData(INode *node);
	INode *GetMeshTopoDataNode(int index);
	bool LockMeshTopoData();					// this prevent the meshtopo data from getting reevaled so you can lock it if doing an operation on another thread NOTE while locked the node selection cannot change
	void UnlockMeshTopoData();					// this lets the meshtopo data from getting reevaled

	// From Peel Mode Dialog
	bool GetAutoPackEnabled();
	void SetAutoPackEnabled(bool bAutoPack);
	bool GetLivePeelEnabled() const;
	void SetLivePeelEnabled(bool bLivePeel);
	void SyncPeelModeSettingToIniFile();

	int ControllerAdd();
	int ControllerCount();
	Control* Controller(int index);
	void UpdateControllers(TimeValue t, MeshTopoData* pData);

	BOOL	fnGetRotationsRespectAspect();
	void	fnSetRotationsRespectAspect(BOOL respect);

	TSTR    GetMacroStr(const TCHAR *macroStr);

	//For suit alignment
	//if the combination of pressed key and mouse button  is consistent with 
	//the setting by UI or MAXScript,the return value is true.Otherwise,it is false
	static bool CanEnterCustomizedMode();
	//Enter the customized mode
	static void EnterCustomizedMode(UnwrapMod* pMod);
	//if the customized mode is set and the corresponding pressed key or mouse button pressed is up,
	//the return value is true.Otherwise,otherwise it is false
	static bool CanExitCustomizedMode();
	//End the customized mode
	static void ExitCustomizedMode(UnwrapMod* pMod);
	//get the states of the pressed mouse button and key
	static int GetButtonsStates();
	//change the enum value of the key of customized pan or zoom mode to the int type which bit information reflect the correspond key
	static int KeyOption2ButtonFlag(MaxSDK::CUI::KeyOption modifierKeys);
	//change the enum value of the mouse button of customized pan or zoom mode to the int type which bit information reflect the correspond key
	static int MouseButtonOption2ButtonFlag(MaxSDK::CUI::MouseButtonOption mouseButtons);
	//get the match result that decide whether the operation mode entering customized pan mode or zoom mode
	static void GetMatchResult(bool &bCustomizedPan,bool &bCustomizedZoom);

	// clear the selection preview bitarray
	bool ClearTVSelectionPreview();
	// Assign the selection preview bitarray as selection bitarray
	bool ConvertPreviewSelToSelected();
	
	// window handles for tool dialog of uvw editor
	static HWND stitchHWND;
	static HWND flattenHWND;
	static HWND unfoldHWND;
	static HWND sketchHWND;
	static HWND normalHWND;
	static HWND packHWND;
	static HWND relaxHWND;
	static HWND renderUVWindow;

	static HWND hTextures; // texture list combobox handle
	static HWND hMatIDs; // material id list combobox handle

	static HWND hDialogWnd; // uvw editor handle
	static HWND hView; // uvw editor view window handle

	// handles for rollout window in modify panels
	static HWND hSelParams;
	static HWND hMatIdParams;
	static HWND hEditUVParams;
	static HWND hPeelParams;
	static HWND hMapParams;
	static HWND hWrapParams;
	static HWND hParams;
	static HWND hConfigureParams;
	
	static HWND hOptionshWnd; // preference dialog of uvw editor
	static HWND hSelRollup; // TODO: check if this is the same as hSelParams
	static HWND hRelaxDialog;
	static int iToolBarHeight;
	
	static WINDOWPLACEMENT sketchWindowPos;
	static WINDOWPLACEMENT packWindowPos;
	static WINDOWPLACEMENT stitchWindowPos;
	static WINDOWPLACEMENT flattenWindowPos;
	static WINDOWPLACEMENT unfoldWindowPos;
	static WINDOWPLACEMENT normalWindowPos;
	static WINDOWPLACEMENT windowPos;
	static WINDOWPLACEMENT relaxWindowPos;
	static WINDOWPLACEMENT peltWindowPos;

	static ISpinnerControl *iSetMatIDSpinnerEdit;
	static ISpinnerControl *iSelMatIDSpinnerEdit;
	static ISpinnerControl *iMapID;
	static HCURSOR selCur  ;
	static HCURSOR moveCur ;
	static HCURSOR moveXCur;
	static HCURSOR moveYCur;
	static HCURSOR rotCur ;
	static HCURSOR scaleCur;
	static HCURSOR scaleXCur;
	static HCURSOR scaleYCur;

	static HCURSOR zoomCur;
	static HCURSOR zoomRegionCur;
	static HCURSOR panCur ;
	static HCURSOR weldCur ;
	static HCURSOR weldCurHit ;

	static HCURSOR sketchCur ;
	static HCURSOR sketchPickCur ;
	static HCURSOR sketchPickHitCur ;

	static HCURSOR circleCur[16];

	static IObjParam  *ip;
	static UnwrapMod *editMod; // the unwrap mod which is being activated in the modifiy panel

	static MouseManager mouseMan;

	static MoveMode *moveMode;
	static RotateMode *rotMode;
	static ScaleMode *scaleMode;
	static PanMode *panMode;
	static ZoomMode *zoomMode;
	static ZoomRegMode *zoomRegMode;
	static WeldMode *weldMode;
	static FreeFormMode *freeFormMode;
	static SketchMode *sketchMode;

	//For suit alignment
	//record the original mode before the  mode will be changed when customized mode is set
	static int							iOriginalMode;
	static int							iOriginalOldMode;
	//indicate whether the customized zoom mode  is set
	static bool						bCustomizedZoomMode;
	//indicate whether the customized pan mode  is set
	static bool						bCustomizedPanMode;
	//record mouse pointer information when mouse moving for the initialization of customized mode 
	static WPARAM						wParam;
	static LPARAM						lParam;

	//PELT
	static PeltStraightenMode *peltStraightenMode;
	static RightMouseMode *rightMode;
	static MiddleMouseMode *middleMode;
	static UnwrapSelectModBoxCMode *selectMode;
	static MoveModBoxCMode *moveGizmoMode;
	static RotateModBoxCMode *rotGizmoMode;
	static UScaleModBoxCMode *uscaleGizmoMode;
	static NUScaleModBoxCMode *nuscaleGizmoMode;
	static SquashModBoxCMode *squashGizmoMode;		
	static PaintSelectMode *paintSelectMode;
	static PaintMoveMode*	paintMoveMode;
	static RelaxMoveMode*   relaxMoveMode;

	static COLORREF lineColor, selColor, selPreviewColor;
	static COLORREF openEdgeColor;
	static COLORREF handleColor;
	static COLORREF freeFormColor;
	static COLORREF sharedColor;
	static COLORREF backgroundColor;
	static COLORREF peelColor;
	static COLORREF gridColor;

	static HWND hSnapSettingDialog;

	Tab<TSTR*> namedSel;		
	Tab<DWORD> ids;

	Tab<TSTR*> namedVSel;		
	Tab<DWORD> idsV;

	Tab<TSTR*> namedESel;		
	Tab<DWORD> idsE;

	UnwrapActionCallback* pCallback;

	BOOL mirrorGeoSelectionStatus;
	MirrorAxisOptions mirrorGeoSelectionAxis;
	float mirrorGeoThreshold;

	float falloffStr;	//this is the soft selection falloff distance
	BOOL forceUpdate;	//this is a flag that lets the user disable the modifier so it does not update
						//usually set when they want to lock the UVs of a specific topology
	BOOL getFaceSelectionFromStack;  //this flag copies the incoming face selection into the uvw face selection
	int version;			//the version used for file io
	BOOL oldDataPresent;	//this is whether older max file is detected	
	Matrix3 mMapGizmoTM; //this is the world space representation of our quick map gizmo
	Matrix3& GetMapGizmoTM();
	DWORD flags;		//this is just some flags used to setup the tmcontrol

	//this is the zoom, pan position and aspect ratio of the dialog display
	float zoom, aspect, xscroll, yscroll;
	Tab<int> dropDownListIDs;	//this is a list of all the matids on the nodes

	UBYTE *image;
	int CurrentMap;  //this is the index into unwrap_texmaplist unless grid bitmap in which case it is set to TEXTURECHECKERINDEX -1 so it needs to be checked before accessing the pb array

	//store the UVOffset and the image data for multi-tile material
	struct multiTileElement
	{
		Painter2D::uvOffset				uvInfor;
		UBYTE*							singleImage;

		multiTileElement()
			: uvInfor(0, 0)
			, singleImage(nullptr)
		{}
		
		multiTileElement( int u, int v, UBYTE *image )
			: uvInfor(u, v)
			, singleImage(image)
		{}
	};
	std::vector<multiTileElement> imagesContainer;

	int iw, ih, uvw, move,scale;
	int rendW, rendH;
	int channel;
	static int pixelCornerSnap;
	static int pixelCenterSnap;
	int isBitmap;
	int bitmapWidth, bitmapHeight;
	BOOL useBitmapRes;
	// Pixel Units support
	BOOL displayPixelUnits;
	int scalePixelUnitsU, scalePixelUnitsV;

	int zoomext;
	int lockSelected;
	int mirror;
	int hide;
	int freeze;
	int incSelected;
	int falloff;
	int falloffSpace;

	BOOL showMap;
	BOOL updateCache;
	BOOL showMultiTile;

	static float weldThreshold;
	static BOOL update;
	static int showVerts;
	

	//filter stuff
	int filterSelectedFaces;
	int matid;
	Tab<int> filterMatID;
	BitArray vertMatIDList;
	inline int GetFilterMatID()
	{
		int id = -1;
		if ((matid >= 0) && matid < filterMatID.Count())
			id = filterMatID[matid];
		return id;
	}
	
	static int mode;
	static int oldMode;
	static int closingMode;//the mode when the uveditor is closing!

	int axis;

	static BOOL viewValid, tileValid;

	Point2 center;
	int centeron;

	BOOL	useCenter;
	Point3  tempCenter;
	Point3  tempVert;
	HWND	tempHwnd;
	int		tempDir;
	float	tempAmount;
	int		tempWhich;

	BOOL	lockAspect;
	float	mapScale;

	//UNFOLD STUFF
	//cluster list is a tab of a cluster of faces
	//it contains a list of indexs, the average normal for the  
	Tab<ClusterClass*> clusterList;

	virtual int GetClusterCount();
	virtual IClusterInternal* GetCluster(int idx);
	virtual void RemoveCluster(int idx);
	virtual void SortClusterList(CompareFnc fnc);

	int		normalMethod;
	float	normalSpacing;
	BOOL	normalNormalize;
	BOOL	normalRotate;
	BOOL	normalAlignWidth;

	BOOL	unfoldNormalize;
	int		unfoldMethod;

	BOOL showVertexClusterList;
	
	Point3 n;
	Tab<int> seedFaces;

	BOOL bStitchAlign;
	BOOL bStitchScale;
	float fStitchBias;

	float	flattenAngleThreshold;
	float	flattenSpacing;
	BOOL	flattenNormalize;
	BOOL	flattenRotate;
	BOOL	flattenCollapse;
	BOOL	flattenByMaterialID;

	//Packing vars are used by UVW editor, should be global
	int packMethod;
	float packSpacing;
	BOOL packNormalize;
	BOOL packRotate;
	BOOL packFillHoles;
	BOOL packRescaleCluster;

	//ABF vars
	static float abfErrorBound;
	static int abfMaxItNum;
	static BOOL showPins;
	static bool useSimplifyModel;

	int freeFormSubMode;
	int scaleCorner;
	int scaleCornerOpposite;
	Point3  freeFormCorners[4];
	Point2  freeFormCornersScreenSpace[4];
	Point3  freeFormEdges[4];
	Point2  freeFormEdgesScreenSpace[4];
	BOOL	freeFromViewValidate;
	eFreeFormMoveAxis freeFormMoveAxisForDisplay;//for display in the editor dialog
	int		freeFormMoveAxisLength;
	eFreeFormMoveAxis freeFormMoveAxisLocked;//If the mouse click, it will work for the mouse movement.

	Point3 selCenter;
	Point3	freeFormPivotOffset;
	Point2	freeFormPivotScreenSpace;
	Box3   freeFormBounds;
	BOOL   inRotation;
	Point3 origSelCenter;
	float currentRotationAngle;

	BOOL	restoreSketchSettings;
	int		restoreSketchSelMode;
	int		restoreSketchType;

	BOOL	restoreSketchInteractiveMode;
	BOOL	restoreSketchDisplayPoints;

	int		restoreSketchCursorSize;

	int		sketchSelMode;
	int		sketchType;
	BOOL	sketchInteractiveMode;
	BOOL	sketchDisplayPoints;
	int		sketchCursorSize;

	static BOOL	floaterWindowActive;
	BOOL	optionsDialogActive;

	int		tWeldHit;
	MeshTopoData*	tWeldHitLD;

	BOOL bringUpPanel;
	static BOOL snapToggle;

	//5.1.05
	BOOL	autoBackground;

	//NEW RELAX
	float	relaxAmount;
	float	relaxStretch;
	int		relaxIteration;
	int		relaxType;  // 0 = relax by face
	// 1 = relax by edge
	// 2 = relax by cenroids (old relax)
	BOOL	relaxBoundary, relaxSaddle;
	//indicate the Relax function is finished or not.
	BOOL	bRelaxFinished;
	
	BOOL minimized;

	PeltData peltData;

	int mapMapMode;

	Point3 axisCenter;
	static PeltPointToPointMode *peltPointToPointMode;
	static TweakMode *tweakMode;

	HWND relaxBySpringHWND;

	int maximizeHeight;
	int maximizeWidth;

	int xWindowOffset,yWindowOffset;

	BOOL checkerWasShowing;

	IParamMap2 *renderUVMap;
	BitmapInfo renderUVbi;
	static int RenderUVCurUTile;
	static int RenderUVCurVTile;

	class SuspendSelSyncGuard : public MaxSDK::Util::Noncopyable
	{
	public:
		SuspendSelSyncGuard(UnwrapMod& mod) : mMod(mod)
		{
			oldValue = mMod.bSuspendSelectionSync;
			mMod.bSuspendSelectionSync = TRUE;
		}
		~SuspendSelSyncGuard()
		{
			mMod.bSuspendSelectionSync = oldValue;
		}
	private:
		UnwrapMod& mMod;
		BOOL oldValue;
	};

	BOOL bSuspendSelectionSync;
	BOOL suppressWarning;

	BOOL bShowFPSinEditor;
	BOOL bMapGizmoTMDirty;
	BOOL bMeshTopoDataContainerDirty; // used to indicate if we need rebuild the meshTopoDataContainer

	//this updates the spline mapping info with out tossing anything if it can
	void	UpdateSplineMappingNode(INode *node);
	//this adds the node and rebuilds the data
	void	SetSplineMappingNode(INode *node);

	//SEE iunwrapMax8.h header file for doc on these
	void		fnSplineMap();

	void fnSplineMap_SelectSpline(int index, BOOL sel);
	BOOL fnSplineMap_IsSplineSelected(int index);

	void fnSplineMap_SelectCrossSection(int index, int crossIndex, BOOL sel);
	BOOL fnSplineMap_IsCrossSectionSelected(int index,int crossIndex);

	void fnSplineMap_ClearSplineSelection();
	void fnSplineMap_ClearCrossSectionSelection();

	int fnSplineMap_NumberOfSplines();
	int fnSplineMap_NumberOfCrossSections(int splineIndex);

	Point3 fnSplineMap_GetCrossSection_Pos(int splineIndex, int crossSectionIndex);
	float fnSplineMap_GetCrossSection_ScaleX(int splineIndex, int crossSectionIndex);
	float fnSplineMap_GetCrossSection_ScaleY(int splineIndex, int crossSectionIndex);
	Quat fnSplineMap_GetCrossSection_Twist(int splineIndex, int crossSectionIndex);

	void fnSplineMap_SetCrossSection_Pos(int splineIndex, int crossSectionIndex, Point3 u);
	void fnSplineMap_SetCrossSection_ScaleX(int splineIndex, int crossSectionIndex, float scaleX);
	void fnSplineMap_SetCrossSection_ScaleY(int splineIndex, int crossSectionIndex, float scaleY);
	void fnSplineMap_SetCrossSection_Twist(int splineIndex, int crossSectionIndex, Quat twist);

	void fnSplineMap_RecomputeCrossSections();

	void fnSplineMap_Fit(BOOL fitAll, float extraScale);
	//Vec must be in worldspace 
	void fnSplineMap_Align(int splineIndex, int crossSectionIndex, Point3 vec);
	void fnSplineMap_AlignSelected(Point3 vec);
	void fnSplineMap_AlignFaceCommandMode();
	void fnSplineMap_AlignSectionCommandMode();

	void fnSplineMap_Copy();
	void fnSplineMap_Paste();
	void fnSplineMap_PasteToSelected(int splineIndex, int crossSectionIndex);

	void fnSplineMap_DeleteSelectedCrossSections();

	BOOL fnSplineMap_HitTestSpline(GraphicsWindow *gw, HitRegion *hr, int &splineIndex, float &u);
	BOOL fnSplineMap_HitTestCrossSection(GraphicsWindow *gw, HitRegion hr,  SplineMapProjectionTypes projType,  Tab<int> &hitSplines, Tab<int> &hitCrossSections);
	void fnSplineMap_AddCrossSectionCommandMode();
	void fnSplineMap_InsertCrossSection(int splineIndex, float u);

	void fnSplineMap_Cancel();
	void fnSplineMap_StartMapMode();
	void fnSplineMap_EndMapMode();

	//gets sets the current mode for UI purposes only 
	int fnSplineMap_GetMode();
	void fnSplineMap_SetMode(int mode);

	//returns the HWND of the spline dialog
	HWND fnSplineMap_GetHWND();

	//this is called to update the Spline Map UI after something has changed
	void fnSplineMap_UpdateUI();

	void fnSplineMap_MoveSelectedCrossSections(Point3 v);//float u);
	void fnSplineMap_RotateSelectedCrossSections(Quat twist);
	void fnSplineMap_ScaleSelectedCrossSections(Point2 scale);

	void fnSplineMap_Dump();

	void fnSplineMap_Resample(int samples);

	void fnSplineMap_SectActiveSpline(int index);

	//these are methods to hold and restore the mapping before the spline map was applied
	//this holds the mapping
	void	HoldCancelBuffer();
	//this will restore the mapping ie should be called if the dialog is canceled
	void	RestoreCancelBuffer();
	//this just frees our cancel buffer
	void	FreeCancelBuffer();
	//if animating this will create controllers for all the selected vertices
	//doing a set vertex will also do this but you sometimes want to do it seperately
	//so the controller
	void  PlugControllers();

	//this function starts, stops or restarts the relax thread depending on the 
	//operation value you send it
	//hWnd is the window handle to the relax dialog
	std::atomic<bool> mInRestart;    //we are in the restart part of relax
	std::atomic<bool> mInRelax;		//we are in the relax thread
	

	enum	{kThreadNoOp, KThreadStart,KThreadReStart,KThreadEnd };
	void	RelaxThreadOp(int operation, HWND hWnd);
	void	PeltThreadOp(int operation, HWND hWnd);
	void	PeltRelaxThreadOp(int operation, HWND hWnd);

	//This just returns true if any thing in the current subobject is selected
	bool	AnyThingSelected();

//this is used to decided if the majority of the surface area of the selected faces are pointing up or to the side.
	BOOL	GetIsMeshUp();

	BOOL	fnGetTweakMode();
	void	fnSetTweakMode(BOOL mode);

	void	fnUVLoop(int mode);
	void	fnUVRing(int mode);

	void fnRegularMapStart(INode *node, BOOL bringUpUI);	
	void fnRegularMapReset();	
	void fnRegularMapAdvanceUV(BOOL uPosDir, BOOL vPosDir, BOOL uNegDir, BOOL vNegDir, BOOL singleStep);	
	void fnRegularMapAdvanceSelected(BOOL singleStep);	
	void fnRegularMapEnd();	
	BOOL fnRegularMapGetNormalize();
	void fnRegularMapSetNormalize(BOOL normalize);

	BOOL fnRegularMapGetAutoWeld();
	void fnRegularMapSetAutoWeld(BOOL autoWeld);

	float fnRegularMapGetAutoWeldThreshold();
	void fnRegularMapSetAutoWeldThreshold(float autoWeldThreshold);

	float fnRegularMapGetIconSize();
	void fnRegularMapSetIconSize(float iconSize);

	BOOL fnRegularMapGetSingleStep();
	void fnRegularMapSetSingleStep(BOOL singleStep);

	void fnRegularMapSetHWND(HWND hwnd);
	HWND fnRegularMapGetHWND();

	int fnRegularMapGetLimit();
	void fnRegularMapSetLimit(int limit);

	void fnRegularMapUpdateUI();

	MeshTopoData* fnRegularMapGetLocalData();

	BOOL fnRegularMapGetPickStartFace();
	void fnRegularMapSetPickStartFace(BOOL start);

	int fnRegularMapGetAutoFit();
	void fnRegularMapSetAutoFit(int autofit);

	void fnRegularMapFitView();

	void fnRegularMapResetFaces();

	void fnRegularMapStartNewCluster(INode *node, int index);

	void fnRegularMapExpand(int expandBy);

	void fnRegularMapFromEdge();

	BOOL fnIsPinned(int index, INode *node);
	void fnPin(int index, INode *node);
	void fnUnpin(int index, INode *node);

	void fnPinSelected(INode *node);
	void fnUnpinSelected(INode *node);

	void fnLSCMInteractive(BOOL start);
	void fnLSCMSolve();
	void fnLSCMReset();

	void fnLSCMInvalidateTopo(MeshTopoData *md);

	//Forces an interactive resolve if in interactive mode
	void LSCMForceResolve();

	void fnRescaleCluster(BitArray *sel, INode *node);
	void RescaleSelectedCluster(BOOL bOnlySelected = TRUE);

	void fnShowToolBar(BOOL visible);

	//this returns all the group IDs in the face selection
	void GetFaceSelectionClusterIDs(BitArray &ids);
	//creates a new group from the current face selection and names it name
	void fnGroupCreate(const MCHAR *name);
	//deletes the group with the name name
	void fnGroupDelete(const MCHAR *name);
	//renames a group
	void fnGroupRename(const MCHAR *name,const MCHAR *newName);
	//selects the group matching name
	void fnGroupSelect(const MCHAR *name);

	//creates the group that are part of the current face slection
	void fnGroupCreateBySelection();
	//deletes the groups that are part of the current face slection
	void fnGroupDeleteBySelection();
	//selects the group 
	void fnGroupSelectBySelection();
	//sets the texel density for the current face selection
	void fnGroupSetTexelDensity(float val);
	//gets the texel density for the current face selection
	float fnGroupGetTexelDensity();

	void    SetupToolBarUIs();
	void    TearDownToolBarUIs();

	void	fnWeldAllShared();
	void	fnWeldSelectedShared();

	void	fnRelaxOneClick();
	void	fnRelaxThreaded(int threadOp);

	void	UpdateToolBars();

	//adds a tools bar to the pblock and creates it
	//   name is the name of the toolbar (must be unique)
	//   pos which corner to dock to 0 upper left, 1 upper right, 2 lower left, 3 lower right, 4 floating
	//   width of the toolbar
	void   fnAddToolBar(int owner, const MCHAR* name, int pos, int x, int y,  int width, BOOL popup);
	// appends a custom tool bar to our list. use bRescale to indicate if we need handle HDPI ourselves
	void	AppendCustomToolBar(int owner, const MCHAR* name, int pos, int x, int y, int width, BOOL popup, bool bRescale);

	//Get/Set whehter the selection preview is on or off for vertex/edge/face selection in UV/Geometry
	bool fnGetSelectionPreview() const;
	void fnSetSelectionPreview(bool bPreview);

	UnwrapCustomUI*   GetUIManager();
	int	AddActionToToolbar(ToolBarFrame *toolBar, int id, bool bRequireDPIRescale = false);

	void SetupPixelUnitSpinner(int id); // reinitialize controls, as when displayPixelUnits changes
	void SetupPixelUnitSpinners(); // reinitialize controls, as when displayPixelUnits changes

	// Pixel Units display
	bool GetDisplayPixelUnits() const;
	void SetDisplayPixelUnits( bool b, bool update=false );
	int GetScalePixelUnits( int axis );			// axis 1 is U, 2 is V
	void SetScalePixelUnits( int s, int axis );	// axis 1 is U, 2 is V
	// true if the spinner supports Pixel Units
	bool IsSpinnerPixelUnits(int spinnerID, int* axis=NULL); // axis 1 is U, 2 is V

	void	fnStraighten();

	void fnFlattenBySmoothingGroup(BOOL rotate, BOOL rescale, float padding) ;
	void fnFlattenByMaterialID(BOOL rotate, BOOL rescale, float padding) ;

	bool mPackTempRescale,mPackTempRotate;
	float mPackTempPadding;

	//if sPeelDetach is true, uvw editor will firstly execute break operation for Quick Peel / Peel Mode operations. Vice versa.
	static bool sPeelDetach;
	static bool sAutoPack;

	//if link the editU and editV when typing in.
	bool mTypeInLinkUV;

	void fnAlignByPivotVertical();
	void fnAlignByPivotHorizontal();
	void AlignByPivot(BOOL vertical);

	//handles the mouse wheel zoom and pan
	// xPos, yPos are the mouse pos for the pan
	//delta is the mouse wheel scroll delta for the zoom
	void WheelZoom(int xPos, int yPos, int delta);

	//One box to record the position and scale of the selections before some operations,for example Peel Reset.
	Box3	mBBoxOfRecord;
	//One bool value to indicate whether the position and scale value stored in one bounding box have been get.
	bool	mbValidBBoxOfRecord;
	//In order to restore the position and scale after Peel Reset,the position and scale value of the selections should be recorded
	//in one bounding box before the Peel Reset operation.
	bool	BuildBBoxOfRecord();
	//After the Peel Reset,the position and scale of cluster should be restored according to the recorded value.
	void	RestoreToBBoxOfRecord();

	// mirror the selection by the input axis.
	virtual BOOL fnGetMirrorSelectionStatus();
	virtual void fnSetMirrorSelectionStatus(BOOL);

	virtual int fnGetMirrorAxis();
	virtual void fnSetMirrorAxis(int);

	virtual float fnGetMirrorThreshold();
	virtual void fnSetMirrorThreshold(float);

	DistortionOptions GetDistortionType(){return mDistortionOption;}
	void SetDistortionType(DistortionOptions eType);

	void MouseGuidedEdgeLoopSelect(HitRecord *hit, int flags);
	void MouseGuidedEdgeRingSelect(HitRecord *hit, int flags);

	virtual void fnSetPaintMoveBrush(bool);
	virtual bool fnGetPaintMoveBrush() const;

	virtual void fnSetRelaxMoveBrush(bool);
	virtual bool fnGetRelaxMoveBrush() const;

	virtual void fnSetPaintFullStrengthSize(float);
	virtual float fnGetPaintFullStrengthSize() const;

	virtual void fnSetPaintFallOffSize(float);
	virtual float fnGetPaintFallOffSize() const;

	virtual void fnSetPaintFallOffType(int);
	virtual int fnGetPaintFallOffType() const;

	//recompute pivot point
	void RecomputePivotOffset();
	//Update the tiling of the regular check and texture checker.
	void UpdateCheckerTiling();
	//Construct the boxes for detecting the moving direction in the free form. 
	void ConstructFreeFormBoxesForMovingDirection(Box3& freeFormBox,Box3& boundXForMove,Box3& boundYForMove,Box3& boundBothForXYMove);

	//if link the editU and editV when use the type-in edit box to scale selection in the 2d editor
	virtual void fnSetTypeInLinkUV(bool newVal);
	virtual bool fnGetTypeInLinkUV() const;

	virtual HWND fnGetEditorHWND() const;
	virtual HWND fnGetEditorViewHWND() const;

	virtual BOOL fnGetTileGridVisible() const;
	virtual void fnSetTileGridVisible( BOOL bVisible );

	virtual void fnSetDisplayPixelUnits(BOOL bDisplayPixelUnits);
	virtual BOOL fnGetDisplayPixelUnits() const;

	// from IMeshTopoDataChangeListener
	virtual int OnTVDataChanged(IMeshTopoData* pTopoData, BOOL bUpdateView);
	virtual int OnTVertFaceChanged(IMeshTopoData* pTopoData, BOOL bUpdateView);
	virtual int OnSelectionChanged(IMeshTopoData* pTopoData, BOOL bUpdateView);
	virtual int OnPinAddedOrDeleted(IMeshTopoData* pTopoData, int index);
	virtual int OnPinInvalidated(IMeshTopoData* pTopoData, int index);
	virtual int OnTopoInvalidated(IMeshTopoData* pTopoData);
	virtual int OnTVVertChanged(IMeshTopoData* pTopoData, TimeValue t, int index, const Point3& p);
	virtual int OnTVVertDeleted(IMeshTopoData* pTopoData, int index);
	virtual int OnMeshTopoDataDeleted(IMeshTopoData* pTopoData);
	virtual int OnNotifyUpdateTexmap(IMeshTopoData* pTopoData);
	virtual int OnNotifyUIInvalidation(IMeshTopoData* pTopoData, BOOL bRedrawAtOnce);
	virtual int OnNotifyFaceSelectionChanged(IMeshTopoData* pTopoData);

	virtual BOOL fnGetNonSquareApplyBitmapRatio() const;
	virtual void fnSetNonSquareApplyBitmapRatio(BOOL bApplyRatio);

protected:
	void StitchMeshTopoDataVerts(MeshTopoData* pTopoData, BOOL bAlign, float fBias, BOOL bScale);
	BOOL BuildMeshTopoDataCluster(MeshTopoData* pTopoData, const Tab<Point3>& normalList, float threshold, BOOL connected, BOOL cleanUpStrayFaces, MeshTopoData::GroupBy groupBy, Tab<ClusterClass*> &clusterList);
	void ExpandSelectionToSeams();
	void ClearMeshTopoDataContainer();
	bool CollectInStackMeshTopoData(INode* pNode, Object* pObj);

private:
	void SelectFaces(BitArray* sel, INode* node, bool bUpdate);
	//Our UI Manager which manages all the tools bars in the dialogs and mod panel
	UnwrapCustomUI   mUIManager;
	//Where the ini file data for all the floaters and toolbars
	static TSTR mToolBarIniFileName;

	void	SetupTransformToolBar(ToolBarFrame *toolBarFrame);
	void	SetupOptionToolBar(ToolBarFrame *toolBarFrame);
	void	SetupTypeInToolBar(ToolBarFrame *toolBarFrame);
	void	SetupViewToolBar(ToolBarFrame *toolBarFrame);
	void	SetupSelectToolBar(ToolBarFrame *toolBarFrame);
	void	SetupSoftSelectToolBar(ToolBarFrame *toolBarFrame);
	void    SetupDefaultWindows();
	void    FillOutToolBars();

	int	AddDefaultActionToBar(ICustToolbar *toolBar, int id);
	void	LoadDefaultSettingsIfNotYet();

	SideBarUI		mSideBarUI; 
	ModifierPanelUI	mModifierPanelUI;

	BOOL MapModesThatCanSelect();  //this just returns is the current map modes can handle selections while in the mode

	void UpdateViewAndModifier();  //this updates the modifier validity and redraws the edit dialog

	void RotateAroundPivot(float angle);

	//this our cancel buffer info to restore the UVWs on cancel
	Tab<TVertAndTFaceRestore*> mCancelBuffer;

	//the handle to our relax thread
	HANDLE mRelaxThreadHandle;
	HANDLE mPeltThreadHandle;
	HANDLE mPeltRelaxThreadHandle;

	//this is a structure that holds all our info we are passing to the relax thread
	RelaxThreadData mRelaxThreadData;

	//this is a structure that holds all our info we are passing to the pelt thread
	PeltThreadData mPeltThreadData;

	//spline mapping data container
	SplineData mSplineMap;

	MeshTopoData *GetModData();
	BOOL IsInStack(INode *node);
	void ScaleAroundAxis(HWND h, float scaleX, float scaleY, Point3 axis);
	void RotateAroundAxis(HWND h, float ang, Point3 axis);
	void BuildEdgeDistortionData();
	BOOL GetPeltMapMode();
	void SetPeltMapMode(BOOL mode);

	void ApplyGizmoPrivate(Matrix3 *defaultTM = NULL);

	void DrawGizmo(TimeValue t, INode* inode, GraphicsWindow *gw);

	BOOL BXPLine(long x1,long y1,long x2,long y2,
		int width, int height,int id,									   
		Tab<int> &map, BOOL clearEnds = TRUE);

	void BXPLine2(long x1,long y1,long x2,long y2,
		WORD r, WORD g, WORD b, WORD alpha,
		Bitmap *map, BOOL wrap);

	void BXPLineFloat(long x1,long y1,long x2,long y2,
		WORD r, WORD g, WORD b, WORD alpha,
		Bitmap *map);

	BOOL BXPTriangleCheckOverlap(
		long x1,long y1,
		long x2,long y2,
		long x3,long y3,
		Bitmap *map,
		BitArray &processedPixels);

	void BXPTriangleFloat(
		long x1,long y1,
		long x2,long y2,
		long x3,long y3,
		WORD r1, WORD g1, WORD b1, WORD alpha1,
		Bitmap *map
		);

	void UnwrapMod::BXPTriangleFloat( 
		BXPInterpData c1, BXPInterpData c2, BXPInterpData c3,
		WORD alpha,
		Bitmap *map, 
		Tab<Point3> &norms,
		Tab<Point3> &pos
		
		);

	float AngleFromVectors(Point3 vec1, Point3 vec2);

	//this makes sure we set up our selections right when we have a face selection passed into the modifier
	//in this case we dont want to allow selection outside the incoming face selection and this makes
	//sure that this occurs
	void CleanUpGeoSelection(MeshTopoData *ld);

	BOOL flattenMax5;

	BOOL absoluteTypeIn;

	BOOL modifierInstanced;

	BOOL applyToWholeObject;

	BOOL loadDefaults;

	BOOL enableSoftSelection;

	BOOL preventFlattening;

	//new		
	float gridSize;
	BOOL  gridVisible;
	BOOL  tileGridVisible;
	BOOL autoMap;

	//these 2 indices are used for snapping
	//this is the vertex and index into the local data list that mouse was over when the
	//user clicked.  This is what we are snapping from
	MeshTopoData *mMouseHitLocalData;
	int mMouseHitVert;

	Point2 mouseHitPos;
	int tickSize;
	static float snapStrength;

	int paintSize;

	// for paint move brush; should this be per modifier settings or a global setting?
	static float sPaintFullStrengthSize; // in uv space
	static float sPaintFallOffSize;// in uv space
	static int sPaintFallOffType; // 0 linear, 1 sinual, 2 fast, 3 slow

	BOOL blendTileToBackGround;
	BOOL filterMap;
	BOOL brightCenterTile;

	BOOL syncSelection;

	int viewSize;
	int filterSize;
	int optionSize;
	int toolSize;

	BOOL showShared;

	BOOL allowSelectionInsideGizmo;

	BOOL polyMode;

	BOOL resetPivotOnSel;
	int hitSize;

	BOOL displayHiddenEdges;

	BOOL openEdgeMode;

	BOOL uvEdgeMode;

	BOOL displayOpenEdges;
	BOOL thickOpenEdges;
	BOOL viewportOpenEdges;

	TimeValue currentTime;

	int fillMode;

	int mTVSubObjectMode;

	//This is a bool that if enabled will pop up the edit dialog when ever the rollup ui is put up
	BOOL alwaysEdit;

	BOOL tvElementMode;

	static BOOL executedStartUIScript;
	//old selection method, where if you single clicked it was culled, dragged it was not
	BOOL oldSelMethod;
	//backface cull
	BOOL ignoreBackFaceCull;
	//planar threshold for select
	BOOL planarMode;
	float planarThreshold;
	//elem mode stuff
	BOOL geomElemMode;

	//edge limit data
	BOOL limitSoftSel;
	int limitSoftSelRange;

	//tile data
	BOOL bTile;
	float fContrast;
	int iTileLimit;
	float fCheckerTiling;

	//stitch parameters
	float gBArea;		//surface area of all the texture faces
	float gSArea;		//the area of the bounding box surrounding the faces
	virtual float GetSArea() { return gSArea; }
	virtual float GetBArea() { return gBArea; }
	virtual void SetSArea(float f) { gSArea = f; }
	virtual void SetBArea(float f) { gBArea = f; }

	float gEdgeHeight, gEdgeWidth;  
	//			float gEdgeLenH,gEdgeLenW;
	int gDebugLevel;	//debug level	0 means no debug info will be displayed
	//				1 means that a minimal amount mostly to the listener window
	//				2 means that a fair amount to the listener window and some display debug info
	//				3 means that a alot of info to the listener window and the display 

	static CopyPasteBuffer copyPasteBuffer;

	int subObjCount;
	//5.1.02 adds new bitmap bg management
	enum { multi_params, };  		// pblock ID

	enum							// multi_params param IDs
	{	multi_mtls,
	multi_ons,
	multi_names,
	multi_ids, };

	BOOL popUpDialog;

	BOOL rotationsRespectAspect;

	BOOL showCounter;

	BOOL relaxBySpringUseOnlyVEdges;
	int relaxBySpringIteration;
	float relaxBySpringStretch;

	float edgeDistortionScale;

	BOOL lockSpringEdges;

	float edgeScale;
	BOOL showEdgeDistortion;
	
	BOOL normalizeMap;
	BOOL alwaysShowSeams;
	Matrix3 mGizmoTM;

	static bool sSelectionPreview; // vertex/edge/face for UV/geometry preview on or off
	//this is a list of all controllers across all the local data
	//we store it in the modifier so the local data can look it up and 
	//dont think anyone has every made a local mod data a referencable
	UVW_VertexControllerClass mUVWControls;

	//this is a list of all the local data assigned to the modifier
	//this is created on each eval which might be overkill but should be safer	
	MeshTopoDataContainer  mMeshTopoData;

	//this is just a structure to load our max9 data into since we moved it to the local mod
	//data now.  This data goes away after the first evaulation 
	UVW_ChannelClass mTVMaps_Max9;	

	//denote if the non-square texture will apply the bitmap ratio in the action of relax,peel and pack.
	BOOL mNonSquareApplyBitmapRatio;

//this just disables all our UI
	void DisableUI();
	void InvalidateMirrorDataForEachMeshTopoData();
	void BuildMirrorDataForEachMeshTopoData();
	void SelectMirrorGeomVerts();

	UnwrapNodeDisplayCallback mUnwrapNodeDisplayCallback;

	//this builds a list of the left and right vertices from an edge list
	void fnSplineMap_GetSplitEdges(MeshTopoData *ld, BitArray geomEdgeSel, BitArray &left, BitArray &right);

	RegularMap mRegularMap;
	ToolLSCM	mLSCMTool;

	ToolGrouping mToolGrouping;

	Tab<Point3> mColors;

	std::map<INode*,MaxSDK::Graphics::ICustomRenderItemPtr> mRenderItemsContainer;

	void UpdateGroupUI();

	void UpdatePivot();

	int GetCurrentMatID(); // return -1 means the current selection has more than 1 material ids assigned.
	void UpdateMatIDUI();
	//Indicate the distortion type: based on the angle or area
	DistortionOptions		mDistortionOption;
	void DisplayOpenEdges(GraphicsWindow * gw, Material openEdgeMaterial, Color oEdgeColor, BOOL boxMode, MeshTopoData * ld, float size);

	template <typename SubHitList, typename ObjectType, typename SubHitRec>
	void _BuildVisibleFaces(MeshTopoData * ld, GraphicsWindow * gw, BitArray &visibleFaces, ObjectType &object, DWORD hitFlags);

	void ClearImagesContainer();
	//Compute the area radio of every node to the first one's geometry area when multi nodes exist.
	void ComputeGeometryAreaRatio(std::vector<float>& vecRadios);

	enum IterateMethod
	{
		IterateMethod_Loop,
		IterateMethod_Ring,
	};
	void _MouseGuidedEdgeSelect(HitRecord *hit, int flags, IterateMethod iter);

	//Different with the regular checker, the texture checker can modify the texture file.
	Mtl* CreateTextureCheckerMtl();
	Mtl* GetTextureCheckerMtl();
	void ShowTextureCheckerMtl();

	inline BOOL IsPixelSnapOn() { return (isBitmap) && fnGetSnapToggle() && (fnGetPixelCenterSnape() ||fnGetPixelCornerSnap());}
};

// action table
// Keyboard Shortcuts stuff
const ActionTableId kUnwrapActions = 0x7bd55e42;
const ActionContextId kUnwrapContext = 0x7bd55e43;
const int   kUnwrapMenuBar = 2292144;

class UnwrapActionCallback : public ActionCallback
{
public:
	BOOL ExecuteAction(int id)
	{
		if (pUnwrap)
			return pUnwrap->WtExecute(id,FALSE);
		return FALSE;
	}
	void SetUnwrap(UnwrapMod *pUnwrap) {this->pUnwrap = pUnwrap;}
private:
	UnwrapMod *pUnwrap;
};

class UnwrapAction : public ActionItem
{
public:

	//ActionItem methods
	virtual BOOL IsChecked()						{if (pUnwrap)
		return pUnwrap->WtIsChecked(id);
	else return FALSE;}
	virtual void GetMenuText(TSTR& menuText)		{menuText.printf(_T("%s"),this->menuText);}
	virtual void GetButtonText(TSTR& buttonText)	{buttonText.printf(_T("%s"),this->buttonText);	}
	virtual void GetCategoryText(TSTR& catText);//		{catText.printf(_T("%s"),GetString(IDS_CAT_ALL_COMMANDS));}
	virtual void GetDescriptionText(TSTR& descText) {descText.printf(_T("%s"),this->descText);}
	virtual BOOL ExecuteAction()					{if (pUnwrap)
		return pUnwrap->WtExecute(id,FALSE);
	else return FALSE;
	}
	virtual int GetId()								{
		return id;
	}
	BOOL IsItemVisible()					{if (pUnwrap)
		return TRUE;
	else return FALSE;}
	virtual BOOL IsEnabled()					{if (pUnwrap)
		return pUnwrap->WtIsEnabled(id);
	else return FALSE;}
	virtual void DeleteThis() {delete this;}

	//WeightTableAction methods
	void Init(int id,const TCHAR *mText, const TCHAR *bText, const TCHAR *cText, const TCHAR *dText)
	{
		pUnwrap = NULL;
		this->id = id;
		menuText.printf(_T("%s"),mText);	
		buttonText.printf(_T("%s"),bText);	
		descText.printf(_T("%s"),dText);	
		catText.printf(_T("%s"),cText);	
	}
	void SetUnwrap(UnwrapMod *pUnwrap) {this->pUnwrap = pUnwrap;}
	void SetID(int id) {this->id = id;}
	void SetNames(const TCHAR *mText, const TCHAR *bText, const TCHAR *cText, const TCHAR *dText )
	{
		menuText.printf(_T("%s"),mText);	
		buttonText.printf(_T("%s"),bText);	
		descText.printf(_T("%s"),dText);	
		catText.printf(_T("%s"),cText);	
	}

private:
	int id;
	UnwrapMod *pUnwrap;
	TSTR buttonText, menuText, descText, catText;
};

class UnwrapClassDesc:public ClassDesc2 {
public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading = FALSE); 
	const TCHAR *	ClassName();// {return UNWRAP_NAME;}
	SClass_ID		SuperClassID() {return OSM_CLASS_ID;}
	Class_ID		ClassID() {return UNWRAP_CLASSID;}
	const TCHAR* 	Category();// {return GetString(IDS_RB_DEFSURFACE);}

	const TCHAR*	InternalName() { return _T("UVWUnwrap"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
	//You only need to add the action stuff to one Class Desc
	int             NumActionTables() { return 1; }
	ActionTable*  GetActionTable(int i) { return GetActions(); }

	ActionTable* GetActions();
};

class AutoPackDisabledGuard : public MaxSDK::Util::Noncopyable
{
public:
	AutoPackDisabledGuard(UnwrapMod* pMod);
	~AutoPackDisabledGuard();
private:
	UnwrapMod* mpMod;
	bool mTempAutoPackEnabled;
};

/*****************************************************************
These next 2 classes are used by the overlap face selection action
*****************************************************************/
//this is just a single element/pixel of our map that we render into
class OverlapCell
{
public:
	MeshTopoData *mLD;	//the local data for this pixel
	int mFaceID;		//the face that rendered into this pixel
};

//this is a map that we use to render our faces into to see which ones overlap
//instead of keeping track of color in each pixel we keep track of face index and local data pointer
class OverlapMap
{
public:
	//intitializes our buffer and clears it out
	void Init()
	{
		mBufferSize = 2000;
		mBuffer = new OverlapCell[mBufferSize*mBufferSize];
		//clear the buffer
		for (int i = 0; i < mBufferSize*mBufferSize; i++)
		{
			mBuffer[i].mLD = NULL;
			mBuffer[i].mFaceID = -1;
		}
	}
	~OverlapMap()
	{
		delete [] mBuffer;
	}

	//this renders a face into our buffer and selects the faces that intersect if there are any
	//	ld - the local data the owns the face we are about to render
	//	faceIndex - the index of the face we want to render/check
	//  pa,pb,pc - this is the normalized UVW coords of the face that we want to check
	void Map(MeshTopoData *ld, int faceIndex, Point3 pa, Point3 pb, Point3 pc);

private:
	//this see if this pixel intersects and faces in the buffer
	//if so it select the face in the buffer and the incoming face
	//	mapIndex is the index into the buffer
	//	ld is the local data of the face we are checking
	//	faceIndex is the index of the face we are cheking
	void Hit(int mapIndex, MeshTopoData *ld, int faceIndex);

	OverlapCell *mBuffer;	//our map buffer
	int mBufferSize;		//the width/height of the buffer

};

// save/load data chunk ID
#define VERTCOUNT_CHUNK	0x0100
#define VERTS_CHUNK		0x0110
#define VERTSEL_CHUNK	0x0120
#define ZOOM_CHUNK		0x0130
#define ASPECT_CHUNK	0x0140
#define XSCROLL_CHUNK	0x0150
#define YSCROLL_CHUNK	0x0160
#define IWIDTH_CHUNK	0x0170
#define IHEIGHT_CHUNK	0x0180
#define SHOWMAP_CHUNK	0x0190
#define UPDATE_CHUNK	0x0200
#define LINECOLOR_CHUNK	0x0210
#define SELCOLOR_CHUNK	0x0220
#define FACECOUNT_CHUNK	0x0230
#define FACE_CHUNK		0x0240
#define UVW_CHUNK		0x0250
#define CHANNEL_CHUNK	0x0260
#define VERTS2_CHUNK	0x0270
#define FACE2_CHUNK		0x0280
#define PREFS_CHUNK		0x0290
#define USEBITMAPRES_CHUNK		0x0300

#define GEOMPOINTSCOUNT_CHUNK		0x320
#define GEOMPOINTS_CHUNK		0x330
#define LOCKASPECT_CHUNK		0x340
#define MAPSCALE_CHUNK		0x350
#define WINDOWPOS_CHUNK     0x360
#define FORCEUPDATE_CHUNK     0x370

#define TILE_CHUNK			0x380
#define TILECONTRAST_CHUNK  0x390
#define TILELIMIT_CHUNK     0x400

#define SOFTSELLIMIT_CHUNK     0x410

#define FLATTENMAP_CHUNK    0x420
#define NORMALMAP_CHUNK     0x430
#define UNFOLDMAP_CHUNK     0x440
#define STITCH_CHUNK		0x450

#define GEOMELEM_CHUNK		0x460
#define PLANARTHRESHOLD_CHUNK		0x470
#define BACKFACECULL_CHUNK		0x480
#define TVELEMENTMODE_CHUNK		0x490
#define ALWAYSEDIT_CHUNK		0x500
#define SHOWCONNECTION_CHUNK		0x510
#define PACK_CHUNK		0x520
#define TVSUBOBJECTMODE_CHUNK		0x530
#define FILLMODE_CHUNK		0x540
#define OPENEDGECOLOR_CHUNK	0x550
#define UVEDGEMODE_CHUNK	0x560
#define MISCCOLOR_CHUNK	0x570
#define HITSIZE_CHUNK	0x580
#define PIVOT_CHUNK	0x590
#define GIZMOSEL_CHUNK	0x600
#define SHARED_CHUNK	0x610
#define SHOWICON_CHUNK	0x620
#define SYNCSELECTION_CHUNK	0x630
#define BRIGHTCENTER_CHUNK	0x640

#define CURSORSIZE_CHUNK	0x650
#define TICKSIZE_CHUNK		0x660
//new
#define GRID_CHUNK		0x670

#define PREVENTFLATTENING_CHUNK		0x680

#define ENABLESOFTSELECTION_CHUNK		0x690
#define CONSTANTUPDATE_CHUNK			0x700
#define APPLYTOWHOLEOBJECT_CHUNK			0x710

//5.1.05
#define AUTOBACKGROUND_CHUNK			0x720

#define THICKOPENEDGE_CHUNK			0x730
#define VIEWPORTOPENEDGE_CHUNK			0x740

#define ABSOLUTETYPEIN_CHUNK			0x750

#define STITCHSCALE_CHUNK			0x760
#define SEAM_CHUNK			0x770
#define VERSION_CHUNK			0x780
#define CURRENTMAP_CHUNK			0x790
#define GEDGESELECTION_CHUNK			0x800
#define UEDGESELECTION_CHUNK			0x810
#define FACESELECTION_CHUNK			0x820
#define RELAX_CHUNK			0x830
#define FALLOFFSPACE_CHUNK			0x840
#define SHOWPELTSEAMS_CHUNK			0x850
#define VERTS3_CHUNK				0x0860
#define FACE5_CHUNK					0x0870
#define GVERTSEL_CHUNK					0x0880

#define SPLINEMAP_CHUNK					0x0890  //early alpha support should only show up in QE and early alpha testing files
#define SPLINEMAP_V2_CHUNK				0x0891  //final version chunk
#define SPLINEMAP_MAPGROUPCOUNT_CHUNK	0x0900
#define SPLINEMAP_MAPGROUPDATA_CHUNK	0x0910
#define CONTROLLER_COUNT_CHUNK			0x0920
#define GROUPING_CHUNK					0x930
#define PACKTEMP_CHUNK					0x940
#define PEELCOLOR_CHUNK					0x950

#define MIRROR_GEOM_SEL_AXIS			0x1000 //store the mirror geometry selection axis.
#define PEEL_MODE_SETTING_CHUNK					0x1010 //store auto-pack/live-peel/edge-to-seam status

#define SHOWMULTITILE_CHUNK				0x1020
#define CHECKERTILING_CHUNK				0x1030

#define TILEGRIDVISIBLE_CHUNK				0x1040
#define DISPLAYPIXELUNITS_CHUNK			0x1050

#define FILTERMAP_CHUNK                      0x1060

