//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "SunPositioner.h"

// local
#include "SunPositioner_UI.h"
#include "autovis.h"
#include "compass.h"
#include "sunlight.h"
#include "WeatherFileDialog.h"
#include "PhysicalSunSkyEnv.h"
#include "natlight.h"
#include "CityList.h"

// max sdk
#include <DllUtilities.h>
#include <gfx.h>
#include <mouseman.h>
#include <decomp.h>
#include <trig.h>
#include <objmode.h>
#include <SubObjAxisCallback.h>
#include <control.h>
#include <AssetManagement/iassetmanager.h>
#include <toneop.h>
#include <DaylightSimulation/IPerezAllWeather.h>
#include <Scene/IPhysicalCamera.h>

// Qt
#include "ui_SunPositioner_Display.h"

#undef min
#undef max

using namespace MaxSDK;
using namespace MaxSDK::AssetManagement;

int getTimeZone(float longi);

namespace 
{
    // Mesh copied from mr physical sun
    const float sun_mesh_verts[][3] = 
    {
        {  0.000f,  -7.416f, -11.250f},
        { 12.000f,   0.000f,  -7.416f},
        {  0.000f,   7.416f, -11.250f},
        {-12.000f,   0.000f,  -7.416f},
        {  0.000f,  -3.316f,  -5.355f},
        {  0.000f,   3.316f,  -5.355f},
        { -3.316f,   5.355f,   0.000f},
        { -5.355f,   0.000f,  -3.316f},
        {  5.355f,   0.000f,  -3.316f},
        { -7.416f, -11.250f,   0.000f},
        {  3.316f,  -5.355f,   0.000f},
        {  7.416f,  -2.833f,   0.000f},
        {  0.000f,  -7.416f,   0.000f},
        {  3.316f,  -5.355f,   0.000f},
        {  7.416f,  -2.833f,   0.000f},
        { -7.416f, -11.250f,   0.000f},
        { -7.416f, -11.250f,   0.000f},
        {  7.416f, -11.250f,   0.000f},
        {  7.416f, -11.250f,   0.000f},
        {  7.416f, -11.250f,   0.000f},
        {  7.416f,  12.000f,   0.000f},
        {  7.416f,  12.000f,   0.000f},
        {  7.416f,  12.000f,   0.000f},
        { -7.416f,  12.000f,   0.000f},
        { -7.416f,  12.000f,   0.000f},
        { -7.416f,  12.000f,   0.000f},
        { -7.416f,   2.833f,   0.000f},
        { -3.316f,   5.355f,   0.000f},
        {  0.000f,   7.416f,   0.000f},
        { -7.416f,   2.833f,   0.000f},
        { -3.316f,   5.355f,   0.000f},
        { -2.962f,   5.586f,   0.000f},
        { -7.416f, -11.250f,   0.000f},
        { -7.416f,  12.000f,   0.000f},
        { -7.416f,  -2.833f,   0.000f},
        { -7.416f,   0.000f,   0.000f},
        { -7.416f,   0.000f,   0.000f},
        { -7.416f,  -2.833f,   0.000f},
        { -3.316f,  -5.355f,   0.000f},
        { -3.316f,  -5.355f,   0.000f},
        {  7.416f,   2.833f,   0.000f},
        {  3.316f,   5.355f,   0.000f},
        {  7.416f,   2.833f,   0.000f},
        {  3.316f,   5.355f,   0.000f},
        {  7.416f,   0.000f,   0.000f},
        {  7.416f, -11.250f,   0.000f},
        {  7.416f,   0.000f,   0.000f},
        {  7.416f,  12.000f,   0.000f}
    };

    const unsigned char sun_mesh_faces[][4] = {
        {10,  9,  1, EDGE_A | EDGE_B         },
        {10,  1, 11,          EDGE_B | EDGE_C},
        {13, 12,  0, EDGE_A | EDGE_B         },
        {13,  0, 14,          EDGE_B | EDGE_C},
        { 4,  3,  1,          EDGE_B         },
        { 5,  1,  3,          EDGE_B         },
        { 6, 30, 31,          EDGE_B         },
        { 7, 32, 33,          EDGE_B         },
        {36, 26,  3, EDGE_A | EDGE_B         },
        {36,  3, 37,          EDGE_B | EDGE_C},
        {38,  3, 19,          EDGE_B | EDGE_C},
        {39, 34,  0, EDGE_A | EDGE_B         },
        {39,  0, 12,          EDGE_B | EDGE_C},
        {38, 37,  3, EDGE_A | EDGE_B         },
        {41, 40,  1, EDGE_A | EDGE_B         },
        {41,  1, 25,          EDGE_B | EDGE_C},
        {43, 42,  2, EDGE_A | EDGE_B         },
        {43,  2, 28,          EDGE_B | EDGE_C},
        {44, 11,  1, EDGE_A | EDGE_B         },
        {44,  1, 40,          EDGE_B | EDGE_C},
        { 8, 47, 45,          EDGE_B         },
        { 4, 15, 16,          EDGE_B         },
        { 4, 16,  2,          EDGE_B         },
        {18,  2, 17,          EDGE_B | EDGE_C},
        { 4,  2, 18,                        0},
        { 4, 18, 19,          EDGE_B         },
        { 4, 19,  3,          EDGE_B         },
        {15,  1,  9,          EDGE_B | EDGE_C},
        { 4,  1, 15,                        0},
        {21,  3, 20,          EDGE_B | EDGE_C},
        { 5,  3, 21,                        0},
        { 5, 21, 22,          EDGE_B         },
        { 5, 22,  0,          EDGE_B         },
        {24,  0, 23,          EDGE_B | EDGE_C},
        { 5,  0, 24,                        0},
        { 5, 24, 25,          EDGE_B         },
        { 5, 25,  1,          EDGE_B         },
        {27,  3, 26,          EDGE_B | EDGE_C},
        { 6,  3, 27,                        0},
        { 6, 27, 28,          EDGE_B         },
        { 6, 28,  2,          EDGE_B         },
        {30,  2, 29,          EDGE_B | EDGE_C},
        { 6,  2, 30,                        0},
        { 6, 31, 20,          EDGE_B         },
        { 6, 20,  3,          EDGE_B         },
        {32,  2, 16,          EDGE_B | EDGE_C},
        { 7,  2, 32,                        0},
        { 7, 33, 23,          EDGE_B         },
        { 7, 23,  0,          EDGE_B         },
        {35,  0, 34,          EDGE_B | EDGE_C},
        { 7,  0, 35,                        0},
        { 7, 35, 29,          EDGE_B         },
        { 7, 29,  2,          EDGE_B         },
        { 8, 45, 17,          EDGE_B         },
        { 8, 17,  2,          EDGE_B         },
        {46,  2, 42,          EDGE_B | EDGE_C},
        { 8,  2, 46,                        0},
        { 8, 46, 14,          EDGE_B         },
        { 8, 14,  0,          EDGE_B         },
        {47,  0, 22,          EDGE_B | EDGE_C},
        { 8,  0, 47,                        0},
    };

    void BuildSunMesh(Mesh& mesh)
    {
        const int vertCount = _countof(sun_mesh_verts);
        mesh.setNumVerts(vertCount);
        for (int i = 0; i < vertCount; ++i) {
            mesh.setVert(i, sun_mesh_verts[i]);
        }

        const int faceCount = _countof(sun_mesh_faces);
        mesh.setNumFaces(faceCount);
        for (int i = 0; i < faceCount; ++i) {
            Face& f = mesh.faces[i];

            f.v[0] = sun_mesh_faces[i][0];
            f.v[1] = sun_mesh_faces[i][1];
            f.v[2] = sun_mesh_faces[i][2];
            f.smGroup = 0;
            f.flags = sun_mesh_faces[i][3];
        }
    }

    void RemoveScaling(Matrix3 &tm) 
    {
        AffineParts ap;
        decomp_affine(tm, &ap);
        tm.IdentityMatrix();
        tm.SetRotate(ap.q);
        tm.SetTrans(ap.t);
    }

    struct DayAndMonth
    {
        DayAndMonth(const int p_day, const int p_month)
            : day(p_day),
            month(p_month)
        {
        }

        bool operator<(const DayAndMonth& rhs) const
        {
            return (month < rhs.month)
                || ((month == rhs.month) && (day < rhs.day));
        }
        bool operator<=(const DayAndMonth& rhs) const
        {
            return (*this < rhs) || (*this == rhs);
        }
        bool operator>(const DayAndMonth& rhs) const
        {
            return !(*this < rhs) && (*this != rhs);
        }
        bool operator>=(const DayAndMonth& rhs) const
        {
            return (*this > rhs) || (*this == rhs);
        }
        bool operator==(const DayAndMonth& rhs) const
        {
            return (day == rhs.day) && (month == rhs.month);
        }
        bool operator!=(const DayAndMonth& rhs) const
        {
            return !(*this == rhs);
        }

        int day;
        int month;
    };
}

SunPositionerObject::ParamBlockAccessor SunPositionerObject::m_pb_accessor;

