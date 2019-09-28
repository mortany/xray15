//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma	once

#include "RapidRTRendererPlugin.h"

// local includes
#include "resource.h"
#include "RapidRenderSession.h"
#include "RRTRendParamDlg.h"
#include "Util.h"
#include "Plugins/NoiseFilter_RenderElement.h"
// max sdk
#include <plugapi.h>
#include <ITabDialog.h>
// IAmgInterface.h is located in sdk samples folder, such that it may be shared with sdk samples plugins, without actually being exposed in the SDK.
#include <../samples/render/AmgTranslator/IAmgInterface.h>

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class RapidRTRendererPlugin
//==================================================================================================

RapidRTRendererPlugin::RapidRTRendererPlugin(const bool loading)
    : m_param_block(nullptr)
{
    if(!loading)
    {
        GetTheClassDescriptor().MakeAutoParamBlocks(this); 
    }
}

RapidRTRendererPlugin::~RapidRTRendererPlugin()
{

}

RapidRTRendererPlugin::ParamBlockAccessor RapidRTRendererPlugin::m_pb_accessor;

// Declare the PBDesc here, rather than at global scope, to avoid constructing this thing on dll load.
ParamBlockDesc2 RapidRTRendererPlugin::m_param_block_desc( 
		0,  // param block ID
		_T("Parameters"),	// internal name
		IDS_GENERAL_PARAMETERS_PB_NAME,  // resource ID for localizable name
		&GetTheClassDescriptor(),           // class descriptor
    	P_AUTO_CONSTRUCT | P_VERSION | P_AUTO_UI_QT | P_MULTIMAP,       // flags
        1,      // version
        0,      // reference number (required because of P_AUTO_CONSTRUCT)

    // Auto-UI (multi-maps)
    3, ParamMapID_RenderParams, ParamMapID_ImageQuality, ParamMapID_Advanced,
 
    ParamID_Termination_EnableIterations, _T("enable_iterations"), TYPE_BOOL, 0, IDS_PARAM_ENABLE_ITERATIONS,
        p_default, false,
        p_end,	
	ParamID_Termination_NumIterations, _T("iterations"), TYPE_INT, 0, IDS_PARAM_ITERATIONS,
		p_default, 50, 
		p_range, 1,	10000000,
	    p_end,		

	ParamID_Termination_Quality_dB, _T("quality_db"), TYPE_FLOAT, 0, IDS_PARAM_QUALITY_DB,
		p_default, rti::ConvergenceLevel::Draft, 
		p_range, 0.0f, 10000.0f,
	    p_end,	

    ParamID_Termination_EnableTime, _T("enable_time"), TYPE_BOOL, 0, IDS_PARAM_ENABLE_TIME,
        p_default, false,
        p_end,	
	ParamID_Termination_TimeInSeconds, _T("time_in_seconds"), TYPE_INT, 0, IDS_PARAM_TIME,
		p_default, 300,         // 5 minutes
		p_range, 1, 100000000,     // 100 million ~= 1157 days
	    p_end,	
	ParamID_Termination_TimeSplitSeconds, _T("time_split_seconds"), TYPE_INT, P_TRANSIENT, IDS_PARAM_TIME_SPLIT_SECONDS,
        p_accessor, &m_pb_accessor,
	    p_end,	
	ParamID_Termination_TimeSplitMinutes, _T("time_split_minutes"), TYPE_INT, P_TRANSIENT, IDS_PARAM_TIME_SPLIT_MINUTES,
        p_accessor, &m_pb_accessor,
	    p_end,	
	ParamID_Termination_TimeSplitHours, _T("time_split_hours"), TYPE_INT, P_TRANSIENT, IDS_PARAM_TIME_SPLIT_HOURS,
        p_accessor, &m_pb_accessor,
	    p_end,	

    ParamID_MotionBlurAllObjects,	_T("motion_blur_all_objects"), TYPE_BOOL, 0, IDS_PARAM_MOTION_BLUR_ALL_OBJECTS,
        p_default, true,
        p_end,	

	ParamID_RenderMethod, _T("render_method"), TYPE_INT, 0, IDS_PARAM_RENDER_METHOD,
		p_default, rti::KERNEL_PATH_TRACE_FAST,
	    p_end,

    ParamID_PointLightDiameter, _T("point_light_diameter"), TYPE_WORLD, 0, IDS_PARAM_POINT_LIGHT_DIAMETER,
        p_defaults_and_ranges_in_meters,
        p_default, 0.01f,    // default == 1 cm
        p_range, 1e-4f, 1e6f,       // range is 0.1mm to 1000km
        p_end,

    ParamID_EnableOutlierClamp, _T("enable_outlier_clamp"), TYPE_BOOL, P_INVISIBLE, IDS_PARAM_ENABLE_OUTLIER_CLAMP,
        p_default, true,
        p_end,	

    ParamID_FilterDiameter, _T("anti_aliasing_filter_diameter"), TYPE_FLOAT, 0, IDS_PARAM_ANTI_ALIASING_FILTER_DIAMETER,
        p_default, 3.0f,
        p_range, 1.0f, 10.0f,
        p_end,

    ParamID_EnableAnimatedNoise, _T("enable_animated_noise"), TYPE_BOOL, 0, IDS_PARAM_ENABLE_ANIMATED_NOISE,
        p_default, true,
        p_end,	

    ParamID_EnableNoiseFilter, _T("enable_noise_filter"), TYPE_BOOL, 0, IDS_PARAM_ENABLE_NOISE_FILTER,
        p_default, false,
        p_end,	
	ParamID_NoiseFilterStrength, _T("noise_filter_strength"), TYPE_FLOAT, 0, IDS_PARAM_NOISE_FILTER_STRENGTH,
		p_default, 0.5f, 
		p_range, 0.0f, 1.0f,
	    p_end,	
    // Exposes the strength as percentage, for UI exposure
	ParamID_NoiseFilterStrength_Percentage, _T("noise_filter_strength_percentage"), TYPE_FLOAT, P_TRANSIENT, IDS_PARAM_NOISE_FILTER_STRENGTH_PERCENTAGE,
        p_accessor, &m_pb_accessor,
	    p_end,	

    ParamID_Texture_Bake_Resolution, _T("texture_bake_resolution"), TYPE_INT, P_INVISIBLE, IDS_PARAM_TEXTURE_BAKE_RESOLUTION,
        p_default, 1024,
        p_range, 1, 16000,
        p_end,	

    ParamID_Maximum_DownResFactor, _T("maximum_interactive_down_res_factor"), TYPE_INT, P_INVISIBLE, IDS_PARAM_MAX_DOWN_RES_FACTOR,
        p_default, 4,
        p_range, 1, 10,
        p_end,	

	p_end							
);

