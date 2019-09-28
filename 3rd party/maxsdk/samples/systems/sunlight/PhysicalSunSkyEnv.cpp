//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "PhysicalSunSkyEnv.h"

// local
#include "PhysicalSunSkyEnv_UI.h"
#include "sunlight.h"
#include "PhysicalSunSkyShader.h"

// max sdk
#include <dllutilities.h>
#include <iparamm2.h>
#include <toneop.h>
#include <DaylightSimulation/IPerezAllWeather.h>
#include <DaylightSimulation/ISunPositioner.h>
#include <IRTShaderNode.h>

#include <d3dx9.h>

#undef min
#undef max

using namespace MaxSDK;
using namespace Max::RapidSLShaders;

namespace
{
    double square(const double val)
    {
        return val * val;
    }

    double cube(const double val)
    {
        return val * val * val;
    }

    // Returns the sun's illuminance in lux
    Color calculate_sun_illuminance(const Point3& sun_direction, const float turbidity)
    {
        if(sun_direction.z >= 0.0f)
        {
            // Approximated sRGB primaries wavelengths, from "Color Imaging" by Reinhard et al., page 391, in *micrometers*
            const Point3 wavelengths = float3(0.605f, 0.540f, 0.445f);
            // Coefficients for the chosen wavelengths, in m^-1, extracted from Preetham paper (table 2)
            const float3 k0 = float3(12.25f, 7.5f, 0.3f);
            const float l = 0.0035f;     // Value suggested by Preetham, converted from cm to m
            const float alpha = 1.3f;
            const float beta = 0.04608f * turbidity - 0.04586f;
            const float m = static_cast<float>(1.0 / (sun_direction.z + 0.15 * ::pow((93.885 - acos(sun_direction.z) * (180.0 / PI)), -1.253)));

            // Calculate the various transmission coefficients from Preetham's paper. Note that there's an error in the paper, which the code sample fixes:
            // m isn't part of the exponent.
            const float3 transmission_rayleight = exp(-m * 0.008735f * pow(wavelengths, -4.08f));
            const float3 transmission_aeorosol = exp(-m * beta * pow(wavelengths, -alpha));
            const float3 transmission_ozone = exp(-k0 * l * m);
            // Transmission for mixed gases and water vapour are omitted because, at the chose wavelengths, they have no impact (their coefficients
            // in the Preetham paper are 0).

            // Calculate the sun color
            // The illuminance of the sun is 128e3 lux before it goes through the atmosphere (http://en.wikipedia.org/wiki/Sunlight)
            // The color of the sun above the atmosphere is about 5900K (http://en.wikipedia.org/wiki/Color_temperature#The_Sun)
            const float3 sun_color = float3(1.05513096f, 0.993359745f, 0.903543472f);
            float3 sun_luminance = 128e3f * sun_color / rgb_luminance(sun_color);

            // Apply atmospheric transmission to the sun color
            sun_luminance *= transmission_rayleight * transmission_aeorosol * transmission_ozone;
            return sun_luminance;
        }
        else
        {
            return float3(0.0f, 0.0f, 0.0f);
        }
    }

    // Calculates the integral for the sun's density function
    float calculate_sun_contribution_integral(
        const float sun_disc_angular_radius,
        const float sun_smooth_angular_radius,
        const float sun_glow_angular_radius,
        const float sun_glow_multiplier)
    {
        // Integrate the sun contribution in three parts (all solved with Wolfram Alpha using the expressions below):
        // * the solid sun disc: integral[0 to a]sin(x) dx
        // * the smooth sun disc edge: integral[a to b]((-cos(s*PI) + 1.0) * 0.5) * sin(x) dx, where s=(x - b) / (a - b)
        //                           = integral[a to b]((-0.5*cos((x - b) / (a - b)*PI) + 0.5)) * sin(x) dx
        // * the glow: integral[0 to c] s exp(-(1.0 - s) 6.0) sin(x) dx, where s=(1.0 - (x / c))
        //           = integral[0 to c] s exp(6s - 6) sin(x) dx, where s=(1.0 - (x / c))
        // Note that this integral is dependent on the sun/glow interpolation used in the shader itself!
        // Note: it's crucial to use doubles here - floats don't have enough precision and provide bogus results.
        const double a = sun_disc_angular_radius;
        const double b = sun_smooth_angular_radius;
        const double c = sun_glow_angular_radius;
        const double pi = PI;
        const double sun_disc_integral = 1.0 - cos(a);
        const double sun_smooth_integral = ((square(pi) - 2 * square(a - b))*cos(a) - square(pi)*cos(b)) / (2 * (a - b + pi)*(-a + b + pi));
        const double sun_glow_integral = (c*((0.0892351 - 0.00247875*square(c))*sin(c) + cube(c) + 24.*c + 0.029745*c*cos(c))) / square((square(c) + 36.));

        // Our integrations were performed over sin(x), which integrates to 2 over the sphere. We want to scale to steradians, which should integrate
        // to 4PI over the sphere, so we must multiply by 2PI.
        const double sun_contribution_integral = 2.0 * PI * (
            sun_disc_integral
            + sun_smooth_integral
            + sun_glow_multiplier * sun_glow_integral);

        return sun_contribution_integral;
    }

