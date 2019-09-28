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

// Max SDK
#include <DaylightSimulation/IPhysicalSunSky.h>
#include <DaylightSimulation/ISunPositioner.h>
#include <IMtlRender_Compatibility.h>
#include <IMaterialBrowserEntryInfo.h>
#include <imtl.h>
#include <iparamb2.h>
#include <IRTShaderParameterBinding.h>

class SunPositionerObject;

//==================================================================================================
// class PhysicalSunSkyEnv
//
// Texmap plugin for the Physical Sun & Sky environment
//
class PhysicalSunSkyEnv : 
    public MaxSDK::IPhysicalSunSky,
    public Texmap,
    public IRTShaderParameterBinding
{
public:			

	explicit PhysicalSunSkyEnv(const bool loading);
	~PhysicalSunSkyEnv();

    static ClassDesc2& get_class_descriptor();

    // Accesses the sun positioner object being referenced by this map
    INode* get_sun_positioner_node() const;
    MaxSDK::ISunPositioner* get_sun_positioner_object(const TimeValue t) const;
    void set_sun_positioner_object(INode* node);

    // -- from IPhysicalSunSky
    virtual ShadingParameters EvaluateShadingParameters(const TimeValue t, Interval& validity) const override;
    virtual std::unique_ptr<IShader> InstantiateShader(const TimeValue t, Interval& validity) const override;
    
    // -- from Texmap
    virtual AColor EvalColor(ShadeContext& sc) override;
    virtual Point3 EvalNormalPerturb(ShadeContext& sc) override;
    virtual int GetUVWSource() override;
    virtual int MapSlotType(int i) override;
    virtual int IsHighDynamicRange( ) const override;

    // -- from MtlBase
    virtual  void Update(TimeValue t, Interval& valid) override;
    virtual void Reset() override;
    virtual Interval Validity(TimeValue t) override;
    virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) override;

    // -- from class Animatable, misc.
    virtual void* GetInterface(ULONG id) override;
    virtual BaseInterface* GetInterface(Interface_ID id) override;
    virtual Class_ID ClassID() override;	
    virtual SClass_ID SuperClassID() override;
    virtual void GetClassName(TSTR&	s) override;
    virtual int NumSubs() override;
    virtual TSTR SubAnimName(int i) override;
    virtual Animatable*	SubAnim(int	i) override;
    virtual void DeleteThis() override;
    virtual int NumParamBlocks() override;
    virtual IParamBlock2* GetParamBlock(int i) override;
    virtual IParamBlock2* GetParamBlockByID(BlockID id) override;

    // -- from ReferenceTarget, ReferenceMaker
    virtual RefTargetHandle	Clone( RemapDir	&remap ) override;
    virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override;
    virtual IOResult Load(ILoad	*iload) override;
    virtual IOResult Save(ISave	*isave) override;
    virtual int NumRefs() override;
    virtual RefTargetHandle	GetReference(int i) override;
    virtual void SetReference(int i, RefTargetHandle rtarg) override;

    // -- from IRTShaderParameterBinding
    virtual void BindParameter(const TCHAR * paramName, LPVOID value)override;
    virtual void BindParameter(const TimeValue time, const TCHAR * paramName, LPVOID value)override;
    virtual void InitializeBinding() override;

private:

    enum ParamBlockID
    {
        kParamBlockID_Main = 0
    };
    enum ReferenceID
    {
        kReferenceID_MainPB = 0,

        kReferenceID_NumRefs
    };
    enum MainParamID
    {
        kMainParamID_SunPositionObject = 0,
        kMainParamID_GlobalIntensity = 1,
        kMainParamID_Haze = 2,
        kMainParamID_DiscIntensity = 3,
        kMainParamID_DiscGlowIntensity = 4,
        kMainParamID_DiscScale = 5,
        kMainParamID_SkyIntensity = 6,
        kMainParamID_NightColor = 7,
        kMainParamID_NightIntensity = 8,
        kMainParamID_HorizonHeight = 9,
        kMainParamID_HorizonBlur = 10,
        kMainParamID_GroundColor = 11,
        kMainParamID_Saturation = 12,
        kMainParamID_Tint = 13,
        kMainParamID_DiscScale_Percent = 14,
        kMainParamID_IlluminanceModel = 15,
        kMainParamID_PerezDiffuseHorizontalIlluminance = 16,        // in lux
        kMainParamID_PerezDirectNormalIlluminance = 17,              // in lux
        kMainParamID_SunEnabled = 18              
    };
    enum IOChunkID
    {
        IOChunkID_MtlBase = 0,
    };
    enum IlluminanceModel
    {
        kIlluminanceModel_Automatic,    // if weather file present on sun positioner, use it with Perez. Otherwise use Preetham.
        IlluminanceModel_Preetham,
        IlluminanceModel_Perez,
    };

    class ClassDescriptor;
    class MainPanelWidget;
    class ParamBlockAccessor;
    class Shader;
    class ShaderGenerator;

    // Returns the direction of the sun currently being used
    Point3 get_sun_direction(const TimeValue t, Interval& validity) const;