// Param block descriptor
ParamBlockDesc2 SunPositionerObject::m_pb_desc(
    kParamBlockID_Main, 
    _T("Parameters"), IDS_PARAMETERS,
    &get_class_descriptor(), 
    P_AUTO_CONSTRUCT | P_AUTO_UI_QT | P_MULTIMAP,

    // Reference ID
    kReferenceID_MainPB,

    // AutoUI
    2,      // num param maps
    kParamMapID_Display,
    kParamMapID_SunPosition, 

    // Parameters
    kMainParamID_ShowCompass, _T("show_compass"), TYPE_BOOL, 0, IDS_SUNPOS_PARAM_SHOWCOMPASS,
        p_default, true,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_CompassRadius, _T("compass_radius"), TYPE_WORLD, P_ANIMATABLE, IDS_SUNPOS_PARAM_COMPASSRADIUS,
        p_default, 10.0f, 
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_SunDistance, _T("sun_distance"), TYPE_WORLD, P_ANIMATABLE, IDS_SUNPOS_PARAM_SUNDISTANCE,
        p_default, 40.0f,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_Mode, _T("mode"), TYPE_INT, 0, IDS_SUNPOS_PARAM_MODE,
        p_default, kMode_SpaceTime,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_JulianDay, _T("julian_day"), TYPE_INT, P_ANIMATABLE, IDS_SUNPOS_PARAM_JULIANDAY,
        p_range, gregorian2julian_int(1, 1, MINYEAR), gregorian2julian_int(12, 31, MAXYEAR),
        p_default, gregorian2julian_int(6, 21, get_current_year()),       // summer solstice, current year
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_TimeInSeconds, _T("time_in_seconds"), TYPE_INT, P_ANIMATABLE, IDS_SUNPOS_PARAM_TIMEINSECONDS,
        p_range, 0, (24*3600),     // 24 hours -> seconds
        p_default, (12*3600),      // noon
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_Hours, _T("hours"), TYPE_INT, P_TRANSIENT, IDS_SUNPOS_PARAM_HOURS,
        p_range, 0, 23,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_Minutes, _T("minutes"), TYPE_INT, P_TRANSIENT, IDS_SUNPOS_PARAM_MINUTES,
        p_range, 0, 59,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_Day, _T("day"), TYPE_INT, P_TRANSIENT, IDS_SUNPOS_PARAM_DAY,
		p_default, 1, // specify a default value so that the ui has a proper reset value
        p_range, 1, 31,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_Month, _T("month"), TYPE_INT, P_TRANSIENT, IDS_SUNPOS_PARAM_MONTH,
		p_default, 1, // specify a default value so that the ui has a proper reset value
        p_range, 1, 12,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_Year, _T("year"), TYPE_INT, P_TRANSIENT, IDS_SUNPOS_PARAM_YEAR,
		p_default, get_current_year(), // specify a default value so that the ui has a proper reset value
        p_range, MINYEAR, MAXYEAR,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_TimeZone, _T("time_zone"), TYPE_FLOAT, P_ANIMATABLE, IDS_SUNPOS_PARAM_TIMEZONE,
        p_default, 0.0f,
        p_range, -24.0f, 24.0f,
        p_accessor, &m_pb_accessor,
        p_end,

    // DST
    kMainParamID_DST, _T("dst"), TYPE_BOOL, P_ANIMATABLE, IDS_SUNPOS_PARAM_DST,
        p_default, true,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_DSTUseDateRange, _T("dst_use_date_range"), TYPE_BOOL, P_ANIMATABLE, IDS_SUNPOS_PARAM_DST_USE_DATE_RANGE,
        p_default, false,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_DSTStartDay, _T("dst_start_day"), TYPE_INT, P_ANIMATABLE, IDS_SUNPOS_PARAM_DST_START_DAY,
        p_default, 15,
        p_range, 1, 31,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_DSTStartMonth, _T("dst_start_month"), TYPE_INT, P_ANIMATABLE, IDS_SUNPOS_PARAM_DST_START_MONTH,
        p_default, 3,
        p_range, 1, 12,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_DSTEndDay, _T("dst_end_day"), TYPE_INT, P_ANIMATABLE, IDS_SUNPOS_PARAM_DST_END_DAY,
        p_default, 1,
        p_range, 1, 31,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_DSTEndMonth, _T("dst_end_month"), TYPE_INT, P_ANIMATABLE, IDS_SUNPOS_PARAM_DST_END_MONTH,
        p_default, 11,
        p_range, 1, 12,
        p_accessor, &m_pb_accessor,
        p_end,

    kMainParamID_Location, _T("location"), TYPE_STRING, P_READ_ONLY, IDS_SUNPOS_PARAM_LOCATION,      // read-only, shouldn't be settable by the user
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_LatitudeDeg, _T("latitude_deg"), TYPE_FLOAT, P_ANIMATABLE, IDS_SUNPOS_PARAM_LATITUDE,
        p_default, 0.0f,
        p_range, -90.0f, 90.0f,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_LongitudeDeg, _T("longitude_deg"), TYPE_FLOAT, P_ANIMATABLE, IDS_SUNPOS_PARAM_LONGITUDE,
        p_default, 0.0f,
        p_range, -180.0f, 180.0f,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_NorthDeg, _T("north_direction_deg"), TYPE_FLOAT, P_ANIMATABLE, IDS_SUNPOS_PARAM_NORTHDIR,
        p_default, 0.0f,
        p_range, -359.99f, 359.99f,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_ManualSunPosition, _T("manual_sun_position"), TYPE_POINT3, P_ANIMATABLE, IDS_SUNPOS_PARAM_MANUALSUNTANSFORM,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_AzimuthDeg, _T("azimuth_deg"), TYPE_FLOAT, P_TRANSIENT, IDS_SUNPOS_PARAM_AZIMUTH_DEG,
        p_range, 0.0f, 360.0f,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_AltitudeDeg, _T("altitude_deg"), TYPE_FLOAT, P_TRANSIENT, IDS_SUNPOS_PARAM_ALTITUDE_DEG,
        p_range, -90.0f, 90.0f,
        p_accessor, &m_pb_accessor,
        p_end,
    // read-only because the weather file requires the dialog for setup
    kMainParamID_WeatherFile, _T("weather_file"), TYPE_FILENAME, P_READ_ONLY | P_READ_SECOND_FLAG_VALUE, P_ENUMERATE_AS_ASSET, IDS_SUNPOS_PARAM_WEATHER_FILE,        
        p_accessor, &m_pb_accessor,
        p_assetTypeID, kExternalLink,
        p_end,
    p_end
);

SunPositionerObject::CreateCallBack SunPositionerObject::m_create_callback;

SunPositionerObject::SunPositionerObject(const bool loading)
    : m_param_block(nullptr),
    m_subobject_type_sun(QObject::tr("Sun"), _T(""), 2),
    m_last_mode_used(kMode_Manual),
    m_weather_file_status(WeatherFileStatus::NotLoaded),
    m_suspendSnap(false)
{  
    if(!loading)
    {
        get_class_descriptor().MakeAutoParamBlocks(this);
    }

    // Build the mesh we'll use to display the sun
    BuildSunMesh(m_sun_mesh);

    // Remember the initial mode 
    if(m_param_block != nullptr)
    {
        Interval dummy_validity;
        m_last_mode_used = static_cast<Mode>(m_param_block->GetInt(kMainParamID_Mode, 0, dummy_validity));

        // Make the controllers linear by default for date and time
        Control* c1 = static_cast<Control*>(CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(LININTERP_FLOAT_CLASS_ID, 0)));
        Control* c2 = static_cast<Control*>(CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(LININTERP_FLOAT_CLASS_ID, 0)));
        m_param_block->SetControllerByID(kMainParamID_JulianDay, 0, c1);
        m_param_block->SetControllerByID(kMainParamID_TimeInSeconds, 0, c2);

        // Set the default city
        // When manually changing coordinates, see if the coordinates match a known city
        MSTR city_name;
        CityList& city_list = CityList::GetInstance();
        city_list.init();
        const CityList::Entry* default_city = city_list.GetDefaultCity();
        if(default_city != nullptr)
        {
            set_city_location(GetCOREInterface()->GetTime(), default_city->latitude, default_city->longitude, default_city->get_ui_name());
        }
    }
}

SunPositionerObject::~SunPositionerObject()
{  
}

SunPositionerObject::ClassDescriptor& SunPositionerObject::get_class_desc_internal()
{
    static ClassDescriptor the_class_desc;
    return the_class_desc;
}

ClassDesc2& SunPositionerObject::get_class_descriptor()
{
    return get_class_desc_internal();
}

const TCHAR* SunPositionerObject::GetObjectName() 
{ 
    return get_class_descriptor().ClassName();
}

void SunPositionerObject::BeginEditParams(
	IObjParam *ip, ULONG flags,Animatable *prev)
{  
    // Create the command-modes if necessary
    ClassDescriptor& class_desc = get_class_desc_internal();
    class_desc.m_command_mode_move.reset(new MoveModBoxCMode(this, ip));
    class_desc.m_command_mode_rotate.reset(new RotateModBoxCMode(this, ip));

    get_class_descriptor().BeginEditParams(ip, this, flags, prev);
}

void SunPositionerObject::EndEditParams(
	IObjParam *ip, ULONG flags,Animatable *next)
{  
    get_class_descriptor().EndEditParams(ip, this, flags, next);

    // Delete the command-modes
    if(DbgVerify(ip != nullptr))
    {
        ClassDescriptor& class_desc = get_class_desc_internal();
        ip->DeleteMode(class_desc.m_command_mode_move.get());
        class_desc.m_command_mode_move.reset();
        ip->DeleteMode(class_desc.m_command_mode_rotate.get());
        class_desc.m_command_mode_rotate.reset();
    }
}

CreateMouseCallBack* SunPositionerObject::GetCreateMouseCallBack() 
{
	m_create_callback.SetObj(this);
	return(&m_create_callback);
}

void SunPositionerObject::SetExtendedDisplay(int flags)
{
	// don't care about these flags
}

