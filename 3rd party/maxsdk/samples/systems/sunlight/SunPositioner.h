//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <DaylightSimulation/ISunPositioner.h>

// Local
#include "CompassRoseObjCreateCallBack.h"
#include "WeatherData.h"
#include "WeatherFileCache.h"
// max sdk
#include <object.h>
#include <iparamb2.h>
// std
#include <memory>

class MoveModBoxCMode;
class RotateModBoxCMode;

class SunPositionerObject : 
    public MaxSDK::ISunPositioner
{
public:			

    static ClassDesc2& get_class_descriptor();

	explicit SunPositionerObject(const bool loading);
	~SunPositionerObject();

    // Returns the position of the sun object, in object space
    Point3 get_sun_position(const TimeValue t, Interval& validity) const;

    // Installs the Physical Sun & Sky environment, optionally overwriting any existing environment.
    // The installed environment will be made to point to this sun positioner.
    void install_sunsky_environment(
        // When true, overwrites any existing environment - when false, only installs a new environment if no environment is present
        const bool overwrite_existing_environment,
        // When true, any existing sky environment will be re-directed to point to 'this' sun positioner object
        const bool redirect_existing_sunsky_environment) const;

    ObjectState Eval(const TimeValue time, Interval& validity);

    // Sets location to given city
    void set_city_location(const TimeValue t, const float latitude, const float longitude, const MCHAR* city_name);

	// From BaseObject
	const TCHAR* GetObjectName();
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	void SetExtendedDisplay(int flags);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CreateMouseCallBack* GetCreateMouseCallBack();
	void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
	void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);

	// From Object
	ObjectState Eval(TimeValue time);
	void InitNodeName(TSTR& s);
	Interval ObjectValidity(TimeValue t);
	int CanConvertToType(Class_ID obtype) {return FALSE;}
	Object* ConvertToType(TimeValue t, Class_ID obtype) {assert(0);return NULL;}
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	int DoOwnSelectHilite()	{ return 1; }
	BOOL HasViewDependentBoundingBox() { return TRUE; }

    // From BaseObject - stuff for handling sub-objects
    virtual int NumSubObjTypes() override;
    virtual ISubObjType *GetSubObjType(int i) override;
    virtual void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc) override;
    virtual void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc) override;
    virtual void ActivateSubobjSel(int level, XFormModes& modes ) override;
    virtual void Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) override;
    virtual void Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin) override;
    virtual void Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) override;
    virtual void TransformCancel(TimeValue t) override;

    // ReferenceTarget/ReferenceMaker
    virtual int NumRefs() override;
    virtual RefTargetHandle GetReference(int i) override;
    virtual void SetReference(int i, RefTargetHandle rtarg) override;

	// Animatable methods
	void DeleteThis() { delete this; }
	Class_ID ClassID() { return get_class_descriptor().ClassID(); }  
	void GetClassName(TSTR& s);
	int IsKeyable(){ return 0;}
    IParamBlock2* GetParamBlock(int i) override;
    IParamBlock2* GetParamBlockByID(short id) override;
    int NumSubs() override;
    Animatable* SubAnim(int i) override;
    TSTR SubAnimName(int i) override;

	//  inherited virtual methods for Reference-management
	RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate );
	RefTargetHandle Clone(RemapDir& remap);

	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

    // -- inherited from ISunPositioner
    virtual Point3 GetSunDirection(const TimeValue t, Interval& validity) const override;
    virtual bool GetWeatherMeasurements(WeatherMeasurements& measurements, const TimeValue t, Interval& validity) const override;
    virtual bool GetDayOfYear(int& day_of_year, const TimeValue t, Interval& validity) const override;

    // -- inherited from LightObject
    virtual RefResult EvalLightState(TimeValue time, Interval& valid, LightState *ls) override;
    virtual void SetUseLight(int onOff) override;
    virtual BOOL GetUseLight(void) override;
    virtual void SetHotspot(TimeValue time, float f) override; 
    virtual float GetHotspot(TimeValue t, Interval& valid = Interval(0,0)) override;
    virtual void SetFallsize(TimeValue time, float f) override; 
    virtual float GetFallsize(TimeValue t, Interval& valid = Interval(0,0)) override;
    virtual void SetAtten(TimeValue time, int which, float f) override; 
    virtual float GetAtten(TimeValue t, int which, Interval& valid = Interval(0,0)) override;
    virtual void SetTDist(TimeValue time, float f) override; 
    virtual float GetTDist(TimeValue t, Interval& valid = Interval(0,0)) override;
    virtual void SetConeDisplay(int s, int notify=TRUE) override;
    virtual BOOL GetConeDisplay(void) override;