    // Estimates the diffuse illuminance, on a horizontal surface, from the sky by way of sampling.
    // The returned value is an RGB illuminance. The luminance (Y) of the color is returned as a separate parameter reference.
    float3 estimate_diffuse_horizontal_sky_illuminance(
        const float3 sun_direction,
        const float3 night_luminance,
        const float night_falloff,
        const Point3& perez_A, const Point3& perez_B, const Point3& perez_C, const Point3& perez_D, const Point3& perez_E, const Point3& perez_Z,
        float& Y)
    {
        float3 result = float3(0.0);
        Y = 0.0f;

        // Generate a bunch of samples, following a lambertian distribution
        const unsigned int num_horizontal_samples = 30;
        const unsigned int num_vertical_samples = num_horizontal_samples / 2;
        for(unsigned int horizontal_sample_index = 0; horizontal_sample_index < num_horizontal_samples; ++horizontal_sample_index)
        {
            const float horizontal_sample = horizontal_sample_index / float(num_horizontal_samples);
            const float horizontal_angle = (horizontal_sample * 2.0 * PI);
            for(unsigned int vertical_sample_index = 0; vertical_sample_index < num_vertical_samples; ++vertical_sample_index)
            {
                const float vertical_sample = (vertical_sample_index / float(num_vertical_samples));
                const float z = 1.0 - vertical_sample; // 1.0-x, to force sampling the zenith and disable sampling the horizon
                const float x = sqrt(1.0 - z*z) * cos(horizontal_angle);
                const float y = sqrt(1.0 - z*z) * sin(horizontal_angle);
                const float3 sample_direction = float3(x, y, z);

                const float cosine_angle_between_sun_and_ray_directions = dot(sample_direction, sun_direction);
                const float angle_between_sun_and_ray_directions = acos(cosine_angle_between_sun_and_ray_directions);
                const float3 contrib_rgb = compute_sky_contribution_rgb(
                    sun_direction,
                    sample_direction,
                    angle_between_sun_and_ray_directions, cosine_angle_between_sun_and_ray_directions,
                    night_luminance,
                    night_falloff,
                    perez_A, perez_B, perez_C, perez_D, perez_E, perez_Z);
                const float3 perez_Yxy = compute_perez_sky_Yxy(
                    sun_direction,
                    sample_direction,
                    angle_between_sun_and_ray_directions, cosine_angle_between_sun_and_ray_directions,
                    night_falloff,
                    perez_A, perez_B, perez_C, perez_D, perez_E, perez_Z);

                result += sample_direction.z * contrib_rgb;
                Y += perez_Yxy.x;
            }
        }

        // Multiply by PI to complete integration of cosine (which integrates to PI)
        result *= static_cast<float>(PI / (num_horizontal_samples * num_vertical_samples));
        Y *= static_cast<float>(PI / (num_horizontal_samples * num_vertical_samples));
        return result;
    }
}

//==================================================================================================
// class PhysicalSunSkyEnv
//==================================================================================================

PhysicalSunSkyEnv::ParamBlockAccessor PhysicalSunSkyEnv::m_pb_accessor;

PhysicalSunSkyEnv::ShaderGenerator PhysicalSunSkyEnv::m_shader_generator(
    IShaderGenerator::GetInterfaceID(), _T("PhysSunSky_ShaderGenerator"), 0, nullptr, FP_CORE,
    // No published properties
    p_end);