void SunPositionerObject::GetLocalBoundBox(
	TimeValue t, INode* inode, ViewExp* vpt, Box3& box ) 
{
    Interval dummy_interval;
	Matrix3 tm(1);
	tm.SetTrans(inode->GetObjectTM(t).GetTrans());
	box = GetCompassRoseAxisBoundingBox(vpt,tm, get_show_compass(t) ? get_compass_radius(t, dummy_interval) : 0.0f, TRUE);

    // Add the sun mesh's bounding box
    if(DbgVerify(vpt != nullptr))
    {
        const Point3 sun_position = get_sun_position(t, dummy_interval);
        const Matrix3 sun_tm = get_sun_mesh_transform(tm, sun_position, *vpt);
        box += m_sun_mesh.getBoundingBox(const_cast<Matrix3*>(&sun_tm));
    }
}

void SunPositionerObject::GetWorldBoundBox(
	TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
    Interval dummy_interval;
	Matrix3 tm;
	tm = inode->GetObjectTM(t);
	box = GetCompassRoseAxisBoundingBox(vpt,tm,get_show_compass(t) ? get_compass_radius(t, dummy_interval) : 0.0f, FALSE);

    // Add the sun mesh's bounding box
    if(DbgVerify(vpt != nullptr))
    {
        const Point3 sun_position = get_sun_position(t, dummy_interval);
        const Matrix3 sun_tm = get_sun_mesh_transform(tm, sun_position, *vpt);
        box += m_sun_mesh.getBoundingBox(const_cast<Matrix3*>(&sun_tm));
    }

	assert(!box.IsEmpty());
}


// From BaseObject
int SunPositionerObject::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) {

	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

    int result = false;

    GraphicsWindow *gw = vpt->getGW();  
    const DWORD savedLimits = gw->getRndLimits();
    {

        HitRegion hitRegion;
	    gw->setTransform(idTM);
	    MakeHitRegion(hitRegion, type, crossing, 8, p);

	    gw->setRndLimits((savedLimits|GW_PICK) & ~GW_ILLUM & ~GW_Z_BUFFER);
	    gw->setHitRegion(&hitRegion);
	    gw->clearHitCode();

        const Matrix3 tm = inode->GetObjectTM(t);    

        // Hit test the compass
        result = HitTestCompass(t, *vpt, *gw, *inode);

        // Hit test the sun mesh
        if(result ==  0)
        {
            gw->clearHitCode();
            result = HitTestSun(t, *vpt, *gw, *inode, flags);
        }

    }
	gw->setRndLimits(savedLimits);

	return result;
}

int SunPositionerObject::HitTestCompass(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node) 
{
    DrawCompass(t, vpt, gw, node);
    const int result = gw.checkHitCode();
    return result;
}

int SunPositionerObject::HitTestSun(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node, const int flags) 
{
    DrawSun(t, vpt, gw, node, flags);
    gw.marker(const_cast<Point3 *>(&Point3::Origin), X_MRKR);
    const int result = gw.checkHitCode();
    return result;
}

void SunPositionerObject::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt)
{
    if(!m_suspendSnap)
    {
	    if ( ! vpt || ! vpt->IsAlive() )
	    {
		    // why are we here
		    DbgAssert(!_T("Invalid viewport!"));
		    return;
	    }

	    Matrix3 tm = inode->GetObjectTM(t); 
	    GraphicsWindow *gw = vpt->getGW();  
	    gw->setTransform(tm);

	    Matrix3 invPlane = Inverse(snap->plane);
	    // Check for upside-down coordinate system in viewport
	    float ybias = (float)gw->getWinSizeY() - 1.0f;

	    // Make sure the vertex priority is active and at least as important as the best snap so far
	    if(snap->vertPriority > 0 && snap->vertPriority <= snap->priority) {
		    Point2 fp = Point2((float)p->x, (float)p->y);
		    Point3 screen3;
		    Point2 screen2;

		    Point3 thePoint(0,0,0);
		    // If constrained to the plane, make sure this point is in it!
		    if(snap->snapType == SNAP_2D || snap->flags & SNAP_IN_PLANE) {
			    Point3 test = thePoint * tm * invPlane;
			    if(fabs(test.z) > 0.0001)  // Is it in the plane (within reason)?
				    return;
		    }
		    //gw->transPointFWin(&thePoint,&screen3);  //this is for MAX 1.2
		    gw->transPoint(&thePoint, &screen3);
		    screen3.y = ybias - screen3.y;
		    screen2.x = screen3.x;
		    screen2.y = screen3.y;

		    // Are we within the snap radius?
		    int len = (int)Length(screen2 - fp);
		    if(len <= snap->strength) {
			    // Is this priority better than the best so far?
			    if(snap->vertPriority < snap->priority) {
				    snap->priority = snap->vertPriority;
				    snap->bestWorld = thePoint * tm;
				    snap->bestScreen = screen2;
				    snap->bestDist = len;
			    }
			    else
				    if(len < snap->bestDist) {
					    snap->priority = snap->vertPriority;
					    snap->bestWorld = thePoint * tm;
					    snap->bestScreen = screen2;
					    snap->bestDist = len;
				    }
		    }
	    }
    }
}

void SunPositionerObject::DrawCompass(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node) 
{
    if(DbgVerify(m_param_block != nullptr))
    {
        Interval dummy_interval;
        const bool show_compass = (m_param_block->GetInt(kMainParamID_ShowCompass, t, dummy_interval) != 0);
        if(show_compass)
        {
            Matrix3 compass_tm = node.GetObjectTM(t);

            const float north_deg = m_param_block->GetFloat(kMainParamID_NorthDeg, t, dummy_interval);
            const float north_rad = (north_deg * (PI / 180.0f));

            // Apply north offset, as rotation around Z
            compass_tm.PreRotateZ(-north_rad);

            gw.setTransform(compass_tm);
            DrawCompassRoseAxis(&vpt, get_compass_radius(t, dummy_interval), node.Selected(), node.IsFrozen());
        }
    }
}

void SunPositionerObject::DrawSun(const TimeValue t, ViewExp& vpt, GraphicsWindow& gw, INode& node, const int flags) 
{
    const Matrix3 node_tm = node.GetObjectTM(t);

    // Determine what color to draw the sun with
    Color wireframe_color = Color(node.GetWireColor());
    if (node.Selected())
    {
        // Draw sun with sub-selection color, if its sub-selection is active
        wireframe_color = (GetSubObjectLevel() == (kSubObjectLevel_Sun + 1)) ? GetSubSelColor() : GetSelColor();
    }
    else if(!node.IsFrozen() && !node.Dependent())	
    {
        // Draw sun black if it's off
        wireframe_color = GetUseLight() ? Color(node.GetWireColor()) : Color(0.0f, 0.0f, 0.0f);
    }
    else if(node.IsFrozen())
    {
        wireframe_color = GetFreezeColor();
    }
    gw.setColor(LINE_COLOR, wireframe_color);

    // Calculate the sun position in object space
    Interval dummy_validity;
    const Point3 sun_position = get_sun_position(t, dummy_validity);

    // Draw the sun mesh
    {
        // Create the transform for the sun, in object space
        const Matrix3 sun_tm = get_sun_mesh_transform(node_tm, sun_position, vpt);

        // Draw the sun mesh
        gw.setTransform(sun_tm);
        m_sun_mesh.render(  &gw, gw.getMaterial(), (flags&USE_DAMAGE_RECT) ? &vpt.GetDammageRect() : NULL, COMP_ALL);	
    }

    // Draw the target line
    {
        gw.setTransform(node_tm);
        Point3 v[2] =
        {
            Point3(0,0,0),
            sun_position
        };
        gw.polyline( 2, v, NULL, NULL, FALSE, NULL );

        // 'X' marker at sun target
        gw.marker(const_cast<Point3*>(&Point3::Origin), X_MRKR);
    }

}

Matrix3 SunPositionerObject::get_sun_mesh_transform(const Matrix3& node_tm, const Point3& sun_position, ViewExp& vpt) const
{
    // Create the transform for the sun, in object space
    Matrix3 sun_tm;
    sun_tm.SetFromToUp(sun_position, Point3(0.0f, 0.0f, 0.0f), Point3(0.0f, 0.0f, 1.0f));
    // Transform into world space
    sun_tm = sun_tm * node_tm;

    // Scale the sun widget to the desired size
    RemoveScaling(sun_tm);
    const float scaleFactor = vpt.NonScalingObjectSize() * vpt.GetVPWorldWidth(sun_tm.GetTrans()) / 360.0f;
    sun_tm.PreScale(Point3(scaleFactor, scaleFactor, scaleFactor));

    return sun_tm;
}

int SunPositionerObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
{
    if(DbgVerify((vpt != nullptr) && vpt->IsAlive() && (inode != nullptr)))
    {
        GraphicsWindow *gw = vpt->getGW();
        if(DbgVerify(gw != nullptr))
        {
            const DWORD rlim = gw->getRndLimits();
            {
                gw->setRndLimits(GW_WIREFRAME | GW_EDGES_ONLY | GW_BACKCULL | (gw->getRndMode() & GW_Z_BUFFER));
                // Draw the compass
                DrawCompass(t, *vpt, *gw, *inode);

                // Draw the sun
                DrawSun(t, *vpt, *gw, *inode, flags);
                gw->setRndLimits(rlim);
            }
        }
    }

	return(0);
}