private:

    static ParamBlockDesc2 m_pb_desc;
    static ParamBlockAccessor m_pb_accessor;
    IParamBlock2* m_param_block;

    // Static instance of this core interface
    static ShaderGenerator m_shader_generator;
    
    // The shader instance that was created by the call to Update()
    std::unique_ptr<IShader> m_shader;
    Interval m_shader_valdity;

    static AColor getLightScale(const TimeValue t, Interval &valid);
};

//==================================================================================================
// class PhysicalSunSkyEnv::ClassDescriptor
//
class PhysicalSunSkyEnv::ClassDescriptor : 
    public ClassDesc2,
    public IMtlRender_Compatibility_MtlBase,
    public IMaterialBrowserEntryInfo
{
public:

    ClassDescriptor();
    ~ClassDescriptor();

    // -- from ClassDesc
    virtual int IsPublic() override;
    virtual void *Create(BOOL loading) override;
    virtual const TCHAR *ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const TCHAR* Category() override;
    virtual HINSTANCE HInstance() override;
    virtual const MCHAR* InternalName() override;
    virtual FPInterface* GetInterface(Interface_ID id) override;

    // -- from ClassDesc2
    virtual MaxSDK::QMaxParamBlockWidget* CreateQtWidget(
        ReferenceMaker& owner,
        IParamBlock2& paramBlock,
        const MapID paramMapID,  
        MSTR& rollupTitle, 
        int& rollupFlags, 
        int& rollupCategory) override;

    // -- from IMtlRender_Compatibility_MtlBase
    virtual bool IsCompatibleWithRenderer(ClassDesc& rendererClassDesc) override;

    // -- from IMaterialBrowserEntryInfo
    virtual const MCHAR* GetEntryName() const override;
    virtual const MCHAR* GetEntryCategory() const override;
    virtual Bitmap* GetEntryThumbnail() const override;

private:

    // Storage for the class name, such that we may return a TCHAR*.
    const MSTR m_class_name;
};

//==================================================================================================
// class PhysicalSunSkyEnv::ParamBlockAccessor
//
class PhysicalSunSkyEnv::ParamBlockAccessor : public PBAccessor
{
public:

    // -- inherited from PBAccessor
    virtual void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) override;
    virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
};

//==================================================================================================
// class PhysicalSunSkyEnv::Shader
//
// Implementation of the environment shader object.
//
class PhysicalSunSkyEnv::Shader : public IPhysicalSunSky::IShader
{
public:

    explicit Shader(const ShadingParameters& shading_parameters);
    ~Shader();

    // -- inherited from IPhysicalSunSky::IShader
    virtual Color Evaluate(const Point3& direction) const override;

private:

    const ShadingParameters m_shading_parameters;
};

//==================================================================================================
// class PhysicalSunSkyEnv::ShaderGenerator
//
class PhysicalSunSkyEnv::ShaderGenerator : 
    public IPhysicalSunSky::IShaderGenerator
{
public:

    // Interface constructor declarator
    DECLARE_DESCRIPTOR_INIT(ShaderGenerator);

    // -- inherited from IShaderGenerator
    virtual ShadingParameters CreateShadingParameters(
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
        Interval& validity) const override;
    virtual std::unique_ptr<IShader> InstantiateShader(const ShadingParameters& shading_parameters) const override;
};