ParamBlockDesc2 PhysicalSunSkyEnv::m_pb_desc(
    kParamBlockID_Main, 
    _T("Parameters"), IDS_PARAMETERS,
    &get_class_descriptor(), 
    P_AUTO_CONSTRUCT | P_AUTO_UI_QT,

    // Reference ID
        kReferenceID_MainPB,

    // AutoUI: nothing (because of Qt UI)

    // Parameters
    //!! TODO: Setup localized strings
    kMainParamID_SunPositionObject, _T("sun_position_object"), TYPE_INODE, 0, IDS_PHYSSUNSKY_SUNPOSITIONOBJECT,
        p_classID, Class_ID(ISunPositioner::GetClassID()),      // Only allow sun positioner objects to be picked
        p_end,
    kMainParamID_GlobalIntensity, _T("global_intensity"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_GLOBALINTENSITY,
        p_default, 1.0f,
        p_range, 0.0f, 1e6f,
        p_end,
    kMainParamID_Haze, _T("haze"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_HAZE,
        p_default, 0.0f,
        p_range, 0.0f, 1.0f,
        p_end,
    kMainParamID_SunEnabled, _T("sun_enabled"), TYPE_BOOL, P_ANIMATABLE, IDS_PHYSSUNSKY_SUNENABLED,
        p_default, true,
        p_end,
    kMainParamID_DiscIntensity, _T("sun_disc_intensity"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_DISCINTENSITY,
        p_default, 1.0f,
        p_range, 0.0f, 1e6f,
        p_end,
    kMainParamID_DiscGlowIntensity, _T("sun_glow_intensity"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_GLOWINTENSITY,
        p_default, 1.0f,
        p_range, 0.0f, 1e6f,
        p_end,
    kMainParamID_DiscScale, _T("sun_disc_scale"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_DISCSCALE,
        p_default, 1.0f,
        p_range, 0.10f, 100.0f,
        p_end,
    kMainParamID_DiscScale_Percent, _T("sun_disc_scale_percent"), TYPE_FLOAT, P_TRANSIENT, IDS_PHYSSUNSKY_DISCSCALE_PERCENT,
        p_accessor, &m_pb_accessor,
        p_range, 10.0f, 10000.0f,
        p_end,
    kMainParamID_SkyIntensity, _T("sky_intensity"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_SKYINTENSITY,
        p_default, 1.0f,
        p_range, 0.0f, 1e6f,
        p_end,
    kMainParamID_NightColor, _T("night_color"), TYPE_FRGBA, P_ANIMATABLE, IDS_PHYSSUNSKY_NIGHTCOLOR,
        p_default, AColor(1.0f, 1.0f, 1.0f),
        p_end,
    kMainParamID_NightIntensity, _T("night_intensity"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_NIGHTINTENSITY,
        p_default, 1.0f,        // 1 cd/m^2 (https://en.wikipedia.org/wiki/Orders_of_magnitude_(luminance) says 1mcd/m^2, but here we're assuming a well lit/hazy/polluted sky).
        p_range, 0.0f, 1e20f,
        p_end,
    kMainParamID_HorizonHeight, _T("horizon_height_deg"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_HORIZONHEIGHT,
        p_default, 0.0f,
        p_range, -45.0f, 45.0f,
        p_end,
    kMainParamID_HorizonBlur, _T("horizon_blur_deg"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_HORIZONBLUR,
        p_default, 0.1f,    //!! TODO: validate this default, or find a better one
        p_range, 0.0f, 45.0f,
        p_end,
    kMainParamID_GroundColor, _T("ground_color"), TYPE_FRGBA, P_ANIMATABLE, IDS_PHYSSUNSKY_GROUNDCOLOR,
        p_default, AColor(0.2f, 0.2f, 0.2f),
        p_end,
    kMainParamID_Saturation, _T("saturation"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_SATURATION,
        p_default, 1.0f,
        p_range, 0.0f, 2.0f,
        p_end,
    kMainParamID_Tint, _T("tint"), TYPE_FRGBA, P_ANIMATABLE, IDS_PHYSSUNSKY_TINT,
        p_default, AColor(1.0f, 1.0f, 1.0f),
        p_end,
    kMainParamID_IlluminanceModel, _T("illuminance_model"), TYPE_INT, 0, IDS_PHYSSUNSKY_ILLUMINANCE_MODEL,
        p_default, kIlluminanceModel_Automatic,
        p_end,
    kMainParamID_PerezDiffuseHorizontalIlluminance, _T("perez_diffuse_horizontal_illuminance"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_PEREZ_DIFFHORILL,
        p_default, 10000.0f,    // in lux
        p_range, 0.0f, 1e12f,
        p_accessor, &m_pb_accessor,
        p_end,
    kMainParamID_PerezDirectNormalIlluminance, _T("perez_direct_normal_illuminance"), TYPE_FLOAT, P_ANIMATABLE, IDS_PHYSSUNSKY_PEREZ_DIRNORILL,
        p_default, 50000.0f,    // in lux
        p_range, 0.0f, 1e12f,
        p_accessor, &m_pb_accessor,
        p_end,
    p_end
);

PhysicalSunSkyEnv::PhysicalSunSkyEnv(const bool loading)
    : m_param_block(nullptr)
{
    if(!loading)
    {
        get_class_descriptor().MakeAutoParamBlocks(this);
    }
}

PhysicalSunSkyEnv::~PhysicalSunSkyEnv()
{

}

INode* PhysicalSunSkyEnv::get_sun_positioner_node() const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        return m_param_block->GetINode(kMainParamID_SunPositionObject, 0);
    }
    else
    {
        return nullptr;
    }
}

ISunPositioner* PhysicalSunSkyEnv::get_sun_positioner_object(const TimeValue t) const
{
    // Fetch the node
    INode* const sun_positioner_node = get_sun_positioner_node();
    if(sun_positioner_node != nullptr)
    {
        // Evaluate the node
        const ObjectState object_state = sun_positioner_node->EvalWorldState(t);

        // Fetch the underlying object
        ISunPositioner* const sun_positioner = dynamic_cast<ISunPositioner*>(object_state.obj);
        DbgAssert(sun_positioner == object_state.obj);      // Only sun positioner objects should be assignable
        return sun_positioner;
    }
    else
    {
        return nullptr;
    }
}

void PhysicalSunSkyEnv::set_sun_positioner_object(INode* node)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        DbgVerify(m_param_block->SetValue(kMainParamID_SunPositionObject, 0, node));
    }
}

AColor PhysicalSunSkyEnv::EvalColor(ShadeContext& sc)
{
    if(DbgVerify(m_shader != nullptr))
    {
       
        if(sc.mode != SCMODE_TEXMAP)
        {
            const Color col = m_shader->Evaluate(sc.VectorTo(sc.V(), REF_WORLD));
            return AColor(col, 0.0f);
        }
        else
        {
            // Project top hemisphere onto UVW plane
            // Stereographic projection: https://en.wikipedia.org/wiki/Stereographic_projection
            const Point3 uvw = sc.UVW();
            const float u = (uvw.x - 0.5f) * 2.0f;  // [-1,1]
            const float v = (uvw.y - 0.5f) * 2.0f;  // [-1,1]
            const float x = (2.0f * u);
            const float y = (2.0f * v);
            const float z = -(-1.0f + u*u + v*v);
            const Point3 direction = Point3(x, y, z) / (1.0f + u*u + v*v);

            Color col = m_shader->Evaluate(direction);

            // Apply exposure control
            ToneOperatorInterface* const toneOpInt = dynamic_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
            ToneOperator* const toneOp = (toneOpInt != nullptr) ? toneOpInt->GetToneOperator() : nullptr;
            if(toneOp != nullptr)
            {
                toneOp->ScaledToRGB(col);
            }

            return AColor(col, 0.0f);
        }
    }
    else
    {
        // Update() wasn't called!
        return AColor(0.0f, 0.0f, 0.0f);
    }
}

Point3 PhysicalSunSkyEnv::EvalNormalPerturb(ShadeContext& sc)
{
    // No bump mapping support
    return Point3(0.0f, 0.0f, 0.0f);
}

int PhysicalSunSkyEnv::GetUVWSource()
{
    // Bogus - doesn't matter
    return UVWSRC_EXPLICIT;
}

int PhysicalSunSkyEnv::MapSlotType(int i)
{
    return MAPSLOT_ENVIRON;
}

int PhysicalSunSkyEnv::IsHighDynamicRange() const
{
    return true;
}

void PhysicalSunSkyEnv::Update(TimeValue t, Interval& valid)
{
    // Re-instantiate the shader at the given time
    m_shader = InstantiateShader(t, valid);
}

void PhysicalSunSkyEnv::Reset()
{
    get_class_descriptor().Reset(this);
}

Interval PhysicalSunSkyEnv::Validity(TimeValue t)
{
    Interval validity = FOREVER;
    Update(t, validity);
    return validity;
}

ParamDlg* PhysicalSunSkyEnv::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
{
    return get_class_descriptor().CreateParamDlgs(hwMtlEdit, imp, this);
}

void* PhysicalSunSkyEnv::GetInterface(ULONG id)
{
    return MtlBase::GetInterface(id);
}

BaseInterface* PhysicalSunSkyEnv::GetInterface(Interface_ID id)
{
    if (id == IRTSHADERPARAMETERBINDING_INTERFACE_ID)
    {
        IRTShaderParameterBinding * binding = this;
        return binding;
    }

    return MtlBase::GetInterface(id);
}

Class_ID PhysicalSunSkyEnv::ClassID()
{
    return get_class_descriptor().ClassID();
}
	
SClass_ID PhysicalSunSkyEnv::SuperClassID()
{
    return get_class_descriptor().SuperClassID();
}

void PhysicalSunSkyEnv::GetClassName(TSTR&	s)
{
    s = get_class_descriptor().ClassName();
}

int PhysicalSunSkyEnv::NumSubs()
{
    return 1;
}

TSTR PhysicalSunSkyEnv::SubAnimName(int i)
{
    switch(i)
    {
    case 0:
        return MaxSDK::GetResourceStringAsMSTR(IDS_PARAMETERS);
    default:
        DbgAssert(false);
        return TSTR();
    }
}

Animatable*	PhysicalSunSkyEnv::SubAnim(int	i)
{
    switch(i)
    {
    case 0:
        return m_param_block;
    default:
        DbgAssert(false);
        return nullptr;
    }
}

void PhysicalSunSkyEnv::DeleteThis()
{
    delete this;
}

int PhysicalSunSkyEnv::NumParamBlocks()
{
    return 1;
}

IParamBlock2* PhysicalSunSkyEnv::GetParamBlock(int i)
{
    switch(i)
    {
    case 0:
        return m_param_block;
    default:
        DbgAssert(false);
        return nullptr;
    }
}

IParamBlock2* PhysicalSunSkyEnv::GetParamBlockByID(BlockID id)
{
    switch(id)
    {
    case kParamBlockID_Main:
        return m_param_block;
    default:
        DbgAssert(false);
        return nullptr;
    }
}

RefTargetHandle	PhysicalSunSkyEnv::Clone( RemapDir	&remap )
{
    PhysicalSunSkyEnv* newob = new PhysicalSunSkyEnv(true);   

    const int num_refs = NumRefs();
    for(int i = 0; i < num_refs; ++i)
    {
        newob->ReplaceReference(i, remap.CloneRef(GetReference(i))); 
    }

    newob->MtlBase::operator=(*this);

    BaseClone(this, newob, remap);

    return newob;
}

RefResult PhysicalSunSkyEnv::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
    // We need to update our UI whenever the sun positioner changes, as we have dependencies on its weather data file
    if(m_param_block != nullptr)
    {
        IParamMap2* const map = m_param_block->GetMap(0);
        if(map != nullptr)
        {
            map->Invalidate();
        }
    }
    return REF_SUCCEED;
}

IOResult PhysicalSunSkyEnv::Load(ILoad	*iload)
{
    for(IOResult open_chunk_res = iload->OpenChunk(); open_chunk_res == IO_OK; open_chunk_res = iload->OpenChunk())
    {
        IOResult res = IO_OK;
        const int id = iload->CurChunkID();
        switch(id) 
        {
        case IOChunkID_MtlBase:
            res = MtlBase::Load(iload);
            break;
        }
        iload->CloseChunk();
        if (res!=IO_OK)
        {
            return res;
        }
    }

    return IO_OK;
}

IOResult PhysicalSunSkyEnv::Save(ISave	*isave)
{
    if(isave != nullptr)
    {
        isave->BeginChunk(IOChunkID_MtlBase);
        IOResult res = MtlBase::Save(isave);
        if (res!=IO_OK) 
            return res;
        isave->EndChunk();
    }

    return IO_OK;
}

int PhysicalSunSkyEnv::NumRefs()
{
    return kReferenceID_NumRefs;
}

RefTargetHandle	PhysicalSunSkyEnv::GetReference(int i)
{
    switch(i)
    {
    case kReferenceID_MainPB:
        return m_param_block;
    default:
        return nullptr;
    }
}

void PhysicalSunSkyEnv::SetReference(int i, RefTargetHandle rtarg)
{
    switch(i)
    {
    case kReferenceID_MainPB:
        m_param_block = dynamic_cast<IParamBlock2*>(rtarg);
        DbgAssert(m_param_block == rtarg);
        break;
    }
}

ClassDesc2& PhysicalSunSkyEnv::get_class_descriptor()
{
    static ClassDescriptor class_desc;
    return class_desc;
}

Point3 PhysicalSunSkyEnv::get_sun_direction(const TimeValue t, Interval& validity) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        // Fetch the sun position object
        INode* const sun_positioner_node = get_sun_positioner_node();
        if(sun_positioner_node != nullptr)
        {
            // Evaluate the node
            const ObjectState object_state = sun_positioner_node->EvalWorldState(t);

            // Fetch the underlying object
            ISunPositioner* const sun_positioner = dynamic_cast<ISunPositioner*>(object_state.obj);
            DbgAssert(sun_positioner == object_state.obj);      // Only sun positioner objects should be assignable
            if(sun_positioner != nullptr)
            {
                Point3 sun_direction = sun_positioner->GetSunDirection(t, validity);
                
                // Apply the node's transform matrix
                Interval node_tm_validity = FOREVER;        // GetObjTMAfterWSM() overwrites the validity interval
                const Matrix3 node_tm = sun_positioner_node->GetObjTMAfterWSM(t, &node_tm_validity);
                sun_direction = node_tm.VectorTransform(sun_direction).Normalize();
                validity &= node_tm_validity;

                return sun_direction;
            }
        }
    }

    // All failed - put the sun at the zenith
    return Point3(0.0f, 0.0f, 1.0f);
}

PhysicalSunSkyEnv::ShadingParameters PhysicalSunSkyEnv::EvaluateShadingParameters(const TimeValue t, Interval& validity) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        // Fetch param block values
        const float pb_global_intensity = m_param_block->GetFloat(kMainParamID_GlobalIntensity, t, validity);
        const float pb_haze = m_param_block->GetFloat(kMainParamID_Haze, t, validity);
        const bool pb_sun_enabled = (m_param_block->GetInt(kMainParamID_SunEnabled, t, validity) != 0);
        const float pb_sun_disc_intensity = pb_sun_enabled ? m_param_block->GetFloat(kMainParamID_DiscIntensity, t, validity) : 0.0f;
        const float pb_sun_glow_intensity = pb_sun_enabled ? m_param_block->GetFloat(kMainParamID_DiscGlowIntensity, t, validity) : 0.0f;
        const float pb_sun_disc_scale = m_param_block->GetFloat(kMainParamID_DiscScale, t, validity);
        const float pb_sky_intensity = m_param_block->GetFloat(kMainParamID_SkyIntensity, t, validity);
        const Color pb_night_color = m_param_block->GetColor(kMainParamID_NightColor, t, validity);
        const float pb_night_intensity = m_param_block->GetFloat(kMainParamID_NightIntensity, t, validity);
        const float pb_horizon_height = m_param_block->GetFloat(kMainParamID_HorizonHeight, t, validity);
        const float pb_horizon_blur = m_param_block->GetFloat(kMainParamID_HorizonBlur, t, validity);
        const Color pb_ground_color = m_param_block->GetColor(kMainParamID_GroundColor, t, validity);
        const float pb_saturation = m_param_block->GetFloat(kMainParamID_Saturation, t, validity);
        const Color pb_tint = m_param_block->GetColor(kMainParamID_Tint, t, validity);

        const Point3 original_sun_direction = get_sun_direction(t, validity);

        // Override the Perez coefficients, if needed, according to the measured data provided
        bool use_custom_perez = false;
        float perez_A = 0.0f;
        float perez_B = 0.0f;
        float perez_C = 0.0f;
        float perez_D = 0.0f;
        float perez_E = 0.0f;
        float perez_diffuse_horizontal_illuminance = 0.0f;
        float perez_direct_normal_illuminance = 0.0f;
        {
            const IlluminanceModel illuminance_model = static_cast<IlluminanceModel>(m_param_block->GetInt(kMainParamID_IlluminanceModel, t, validity));
            ISunPositioner::WeatherMeasurements weather_measurements;
            if(illuminance_model == kIlluminanceModel_Automatic)
            {
                // Obtain the measured values from the weather data file, if possible
                ISunPositioner* const sun_positioner = get_sun_positioner_object(t);
                if(sun_positioner != nullptr)
                {
                    use_custom_perez = sun_positioner->GetWeatherMeasurements(weather_measurements, t, validity);
                }
            }
            else if(illuminance_model == IlluminanceModel_Perez)
            {
                weather_measurements.diffuse_horizontal_illuminance = m_param_block->GetFloat(kMainParamID_PerezDiffuseHorizontalIlluminance, t, validity);
                weather_measurements.direct_normal_illuminance = m_param_block->GetFloat(kMainParamID_PerezDirectNormalIlluminance, t, validity);
                weather_measurements.dew_point_temperature_valid = false;
                use_custom_perez = true;
            }

            // Derive the Perez coefficients from the weather data, if needed
            if(use_custom_perez)
            {
                // Convert the illuminance values to Perez coefficients. Then use these coefficients for the sky luminance - but keep the Preetham
                // chromacity.
                IPerezAllWeather* const paw = IPerezAllWeather::GetInterface();
                if(DbgVerify(paw != nullptr))
                {
                    // Get the day of year from the sun positioner object - if not available, then use a bogus value
                    ISunPositioner* const sun_positioner = get_sun_positioner_object(t);
                    int day_of_year = 0;
                    if((sun_positioner == nullptr) || !sun_positioner->GetDayOfYear(day_of_year, t, validity))
                    {
                        day_of_year = 183;      // default to mid-year
                    }
                    // Calculate the angle between the sun and zenith
                    // Apparently, the code breaks if we have a value >= to 90.0 deg. So let's clamp it to 89.999 deg.
                    const float angle_between_sun_and_zenith = std::min(acosf(original_sun_direction.z), 89.999f * float(PI) / 180.0f);

                    const float atmWaterContent = paw->GetAtmosphericPrecipitableWaterContent(weather_measurements.dew_point_temperature_valid, weather_measurements.dew_point_temperature);
                    IPerezAllWeather::PerezParams perez_params;
                    paw->GetPerezParamsFromIlluminances(day_of_year, atmWaterContent, angle_between_sun_and_zenith, weather_measurements.diffuse_horizontal_illuminance, weather_measurements.direct_normal_illuminance, perez_params);
                    perez_A = perez_params.a;
                    perez_B = perez_params.b;
                    perez_C = perez_params.c;
                    perez_D = perez_params.d;
                    perez_E = perez_params.e;
                    perez_diffuse_horizontal_illuminance = weather_measurements.diffuse_horizontal_illuminance;
                    perez_direct_normal_illuminance = weather_measurements.direct_normal_illuminance;
                }
            }
        }

        // Let the shader generator interface generate the shading parameters
        return m_shader_generator.CreateShadingParameters(
            pb_global_intensity,
            pb_haze,
            pb_sun_enabled,
            pb_sun_disc_intensity,
            pb_sun_glow_intensity,
            pb_sun_disc_scale,
            pb_sky_intensity,
            pb_night_color,
            pb_night_intensity,
            pb_horizon_height,
            pb_horizon_blur,
            pb_ground_color,
            pb_saturation,
            pb_tint,
            original_sun_direction,
            use_custom_perez,
            perez_A,
            perez_B,
            perez_C,
            perez_D,
            perez_E,
            perez_diffuse_horizontal_illuminance,
            perez_direct_normal_illuminance,
            t,
            validity);
    }
    else
    {
        return ShadingParameters();
    }
}

std::unique_ptr<PhysicalSunSkyEnv::IShader> PhysicalSunSkyEnv::InstantiateShader(const TimeValue t, Interval& validity) const
{
    const ShadingParameters shading_params = EvaluateShadingParameters(t, validity);
    return std::unique_ptr<PhysicalSunSkyEnv::IShader>(new Shader(shading_params));
}

//==================================================================================================
// class PhysicalSunSkyEnv::ClassDescriptor
//==================================================================================================

PhysicalSunSkyEnv::ClassDescriptor::ClassDescriptor()
    : m_class_name(MaxSDK::GetResourceStringAsMSTR(IDS_PHYSSUNSKY_CLASSNAME))
{
    IMtlRender_Compatibility_MtlBase::Init(*this);
}

PhysicalSunSkyEnv::ClassDescriptor::~ClassDescriptor()
{

}

int PhysicalSunSkyEnv::ClassDescriptor::IsPublic()
{
    return true;
}

void* PhysicalSunSkyEnv::ClassDescriptor::Create(BOOL loading)
{
    Texmap* texmap = new PhysicalSunSkyEnv(loading != 0);
    return texmap;
}

const TCHAR* PhysicalSunSkyEnv::ClassDescriptor::ClassName()
{
    return m_class_name;
}

SClass_ID PhysicalSunSkyEnv::ClassDescriptor::SuperClassID()
{
    return TEXMAP_CLASS_ID;
}

Class_ID PhysicalSunSkyEnv::ClassDescriptor::ClassID()
{
    return IPhysicalSunSky::GetClassID();
}

const TCHAR* PhysicalSunSkyEnv::ClassDescriptor::Category()
{
    //!! TODO: Verify this is valid
    return TEXMAP_CAT_ENV;
}

HINSTANCE PhysicalSunSkyEnv::ClassDescriptor::HInstance()
{
    return MaxSDK::GetHInstance();
}

const MCHAR* PhysicalSunSkyEnv::ClassDescriptor::InternalName() 
{
    return _T("PhysicalSunSkyEnv");
}

MaxSDK::QMaxParamBlockWidget* PhysicalSunSkyEnv::ClassDescriptor::CreateQtWidget(
    ReferenceMaker& /*owner*/,
    IParamBlock2& paramBlock,
    const MapID paramMapID,  
    MSTR& rollupTitle, 
    int& rollupFlags, 
    int& rollupCategory)
{
    MainPanelWidget* const widget = new MainPanelWidget(paramBlock);
    
    rollupTitle = ClassName();

    return widget;
}

bool PhysicalSunSkyEnv::ClassDescriptor::IsCompatibleWithRenderer(ClassDesc& rendererClassDesc)
{
    //!! TODO
    return true;
}


FPInterface* PhysicalSunSkyEnv::ClassDescriptor::GetInterface(Interface_ID id)
{
    if(id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
    {
        return static_cast<IMaterialBrowserEntryInfo*>(this);
    }
    else
    {
        return ClassDesc2::GetInterface(id);
    }
}

const MCHAR* PhysicalSunSkyEnv::ClassDescriptor::GetEntryName() const 
{
    return const_cast<PhysicalSunSkyEnv::ClassDescriptor*>(this)->ClassName();
}

const MCHAR* PhysicalSunSkyEnv::ClassDescriptor::GetEntryCategory() const 
{
    static const MSTR str = QObject::tr("Maps\\Environment");
    return str;
}

Bitmap* PhysicalSunSkyEnv::ClassDescriptor::GetEntryThumbnail() const 
{
    return nullptr;
}

//==================================================================================================
// class PhysicalSunSkyEnv::ParamBlockAccessor
//==================================================================================================

void PhysicalSunSkyEnv::ParamBlockAccessor::Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) 
{
    PhysicalSunSkyEnv* const sun_sky = dynamic_cast<PhysicalSunSkyEnv*>(owner);
    IParamBlock2* const param_block = (sun_sky != nullptr) ? sun_sky->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case kMainParamID_DiscScale_Percent:
            // Report percentage from 1.0-based scale
            {
                const float regular_scale = param_block->GetFloat(kMainParamID_DiscScale, t, valid);
                v.f = regular_scale * 100.0f;
            }
            break;
        case kMainParamID_PerezDiffuseHorizontalIlluminance:
        case kMainParamID_PerezDirectNormalIlluminance:
            {
                // If using weather data file, then get the value from the sun positioner
                const IlluminanceModel illuminance_model = static_cast<IlluminanceModel>(param_block->GetInt(kMainParamID_IlluminanceModel, t, valid));
                if(illuminance_model == kIlluminanceModel_Automatic)
                {
                    // Obtain the measured values from the weather data file, if possible
                    ISunPositioner* const sun_positioner = sun_sky->get_sun_positioner_object(t);
                    if(sun_positioner != nullptr)
                    {
                        ISunPositioner::WeatherMeasurements weather_measurements;
                        if(sun_positioner->GetWeatherMeasurements(weather_measurements, t, valid))
                        {
                            v.f = (id == kMainParamID_PerezDiffuseHorizontalIlluminance)
                                ? weather_measurements.diffuse_horizontal_illuminance
                                : weather_measurements.direct_normal_illuminance;
                        }
                    }
                }
            }
            break;
        }
    }
}

