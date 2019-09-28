/*==============================================================================
Copyright 2009 Autodesk, Inc.  All rights reserved. 

This computer source code and related instructions and comments are the unpublished
confidential and proprietary information of Autodesk, Inc. and are protected under 
applicable copyright and trade secret law.  They may not be disclosed to, copied
or used by any third party without the prior written consent of Autodesk, Inc.

//**************************************************************************/

#include "IGrip.h"

#ifndef __EDIT_POLY_GRIPS_H
#define __EDIT_POLY_GRIPS_H

class EditPolyMod;
class PolyOperation;

//icon defines

#define STR_ICON_DIR					_T("Grips\\")
#define BEVEL_TYPE_PNG					_T("BevelType.png")
#define BEVEL_GROUP_PNG					_T("Group.png")
#define BEVEL_NORMAL_PNG				_T("Normal.png")
#define BEVEL_POLYGON_PNG				_T("Polygon.png")
#define BEVEL_SURFACE_PNG				_T("Surface.png")
#define BEVEL_HEIGHT_PNG				_T("Height.png")
#define BEVEL_OUTLINE_PNG				_T("OutlineAmount.png")
#define BEVEL_BIAS_PNG					_T("Bias.png")
#define EXTRUDE_TYPE_PNG				_T("ExtrudeType.png")
#define EXTRUDE_GROUP_PNG				_T("Group.png")
#define EXTRUDE_NORMAL_PNG				_T("Normal.png")
#define EXTRUDE_POLYGON_PNG				_T("Polygon.png")
#define EXTRUDE_SURFACE_PNG				_T("Surface.png")
#define EXTRUDE_HEIGHT_PNG				_T("Height.png")
#define EXTRUDE_WIDTH_PNG				_T("Width.png")
#define EXTRUDE_BIAS_PNG				_T("Bias.png")
#define OUTLINE_AMOUNT_PNG				_T("OutlineAmount.png")
#define INSET_TYPE_PNG                  _T("InsetType.png") 
#define INSET_AMOUNT_PNG				_T("InsetAmount.png")
#define INSET_GROUP_PNG					_T("Group.png")
#define INSET_POLYGON_PNG				_T("Polygon.png")
#define CONNECT_EDGE_SEGMENTS_PNG  _T("Segments.png")
#define CONNECT_EDGE_PINCH_PNG        _T("SegmentPinch.png")
#define CONNECT_EDGE_SLIDE_PNG         _T("SegmentSlide.png")
#define RELAX_AMOUNT_PNG                  _T("Value.png")
#define RELAX_ITERATIONS_PNG             _T("RelaxIteration.png")
#define RELAX_BOUNDARY_POINTS_PNG             _T("HoldBoundary.png")
#define RELAX_OUTER_POINTS_PNG             _T("HoldOuterPoints.png")
#define CHAMFER_VERTICES_AMOUNT_PNG   _T("Value.png")
#define CHAMFER_VERTICES_OPEN_PNG   _T("Hole.png")
#define CHAMFER_EDGE_AMOUNT_PNG         _T("Value.png")
#define CHAMFER_EDGE_STANDARD_PNG       _T("StandardChamfer.png")
#define CHAMFER_EDGE_QUAD_PNG           _T("QuadChamfer.png")
#define CHAMFER_EDGE_SEGMENT_PNG        _T("Segments.png")
#define CHAMFER_EDGE_TENSION_PNG        _T("Tension.png")
#define CHAMFER_EDGE_OPEN_PNG        _T("Hole.png")
#define CHAMFER_EDGE_INVERT_PNG              _T("Invert.png")
#define CHAMFER_EDGE_SMOOTH_PNG				_T("EdgeChamferSmooth.png")
#define CHAMFER_EDGE_SMOOTH_CHAMFERS_PNG	_T("EdgeChamferSmoothChamfers.png")
#define CHAMFER_EDGE_SMOOTH_ALL_PNG			_T("EdgeChamferSmoothAll.png")
#define CHAMFER_EDGE_SMOOTH_THRESHOLD_PNG	_T("EdgeChamferSmoothThreshold.png")
#define CHAMFER_EDGE_QUAD_INTERSECTIONS_PNG	_T("QuadIntersections.png")
#define BRIDGE_BORDER_TYPE_PNG                _T("ByEdge.png")
#define BRIDGE_POLYGON_TYPE_PNG                _T("ByPolygon.png")
#define BRIDGE_EDGE_TYPE_PNG                _T("ByEdge.png")
#define BRIDGE_SELECTED_PNG          _T("Selected.png")
#define BRIDGE_SPECIFIC_PNG           _T("Specific.png")
#define BRIDGE_SEGMENTS_PNG        _T("Segments.png")
#define BRIDGE_TAPER_PNG              _T("Taper.png")
#define BRIDGE_TWIST1_PNG             _T("Twist1.png")
#define BRIDGE_TWIST2_PNG             _T("Twist2.png")
#define BRIDGE_BIAS_PNG                _T("Bias.png")
#define BRIDGE_SMOOTH_PNG          _T("SmoothMesh.png")
#define BRIDGE_ADJACENT_PNG        _T("BridgeAdjacent.png")
#define BRIDGE_REVERSE_TRI_PNG    _T("ReverseTri.png")
#define BRIDGE_EDGE_PICK1_PNG          _T("BridgeEdgePick1.png")
#define BRIDGE_EDGE_PICK2_PNG           _T("BridgeEdgePick2.png")
#define BRIDGE_POLYGON_PICK1_PNG		_T("BridgePolygonPick1.png")
#define BRIDGE_POLYGON_PICK2_PNG		_T("BridgePolygonPick2.png")
#define BRIDGE_BORDER_PICK1_PNG		_T("BridgeBorderPick1.png")
#define BRIDGE_BORDER_PICK2_PNG		_T("BridgeBorderPick2.png")
#define HINGE_FROM_EDGE_ANGLE_PNG    _T("Angle_16.png")
#define HINGE_FROM_EDGE_SEGMENTS_PNG    _T("Segments.png")
#define HINGE_PICK_PNG		           _T("HingePick.png")
#define EXTRUDE_ALONG_SPLINE_SEGMENTS_PNG   _T("Segments.png")
#define EXTRUDE_ALONG_SPLINE_TAPER_AMOUNT_PNG   _T("Value.png")
#define EXTRUDE_ALONG_SPLINE_TAPER_CURVE_PNG    _T("TaperCurve.png") 
#define EXTRUDE_ALONG_SPLINE_TWIST_PNG              _T("Twist.png")
#define EXTRUDE_ALONG_SPLINE_ROTATION_PNG         _T("Angle_16.png")
#define EXTRUDE_ALONG_SPLINE_TO_FACE_NORMAL_PNG         _T("LocalNormal.png")
#define EXTRUDE_ALONG_SPLINE_PICK_PNG           _T("ExtrudeAlongSplinePick.png")
#define WELD_THRESHOLD_PNG                      _T("Value.png")
#define MSMOOTH_SMOOTHNESS_PNG                  _T("SmoothMesh.png")
#define MSMOOTH_SMOOTHING_GROUPS_PNG                  _T("SmoothGroups.png")
#define MSMOOTH_SMOOTH_MATERIALS_PNG                  _T("SmoothMaterials.png")
#define TESSELLATE_EDGE_PNG                           _T("ByEdge.png")
#define  TESSELLATE_FACE_PNG                          _T("ByPolygon.png")
#define  TESSELLATE_TYPE_PNG                          _T("Group.png")
#define  TESSELLATE_TENSION_PNG                     _T("Value.png")
#define PICK_EMPTY_PNG						_T("PickButtonEmpty.png")
#define PICK_FULL_PNG						_T("PickButtonFull.png")
#define FALLOFF_PNG							_T("Falloff.png")
#define BUBBLE_PNG							_T("Bubble.png")
#define PINCH_PNG							_T("Pinch.png")
#define SETFLOW_PNG							_T("SetFlow.png")
#define LOOPSHIFT_PNG							_T("Loopshift.png")
#define RINGSHIFT_PNG							_T("RingShift.png")
#define EDGECREASE_PNG                        _T("EdgeCrease.png")
#define EDGEWEIGHT_PNG                      _T("EdgeWeight.png")
#define VERTEXCREASE_PNG                        _T("EdgeCrease.png")	// Same image as edge
#define VERTEXWEIGHT_PNG                  _T("VertexWeight.png")

