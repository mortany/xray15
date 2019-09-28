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

// max sdk
#include <RenderingAPI/Renderer/UnifiedRenderer.h>
#include <NotificationAPI/InteractiveRenderingAPI_Subscription.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK::RenderingAPI;
using namespace MaxSDK::NotificationAPI;

class RapidRTRendererPlugin : 
    public UnifiedRenderer,
    private PostLoadCallback
{
public:

    enum ParameterID;
    enum ParamMapID
    {
        ParamMapID_RenderParams,
        ParamMapID_ImageQuality,
        ParamMapID_Advanced
    };

    struct QualityPreset;
    class ClassDescriptor;

    static ClassDescriptor& GetTheClassDescriptor();

    // -- from UnifiedRenderer
    virtual UnifiedRenderer::ClassDescriptor& GetClassDescriptor() const override;
    virtual IOResult Load_UnifiedRenderer(ILoad& iload) override;
    virtual IOResult Save_UnifiedRenderer(ISave& isave) const override;
    virtual void* GetInterface_UnifiedRenderer(ULONG id) override;
    virtual BaseInterface* GetInterface_UnifiedRenderer(Interface_ID id) override;
    virtual std::unique_ptr<IOfflineRenderSession> CreateOfflineSession(IRenderSessionContext& sessionContext) override;
    virtual std::unique_ptr<IInteractiveRenderSession> CreateInteractiveSession(IRenderSessionContext& sessionContext) override;
    virtual bool SupportsInteractiveRendering() const override;
    virtual	void AddTabToDialog(ITabbedDialog* dialog, ITabDialogPluginTab*	tab) override;
    virtual int NumRefs() override;
    virtual RefTargetHandle	GetReference(int i) override;
    virtual void SetReference(int	i, RefTargetHandle rtarg) override;
    virtual int NumParamBlocks() override;
    virtual IParamBlock2* GetParamBlock(int i) override;
    virtual IParamBlock2* GetParamBlockByID(BlockID id) override;
    virtual bool IsStopSupported() const;
    virtual PauseSupport IsPauseSupported() const;
    virtual bool MotionBlurIgnoresNodeProperties() const override;

    // -- from Renderer
    virtual RendParamDlg* CreateParamDialog(IRendParams* ir, BOOL prog) override;
    virtual void GetVendorInformation(MSTR& info) const override;
    virtual void GetPlatformInformation(MSTR& info) const override;

    // -- from IRendererRequirements
    virtual bool HasRequirement(Requirement requirement) override;

    // Returns the set of quality presets. The presets are sorted in ascending order of quality.
    static size_t get_num_quality_presets();
    static const QualityPreset& get_quality_preset(const size_t index);
    // Returns a string containing the name of the quality preset in which the given decibel value fits (only useful for log reporting)
    static TSTR get_matching_quality_preset_name(const float quality_db);

protected:

    // -- from UnifiedRenderer
    virtual bool CompatibleWithAnyRenderElement() const override;
    virtual bool CompatibleWithRenderElement(IRenderElement& pIRenderElement) const override;
    virtual bool GetEnableInteractiveMEditSession() const override;

private:

    class ParamBlockAccessor;

    // Private constructor enforces creation through class descriptor
    RapidRTRendererPlugin(const bool loading);
    virtual ~RapidRTRendererPlugin();

    // inherited from class PostLoadCallback
    virtual void proc(ILoad *iload) override;

private:

    static QualityPreset m_quality_presets[];

    // The one param block
    static ParamBlockAccessor m_pb_accessor;
    static ParamBlockDesc2 m_param_block_desc;
    IParamBlock2* m_param_block;
};

enum RapidRTRendererPlugin::ParameterID
{
    // Rendering method
    ParamID_RenderMethod = 24,        

    // Scene translation options
    ParamID_PointLightDiameter = 25,
    ParamID_MotionBlurAllObjects = 26,

    // Termination criteria
    ParamID_Termination_EnableIterations = 28,
    ParamID_Termination_NumIterations = 0,
    //ParamID_Termination_EnableQuality = 29,       // remove
    ParamID_Termination_Quality_dB = 30,
    ParamID_Termination_EnableTime = 31,
    ParamID_Termination_TimeInSeconds = 32,
    // Computed/transient time parameters, exposed for the sake of the auto UI
    ParamID_Termination_TimeSplitSeconds = 34,
    ParamID_Termination_TimeSplitMinutes = 35,
    ParamID_Termination_TimeSplitHours = 36,

    // Internal/hidden options
    ParamID_EnableOutlierClamp = 33,

    // Image quality
    ParamID_FilterDiameter = 37,

    ParamID_EnableAnimatedNoise = 38,

    // Noise filter
    ParamID_EnableNoiseFilter = 40,
    ParamID_NoiseFilterStrength = 41,
    ParamID_NoiseFilterStrength_Percentage = 42,

    ParamID_Texture_Bake_Resolution = 43,

    // Maximum down-res factor for initial iteration of interactive rendering
    ParamID_Maximum_DownResFactor = 44,

    //*********************************
    // Use this value, and increment it, whenever a new ID is added.
    // NEVER re-use old IDs!
    ParamID_NextID = 45
    //*********************************
};

struct RapidRTRendererPlugin::QualityPreset
{
    // Quality value in dB
    float quality_db;
    // Resource string for UI display
    int resource_string_id;
};

class RapidRTRendererPlugin::ClassDescriptor : public UnifiedRenderer::ClassDescriptor
{
public:

    ClassDescriptor();
    ~ClassDescriptor();

    // -- from UnifiedRenderer::ClassDescriptor
    virtual UnifiedRenderer* CreateRenderer(const bool loading) override;
    virtual	const TCHAR* InternalName() override;
    virtual	HINSTANCE HInstance() override;

    // -- from IMtlRender_Compatibility_Renderer
    virtual bool IsCompatibleWithMtlBase(ClassDesc& mtlBaseClassDesc) override;

    // -- from ClassDesc
    virtual const MCHAR* ClassName() override;
    virtual Class_ID ClassID() override;

protected:

    // -- from ClassDesc2
    virtual MaxSDK::QMaxParamBlockWidget* CreateQtWidget(
        ReferenceMaker& owner,
        IParamBlock2& paramBlock,
        const MapID paramMapID,  
        MSTR& rollupTitle, 
        int& rollupFlags, 
        int& rollupCategory) override;

private:

};

class RapidRTRendererPlugin::ParamBlockAccessor : public PBAccessor
{
public:

    // -- inherited from PBAccessor
    virtual void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) override;
    virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
};


}}  // namespace