void PhysicalSunSkyEnv::ParamBlockAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) 
{
    PhysicalSunSkyEnv* const sun_sky = dynamic_cast<PhysicalSunSkyEnv*>(owner);
    IParamBlock2* const param_block = (sun_sky != nullptr) ? sun_sky->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case kMainParamID_DiscScale_Percent:
            // Report percentage from 1.0-based scale
            {
                const float regular_scale = v.f * 0.01f;
                param_block->SetValue(kMainParamID_DiscScale, t, regular_scale);
            }
            break;
        }
    }
}

//==================================================================================================
// class PhysicalSunSkyEnv::Shader
//==================================================================================================

PhysicalSunSkyEnv::Shader::Shader(const ShadingParameters& shading_parameters)
    : m_shading_parameters(shading_parameters)
{

}

PhysicalSunSkyEnv::Shader::~Shader()
{

}

Color PhysicalSunSkyEnv::Shader::Evaluate(const Point3& direction) const 
{
    // Call the RapidSL shader code
    const Color result = Max::RapidSLShaders::compute_sunsky_env_color(
        direction,
        m_shading_parameters.horizon_height,
        m_shading_parameters.horizon_blur,
        m_shading_parameters.global_multiplier,
        m_shading_parameters.sun_illuminance,
        m_shading_parameters.sun_luminance,
        m_shading_parameters.sun_glow_intensity,
        m_shading_parameters.sky_contribution_multiplier,
        m_shading_parameters.sky_ground_contribution,
        m_shading_parameters.sun_disc_angular_radius,
        m_shading_parameters.sun_smooth_angular_radius,
        m_shading_parameters.sun_glow_angular_radius,
        m_shading_parameters.color_saturation,
        m_shading_parameters.color_tint,
        m_shading_parameters.ground_color,
        m_shading_parameters.night_luminance,
        m_shading_parameters.night_falloff,
        m_shading_parameters.sun_direction,
        m_shading_parameters.sun_direction_for_sky_contribution,
        m_shading_parameters.perez_A,
        m_shading_parameters.perez_B,
        m_shading_parameters.perez_C,
        m_shading_parameters.perez_D,
        m_shading_parameters.perez_E,
        m_shading_parameters.perez_Z);

    return result;
}