RendParamDlg* RapidRTRendererPlugin::CreateParamDialog(IRendParams *ir, BOOL prog) 
{
    if(!prog)
    {
        return GetClassDescriptor().CreateParamDialogs(ir, *this);
    }
    else
    {
        // This is temporarily disabled, pending resolution of MAXX-21389
        return nullptr;
    }
}

void RapidRTRendererPlugin::GetVendorInformation(MSTR& info) const 
{
    info = ClassName();
}

void RapidRTRendererPlugin::GetPlatformInformation(MSTR& /*info*/) const 
{
    // Nothing
}

void RapidRTRendererPlugin::AddTabToDialog(ITabbedDialog* dialog, ITabDialogPluginTab* tab)
{
    // The width of	the	render rollout in	dialog units.
    static const int kRendRollupWidth	=	222;
    dialog->AddRollout(MaxSDK::GetResourceStringAsMSTR(IDS_RAPID_CLASS_NAME),	NULL,
        Class_ID(0x4ed55c01, 0x217574a0), tab,	-1,	kRendRollupWidth,	0,
        0, ITabbedDialog::kSystemPage);
}

std::unique_ptr<IOfflineRenderSession> RapidRTRendererPlugin::CreateOfflineSession(IRenderSessionContext& sessionContext)
{
        return std::unique_ptr<IOfflineRenderSession>(new RapidRenderSession(sessionContext, false));
    }

std::unique_ptr<IInteractiveRenderSession> RapidRTRendererPlugin::CreateInteractiveSession(IRenderSessionContext& sessionContext)
{
        return std::unique_ptr<IInteractiveRenderSession>(new RapidRenderSession(sessionContext, true));
    }

bool RapidRTRendererPlugin::SupportsInteractiveRendering() const 
{
    return true;
}

bool RapidRTRendererPlugin::IsStopSupported() const 
{
    return true;
}

RapidRTRendererPlugin::PauseSupport RapidRTRendererPlugin::IsPauseSupported() const
{
    return PauseSupport::Full;
}