RefResult SunPositionerObject::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate ) 
{
    // If the parameter block has changed
    if((hTarget == m_param_block) && (m_param_block != nullptr))
    {
        Interval dummy_interval;
        const TimeValue t = GetCOREInterface()->GetTime();
        ParamID changing_param = m_param_block->LastNotifyParamID();
        SunPositionerObject::m_pb_desc.InvalidateUI(changing_param);

        // Check if the mode has changed
        const Mode current_mode = static_cast<Mode>(m_param_block->GetInt(kMainParamID_Mode, t, dummy_interval));
        if(current_mode != m_last_mode_used)
        {
            // Remember the last mode used (this also prevents recursive loops to this method)
            const Mode last_mode = m_last_mode_used;
            m_last_mode_used = current_mode;

            // When the mode changes to manual, we want to preserve the position that was already in use
            if(current_mode == kMode_Manual)
            {
                // Query the sun position for the last used mode
                const Point3 sun_position = get_sun_position(t, dummy_interval, last_mode);

                // Initialize the manual position (suspend animation to avoid animating the manual position)
                SuspendAnimate();
                m_param_block->SetValue(kMainParamID_ManualSunPosition, t, sun_position);
                ResumeAnimate();
            }
        }
    }

	return REF_SUCCEED;
}

ObjectState SunPositionerObject::Eval(TimeValue time)
{
    // Update the weather data 
    Interval dummy_validity;
    return Eval(time, dummy_validity);
}

ObjectState SunPositionerObject::Eval(const TimeValue time, Interval& validity)
{
    // Update the weather data 
    update_weather_file_data(time, validity);
	return ObjectState(this);
}

Interval SunPositionerObject::ObjectValidity(TimeValue t)
{
    // Validity of the object is that of the sun direction    
    Interval validity = FOREVER;
    Eval(t, validity);
    return validity;
}

void SunPositionerObject::set_city_location(const TimeValue t, const float latitude, const float longitude, const MCHAR* city_name)
{
    // Approximate the time zone based on longitude
    const float time_zone = getTimeZone(longitude);
    m_param_block->SetValue(kMainParamID_TimeZone, t, time_zone);
    m_param_block->SetValue(kMainParamID_LatitudeDeg, t, latitude);
    m_param_block->SetValue(kMainParamID_LongitudeDeg, t, longitude);
    // Set location name last, to avoid it being reset by setting the coordinates.
    m_param_block->SetValue(kMainParamID_Location, t, city_name);
}

RefTargetHandle SunPositionerObject::Clone(RemapDir& remap) 
{
	SunPositionerObject* newob = new SunPositionerObject(true);   

    const int num_refs = NumRefs();
    for(int i = 0; i < num_refs; ++i)
    {
        newob->ReplaceReference(i, remap.CloneRef(GetReference(i))); 
    }

	BaseClone(this, newob, remap);

	return(newob);
}

IOResult SunPositionerObject::Load(ILoad *iload)
{
    for (IOResult open_chunk_res = iload->OpenChunk(); open_chunk_res == IO_OK; open_chunk_res = iload->OpenChunk())
    {
        IOResult res = IO_OK;
        const int id = iload->CurChunkID();
        switch (id)
        {
        case IOChunkID_WeatherFileData:
            res = m_weather_file_ui_data.Load(iload);
            break;
        }
        iload->CloseChunk();
        if (res != IO_OK)
        {
            return res;
        }
    }

    return IO_OK;

}

IOResult SunPositionerObject::Save(ISave *isave)
{
    if (isave != nullptr)
    {
        isave->BeginChunk(IOChunkID_WeatherFileData);
        const IOResult res = m_weather_file_ui_data.Save(isave);
        if (res != IO_OK)
            return res;
        isave->EndChunk();
    }

    return IO_OK;
}

void SunPositionerObject::InitNodeName( TSTR& s )
{
	s = MaxSDK::GetResourceStringAsMSTR(IDS_SUNPOS_DEFAULT_OBJ_NAME);
}

void SunPositionerObject::GetClassName( TSTR& s )
{
    s = get_class_descriptor().ClassName();
}

RefResult SunPositionerObject::EvalLightState(TimeValue time, Interval& valid, LightState *ls)
{
    if(DbgVerify(ls != nullptr))
    {
		ls->color = Color(1,1,1);
		ls->intens = 100.0f;
		ls->type = DIRECT_LGT;
		ls->hotsize = 1.0e5f;
		ls->fallsize = 1.0e5f;
		ls->useAtten = 0;
		ls->aspect = 1.0f;
		ls->overshoot = FALSE;
		ls->shadow = FALSE;
		ls->shape = 1;
		return REF_SUCCEED;
    }
	return REF_FAIL;
}

void SunPositionerObject::SetUseLight(int onOff)
{
    //!! TODO
}

BOOL SunPositionerObject::GetUseLight(void)
{
    //!! TODO
    return true;
}

void SunPositionerObject::SetHotspot(TimeValue time, float f)
{
    //!! TODO
}
 
float SunPositionerObject::GetHotspot(TimeValue t, Interval& valid)
{
    //!! TODO
    return 0.0f;
}

void SunPositionerObject::SetFallsize(TimeValue time, float f)
{
    //!! TODO
}
 
float SunPositionerObject::GetFallsize(TimeValue t, Interval& valid)
{
    //!! TODO
    return 0.0f;
}

void SunPositionerObject::SetAtten(TimeValue time, int which, float f)
{
    //!! TODO
}
 
float SunPositionerObject::GetAtten(TimeValue t, int which, Interval& valid)
{
    //!! TODO
    return 0.0f;
}

void SunPositionerObject::SetTDist(TimeValue time, float f)
{
    //!! TODO
}
 
float SunPositionerObject::GetTDist(TimeValue t, Interval& valid)
{
    //!! TODO
    return 0.0f;
}

void SunPositionerObject::SetConeDisplay(int s, int notify)
{
    //!! TODO
}

BOOL SunPositionerObject::GetConeDisplay(void)
{
    //!! TODO
    return false;
}

void SunPositionerObject::update_weather_file_data(const TimeValue t, Interval& validity)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        // Re-load the weather data file if necessary
        if(m_weather_file_status != WeatherFileStatus::LoadedSuccessfully)
        {
            m_weather_file_cache.Clear();
            m_weather_data_validity.SetEmpty();

            const AssetUser asset = m_param_block->GetAssetUser(kMainParamID_WeatherFile, t, validity);
            const MSTR file_path = asset.GetFullFilePath();
            if(file_path.Length() > 0)      // file exists?
            {
                m_weather_file_cache.ReadWeatherData(file_path);
                const int num_entries = m_weather_file_cache.FilterEntries(m_weather_file_ui_data);
                m_weather_file_status = (num_entries > 0) ? WeatherFileStatus::LoadedSuccessfully : WeatherFileStatus::LoadError;
            }
            else
            {
                m_weather_file_status = (asset.GetFileName().Length() > 0) ? WeatherFileStatus::FileMissing : WeatherFileStatus::NotLoaded;
            }
        }

        // Setup the parameters based on the weather data
        if(m_weather_file_status == WeatherFileStatus::LoadedSuccessfully) 
        {
            if(!m_weather_data_validity.InInterval(t))
            {
                // Suspend animation, since we're hacking these parameters into the param block
                AnimateSuspend animated_suspend;

                const MSTR city_name = 
                    m_weather_file_cache.mWeatherHeaderInfo.mCity 
                    + _T(", ") 
                    + m_weather_file_cache.mWeatherHeaderInfo.mRegion 
                    + _T(", ") 
                    + m_weather_file_cache.mWeatherHeaderInfo.mCountry;

                // Set location parameters
                m_param_block->SetValue(kMainParamID_Location, t, city_name);
                m_param_block->SetValue(kMainParamID_LatitudeDeg, t, m_weather_file_cache.mWeatherHeaderInfo.mLat);
                m_param_block->SetValue(kMainParamID_LongitudeDeg, t, m_weather_file_cache.mWeatherHeaderInfo.mLon);
                m_param_block->SetValue(kMainParamID_TimeZone, t, m_weather_file_cache.mWeatherHeaderInfo.mTimeZone);

                // Set time/date
                const DaylightWeatherData entry = m_weather_file_cache.GetNthEntry(t);
                // Set year and month before day, to make sure the day doesn't get clamped
                m_param_block->SetValue(kMainParamID_Year, t, entry.mYear);
                m_param_block->SetValue(kMainParamID_Month, t, entry.mMonth);
                m_param_block->SetValue(kMainParamID_Day, t, entry.mDay);
                m_param_block->SetValue(kMainParamID_Hours, t, entry.mHour);
                m_param_block->SetValue(kMainParamID_Minutes, t, entry.mMinute);

                // Apply DST start/end dates if applicable
                if((m_weather_file_cache.mWeatherHeaderInfo.mDaylightSavingsStart.x > 0)
                    && (m_weather_file_cache.mWeatherHeaderInfo.mDaylightSavingsStart.y > 0))
                {
                    m_param_block->SetValue(kMainParamID_DST, t, true);
                    m_param_block->SetValue(kMainParamID_DSTUseDateRange, t, true);
                    m_param_block->SetValue(kMainParamID_DSTStartDay, t, m_weather_file_cache.mWeatherHeaderInfo.mDaylightSavingsStart.y);
                    m_param_block->SetValue(kMainParamID_DSTStartMonth, t, m_weather_file_cache.mWeatherHeaderInfo.mDaylightSavingsStart.x);
                    m_param_block->SetValue(kMainParamID_DSTEndDay, t, m_weather_file_cache.mWeatherHeaderInfo.mDaylightSavingsEnd.y);
                    m_param_block->SetValue(kMainParamID_DSTEndMonth, t, m_weather_file_cache.mWeatherHeaderInfo.mDaylightSavingsEnd.x);
                }
                else
                {
                    // DST disabled, because no date range
                    m_param_block->SetValue(kMainParamID_DST, t, false);
                    m_param_block->SetValue(kMainParamID_DSTUseDateRange, t, false);
                }

                m_weather_data_validity = Interval(t, t);
            }

            validity &= m_weather_data_validity;
        }
    }
}