private:

    enum ParamBlockID
    {
        kParamBlockID_Main = 0
    };
    enum ReferenceID
    {
        kReferenceID_MainPB = 0
    };
    enum ParamMapID
    {
        kParamMapID_Display,
        kParamMapID_SunPosition,
    };
    enum MainParamID
    {
        kMainParamID_ShowCompass = 0,
        kMainParamID_CompassRadius = 1,
        kMainParamID_SunDistance = 2,
        kMainParamID_Mode = 3,
        kMainParamID_Hours = 4,
        kMainParamID_Minutes = 5,
        kMainParamID_Day = 6,
        kMainParamID_Month = 7,
        kMainParamID_Year = 8,
        kMainParamID_TimeZone = 9,
        kMainParamID_DST = 10,
        kMainParamID_Location = 11,
        kMainParamID_LatitudeDeg = 12,
        kMainParamID_LongitudeDeg = 13,
        kMainParamID_NorthDeg = 14,
        kMainParamID_ManualSunPosition = 15,
        kMainParamID_JulianDay = 16,
        kMainParamID_TimeInSeconds = 17,
        kMainParamID_AzimuthDeg = 18,
        kMainParamID_AltitudeDeg = 19,
        kMainParamID_WeatherFile = 20,
        kMainParamID_DSTUseDateRange = 21,
        kMainParamID_DSTStartDay = 22,
        kMainParamID_DSTStartMonth = 23,
        kMainParamID_DSTEndDay = 24,
        kMainParamID_DSTEndMonth = 25,
    };
    enum Mode
    {
        kMode_Manual = 0,
        kMode_SpaceTime = 1,
        kMode_WeatherFile = 2
    };
    enum SubObjectLevel
    {
        kSubObjectLevel_Sun,

        kSubObjectLevel_Count
    };
    enum IOChunkID
    {
        IOChunkID_WeatherFileData = 0,
    };
    enum class WeatherFileStatus
    {
        NotLoaded,
        LoadedSuccessfully,
        FileMissing,
        LoadError
    };

    class ClassDescriptor;
    class CreateCallBack;
    class ParamBlockAccessor;
    class SunPositionRollupWidget;

    static ClassDescriptor& get_class_desc_internal();

    // Returns the sun positioning mode. Guaranteed to return a valid enum value.
    Mode get_sun_mode() const;

    int HitTestCompass(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node);
    int HitTestSun(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node, const int flags);
    void DrawCompass(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node);
    void DrawSun(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node, const int flags);

    // Returns the sun direction from the current date, time, & location parameters
    Point3 calc_sun_direction_from_spacetime(const TimeValue t, Interval& validity) const;

    // Generic sub-object transformation method, called by Move(), Rotate(), etc.
    void Transform(const TimeValue t, const Matrix3& partm, const Matrix3& tmAxis, const Matrix3& xfrm, const bool is_rotation);

    // Calculates the sun position using for the given mode
    Point3 get_sun_position(const TimeValue t, Interval& validity, const Mode mode) const;

    bool get_show_compass(const TimeValue t) const;
    float get_compass_radius(const TimeValue t, Interval& valid) const;

    // Modifies the given node transform for drawing the sun mesh at the appropriate size
    Matrix3 get_sun_mesh_transform(const Matrix3& node_tm, const Point3& sun_position, ViewExp& vpt) const;

    static int get_current_year();

    // Launches the weather file setup dialog
    void do_setup_weather_file_dialog(const HWND parent_hwnd);
    // Updates the date/time/location parameters according to the weather data file, if any.
    // This is to be called prior to accessing the parameters.
    void update_weather_file_data(const TimeValue t, Interval& validity);

    // Returns whether DST is to be enabled at the given time
    bool get_dst_enabled(const TimeValue t, Interval& validity) const;

private:

	static CreateCallBack m_create_callback;
    
    static ParamBlockAccessor m_pb_accessor;        // declare BEFORE the PBDesc  
    static ParamBlockDesc2 m_pb_desc;
    IParamBlock2* m_param_block;

    Mesh m_sun_mesh;

    // Sub-object identifiers 
    GenSubObjType m_subobject_type_sun;     

    // Remembers the last mode used, such that we can adopt that mode's position when we switch to manual
    Mode m_last_mode_used;

    // Snap suspension flag (TRUE during creation only)
    bool m_suspendSnap;

    // The data setup by the weather file UI. Saved and loaded.
    WeatherUIData m_weather_file_ui_data;
    WeatherFileStatus m_weather_file_status;
    WeatherFileCacheFiltered m_weather_file_cache; 
    // Saves the validity interval for the weather data that was last pushed to the param block
    Interval m_weather_data_validity;
};

class SunPositionerObject::ClassDescriptor : public ClassDesc2
{
public:
    virtual int IsPublic() override;
    virtual void *Create(BOOL loading) override;
    virtual const TCHAR *ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const TCHAR* Category() override;
    virtual HINSTANCE HInstance() override;
    virtual const MCHAR* InternalName() override;

    // -- from ClassDesc2
    virtual MaxSDK::QMaxParamBlockWidget* CreateQtWidget(
        ReferenceMaker& owner,
        IParamBlock2& paramBlock,
        const MapID paramMapID,  
        MSTR& rollupTitle, 
        int& rollupFlags, 
        int& rollupCategory) override;
public:

    // command modes, stored in the class descriptor because the command-panel UI is not necessarily specific to a single object
    std::unique_ptr<MoveModBoxCMode> m_command_mode_move;
    std::unique_ptr<RotateModBoxCMode> m_command_mode_rotate;
};

class SunPositionerObject::CreateCallBack : public CreateMouseCallBack 
{
public:
    int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
    void SetObj(SunPositionerObject *obj);

private:
    Point3 obj_pos_in_world_space;
    IPoint2 m_obj_pos_in_screen_space;
    SunPositionerObject *ob;
};

class SunPositionerObject::ParamBlockAccessor : public PBAccessor
{
public:

    // -- from PBAccessor
    virtual void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) override;
    virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;

private:

    bool m_disable_set = false;
};