bool RapidRTRendererPlugin::HasRequirement(const IRendererRequirements::Requirement requirement) 
{
    switch(requirement)
    {
    case IRendererRequirements::kRequirement_Wants32bitFPOutput:
        // Rapid always needs floating-point buffer (because we process the exposure control in post)
        return true;
    case IRendererRequirements::kRequirement_NoGBufferForToneOpPreview:
        // No GBuffers generated by Rapid.
        return true;
    case IRendererRequirements::kRequirement_SupportsConcurrentRendering:
        // We support concurrent MEdit/ActiveShade renders.
        return true;
    default:
        // Don't care about other requirements
        return false;
    }
}

bool RapidRTRendererPlugin::MotionBlurIgnoresNodeProperties() const 
{
    const bool blur_all_objects = (m_param_block != nullptr) && (m_param_block->GetInt(RapidRTRendererPlugin::ParamID_MotionBlurAllObjects) != 0);
    return blur_all_objects;
}

RapidRTRendererPlugin::QualityPreset RapidRTRendererPlugin::m_quality_presets[] =
{
    {1.0f, IDS_QUALITY_MINIMUM},
    {rti::ConvergenceLevel::Draft, IDS_QUALITY_DRAFT},
    {rti::ConvergenceLevel::Medium, IDS_QUALITY_MEDIUM},
    {rti::ConvergenceLevel::Production, IDS_QUALITY_HIGH},
    {rti::ConvergenceLevel::Excellent, IDS_QUALITY_VERY_HIGH},
    {100.0f, IDS_QUALITY_MAXIMUM}        // High enough that it should never be reached
};

size_t RapidRTRendererPlugin::get_num_quality_presets() 
{
    return _countof(m_quality_presets);

}

const RapidRTRendererPlugin::QualityPreset& RapidRTRendererPlugin::get_quality_preset(const size_t index) 
{
    if(DbgVerify(index < _countof(m_quality_presets)))
    {
        return m_quality_presets[index];
    }
    else
    {
        return m_quality_presets[_countof(m_quality_presets) - 1];
    }
}

TSTR RapidRTRendererPlugin::get_matching_quality_preset_name(const float quality_db)
{
    for(size_t i = 0; i < _countof(m_quality_presets); ++i)
    {
        // Reverse iterate
        const QualityPreset& preset = m_quality_presets[_countof(m_quality_presets) - i - 1];
        if(quality_db >= preset.quality_db)
        {
            return MaxSDK::GetResourceStringAsMSTR(preset.resource_string_id);
        }
    }

    // No preset found... smaller than minimum preset
    return TSTR(_M("< ")) + MaxSDK::GetResourceStringAsMSTR(m_quality_presets[0].resource_string_id);
}

RapidRTRendererPlugin::ClassDescriptor& RapidRTRendererPlugin::GetTheClassDescriptor()
{
    static ClassDescriptor desc;
    return desc;
}

UnifiedRenderer::ClassDescriptor& RapidRTRendererPlugin::GetClassDescriptor() const 
{
    return GetTheClassDescriptor();
}

IOResult RapidRTRendererPlugin::Load_UnifiedRenderer(ILoad& iload) 
{
    iload.RegisterPostLoadCallback(this);

    return IO_OK;
}

void RapidRTRendererPlugin::proc(ILoad* /*iload*/)
{
    // Retrieve the param block's post-load info to deal with parameter conversion
    ParamBlockDesc2* desc = (m_param_block != nullptr) ? m_param_block->GetDesc() : nullptr;
    if(DbgVerify(desc != nullptr))
    {
        IParamBlock2PostLoadInfo* post_load_info = dynamic_cast<IParamBlock2PostLoadInfo*>(desc->GetInterface(IPARAMBLOCK2POSTLOADINFO_ID));
        if(post_load_info != nullptr)
        {
            // Determine which parameters were loaded or not
            bool quality_loaded = false;
            bool time_loaded = false;
            bool enable_iterations_loaded = false;
            const IntTab& loaded_param_ids = post_load_info->GetParamLoaded();
            for(int i = 0; i < loaded_param_ids.Count(); ++i)
            {
                const int current_id = loaded_param_ids[i];
                switch(current_id)
                {
                case ParamID_Termination_EnableIterations:
                    enable_iterations_loaded = true;
                    break;
                case ParamID_Termination_EnableTime:
                    time_loaded = true;
                    break;
                case ParamID_Termination_Quality_dB:
                    quality_loaded = true;
                    break;
                default:
                    break;
                }
            }
            
            if(!quality_loaded)
            {
                // Disable quality to respect legacy behaviour
                m_param_block->SetValue(ParamID_Termination_Quality_dB, 0, 0.0f);
            }
            if(!time_loaded)
            {
                // Disable time to respect legacy behaviour
                m_param_block->SetValue(ParamID_Termination_EnableTime, 0, false);
            }
            if(!enable_iterations_loaded)
            {
                // Enable iterations - legacy files may have an "infinite iterations" parameter, but we ignore that
                m_param_block->SetValue(ParamID_Termination_EnableIterations, 0, true);
            }
        }
    }
}