void SunPositionerObject::do_setup_weather_file_dialog(const HWND parent_hwnd)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        const AssetUser existing_weahter_file_asset = m_param_block->GetAssetUser(kMainParamID_WeatherFile, 0);

        // Setup he weather file dialog
        WeatherFileDialog dlg;
        WeatherUIData data(m_weather_file_ui_data);
        data.mFileName = existing_weahter_file_asset.GetFullFilePath();
        data.mResetTimes = (data.mFileName.Length() == 0);

        // Launch the weather file dialog
        const Interval oldRange = GetCOREInterface()->GetAnimRange();
        if (dlg.LaunchDialog(parent_hwnd, data))
        {
            m_weather_file_ui_data = data;
            IAssetManager* assetMgr = IAssetManager::GetInstance();
            if (assetMgr)
            {
                const AssetUser asset = assetMgr->GetAsset(data.mFileName, kExternalLink);
                m_param_block->SetValue(kMainParamID_WeatherFile, 0, asset);
            }
        }
        else
        {
            //set the range back we may have changed it!
            const Interval currentRange = GetCOREInterface()->GetAnimRange();
            if (currentRange != oldRange)
            {
                GetCOREInterface()->SetAnimRange(oldRange);
            }
        }
    }
}

bool SunPositionerObject::get_dst_enabled(const TimeValue t, Interval& validity) const
{
    const bool use_dst = (m_param_block->GetInt(kMainParamID_DST, t, validity) != 0);
    if(use_dst)
    {
        const bool use_dst_date_range = (m_param_block->GetInt(kMainParamID_DSTUseDateRange, t, validity) != 0);
        if(use_dst_date_range)
        {
            // Check against date
            const DayAndMonth start_date = DayAndMonth(
                m_param_block->GetInt(kMainParamID_DSTStartDay, t, validity), 
                m_param_block->GetInt(kMainParamID_DSTStartMonth, t, validity));
            const DayAndMonth end_date = DayAndMonth(
                m_param_block->GetInt(kMainParamID_DSTEndDay, t, validity),
                m_param_block->GetInt(kMainParamID_DSTEndMonth, t, validity));
            const DayAndMonth current_date = DayAndMonth(
                m_param_block->GetInt(kMainParamID_Day, t, validity),
                m_param_block->GetInt(kMainParamID_Month, t, validity));

            if(start_date <= end_date)
            {
                return (current_date >= start_date) && (current_date < end_date);
            }
            else
            {
                return (current_date >= start_date) || (current_date < end_date);
            }
        }
        else
        {
            // DST always enabled
            return true;
        }
    }
    else
    {
        return false;
    }
}

Point3 SunPositionerObject::calc_sun_direction_from_spacetime(const TimeValue t, Interval& validity) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        const int hours_ui = m_param_block->GetInt(kMainParamID_Hours, t, validity);
        const int minutes_ui = m_param_block->GetInt(kMainParamID_Minutes, t, validity);
        const int day = m_param_block->GetInt(kMainParamID_Day, t, validity);
        const int month = m_param_block->GetInt(kMainParamID_Month, t, validity);
        const int year = m_param_block->GetInt(kMainParamID_Year, t, validity);
        const float zone = m_param_block->GetFloat(kMainParamID_TimeZone, t, validity);
        const bool dst = get_dst_enabled(t, validity);
        const float latitude_deg = m_param_block->GetFloat(kMainParamID_LatitudeDeg, t, validity);
        const float longitude_deg = m_param_block->GetFloat(kMainParamID_LongitudeDeg, t, validity);
        const float north_deg = m_param_block->GetFloat(kMainParamID_NorthDeg, t, validity);

        // Take the time zone and DST into account
        const float hours_float = hours_ui + (minutes_ui / 60.0f) - zone - (dst ? 1.0f : 0.0f);
        const int hours = static_cast<int>(hours_float);
        const int minutes = static_cast<int>((hours_float - hours) * 60.0f);
        const int seconds = 0;  // cause we don't care - there's no way our model is precise enough for seconds to matter!

        const float latitude_rad = latitude_deg * (PI / 180.0f);
        const float longitude_rad = longitude_deg * (PI / 180.0f);

        double sun_altitude = 0.0f;
        double sun_azimuth = 0.0f;
        double solar_time = 0.0f;
        // Need to negate longitude - this function users west as positive
        sunLocator(latitude_rad, -longitude_rad, month, day, year, hours, minutes, seconds, &sun_altitude, &sun_azimuth, &solar_time);

        // Apply north offset
        sun_azimuth += (north_deg * (PI / 180.0f));

        // Convert altitude & azimuth to direction
        const double cos_altitude = cos(sun_altitude);
        Point3 sun_direction(
            cos_altitude * sin(sun_azimuth),
            cos_altitude * cos(sun_azimuth),
            sin(sun_altitude));

        return sun_direction.Normalize();
    }
    else
    {
        return Point3(0.0f, 0.0f, 1.0f);
    }
}
Point3 SunPositionerObject::get_sun_position(const TimeValue t, Interval& validity) const
{
    return get_sun_position(t, validity, get_sun_mode());
}

Point3 SunPositionerObject::get_sun_position(const TimeValue t, Interval& validity, const Mode mode) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        switch(mode)
        {
        case kMode_Manual:
            {
                // Use the manual transform matrix
                const Point3 sun_position = m_param_block->GetPoint3(kMainParamID_ManualSunPosition, t, validity);
                if(sun_position != Point3(0.0f, 0.0f, 0.0f))
                {
                    return sun_position;
                }
                else
                {
                    // Fallback to using the space-time calculation
                }
            }
            break;
        case kMode_SpaceTime:
            // Use the space-time calculation, below
            break;
        case kMode_WeatherFile:
            // Assume the weather data has already been pushed to the param block, and use the regular calculation below
            break;
        default:
            DbgAssert(false);
            break;
        }

        // Calculate the sun's direction from its spacetime info
        const Point3 sun_direction = calc_sun_direction_from_spacetime(t, validity);
        const float sun_distance = m_param_block->GetFloat(kMainParamID_SunDistance, t, validity);
        const Point3 sun_position = sun_direction * sun_distance;
        return sun_position;
    }
    else
    {
        return Point3(0.0f, 0.0f, 0.0f);
    }
}

Point3 SunPositionerObject::GetSunDirection(const TimeValue t, Interval& validity)  const
{
    // Derive the direction from the sun's position
    const Point3 sun_position = get_sun_position(t, validity);
    return sun_position.Normalize();
}

bool SunPositionerObject::GetWeatherMeasurements(WeatherMeasurements& measurements, const TimeValue t, Interval& validity) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        if((get_sun_mode() == kMode_WeatherFile) && (m_weather_file_status == WeatherFileStatus::LoadedSuccessfully))
        {
            // Get the weather entry for the desired frame
            const DaylightWeatherData entry = m_weather_file_cache.GetNthEntry(t);
            validity &= Interval(t, t);

            // Get the dew point temperature from the weather data
            float dry_bulb_temperature = 0.0f;
            measurements.dew_point_temperature_valid = NatLightAssembly::GetTempsLocal(entry, dry_bulb_temperature, measurements.dew_point_temperature);

            // Get the illuminance values from the weather data
            float globalHorizontalIlluminance = 0.0f;
            float zenithLuminance = 0.0f;
            if(NatLightAssembly::GetIlluminanceLocal(entry, globalHorizontalIlluminance, measurements.direct_normal_illuminance, measurements.diffuse_horizontal_illuminance, zenithLuminance))
            {
                return true;
            }
            else
            {
                // We don't have valid illuminance values, but see if we have irradiance values - which can be converted.
                float globalHorizontalRadiation = 0.0f;
                float directNormalRadiation = 0.0f;
                float diffuseHorizontalRadiation = 0.0f;
                if(NatLightAssembly::GetRadiationLocal(entry, globalHorizontalRadiation, directNormalRadiation, diffuseHorizontalRadiation))
                {
                    IPerezAllWeather* const paw = IPerezAllWeather::GetInterface();
                    if(DbgVerify(paw != nullptr))
                    {
                        // Calculate the angle between the sun and the zenith (0,0,1)
                        const Point3 sun_direction = GetSunDirection(t, validity);
                        // Apparently, the code breaks if we have a value >= to 90.0 deg. So let's clamp it to 89.999 deg.
                        const float angle_between_sun_and_zenith = std::min(acosf(sun_direction.z), 89.999f * PI / 180.0f);

                        // Calculate which day of the year it is
                        const int day_in_year = GetMDays(isleap(entry.mYear), entry.mMonth) + entry.mDay;

                        // Get values from Perez All-Weather model
                        const float sky_brightness = paw->CalcSkyBrightness(day_in_year, angle_between_sun_and_zenith, diffuseHorizontalRadiation, directNormalRadiation);
                        const float sky_clearness = paw->CalcSkyClearness(angle_between_sun_and_zenith, diffuseHorizontalRadiation, directNormalRadiation);
                        const float atmWaterContent = paw->GetAtmosphericPrecipitableWaterContent(measurements.dew_point_temperature_valid, measurements.dew_point_temperature);

                        measurements.diffuse_horizontal_illuminance = paw->GetDiffuseHorizontalIlluminance(diffuseHorizontalRadiation, sky_brightness, sky_clearness, angle_between_sun_and_zenith, atmWaterContent);
                        measurements.direct_normal_illuminance = paw->GetDirectNormalIlluminance(directNormalRadiation, sky_brightness, sky_clearness, angle_between_sun_and_zenith, atmWaterContent);

                        return true;
                    }
                }
                else
                {
                    // No usable/valid data in weather data file
                    return false;
                }
            }
        }
    }

    return false;
}