//==================================================================================================
// class PhysicalSunSkyEnv::ShaderGenerator
//==================================================================================================

PhysicalSunSkyEnv::ShadingParameters PhysicalSunSkyEnv::ShaderGenerator::CreateShadingParameters(
    const float global_intensity,
    const float haze,
    const bool sun_enabled,
    const float sun_disc_intensity,
    const float sun_glow_intensity,
    const float sun_disc_scale,
    const float sky_intensity,
    const Color night_color,
    const float night_intensity,
    const float horizon_height_deg,
    const float horizon_blur_deg,
    const Color ground_color,
    const float saturation,
    const Color tint,
    const Point3 sun_direction,
    const bool use_custom_perez_coefficients,
    const float custom_perez_A,
    const float custom_perez_B,
    const float custom_perez_C,
    const float custom_perez_D,
    const float custom_perez_E,
    const float custom_perez_diffuse_horizontal_illuminance,
    const float custom_perez_direct_normal_illuminance,
    const TimeValue t,
    Interval& validity) const
{
    ShadingParameters params;

    // Fetch tone operator and global system values
    ToneOperatorInterface* const toneOpInt = dynamic_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
    ToneOperator* const toneOp = (toneOpInt != nullptr) ? toneOpInt->GetToneOperator() : nullptr;
    const float physical_scale = ((toneOp != nullptr) && (toneOp->Active(t))) ? toneOp->GetPhysicalUnit(t, validity) : 1500.0f;     // 1500 is the system-wide default value
    const float global_light_level = GetCOREInterface()->GetLightLevel(t, validity);
    const Color global_light_tint = GetCOREInterface()->GetLightTint(t, validity);

    // Remap haze (in [0,1]) to turbidity in [2, 17] (required by Preetham paper)
    const float turbidity = (haze * 15.0f) + 2.0f;

    // Get sun direction
    const Point3 original_sun_direction = sun_direction;

    // Internal horizon height uses the sine of the angle
    params.horizon_height = sinf(static_cast<float>(horizon_height_deg * (PI / 180.0)));
    params.horizon_blur = sinf(static_cast<float>(horizon_blur_deg * (PI / 180.0)));
    // The sun direction is adjusted according to the horizon height, such that the sun still sets at the horizon.
    const Point3 adjusted_sun_direction = Normalize(Point3(original_sun_direction.x, original_sun_direction.y, original_sun_direction.z - params.horizon_height));

    // Thee result is multiplied by PI to work around the problem that lighting is generally too bright by PI (everywhere). This problem originates
    // from the lack of a division by PI in the standard material's lambertian BRDF (see MAXX-21782).
    params.global_multiplier = global_intensity * global_light_tint * global_light_level * static_cast<float>(PI) / physical_scale;

    params.sun_glow_intensity = 0.0025f * sun_glow_intensity;
    // 0.00465 is the sun's average angular radius, in radians
    params.sun_disc_angular_radius = 0.00465 * sun_disc_scale;
    // We make the sun smoother as the glow intensity is increased
    params.sun_smooth_angular_radius = params.sun_disc_angular_radius * (1.01f + sun_glow_intensity * 0.2f);
    params.sun_glow_angular_radius = params.sun_smooth_angular_radius * 10.0f;
    params.color_saturation = saturation;
    params.color_tint = tint;
    params.ground_color = ground_color;
    // Same bogus multiplication by PI as above (see MAXX-21782)
    params.night_luminance = night_color * night_intensity;
    params.sun_direction = adjusted_sun_direction;

    // If the sun is below the horizon, we want a smooth transition to a black sky (to avoid an abrupt transition to black when the sun dips
    // below the horizon). We therefore render the sky as if the sun were very slightly above the horizon, but apply a falloff such that the sky 
    // become completely black when at nadir. 
    params.sky_contribution_multiplier = sky_intensity;
    params.sun_direction_for_sky_contribution = adjusted_sun_direction;
    const float sun_z_clamp_for_sky = 0.01f;
    if(params.sun_direction_for_sky_contribution.z <= sun_z_clamp_for_sky)
    {
        // See http://en.wikipedia.org/wiki/Twilight#Civil_twilight
        // Transition to black when we reach 18 degrees below the horizon
        const float z_black = -cos((90.0f - 18.0f) * PI / 180.0f);

        if(params.sun_direction_for_sky_contribution.z > z_black)
        {
            // Calculate falloff in [0,1]
            params.night_falloff = 1.0f - ((params.sun_direction_for_sky_contribution.z - sun_z_clamp_for_sky) / (z_black - sun_z_clamp_for_sky));
            // Apply exponential falloff
            params.night_falloff = powf(params.night_falloff, 4.0f);
        }
        else
        {
            // Sky is black
            params.night_falloff = 0.0f;
        }

        // Clamp sun height for purposes of sky contribution
        params.sun_direction_for_sky_contribution.z = sun_z_clamp_for_sky;
        params.sun_direction_for_sky_contribution = normalize(params.sun_direction_for_sky_contribution);
    }
    else
    {
        params.night_falloff = 1.0f;
    }

    // Calculate the Perez coefficients, according to the Preetham paper
    {
        params.perez_A.x = 0.1787 * turbidity + -1.4630;
        params.perez_B.x = -0.3554 * turbidity + 0.4275;
        params.perez_C.x = -0.0227 * turbidity + 5.3251;
        params.perez_D.x = 0.1206 * turbidity + -2.5771;
        params.perez_E.x = -0.0670 * turbidity + 0.3703;
        params.perez_A.y = -0.0193 * turbidity - 0.2592
            // This piece of magic eliminates the pink shade at the horizon
            - 0.03 + sqrt(params.sun_direction_for_sky_contribution.z) * 0.09;
        params.perez_B.y = -0.0665 * turbidity + 0.0008;
        params.perez_C.y = -0.0004 * turbidity + 0.2125;
        params.perez_D.y = -0.0641 * turbidity - 0.8989;
        params.perez_E.y = -0.0033 * turbidity + 0.0452;
        params.perez_A.z = -0.0167 * turbidity - 0.2608;
        params.perez_B.z = -0.0950 * turbidity + 0.0092;
        params.perez_C.z = -0.0079 * turbidity + 0.2102;
        params.perez_D.z = -0.0441 * turbidity - 1.6537;
        params.perez_E.z = -0.0109 * turbidity + 0.0529;
        // Calculate zenith values
        const float T = turbidity;
        const float T2 = T * T;
        const float theta_s = acos(params.sun_direction_for_sky_contribution.z);
        const float theta_s_2 = theta_s * theta_s;
        const float theta_s_3 = theta_s_2 * theta_s;
        const float chi = (4.0 / 9.0 - T / 120.0) * (PI - 2.0 * theta_s);
        params.perez_Z.x = 1000.0f * (((4.0453 * T - 4.9710) * tan(chi)) + (-0.2155 * T + 2.4192));
        params.perez_Z.y =
            (T2 * 0.00166 + T * -0.02903 + 0.11693) * theta_s_3
            + (T2 * -0.00375 + T * 0.06377 + -0.21196) * theta_s_2
            + (T2 * 0.00209 + T * -0.03202 + 0.06052) * theta_s
            + (T * 0.00394 + 0.25886);
        params.perez_Z.z =
            (T2 * 0.00275 + T * -0.04214 + 0.15346) * theta_s_3
            + (T2 * -0.00610 + T * 0.08970 + -0.26756) * theta_s_2
            + (T2 * 0.00317 + T * -0.04153 + 0.06670) * theta_s
            + (T * 0.00516 + 0.26688);

        // Override the Perez coefficients, if needed
        if(use_custom_perez_coefficients)
        {
            params.perez_A.x = custom_perez_A;
            params.perez_B.x = custom_perez_B;
            params.perez_C.x = custom_perez_C;
            params.perez_D.x = custom_perez_D;
            params.perez_E.x = custom_perez_E;
        }
    }

    // Calculate the overall/average/integrated contribution from the sky (only the sky, not the sun), to be used for fudging
    // illumination on the procedural ground plane.
    float nominal_diffuse_horizontal_illuminance_Y = 0.0f;
    const float3 nominal_diffuse_horizontal_illuminance_rgb =
        estimate_diffuse_horizontal_sky_illuminance(
            params.sun_direction_for_sky_contribution,
            params.night_luminance,
            params.night_falloff,
            params.perez_A, params.perez_B, params.perez_C, params.perez_D, params.perez_E, params.perez_Z,
            nominal_diffuse_horizontal_illuminance_Y);

    // If a diffuse horizontal illuminance was specified, we try to match it by normalizing the sky illuminance
    if(use_custom_perez_coefficients && (custom_perez_diffuse_horizontal_illuminance > 0.0f))
    {
        params.sky_contribution_multiplier *= (custom_perez_diffuse_horizontal_illuminance / nominal_diffuse_horizontal_illuminance_Y);
    }

    // Fake ground plane illuminance from the sky: pre-calculated diffuse reflected luminance for 100% reflectivity (ground color applied in shader)
    params.sky_ground_contribution = params.sky_contribution_multiplier * nominal_diffuse_horizontal_illuminance_rgb / static_cast<float>(PI);

    // Calculate the sun's total illuminance
    params.sun_illuminance = sun_disc_intensity * calculate_sun_illuminance(adjusted_sun_direction, turbidity);

    // Normalize the sun's illuminance if a custom illuminance was passed
    if(use_custom_perez_coefficients && (custom_perez_direct_normal_illuminance > 0.0f))
    {
        params.sun_illuminance *= custom_perez_direct_normal_illuminance / rgb_luminance(params.sun_illuminance);
    }

    // Calculate the sun's luminance, relative to its distribution function (such that the sun+glow will, overall, emit
    // "sun_illuminance".
    const float sun_contribution_integral = calculate_sun_contribution_integral(
        params.sun_disc_angular_radius,
        params.sun_smooth_angular_radius,
        params.sun_glow_angular_radius,
        params.sun_glow_intensity);
    params.sun_luminance = params.sun_illuminance / sun_contribution_integral;

    return params;
}

