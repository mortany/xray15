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
#include <renderelements.h>
#include <iparamb2.h>

namespace Max
{;
namespace RapidRTTranslator
{;

class NoiseFilter_RenderElement : 
    // I would derive from IRenderElement, but I need to be a MaxRenderElement to be accessible through maxscript
    public IRenderElement
{
public:

    class ClassDescriptor;

    static ClassDescriptor& GetTheClassDescriptor();

    explicit NoiseFilter_RenderElement(const bool loading);
    ~NoiseFilter_RenderElement();

    // Returns the mix amount for the filtered frame buffer
    // 1.0 = 100% filtered
    float get_filtered_mix_amount() const;

    // -- inherited from IRenderElement
    virtual void SetEnabled(BOOL enabled) override;
    virtual BOOL IsEnabled() const override;
    virtual void SetFilterEnabled(BOOL filterEnabled) override;
    virtual BOOL IsFilterEnabled() const  override;
    virtual BOOL BlendOnMultipass() const  override;
    virtual BOOL AtmosphereApplied() const  override;
    virtual BOOL ShadowsApplied() const  override;
    virtual void SetElementName(const MCHAR* newName) override; 
    virtual const MCHAR* ElementName() const  override;
    virtual void SetPBBitmap(PBBitmap* &pPBBitmap) const override;
    virtual void GetPBBitmap(PBBitmap* &pPBBitmap) const override;
    virtual IRenderElementParamDlg *CreateParamDialog(IRendParams *ip) override;
    virtual BOOL SetDlgThing(IRenderElementParamDlg* dlg) override;
    virtual void* GetInterface(ULONG id)  override;
    virtual void ReleaseInterface(ULONG id, void *i) override;

    // -- inherited from ReferenceTarget
    virtual RefTargetHandle	Clone( RemapDir	&remap ) override;
    virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override;

    // -- inherited from ReferenceMaker
    virtual IOResult Load(ILoad	*iload) override;
    virtual IOResult Save(ISave	*isave) override;
    virtual int NumRefs() override;
    virtual RefTargetHandle	GetReference(int i) override;
    virtual void SetReference(int i, RefTargetHandle rtarg) override;

    // -- inherited from Animatable
    virtual Class_ID ClassID() override;	
    virtual SClass_ID SuperClassID() override;
    virtual void GetClassName(TSTR&	s) override;
    virtual int NumSubs() override;
    virtual TSTR SubAnimName(int i) override;
    virtual Animatable*	SubAnim(int	i) override;
    virtual int NumParamBlocks() override;
    virtual IParamBlock2* GetParamBlock(int i) override;
    virtual IParamBlock2* GetParamBlockByID(BlockID id) override;
    virtual void DeleteThis() override;

protected:

private:

    enum ParameterID;
    class ParamDlg;
    class ParamBlockAccessor;

private:

    static ParamBlockDesc2 m_param_block_desc;
    static ParamBlockAccessor m_pb_accessor;
    IParamBlock2* m_param_block;
};

class NoiseFilter_RenderElement::ClassDescriptor : public ClassDesc2
{
public:

    ClassDescriptor();
    ~ClassDescriptor();

    virtual	int	IsPublic() override;
    virtual	void* Create(BOOL loading) override final;
    virtual	SClass_ID SuperClassID() override final;
    virtual	const TCHAR* Category() override;
    virtual	const TCHAR* InternalName() override;
    virtual	HINSTANCE HInstance() override;
    virtual const MCHAR* ClassName() override;
    virtual Class_ID ClassID() override;

    // -- from ClassDesc2
    virtual MaxSDK::QMaxParamBlockWidget* CreateQtWidget(
        ReferenceMaker& owner,
        IParamBlock2& paramBlock,
        const MapID paramMapID,  
        MSTR& rollupTitle, 
        int& rollupFlags, 
        int& rollupCategory) override;
};

class NoiseFilter_RenderElement::ParamBlockAccessor : public PBAccessor
{
public:

    // -- inherited from PBAccessor
    virtual void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) override;
    virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) override;
};


}}  // namespace