bool SunPositionerObject::GetDayOfYear(int& day_of_year, const TimeValue t, Interval& validity) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        const int day = m_param_block->GetInt(kMainParamID_Day, t, validity);
        const int month = m_param_block->GetInt(kMainParamID_Month, t, validity);
        const int year = m_param_block->GetInt(kMainParamID_Year, t, validity);
    
        day_of_year = GetMDays(isleap(year), month) + day;
        return true;
    }
    else
    {
        return false;
    }
}

SunPositionerObject::Mode SunPositionerObject::get_sun_mode() const
{
    Interval dummy_interval;
    const int mode = DbgVerify(m_param_block != nullptr) ? m_param_block->GetInt(kMainParamID_Mode, 0, dummy_interval) : kMode_Manual;
    switch(mode)
    {
    case kMode_Manual:
        return kMode_Manual;
    case kMode_SpaceTime:
        return kMode_SpaceTime;
    case kMode_WeatherFile:
        return kMode_WeatherFile;
    default:
        DbgAssert(false);
        return kMode_Manual;
    }
}

int SunPositionerObject::NumRefs() 
{
    return 1;
}

RefTargetHandle SunPositionerObject::GetReference(int i) 
{
    switch(i)
    {
    case kReferenceID_MainPB:
        return m_param_block;
    default:
        return nullptr;
    }
}

void SunPositionerObject::SetReference(int i, RefTargetHandle rtarg) 
{
    switch(i)
    {
    case kReferenceID_MainPB:
        m_param_block = dynamic_cast<IParamBlock2*>(rtarg);
        break;
    }
}

IParamBlock2* SunPositionerObject::GetParamBlock(int i) 
{
    switch(i)
    {
    case 0:
        return m_param_block;
    default:
        return nullptr;
    }
}

IParamBlock2* SunPositionerObject::GetParamBlockByID(short id) 
{
    switch(id)
    {
    case kParamBlockID_Main:
        return m_param_block;
    default:
        return nullptr;
    }
}

int SunPositionerObject::NumSubs() 
{
    return 1;
}

Animatable* SunPositionerObject::SubAnim(int i) 
{
    switch(i)
    {
    case 0:
        return m_param_block;
    default:
        return nullptr;
    }
}

TSTR SunPositionerObject::SubAnimName(int i) 
{
    switch(i)
    {
    case 0:
        return MaxSDK::GetResourceStringAsMSTR(IDS_PARAMETERS);
    default:
        return TSTR();
    }
}

int SunPositionerObject::NumSubObjTypes() 
{
    return kSubObjectLevel_Count;
}

ISubObjType* SunPositionerObject::GetSubObjType(int i) 
{
    switch(i)
    {
    case -1:
        if(GetSubObjectLevel() > 0)
        {
            return GetSubObjType(GetSubObjectLevel() - 1);
        }
        else
        {
            return nullptr;
        }
        break;
    case kSubObjectLevel_Sun:
        return &m_subobject_type_sun;
    default:
        DbgAssert(false);
        return nullptr;
    }
}

void SunPositionerObject::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc) 
{
    if(DbgVerify((cb != nullptr) && (node != nullptr)))
    {
        Matrix3 tm = node->GetObjectTM(t);	

        const int sub_obj_level = GetSubObjectLevel();
        if(sub_obj_level > 0)       // 0 == object-level
        {
            switch(sub_obj_level - 1)
            {
            case kSubObjectLevel_Sun:
                // Use the sun position
                {
                    Interval dummy_interval;
                    const Point3 sun_position = get_sun_position(t, dummy_interval);
                    tm.PreTranslate(sun_position);
                }
                break;
            default:
                DbgAssert(false);
                break;
            }
        }

        const Point3 center = tm.PointTransform(Point3(0.0f, 0.0f, 0.0f));
        cb->Center(center, 0);
    }
}

void SunPositionerObject::GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc) 
{
    if(DbgVerify((cb != nullptr) && (node != nullptr) && (m_param_block != nullptr)))
    {
        Matrix3 tm = node->GetObjectTM(t);	

        const int sub_obj_level = GetSubObjectLevel();
        if(sub_obj_level > 0)       // 0 == object-level
        {
            switch(sub_obj_level - 1)
            {
            case kSubObjectLevel_Sun:
                // Create a transform at the sun's position, looking at the compass rose's center
                {
                    Interval dummy_interval;
                    const Point3 sun_position = get_sun_position(t, dummy_interval);
                    Matrix3 sun_tm;
                    sun_tm.SetFromToUp(sun_position, Point3(0.0f, 0.0f, 0.0f), Point3(0.0f, 0.0f, 1.0f));
                    // Transform into world space
                    tm = sun_tm * tm;
                }
                break;
            default:
                DbgAssert(false);
                break;
            }
        }

        cb->TM(tm, 0);
    }
}

void SunPositionerObject::ActivateSubobjSel(int level, XFormModes& modes ) 
{
    if(level == 0) 
    {
        // Object mode: do nothing
    }
    else
    {
        switch(level - 1)
        {
        case kSubObjectLevel_Sun:
            {
                //!! TODO: Only activate if in manual mode?
                ClassDescriptor& class_desc = get_class_desc_internal();
                modes = XFormModes(
                    class_desc.m_command_mode_move.get(),   // translation
                    class_desc.m_command_mode_rotate.get(),    // rotation
                    nullptr,    // scale
                    nullptr,     // uscale
                    nullptr,     // squash
                    nullptr);    // select
            }
            break;
        default:
            DbgAssert(false);
            break;
        }
    }

    // Trigger a redraw (as selected sub-components are drawn in a different color)
    NotifyDependents(FOREVER, PART_SELECT | PART_SUBSEL_TYPE | PART_DISPLAY, REFMSG_CHANGE);
}

void SunPositionerObject::Transform(const TimeValue t, const Matrix3& partm, const Matrix3& tmAxis, const Matrix3& xfrm, const bool is_rotation)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        const int sub_object_level = GetSubObjectLevel();
        if((sub_object_level - 1) == kSubObjectLevel_Sun)
        {
            const Matrix3 tm  = partm * Inverse(tmAxis);
            const Matrix3 itm = Inverse(tm);
            const Matrix3 myxfm = tm * xfrm * itm;

            Interval dummy_validity;
            const Point3 existing_sun_position = get_sun_position(t, dummy_validity);
            //const Point3 position_offset = myxfm.PointTransform(Point3(0.0f, 0.0f, 0.0f));
            //const Point3 new_sun_position = existing_sun_position + position_offset;
            const Point3 new_sun_position = 
                is_rotation
                ? myxfm.VectorTransform(existing_sun_position)
                : myxfm.PointTransform(existing_sun_position);

            // Force the mode to manual when manipulated by the user
            m_param_block->SetValue(kMainParamID_Mode, t, kMode_Manual);

            m_param_block->SetValue(kMainParamID_ManualSunPosition, t, new_sun_position);
        }
    }
}

void SunPositionerObject::Move( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL /*localOrigin*/) 
{
    Transform(t, partm, tmAxis, TransMatrix(val), false);
}

void SunPositionerObject::Rotate( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL /*localOrigin*/) 
{
    // Transform the quaternion to a rotation matrix
    Matrix3 mat;
    val.MakeMatrix(mat);

    Transform(t, partm, tmAxis, mat, true);
}

void SunPositionerObject::Scale( TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin) 
{
    // Scale not supported
}

void SunPositionerObject::TransformCancel(TimeValue t) 
{
    //!! TODO
}

bool SunPositionerObject::get_show_compass(const TimeValue t) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        return (m_param_block->GetInt(kMainParamID_ShowCompass, t) != 0);
    }
    else
    {
        return false;
    }
}

float SunPositionerObject::get_compass_radius(const TimeValue t, Interval& valid) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        return m_param_block->GetFloat(kMainParamID_CompassRadius, t, valid);
    }
    else
    {
        return 0.0f;
    }
}

int SunPositionerObject::get_current_year()
{
    SYSTEMTIME current_time;
    GetLocalTime(&current_time);
    return current_time.wYear;
}