std::unique_ptr<PhysicalSunSkyEnv::IShader> PhysicalSunSkyEnv::ShaderGenerator::InstantiateShader(const ShadingParameters& shading_parameters) const 
{
    return std::unique_ptr<PhysicalSunSkyEnv::IShader>(new Shader(shading_parameters));
}

void PhysicalSunSkyEnv::ShaderGenerator::init()
{
    // nothing to do
}

AColor PhysicalSunSkyEnv::getLightScale(const TimeValue t, Interval &valid)
{
    float physScale = 1500.f;
    ToneOperatorInterface* toneOpInt = dynamic_cast<ToneOperatorInterface*>(GetCOREInterface(TONE_OPERATOR_INTERFACE));
    if (toneOpInt) {
        ToneOperator *toneOp = toneOpInt->GetToneOperator();
        if (toneOp && toneOp->Active(t))
            physScale = toneOp->GetPhysicalUnit(t, valid);
    }

    const float  lev = GetCOREInterface()->GetLightLevel(t, valid);
    const Point3 col = GetCOREInterface()->GetLightTint(t, valid);

    return AColor(col.x * (lev / physScale),
        col.y * (lev / physScale),
        col.z * (lev / physScale));
}

D3DCOLORVALUE GetD3DColor(const AColor &c)
{
    D3DCOLORVALUE dc;
    dc.r = c.r;
    dc.g = c.g;
    dc.b = c.b;
    dc.a = c.a;
    return dc;
}