class EPManipulatorGrip : public IBaseGrip2
{
	
public:
	enum OurGrips{eFalloff = 0,  ePinch, eBubble, eSetFlow, eLoopShift,
					eRingShift, eEdgeCrease, eEdgeWeight,  eVertexCrease, eVertexWeight,
					// The following is a semaphore that indicates the total enum count.
					// Please leave as the last entry for proper operation
					EPManipulatorGripCount
};

	EPManipulatorGrip():mpEditPoly(NULL){};
	virtual ~EPManipulatorGrip(){};
    void Init(EditPolyMod* editPoly){mpEditPoly = editPoly;}
	EditPolyMod*  GetEditPolyMod() { return mpEditPoly; }
	void SetEditPolyMod( EditPolyMod* pEdPolyMod ){ mpEditPoly = pEdPolyMod; }
	void SetUpUI(){GetIGripManager()->ResetAllUI();}  //called after the grip is enabled to set up init states that are called via IGripManager
	void SetUpVisibility(BitArray &manipMask);  //should be called after it's enabled we will reset everything
	void SetFalloff(bool val){ mSSFalloff = val; ResetGrip();}
	void SetBubble(bool val){ mSSBubble = val; ResetGrip();}
	void SetPinch(bool val){ mSSPinch = val; ResetGrip();}
	void SetSetFlow(bool val) {mSetFlow = val; ResetGrip();}
	void SetLoophift(bool val){mLoopShift = val; ResetGrip();}
	void SetRingShift(bool val) {mRingShift = val; ResetGrip();}
	void SetEdgeCrease( bool val){ mEdgeCrease = val ; ResetGrip(); }
	void SetEdgeWeight( bool val){ mEdgeWeight = val ; ResetGrip(); }
	void SetVertexCrease( bool val){ mVertexCrease = val ; ResetGrip(); }
	void SetVertexWeight( bool val){ mVertexWeight = val ; ResetGrip(); }
	void UpdateGripVisibility();
	void ResetGrip();
	BaseInterface* GetInterface(Interface_ID id);