void SunPositionerObject::install_sunsky_environment(const bool overwrite_existing_environment, const bool redirect_existing_sunsky_environment) const
{
    // Find a node that references this sun positioner widget
    INode* const node = GetCOREInterface7()->FindNodeFromBaseObject(const_cast<SunPositionerObject*>(this));
    DbgAssert(node != nullptr);

    // Get the current environment map
    Texmap* const env_map = GetCOREInterface()->GetEnvironmentMap();
    PhysicalSunSkyEnv* sunsky_env = dynamic_cast<PhysicalSunSkyEnv*>(env_map);
    if(sunsky_env == nullptr)
    {
        // Overwrite existing environment only if desired
        if((env_map == nullptr) || overwrite_existing_environment)
        {
            theHold.Begin();

            sunsky_env = new PhysicalSunSkyEnv(false);
            sunsky_env->SetName(sunsky_env->ClassName());       // set name, otherwise the name will be empty
            // Make the new environment point to this sun positioner widget
            sunsky_env->set_sun_positioner_object(node);
            GetCOREInterface()->SetEnvironmentMap(sunsky_env);
            
            // Make sure the environment map is enabled
            GetCOREInterface()->SetUseEnvironmentMap(true);

            // Install an exposure control, if there isn't one already
            ToneOperatorInterface* const toneOpInt = dynamic_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
            ToneOperator* toneOp = (toneOpInt != nullptr) ? toneOpInt->GetToneOperator() : nullptr;
            if((toneOpInt!= nullptr) && (toneOp == nullptr))
            {
                // Create a new instance of the physical camera exposure control
                toneOp = static_cast<ToneOperator*>(GetCOREInterface()->CreateInstance(TONE_OPERATOR_CLASS_ID, IPhysicalCameraToneOperator::GetClassID()));
                if(toneOp != nullptr)
                {
                    toneOpInt->SetToneOperator(toneOp);

                    // Set a realistic EV for a daylight scene
                    IPhysicalCameraToneOperator* const phys_tone_op = dynamic_cast<IPhysicalCameraToneOperator*>(toneOp);
                    if(DbgVerify(phys_tone_op != nullptr))
                    {
                        IParamBlock2* const pblock = phys_tone_op->GetParamBlock(0);
                        if(DbgVerify(pblock != nullptr))
                        {
                            // Disable animation
                            AnimateSuspend animate_suspend;
                            pblock->SetValueByName(_T("use_global_ev"), 1, 0);
                            pblock->SetValueByName(_T("global_ev"), 15.0f, 0);
                        }
                    }
                }
            }

            // Enable the "process env/background" flag on the exposure control
            if(toneOp != nullptr)
            {
                toneOp->SetProcessBackground(true);
            }

            theHold.Accept(MSTR(QObject::tr("Setup Sun & Sky Environment")));
        }
    }
    else
    {
        // Re-direct the existing sky env. only if desired, or if the env. doesn't currently point to a sun positioner.
        if(redirect_existing_sunsky_environment || (sunsky_env->get_sun_positioner_node() == nullptr))
        {
            sunsky_env->set_sun_positioner_object(node);
        }
    }
}


//==================================================================================================
// class SunPositionerObject::ClassDescriptor
//==================================================================================================

int SunPositionerObject::ClassDescriptor::IsPublic() 
{
    return true; 
}

void* SunPositionerObject::ClassDescriptor::Create(BOOL loading) 
{
    return static_cast<LightObject*>(new SunPositionerObject(loading != 0)); 
}

const TCHAR* SunPositionerObject::ClassDescriptor::ClassName() 
{
    static MSTR str = QObject::tr("Sun Positioner"); 
    return str;
}   

SClass_ID SunPositionerObject::ClassDescriptor::SuperClassID() 
{
    return LIGHT_CLASS_ID; 
}

Class_ID SunPositionerObject::ClassDescriptor::ClassID() 
{
    return ISunPositioner::GetClassID();
}

const TCHAR* SunPositionerObject::ClassDescriptor::Category() 
{ 
    static const MSTR str = QObject::tr("Photometric");
    return str;
}       

HINSTANCE SunPositionerObject::ClassDescriptor::HInstance() 
{ 
    return MaxSDK::GetHInstance(); 
}

const MCHAR* SunPositionerObject::ClassDescriptor::InternalName()
{
    return _T("SunPositioner");
}

MaxSDK::QMaxParamBlockWidget* SunPositionerObject::ClassDescriptor::CreateQtWidget(
    ReferenceMaker& owner,
    IParamBlock2& paramBlock,
    const MapID paramMapID,  
    MSTR& rollupTitle, 
    int& rollupFlags, 
    int& rollupCategory) 
{
    switch(paramMapID)
    {
    case kParamMapID_Display:
        {
            rollupTitle = MaxSDK::GetResourceStringAsMSTR(IDS_SUNPOS_DISPLAY_ROLLOUT_TITLE);

            // Simple/dummy wrapper, don't care about the param block
            class MyWidgetClass : public MaxSDK::QMaxParamBlockWidget
            {
            public:
                virtual void SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const /*param_block*/) override {}
                virtual void UpdateUI(const TimeValue /*t*/) override {}
                virtual void UpdateParameterUI(const TimeValue /*t*/, const ParamID /*param_id*/, const int /*tab_index*/) override {}
            };

            // Create the host Qt dialog
            MyWidgetClass* qt_dialog = new MyWidgetClass();
            Ui_SunPositioner_Display ui_builder;
            ui_builder.setupUi(qt_dialog);

            return qt_dialog;
        }
        break;
    case kParamMapID_SunPosition:
        {
            rollupTitle = MaxSDK::GetResourceStringAsMSTR(IDS_SUNPOS_SPACETIME_ROLLOUT_TITLE);
            SunPositionRollupWidget* const widget = new SunPositionRollupWidget(paramBlock);
            return widget;
        }
        break;
    default:
        DbgAssert(false);
        return nullptr;
    }
}


//==================================================================================================
// class SunPositionerObject::ParamBlockAccessor 
//==================================================================================================

void SunPositionerObject::ParamBlockAccessor::Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) 
{
    SunPositionerObject* const sun_pos = dynamic_cast<SunPositionerObject*>(owner);
    IParamBlock2* const param_block = (sun_pos != nullptr) ? sun_pos->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case kMainParamID_Day:
        case kMainParamID_Month:
        case kMainParamID_Year:
            // Convert from julian day
            {
                const int julian_day = param_block->GetInt(kMainParamID_JulianDay, t, valid);
                int day = 0, month = 0, year = 0;
                julian2gregorian_int(julian_day, &month, &day, &year);
                switch(id)
                {
                case kMainParamID_Day:
                    v.i = day;
                    break;
                case kMainParamID_Month:
                    v.i = month;
                    break;
                case kMainParamID_Year:
                    v.i = year;
                    break;
                }
            }
            break;

        case kMainParamID_Hours:
        case kMainParamID_Minutes:
            // Convert from 'seconds from midnight'
            {
                const int seconds_from_midnight = param_block->GetInt(kMainParamID_TimeInSeconds, t, valid);
                const int hours = seconds_from_midnight / 3600;
                const int minutes = (seconds_from_midnight % 3600) / 60;
                switch(id)
                {
                case kMainParamID_Hours:
                    v.i = hours;
                    break;
                case kMainParamID_Minutes:
                    v.i = minutes;
                    break;
                }
            }
            break;
        case kMainParamID_AzimuthDeg:
        case kMainParamID_AltitudeDeg:
            {
                // Calculate altitude & azimuth from the current sun direction
                const Point3 sun_direction = sun_pos->GetSunDirection(t, valid);
                const float north_deg = param_block->GetFloat(kMainParamID_NorthDeg, t, valid);
                const float altitude_rad = asin(sun_direction.z);
                const float azimuth_rad = angle(sun_direction.x, sun_direction.y);
                switch(id)
                {
                case kMainParamID_AzimuthDeg:
                    v.f = (azimuth_rad * (180.0f / PI)) - north_deg;
                    // Clip to 0-360deg
                    while(v.f < 0.0f)
                    {
                        v.f += 360.0f;
                    }
                    while(v.f >= 360.0f)
                    {
                        v.f -= 360.0f;
                    }
                    break;
                case kMainParamID_AltitudeDeg:
                    v.f = (altitude_rad * (180.0f / PI));
                    break;
                }
            }
            break;
        }
    }
}