IOResult RapidRTRendererPlugin::Save_UnifiedRenderer(ISave& /*isave*/) const 
{
    return IO_OK;
}

void* RapidRTRendererPlugin::GetInterface_UnifiedRenderer(ULONG /*id*/) 
{
    return nullptr;
}

BaseInterface* RapidRTRendererPlugin::GetInterface_UnifiedRenderer(Interface_ID /*id*/) 
{
    return nullptr;
}

int RapidRTRendererPlugin::NumRefs() 
{
    return 1;
}

RefTargetHandle	RapidRTRendererPlugin::GetReference(int i) 
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

void RapidRTRendererPlugin::SetReference(int i, RefTargetHandle rtarg) 
{
    switch(i)
    {
    case 0:
        m_param_block = dynamic_cast<IParamBlock2*>(rtarg);
        DbgAssert(m_param_block == rtarg);
        break;
    default:
        DbgAssert(false);
        break;
    }
}

int RapidRTRendererPlugin::NumParamBlocks() 
{
    return 1;
}

IParamBlock2* RapidRTRendererPlugin::GetParamBlock(int i) 
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

IParamBlock2* RapidRTRendererPlugin::GetParamBlockByID(BlockID id) 
{
    switch(id)
    {
    case 0:
        return m_param_block;
    default:
        return nullptr;
    }
}

bool RapidRTRendererPlugin::CompatibleWithAnyRenderElement() const 
{
    return true;
}

bool RapidRTRendererPlugin::CompatibleWithRenderElement(IRenderElement& pIRenderElement) const 
{
    if(pIRenderElement.ClassID() == NoiseFilter_RenderElement::GetTheClassDescriptor().ClassID())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool RapidRTRendererPlugin::GetEnableInteractiveMEditSession() const
{
    return true;
}

//==================================================================================================
// class RapidRTRendererPlugin::ClassDescriptor
//==================================================================================================

RapidRTRendererPlugin::ClassDescriptor::ClassDescriptor()
{

}

RapidRTRendererPlugin::ClassDescriptor::~ClassDescriptor()
{

}

UnifiedRenderer* RapidRTRendererPlugin::ClassDescriptor::CreateRenderer(const bool loading) 
{
    return new RapidRTRendererPlugin(loading);
}

const TCHAR* RapidRTRendererPlugin::ClassDescriptor::InternalName() 
{
    return _T("ART Renderer"); 
}

HINSTANCE RapidRTRendererPlugin::ClassDescriptor::HInstance() 
{
    return GetHInstance();
}

bool RapidRTRendererPlugin::ClassDescriptor::IsCompatibleWithMtlBase(ClassDesc& mtlBaseClassDesc) 
{
    const Class_ID targetClassID = mtlBaseClassDesc.ClassID();

    if(IsCompatibleWrapperMaterial(mtlBaseClassDesc))
    {
        return true;
    }
    else if(IAMGInterface::IsSupported(_T("RapidRT"), mtlBaseClassDesc.SuperClassID(), targetClassID))
    {
        return true;
    }
    else
    {
        const MSTR internal_name = mtlBaseClassDesc.InternalName();
        if(internal_name == _T("mip_rayswitch_environment"))
        {
            return true;
        }
    }

    return false;
}

const MCHAR* RapidRTRendererPlugin::ClassDescriptor::ClassName() 
{
    static const MSTR name = MaxSDK::GetResourceStringAsMSTR(IDS_RAPID_CLASS_NAME);
    return name;
}

Class_ID RapidRTRendererPlugin::ClassDescriptor::ClassID() 
{
    return RapidRTRenderer_CLASS_ID; 
}

MaxSDK::QMaxParamBlockWidget* RapidRTRendererPlugin::ClassDescriptor::CreateQtWidget(
    ReferenceMaker& /*owner*/,
    IParamBlock2& paramBlock,
    const MapID paramMapID,  
    MSTR& rollupTitle, 
    int& /*rollupFlags*/, 
    int& /*rollupCategory*/) 
{
    switch(paramMapID)
    {
    case ParamMapID_RenderParams:
        rollupTitle = MaxSDK::GetResourceStringAsMSTR(IDS_ROLLOUT_TITLE_MAIN);
        return new RenderParamsQtDialog(paramBlock);
    case ParamMapID_ImageQuality:
        rollupTitle = MaxSDK::GetResourceStringAsMSTR(IDS_ROLLOUT_TITLE_IMAGE_QUALITY);
        return new ImageQualityQtDialog();
    case ParamMapID_Advanced:
        rollupTitle = MaxSDK::GetResourceStringAsMSTR(IDS_ROLLOUT_TITLE_ADVANCED);
        return new AdvancedParamsQtDialog();
    default:
        DbgAssert(false);
        return nullptr;
    }
}

//==================================================================================================
// class RapidRTRendererPlugin::ParamBlockAccessor
//==================================================================================================

void RapidRTRendererPlugin::ParamBlockAccessor::Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int /*tabIndex*/, TimeValue t, Interval &valid)
{
    RapidRTRendererPlugin* const renderer = dynamic_cast<RapidRTRendererPlugin*>(owner);
    IParamBlock2* const param_block = (renderer != nullptr) ? renderer->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case ParamID_Termination_TimeSplitSeconds:
            {
                // Split time into hours/minutes/seconds
                const int total_seconds = param_block->GetInt(ParamID_Termination_TimeInSeconds, t, valid);
                const RRTUtil::Time_HMS hms = RRTUtil::Time_HMS::from_seconds(total_seconds);
                v.i = hms.seconds;
            }
            break;
        case ParamID_Termination_TimeSplitMinutes:
            {
                // Split time into hours/minutes/seconds
                const int total_seconds = param_block->GetInt(ParamID_Termination_TimeInSeconds, t, valid);
                const RRTUtil::Time_HMS hms = RRTUtil::Time_HMS::from_seconds(total_seconds);
                v.i = hms.minutes;
            }
            break;
        case ParamID_Termination_TimeSplitHours:
            {
                // Split time into hours/minutes/seconds
                const int total_seconds = param_block->GetInt(ParamID_Termination_TimeInSeconds, t, valid);
                const RRTUtil::Time_HMS hms = RRTUtil::Time_HMS::from_seconds(total_seconds);
                v.i = hms.hours;
            }
            break;
        case ParamID_NoiseFilterStrength_Percentage:
            {
                // Convert strength from [0,1] to [0,100]
                const float strength = param_block->GetFloat(ParamID_NoiseFilterStrength, t, valid);
                v.f = strength * 100.0f;
            }
            break;
        }
    }
}