void PhysicalSunSkyEnv::InitializeBinding()
{
}

void PhysicalSunSkyEnv::BindParameter(const TCHAR * paramName, LPVOID value)
{
    TimeValue t = GetCOREInterface()->GetTime();
    BindParameter(t, paramName, value);
}

void PhysicalSunSkyEnv::BindParameter(const TimeValue time, const TCHAR * paramName, LPVOID value)
{
    Interval valid  = FOREVER;
    float fval      = 0.0f;
    TimeValue t     = time;

    if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_on")) == 0)
    {
        *(BOOL*)value = TRUE;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_y_is_up")) == 0)
    {
        *(BOOL*)value = FALSE;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_multiplier")) == 0)
    {
        //No multiplier
        fval = 1.0f;
        *(FLOAT*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_sun_direction")) == 0)
    {
        Point3 pvalue = get_sun_direction(t, valid);
        D3DXVECTOR3* dpvalue = reinterpret_cast<D3DXVECTOR3*>(&pvalue);
        *(D3DXVECTOR3*)value = *dpvalue;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_rgb_unit_conversion")) == 0)
    {
        AColor col(0.000666667f, 0.000666667f, 0.000666667f, 1.0f);
        col = getLightScale(t, valid);
        *(D3DCOLORVALUE*)value = GetD3DColor(col);
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_ground_color")) == 0)
    {
        AColor col;
        m_param_block->GetValue(kMainParamID_GroundColor, t, col, valid);
        *(D3DCOLORVALUE*)value = GetD3DColor(col);
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_night_color")) == 0)
    {
        AColor col;
        m_param_block->GetValue(kMainParamID_NightColor, t, col, valid);
        *(D3DCOLORVALUE*)value = GetD3DColor(col);
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_haze")) == 0)
    {
        m_param_block->GetValue(kMainParamID_Haze, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_redblueshift")) == 0)
    {
        //m_param_block->GetValue(kMainPID_redness, t, fval, valid);
        fval = 0.f; //Using default value from kMainPID_redness
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_saturation")) == 0)
    {
        m_param_block->GetValue(kMainParamID_Saturation, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_horizon_height")) == 0)
    {
        m_param_block->GetValue(kMainParamID_HorizonHeight, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_horizon_blur")) == 0)
    {
        m_param_block->GetValue(kMainParamID_HorizonBlur, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_sun_disk_intensity")) == 0)
    {
        m_param_block->GetValue(kMainParamID_DiscIntensity, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_sun_disk_scale")) == 0)
    {
        m_param_block->GetValue(kMainParamID_DiscScale, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_sun_glow_intensity")) == 0)
    {
        m_param_block->GetValue(kMainParamID_DiscGlowIntensity, t, fval, valid);
        *(float*)value = fval;
    }
    else if (_tcscmp(paramName, _T("msl_mia_physicalsky_1_visibility_distance")) == 0)
    {
        fval = 0.f; //Use default value for kMainPID_aerial
        //m_param_block->GetValue(kMainPID_aerial, t, fval, valid);
        *(float*)value = fval;
    }
    else {
        DbgAssert(0 && _T("Unknown parameter"));
    }
}