void SunPositionerObject::ParamBlockAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) 
{
    SunPositionerObject* const sun_pos = dynamic_cast<SunPositionerObject*>(owner);
    IParamBlock2* const param_block = (sun_pos != nullptr) ? sun_pos->m_param_block : nullptr;
    if(!m_disable_set && DbgVerify(param_block != nullptr))
    {
        Interval dummy_validity;
        switch(id)
        {
		case kMainParamID_Day:
		case kMainParamID_Month:
		case kMainParamID_Year:
            // Convert input date to julian date
            {
                // Convert julian day to d/m/y
                const int previous_julian_day = param_block->GetInt(kMainParamID_JulianDay, t, dummy_validity);
                int day = 0, month = 0, year = 0;
                julian2gregorian_int(previous_julian_day, &month, &day, &year);
                switch(id)
                {
                case kMainParamID_Day:
                    day = v.i;
                    break;
                case kMainParamID_Month:
                    month = v.i;
                    break;
                case kMainParamID_Year:
                    year = v.i;
                    break;
                }

                // Validate year
				year = qBound< int >( MINYEAR, year, MAXYEAR );

				// Validate month
				month = qBound< int >( 1, month, 12 );

				// Validate day
				const int month_length = GetMonthLengthInDays( month, year );
				day = qBound< int >( 1, day, month_length );

                // Convert back to julian date
                const int new_julian_day = gregorian2julian_int(month, day, year);
                param_block->SetValue(kMainParamID_JulianDay, t, new_julian_day);
            }
            break;
        case kMainParamID_Hours:
        case kMainParamID_Minutes:
            // Convert input to seconds from midnight
            {
                const int previous_time_in_seconds = param_block->GetInt(kMainParamID_TimeInSeconds, t, dummy_validity);
                int hours = previous_time_in_seconds / 3600;
                int minutes = (previous_time_in_seconds % 3600) / 60;
                switch(id)
                {
                case kMainParamID_Hours:
                    hours = v.i;
                    break;
                case kMainParamID_Minutes:
                    minutes = v.i;
                    break;
                }

				// Validate hours
				hours = qBound< int >( 0, hours, 23 );

				// Validate minutes
				minutes = qBound< int >( 0, minutes, 59 );

                // Convert back
                const int new_time_in_seconds = (hours * 3600) + (minutes * 60);
                param_block->SetValue(kMainParamID_TimeInSeconds, t, new_time_in_seconds);
            }
            break;
        case kMainParamID_AzimuthDeg:
        case kMainParamID_AltitudeDeg:
            // Update the manual sun position based on the specified altitude & azimuth
            {
                const float azimuth_deg = 
                    (id == kMainParamID_AzimuthDeg)
                    ? v.f
                    : param_block->GetFloat(kMainParamID_AzimuthDeg, t, dummy_validity);
                const float altitude_deg = 
                    (id == kMainParamID_AltitudeDeg)
                    ? v.f
                    : param_block->GetFloat(kMainParamID_AltitudeDeg, t, dummy_validity);
                const float north_deg = param_block->GetFloat(kMainParamID_NorthDeg, t, dummy_validity);
                const float azimuth_rad = (azimuth_deg + north_deg) * (PI / 180.0f);
                const float altitude_rad = altitude_deg * (PI / 180.0f);

                // Calculate the new sun direction
                const float cos_altitude = cosf(altitude_rad);
                const Point3 new_sun_direction(
                    cos_altitude * sin(azimuth_rad),
                    cos_altitude * cos(azimuth_rad),
                    sin(altitude_rad));

                // Calculate the new sun position
                const Point3 old_sun_position = sun_pos->get_sun_position(t, dummy_validity);
                const float sun_distance = old_sun_position.Length();
                const Point3 new_sun_position = new_sun_direction * sun_distance;

                // Set the new (manual) position
                param_block->SetValue(kMainParamID_Mode, t, kMode_Manual);
                param_block->SetValue(kMainParamID_ManualSunPosition, t, new_sun_position);
            }
            break;
        case kMainParamID_WeatherFile:
            // Force re-load of weather data file
            sun_pos->m_weather_file_status = WeatherFileStatus::NotLoaded;
            break;
        case kMainParamID_LatitudeDeg:
        case kMainParamID_LongitudeDeg:
            {
                const float longitude = (id == kMainParamID_LongitudeDeg) ? v.f : param_block->GetFloat(kMainParamID_LongitudeDeg, t);
                const float latitude = (id == kMainParamID_LatitudeDeg) ? v.f : param_block->GetFloat(kMainParamID_LatitudeDeg, t);

                // When manually changing coordinates, see if the coordinates match a known city
                MSTR city_name;
                CityList& city_list = CityList::GetInstance();
                city_list.init();
                for(int i = 0; i < city_list.Count(); ++i)
                {
                    const CityList::Entry& entry = city_list[i];
                    if((longitude == entry.longitude) && (latitude == entry.latitude))
                    {
                        city_name = entry.get_ui_name();
                        break;
                    }
                }

                if(city_name.Length() == 0)
                {   
                    city_name = MaxSDK::GetResourceStringAsMSTR(IDS_SUNPOS_CHOOSELOCATION);
                }
                param_block->SetValue(kMainParamID_Location, 0, city_name);
            }
            break;
        case kMainParamID_DSTStartDay:
        case kMainParamID_DSTEndDay:
        case kMainParamID_DSTStartMonth:
        case kMainParamID_DSTEndMonth:
            {
                // The code below is generic, shared for both DST start and end dates. We determine which parameters we're concerned with here.
                const MainParamID day_param_id = ((id == kMainParamID_DSTStartDay) || (id == kMainParamID_DSTStartMonth)) ? kMainParamID_DSTStartDay : kMainParamID_DSTEndDay;
                const MainParamID month_param_id = ((id == kMainParamID_DSTStartDay) || (id == kMainParamID_DSTStartMonth)) ? kMainParamID_DSTStartMonth : kMainParamID_DSTEndMonth;

                // Validate that day for the selected month
                int month = (id == month_param_id) ? v.i : param_block->GetInt(month_param_id, t, dummy_validity);
                int day = (id == day_param_id) ? v.i : param_block->GetInt(day_param_id, t, dummy_validity);

				// Validate month
				month = qBound< int >( 1, month, 12 );

				// Validate day
                const int max_day = GetMonthLengthInDays( month, true );     // assume leap year, since we don't have a year associated with this date
				day = qBound< int >( 1, day, max_day );

                // Disable Set() to avoid recursive loop
                const bool old_disable_set = m_disable_set;
                m_disable_set = true;
                if(id == day_param_id)
                {
                    v.i = day;
                    param_block->SetValue(month_param_id, t, month);
                }
                else
                {
                    param_block->SetValue(day_param_id, t, day);
                    v.i = month;
                }
                m_disable_set = old_disable_set;
            }
            break;
        }

        // Invalidate altitude + azimuth on any parameter that may change the position
        switch(id)
        {
        case kMainParamID_Mode:
        case kMainParamID_DST:
        case kMainParamID_DSTUseDateRange:
        case kMainParamID_DSTStartDay:
        case kMainParamID_DSTStartMonth:
        case kMainParamID_DSTEndDay:
        case kMainParamID_DSTEndMonth:
        case kMainParamID_LatitudeDeg:
        case kMainParamID_LongitudeDeg:
        case kMainParamID_ManualSunPosition:
        case kMainParamID_JulianDay:
        case kMainParamID_TimeInSeconds:
        case kMainParamID_NorthDeg:
            {
                ParamBlockDesc2* const pb_desc = param_block->GetDesc();
                if(DbgVerify(pb_desc != nullptr))
                {
                    pb_desc->InvalidateUI(kMainParamID_AzimuthDeg);
                    pb_desc->InvalidateUI(kMainParamID_AltitudeDeg);
                }
            }
            break;
        }
    }
}

//==================================================================================================
// class SunPositionerObject::CreateCallBack 
//==================================================================================================

void SunPositionerObject::CreateCallBack::SetObj(SunPositionerObject *obj)
{
    ob = obj;
}

int SunPositionerObject::CreateCallBack::proc(
    ViewExp *vpt,
    int msg,
    int point,
    int flags,
    IPoint2 m,
    Matrix3& mat)
{
    if(!vpt || !vpt->IsAlive())
    {
        // why are we here
        DbgAssert(!_T("Invalid viewport!"));
        return FALSE;
    }

    if(msg == MOUSE_POINT)
    {
        switch(point)
        {
        case 0:
            // First click defines the location of the center of the compass rose.
            ob->m_suspendSnap = true;
            obj_pos_in_world_space = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
            mat.SetTrans(obj_pos_in_world_space);
            m_obj_pos_in_screen_space =  m;
            break;
        case 1:
            // Second click defines the radius of the compass rose
            break;
        case 2:
            // Third click defines the rotation of the compass rose
            break;
        case 3:
            // Fourth and final click defines the distance of the sun
            // Install a new sun & sky environment automatically, if and only if one doesn't exist already, upon object creation
            if(ob != nullptr)
            {
                ob->m_suspendSnap = false;
                ob->install_sunsky_environment(false, false);
            }
            return CREATE_STOP;
        }
    }
    else if(msg == MOUSE_MOVE)
    {
        switch(point)
        {
        case 1:
        {
            // First drag defines the radius of the compass rose
            const Point3 p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
            const float axis_length = std::max(10.0f, Length(p1 - obj_pos_in_world_space));
            if(DbgVerify(ob->m_param_block != nullptr))
            {
                ob->m_param_block->SetValue(kMainParamID_CompassRadius, GetCOREInterface()->GetTime(), axis_length);
            }
            break;
        }
        case 2:
        {
            // Second drag rotates the compass rose in 10 degree increments
            const Point3 p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
            const Point3 direction = Normalize(Point3(p1 - obj_pos_in_world_space));
            float angle_rad = acosf(DotProd(direction, Point3(0.0f, 1.0f, 0.0f)));
            const Point3 cross_prod = CrossProd(direction, Point3(0.0f, 1.0f, 0.0f));
            const bool negate_angle = DotProd(cross_prod, Point3(0.0f, 0.0f, 1.0f)) < 0.0f;
            if(negate_angle)
                angle_rad = -angle_rad;
            const float angle_deg = angle_rad * (180.0f / PI);
            const float angle_deg_rounded = 10.0f * round(angle_deg * 0.1f);
            if(DbgVerify(ob->m_param_block != nullptr))
            {
                ob->m_param_block->SetValue(kMainParamID_NorthDeg, GetCOREInterface()->GetTime(), angle_deg_rounded);
            }
            break;
        }
        case 3:
            // Third drag adjusts the distance of the sun
            const Point3 sun_direction = ob->GetSunDirection(GetCOREInterface()->GetTime(), Interval());
            const float sun_distance = fabsf(vpt->SnapLength(vpt->GetCPDisp(obj_pos_in_world_space, sun_direction, m_obj_pos_in_screen_space, m)));
            if(DbgVerify(ob->m_param_block != nullptr))
            {
                ob->m_param_block->SetValue(kMainParamID_SunDistance, GetCOREInterface()->GetTime(), sun_distance);
            }
            break;
        }
    }
    else if(msg == MOUSE_ABORT)
    {
        return CREATE_ABORT;
    }
    else if(msg == MOUSE_FREEMOVE)
    {
        vpt->SnapPreview(m, m, NULL, SNAP_IN_PLANE);
    }

    return CREATE_CONTINUE;
}

//==================================================================================================
// struct SunPositionerObject::WeatherMeasurements
//==================================================================================================

SunPositionerObject::WeatherMeasurements::WeatherMeasurements()
    : diffuse_horizontal_illuminance(0.0f),
    direct_normal_illuminance(0.0f),
    dew_point_temperature(0.0f),
    dew_point_temperature_valid(false)
{
}