void RapidRTRendererPlugin::ParamBlockAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int /*tabIndex*/, TimeValue t) 
{
    RapidRTRendererPlugin* const renderer = dynamic_cast<RapidRTRendererPlugin*>(owner);
    IParamBlock2* const param_block = (renderer != nullptr) ? renderer->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case ParamID_Termination_TimeSplitSeconds:
            {
                // Split time into hours/minutes/seconds
                int total_seconds = param_block->GetInt(ParamID_Termination_TimeInSeconds, t);
                RRTUtil::Time_HMS hms = RRTUtil::Time_HMS::from_seconds(total_seconds);
                // Override the seconds
                hms.seconds = v.i;
                total_seconds = hms.get_total_seconds();
                param_block->SetValue(ParamID_Termination_TimeInSeconds, t, total_seconds);
            }
            break;
        case ParamID_Termination_TimeSplitMinutes:
            {
                // Split time into hours/minutes/seconds
                int total_seconds = param_block->GetInt(ParamID_Termination_TimeInSeconds, t);
                RRTUtil::Time_HMS hms = RRTUtil::Time_HMS::from_seconds(total_seconds);
                // Override the minutes
                hms.minutes = v.i;
                total_seconds = hms.get_total_seconds();
                param_block->SetValue(ParamID_Termination_TimeInSeconds, t, total_seconds);
            }
            break;
        case ParamID_Termination_TimeSplitHours:
            {
                // Split time into hours/minutes/seconds
                int total_seconds = param_block->GetInt(ParamID_Termination_TimeInSeconds, t);
                RRTUtil::Time_HMS hms = RRTUtil::Time_HMS::from_seconds(total_seconds);
                // Override the hours
                hms.hours = v.i;
                total_seconds = hms.get_total_seconds();
                param_block->SetValue(ParamID_Termination_TimeInSeconds, t, total_seconds);
            }
            break;
        case ParamID_NoiseFilterStrength_Percentage:
            {
                // Convert strength from [0,100] to [0,1]
                param_block->SetValue(ParamID_NoiseFilterStrength, t, v.f * 0.01f);
            }
            break;
        }
    }
}



}}  // namespace