	//from IBaseGrip
	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	bool SupportsOkayApplyCancel(){return false;} //from IBaseGrip, we don't support ok,apply, cancel buttons
	bool GetCommandIcon(int which, MSTR &string){return false;}

	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);

	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

private:
	EditPolyMod* mpEditPoly;
	bool mSSFalloff;
	bool mSSBubble;
	bool mSSPinch;
	bool mSetFlow;
	bool mLoopShift;
	bool mRingShift;
	bool mEdgeCrease;
	bool mEdgeWeight;
	bool mVertexCrease;
	bool mVertexWeight;
};

class EditPolyModPolyOperationGrip : public IBaseGrip
{
public:
	EditPolyModPolyOperationGrip();
	virtual ~EditPolyModPolyOperationGrip();
    virtual void Init(EditPolyMod* editPoly);
	EditPolyMod*  GetEditPolyMod();
	virtual void SetUpUI();
	bool SupportsOkayApplyCancel();
protected:
	void CommonOK( TimeValue t );
	void CommonApply( TimeValue t );
	void CommonCancel();
private:
	EditPolyMod* mpEditPoly;
	bool		 mReentryLock; // protect against re-entery into CommonOk/CommonApply/CommonCancel
};

class BevelGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eBevelType = 0, eBevelHeight, eBevelOutline, eBevelBias};

	BevelGrip(){};
	BevelGrip(EditPolyMod *m, PolyOperation *o);
	~BevelGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 

	void SetUpUI();

	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class ExtrudeFaceGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eExtrudeFaceType = 0, eExtrudeFaceHeight, eExtrudeFaceBias};

	ExtrudeFaceGrip(){};
	ExtrudeFaceGrip(EditPolyMod *m, PolyOperation *o);
	~ExtrudeFaceGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 

	void SetUpUI();

	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class ExtrudeEdgeGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eExtrudeEdgeHeight = 0, eExtrudeEdgeWidth};

	ExtrudeEdgeGrip(){};
	ExtrudeEdgeGrip(EditPolyMod *m, PolyOperation *o);
	~ExtrudeEdgeGrip();
	
	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 

	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class ExtrudeVertexGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eExtrudeVertexHeight = 0, eExtrudeVertexWidth};

	ExtrudeVertexGrip(){};
	ExtrudeVertexGrip(EditPolyMod *m, PolyOperation *o);
	~ExtrudeVertexGrip();
	
	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
    bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);

	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class OutlineGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eOutlineAmount = 0};

	OutlineGrip(){};
	OutlineGrip(EditPolyMod *m, PolyOperation *o);
	~OutlineGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);

	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class InsetGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eInsetType = 0, eInsetAmount};

	InsetGrip(){};
	InsetGrip(EditPolyMod *m, PolyOperation *o);
	~InsetGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class ConnectEdgeGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eConnectEdgeSegments = 0, eConnectEdgePinch, eConnectEdgeSlide};

	ConnectEdgeGrip(){};
	ConnectEdgeGrip(EditPolyMod *m, PolyOperation *o);
	~ConnectEdgeGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class RelaxGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eRelaxAmount = 0, eRelaxIterations, eRelaxBoundaryPoints,eRelaxOuterPoints};

	RelaxGrip(){};
	RelaxGrip(EditPolyMod *m, PolyOperation *o);
	~RelaxGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class ChamferVerticesGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eChamferVerticesAmount = 0, eChamferVerticesOpen };

	ChamferVerticesGrip(){};
	ChamferVerticesGrip(EditPolyMod *m, PolyOperation *o);
	~ChamferVerticesGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class ChamferEdgeGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eChamferEdgeType = 0, eChamferEdgeAmount, eChamferEdgeSegments, eChamferEdgeTension, eChamferEdgeOpen, eChamferEdgeInvert, eChamferEdgeQuadIntersections, eChamferEdgeSmooth, eChamferEdgeSmoothType, eChamferEdgeSmoothThreshold};

	ChamferEdgeGrip(){};
	ChamferEdgeGrip(EditPolyMod *m, PolyOperation *o);
	~ChamferEdgeGrip();

	//need to activate grips
	void SetUpUI(); //called to set up init states, also should get called when a paremeter changes to handle undos

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);

	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);
	void MaybeEnableControls();
};

class BridgeEdgeGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{
		eBridgeEdgeSegments = 0, 
		eBridgeEdgeSmooth, 
		eBridgeEdgeAdjacent,
		eBridgeEdgeReverseTri,
		eBridgeEdgeType,
		eBridgeEdgePick1,
		eBridgeEdgePick2
	};

	BridgeEdgeGrip(){};
	BridgeEdgeGrip(EditPolyMod *m, PolyOperation *o);
	~BridgeEdgeGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	//need to activate grips
	void SetUpUI(); //called after the grip is enabled to set up init states that are called via IGripManager

	void SetEdge1PickModeStarted();
	void SetEdge1Picked(int edge);
	void SetEdge1PickDisabled();
	void SetEdge2PickModeStarted();
	void SetEdge2Picked(int edge);
	void SetEdge2PickDisabled();

private:
	MSTR mEdge1Picked;
	MSTR mEdge2Picked;
	MSTR mPick1Icon;
	MSTR mPick2Icon;

};

class BridgeBorderGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{
		eBridgeBorderSegments = 0, 
		eBridgeBorderTaper,
		eBridgeBorderBias,
		eBridgeBorderSmooth, 
		eBridgeBorderTwist1,
		eBridgeBorderTwist2,
		eBridgeBorderType,
		eBridgeBorderPick1,
		eBridgeBorderPick2
	};

	BridgeBorderGrip(){};
	BridgeBorderGrip(EditPolyMod *m, PolyOperation *o);
	~BridgeBorderGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	//need to activate grips
	void SetUpUI(); //called after the grip is enabled to set up init states that are called via IGripManager


	void SetEdge1PickModeStarted();
	void SetEdge1Picked(int edge);
	void SetEdge1PickDisabled();
	void SetEdge2PickModeStarted();
	void SetEdge2Picked(int edge);
	void SetEdge2PickDisabled();

private:
	MSTR mEdge1Picked;
	MSTR mEdge2Picked;
	MSTR mPick1Icon;
	MSTR mPick2Icon;

};

class BridgePolygonGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{
		eBridgePolygonSegments = 0, 
		eBridgePolygonTaper,
		eBridgePolygonBias,
		eBridgePolygonSmooth, 
		eBridgePolygonTwist1,
		eBridgePolygonTwist2,
		eBridgePolygonType,
		eBridgePolygonPick1,
		eBridgePolygonPick2
	};

	BridgePolygonGrip(){};
	BridgePolygonGrip(EditPolyMod *m, PolyOperation *o);
	~BridgePolygonGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	//need to activate grips
	void SetUpUI(); //called after the grip is enabled to set up init states that are called via IGripManager

	void SetPoly1PickModeStarted();
	void SetPoly1Picked(int num);
	void SetPoly1PickDisabled();
	void SetPoly2PickModeStarted();
	void SetPoly2Picked(int num);
	void SetPoly2PickDisabled();

private:
	MSTR mPoly1Picked;
	MSTR mPoly2Picked;
	MSTR mPick1Icon;
	MSTR mPick2Icon;

};

class HingeGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eHingeAngle = 0, eHingeSegments, eHingePick};

	HingeGrip():mEdgePicked(_T("")),mPickIcon(_T("")),i(0){};
	HingeGrip(EditPolyMod *m, PolyOperation *o);
	~HingeGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	void SetUpUI();

	void SetEdgePickModeStarted();
	void SetEdgePicked(int edge);
	void SetEdgePickDisabled();

private:
	MSTR mEdgePicked;
	MSTR mPickIcon;
	ModContextList mList;
	INodeTab mNodes;
	int i;

};

class ExtrudeAlongSplineGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eExtrudeAlongSplineSegments = 0, eExtrudeAlongSplineTaperAmount, eExtrudeAlongSplineTaperCurve,
		eExtrudeAlongSplineTwist, eExtrudeAlongSplineAlignToFaceNormal, eExtrudeAlongSplineRotation,eExtrudeAlongSplinePickSpline};

	ExtrudeAlongSplineGrip(){};
	ExtrudeAlongSplineGrip(EditPolyMod *m, PolyOperation *o);
	~ExtrudeAlongSplineGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	void SetSplinePickModeStarted();
	void SetSplinePicked(INode *node);
	void SetSplinePickDisabled();

	//need to activate grips
	void SetUpUI(); //called after the grip is enabled to set up init states that are called via IGripManager

private:
	MSTR mSplinePicked;
	MSTR mPickIcon;

};

class WeldVerticesGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eWeldVerticesWeldThreshold = 0, eWeldVerticesNum};

	WeldVerticesGrip::WeldVerticesGrip():mStrBefore(_T(" ")),mStrAfter(_T(" ")){}
	WeldVerticesGrip(EditPolyMod *m, PolyOperation *o);
	~WeldVerticesGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	
	void SetNumVerts(int before,int after);
private:
	MSTR mStrBefore;
	MSTR mStrAfter;

};

class WeldEdgesGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eWeldEdgesWeldThreshold = 0, eWeldEdgesNum};

	WeldEdgesGrip::WeldEdgesGrip():mStrBefore(_T("     ")),mStrAfter(_T("     ")){}
	WeldEdgesGrip(EditPolyMod *m, PolyOperation *o);
	~WeldEdgesGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	void SetNumVerts(int before,int after);
private:
	MSTR mStrBefore;
	MSTR mStrAfter;

};

class MSmoothGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eMSmoothSmoothness = 0, eMSmoothSmoothingGroups, eMSmoothMaterials};

	MSmoothGrip(){};
	MSmoothGrip(EditPolyMod *m, PolyOperation *o);
	~MSmoothGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

};

class TessellateGrip :public EditPolyModPolyOperationGrip
{
public:

	enum OurGrips{eTessellateType = 0, eTessellateTension };

	TessellateGrip(){};
	TessellateGrip(EditPolyMod *m, PolyOperation *o);
	~TessellateGrip();

	void Okay(TimeValue t);
	void Cancel();
	void Apply(TimeValue t);
	int GetNumGripItems();
	IBaseGrip::Type GetType(int which);
	void GetGripName(TSTR &string);
	bool GetText(int which,TSTR &string);
	bool GetResolvedIconName(int which,MSTR &string);
	DWORD GetCustomization(int which);
	bool GetComboOptions(int which, Tab<IBaseGrip::ComboLabel*> &radioOptions);
	bool GetCommandIcon(int which, MSTR &string); 
	bool GetValue(int which,TimeValue t,IBaseGrip::GripValue &value);
	bool SetValue(int which,TimeValue t,IBaseGrip::GripValue &value); 

	bool GetAutoScale(int which);
	bool GetScaleInViewSpace(int which, float &depth);
	bool GetScale(int which, IBaseGrip::GripValue &scaleValue);
	bool GetResetValue(int which,IBaseGrip::GripValue &resetValue);
	bool StartSetValue(int which,TimeValue t);
	bool EndSetValue(int which,TimeValue t,bool accepted);
	bool ResetValue(TimeValue t,int which);
	bool GetRange(int which,IBaseGrip::GripValue &minRange, IBaseGrip::GripValue &maxRange);
	bool ShowKeyBrackets(int which,TimeValue t);

	//need to activate grips
	void SetUpUI(); //called after the grip is enabled to set up init states that are called via IGripManager

};


#endif
