//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license
//  agreement provided at the time of installation or download, or which
//  otherwise accompanies this software in either electronic or hard copy form.
//
//////////////////////////////////////////////////////////////////////////////
#pragma	once

#include "NoiseFilter_RenderElement.h"

// local includes
#include "../resource.h"

// max sdk
#include <dllutilities.h>
#include <iparamm2.h>
#include <Qt/QmaxFloatSlider.h>
#include <Qt/QMaxParamBlockWidget.h>

namespace Max
{;
namespace RapidRTTranslator
{;

using namespace MaxSDK;

//==================================================================================================
// class NoiseFilter_RenderElement
//==================================================================================================

enum NoiseFilter_RenderElement::ParameterID
{
    ParamID_Enabled = 0,
    ParamID_Bitmap = 1,
    ParamID_Strength = 2,
    ParamID_Name = 3,
    ParamID_Strength_Percentage = 4
};

NoiseFilter_RenderElement::ParamBlockAccessor NoiseFilter_RenderElement::m_pb_accessor;

ParamBlockDesc2 NoiseFilter_RenderElement::m_param_block_desc( 
		0,  // param block ID
		_T("NoiseFilter_RenderElement_Params"),	// internal name
		IDS_NOISEFILTER_ELEMENT_PARAMBLOCK_NAME,  // resource ID for localizable name
		&GetTheClassDescriptor(),           // class descriptor
    	P_AUTO_CONSTRUCT | P_VERSION | P_AUTO_UI_QT,       // flags
        1,      // version
        0,      // reference number (required because of P_AUTO_CONSTRUCT)
 
    ParamID_Name, _T("elementName"), TYPE_STRING, 0, IDS_NOISEFILTER_ELEMENT_NAME,
        p_end,
    ParamID_Enabled, _T("enabled"), TYPE_BOOL, 0, IDS_NOISEFILTER_ELEMENT_BITMAP,
        p_default, true,
        p_end,	
	ParamID_Bitmap, _T("bitmap"), TYPE_BITMAP, 0, IDS_NOISEFILTER_ELEMENT_BITMAP,
	    p_end,		
	ParamID_Strength, _T("strength"), TYPE_FLOAT, 0, IDS_NOISEFILTER_ELEMENT_STRENGTH,
		p_default, 0.5f, 
		p_range, 0.0f, 1.0f,
	    p_end,	
    // Exposes the strength as percentage, for UI exposure
	ParamID_Strength_Percentage, _T("strength_percentage"), TYPE_FLOAT, P_TRANSIENT, IDS_NOISEFILTER_ELEMENT_STRENGTH_PERCENTAGE,
        p_accessor, &m_pb_accessor,
	    p_end,	

	p_end							
);

NoiseFilter_RenderElement::NoiseFilter_RenderElement(const bool loading)
    : m_param_block(nullptr)
{
    if(!loading)
    {
        GetTheClassDescriptor().MakeAutoParamBlocks(this); 
        SetElementName(GetTheClassDescriptor().ClassName());
    }
}

NoiseFilter_RenderElement::~NoiseFilter_RenderElement()
{

}

NoiseFilter_RenderElement::ClassDescriptor& NoiseFilter_RenderElement::GetTheClassDescriptor()
{
    static ClassDescriptor desc;
    return desc;
}

void NoiseFilter_RenderElement::SetEnabled(BOOL enabled)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        m_param_block->SetValue(ParamID_Enabled, 0, enabled);
    }
}

BOOL NoiseFilter_RenderElement::IsEnabled() const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        Interval dummy_validity;
        return m_param_block->GetInt(ParamID_Enabled, 0, dummy_validity);
    }
    else
    {
        return false;
    }
}

void NoiseFilter_RenderElement::SetFilterEnabled(BOOL /*filterEnabled*/)
{
    // Not used
}

BOOL NoiseFilter_RenderElement::IsFilterEnabled() const 
{
    // Not used
    return true;
}

BOOL NoiseFilter_RenderElement::BlendOnMultipass() const 
{
    return true;
}

BOOL NoiseFilter_RenderElement::AtmosphereApplied() const 
{
    return true;
}

BOOL NoiseFilter_RenderElement::ShadowsApplied() const 
{
    return true;
}

void NoiseFilter_RenderElement::SetElementName(const MCHAR* newName)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        m_param_block->SetValue(ParamID_Name, 0, newName);
    }
}
 
const MCHAR* NoiseFilter_RenderElement::ElementName() const 
{
    if(DbgVerify(m_param_block != nullptr))
    {
        Interval dummy_validity;
        return m_param_block->GetStr(ParamID_Name, 0, dummy_validity);
    }
    else
    {
        return _T("");
    }
}

void NoiseFilter_RenderElement::SetPBBitmap(PBBitmap* &pPBBitmap) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        m_param_block->SetValue(ParamID_Bitmap, 0, pPBBitmap);
    }
}

void NoiseFilter_RenderElement::GetPBBitmap(PBBitmap* &pPBBitmap) const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        Interval dummy_validity;
        pPBBitmap = m_param_block->GetBitmap(ParamID_Bitmap, 0, dummy_validity);
    }
    else
    {
        pPBBitmap = nullptr;
    }
}

IRenderElementParamDlg *NoiseFilter_RenderElement::CreateParamDialog(IRendParams* ip)
{
    return GetTheClassDescriptor().CreateParamDialogs(ip, this);
}

BOOL NoiseFilter_RenderElement::SetDlgThing(IRenderElementParamDlg* /*dlg*/)
{
    return false;
}

void* NoiseFilter_RenderElement::GetInterface(ULONG id) 
{
    return SpecialFX::GetInterface(id);
}

void NoiseFilter_RenderElement::ReleaseInterface(ULONG id, void* i)
{
    return SpecialFX::ReleaseInterface(id, i);
}

RefTargetHandle	NoiseFilter_RenderElement::Clone( RemapDir	&remap )
{
    NoiseFilter_RenderElement* clone = new NoiseFilter_RenderElement(false);

    const int num_refs = NumRefs();
    for(int i = 0; i < num_refs; ++i)
    {
        clone->ReplaceReference(i, remap.CloneRef(GetReference(i))); 
    }
    BaseClone(this, clone, remap);

    return clone;
}

RefResult NoiseFilter_RenderElement::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle /*hTarget*/, PartID& /*partID*/, RefMessage /*message*/, BOOL /*propagate*/)
{
    return REF_SUCCEED;
}

IOResult NoiseFilter_RenderElement::Load(ILoad	*iload)
{
    return __super::Load(iload);
}

IOResult NoiseFilter_RenderElement::Save(ISave	*isave)
{
    return __super::Save(isave);
}

int NoiseFilter_RenderElement::NumRefs() 
{
    return 1;
}

RefTargetHandle	NoiseFilter_RenderElement::GetReference(int i)
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

void NoiseFilter_RenderElement::SetReference(int i, RefTargetHandle rtarg)
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

Class_ID NoiseFilter_RenderElement::ClassID()
{
    return GetTheClassDescriptor().ClassID();
}
	
SClass_ID NoiseFilter_RenderElement::SuperClassID()
{
    return GetTheClassDescriptor().SuperClassID();
}

void NoiseFilter_RenderElement::GetClassName(TSTR&	s)
{
    s = GetTheClassDescriptor().ClassName();
}

int NoiseFilter_RenderElement::NumSubs()
{
    return NumParamBlocks();
}

TSTR NoiseFilter_RenderElement::SubAnimName(int i)
{
    IParamBlock2* param_block = GetParamBlock(i);
    return (param_block != nullptr) ? param_block->GetLocalName() : _T("");    
}

Animatable*	NoiseFilter_RenderElement::SubAnim(int	i)
{
    return GetParamBlock(i);
}

int NoiseFilter_RenderElement::NumParamBlocks()
{
    return 1;
}

IParamBlock2* NoiseFilter_RenderElement::GetParamBlock(int i)
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

IParamBlock2* NoiseFilter_RenderElement::GetParamBlockByID(BlockID id)
{
    switch(id)
    {
    case 0:
        return m_param_block;
    default:
        return nullptr;
    }
}

void NoiseFilter_RenderElement::DeleteThis()
{
    delete this;
}

float NoiseFilter_RenderElement::get_filtered_mix_amount() const
{
    if(DbgVerify(m_param_block != nullptr))
    {
        Interval dummy_validity;
        return m_param_block->GetFloat(ParamID_Strength, 0, dummy_validity);
    }
    else
    {
        return 0.0f;
    }
}

//==================================================================================================
// class NoiseFilter_RenderElement::ClassDescriptor
//==================================================================================================

NoiseFilter_RenderElement::ClassDescriptor::ClassDescriptor()
{

}

NoiseFilter_RenderElement::ClassDescriptor::~ClassDescriptor()
{

}

int	NoiseFilter_RenderElement::ClassDescriptor::IsPublic()
{
    return true;
}

void* NoiseFilter_RenderElement::ClassDescriptor::Create(BOOL loading) 
{
    IRenderElement* const element = new NoiseFilter_RenderElement(!!loading);
    return element;
}

SClass_ID NoiseFilter_RenderElement::ClassDescriptor::SuperClassID() 
{
    return RENDER_ELEMENT_CLASS_ID;
}

const TCHAR* NoiseFilter_RenderElement::ClassDescriptor::Category()
{
    return _T("");
}

const TCHAR* NoiseFilter_RenderElement::ClassDescriptor::InternalName()
{
    return _T("RapidRT Noise Filter");
}

HINSTANCE NoiseFilter_RenderElement::ClassDescriptor::HInstance()
{
    return GetHInstance();
}

const MCHAR* NoiseFilter_RenderElement::ClassDescriptor::ClassName()
{
    static const MSTR name = MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_ELEMENT_CLASSNAME);
    return name;
}

Class_ID NoiseFilter_RenderElement::ClassDescriptor::ClassID()
{
    return Class_ID(0x36b012c5, 0x77bc7161);
}

MaxSDK::QMaxParamBlockWidget* NoiseFilter_RenderElement::ClassDescriptor::CreateQtWidget(
    ReferenceMaker& /*owner*/,
    IParamBlock2& /*paramBlock*/,
    const MapID /*paramMapID*/,  
    MSTR& rollupTitle, 
    int& /*rollupFlags*/, 
    int& /*rollupCategory*/) 
{
    // Simple/dummy wrapper, don't care about the param block
    class MyWidgetClass : public MaxSDK::QMaxParamBlockWidget
    {
    public:
        virtual void SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const /*param_block*/) override {}
        virtual void UpdateUI(const TimeValue /*t*/) override {}
        virtual void UpdateParameterUI(const TimeValue /*t*/, const ParamID /*param_id*/, const int /*tab_index*/) override {}
    };

    // Create the Qt dialog
    MyWidgetClass* qt_dialog = new MyWidgetClass();
    QVBoxLayout* dialog_layout = new QVBoxLayout(qt_dialog);

    // Strength slider
    QGroupBox* strength_group = new QGroupBox(QStringFromID(IDS_FILTER_STRENGTH));
    {
        QmaxFloatSlider* strength_slider = nullptr;
        {
            // Setup tick mark presets
            std::vector<std::pair<QString, float>> tick_marks;
            //tick_marks.push_back(std::pair<QString, float>(QStringFromID(preset.resource_string_id), preset.quality_db));
            tick_marks.push_back(std::pair<QString, float>(QStringFromID(IDS_NOISEFILTER_ELEMENT_UNFILTERED), 0.0f));
            tick_marks.push_back(std::pair<QString, float>(QStringFromID(IDS_NOISEFILTER_ELEMENT_25PCT), 25.0f));
            tick_marks.push_back(std::pair<QString, float>(QStringFromID(IDS_NOISEFILTER_ELEMENT_50PCT), 50.0f));
            tick_marks.push_back(std::pair<QString, float>(QStringFromID(IDS_NOISEFILTER_ELEMENT_75PCT), 75.0f));
            tick_marks.push_back(std::pair<QString, float>(QStringFromID(IDS_NOISEFILTER_ELEMENT_FULLYFILTERED), 100.0f));

            strength_slider = new QmaxFloatSlider(QmaxFloatSlider::TickMarkPosition::Above, tick_marks);
            strength_slider->setSpinboxLabel(QStringFromID(IDS_NOISEFILER_ELEMENT_STRENGTH_LABEL));
            strength_slider->setSpinboxDecimals(0);
            strength_slider->setSpinboxSingleStep(1.0f);
            strength_slider->setSpinboxSuffix(QStringFromID(IDS_NOISEFILTER_ELEMENT_STRENGTH_UNIT));
            strength_slider->setToolTip(QStringFromID(IDS_NOISEFITLER_ELEMENT_STRENGTH_TOOLTIP));
            strength_slider->setObjectName("strength_percentage");
        }

        // Build group box layout
        QVBoxLayout* group_layout = new QVBoxLayout(strength_group);
        group_layout->addWidget(strength_slider);
    }
    dialog_layout->addWidget(strength_group);

    rollupTitle = MaxSDK::GetResourceStringAsMSTR(IDS_NOISEFILTER_ELEMENT_CLASSNAME);
    return qt_dialog;
}

//==================================================================================================
// class NoiseFilter_RenderElement::ParamBlockAccessor
//==================================================================================================

void NoiseFilter_RenderElement::ParamBlockAccessor::Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int /*tabIndex*/, TimeValue t, Interval &valid)
{
    NoiseFilter_RenderElement* const element = dynamic_cast<NoiseFilter_RenderElement*>(owner);
    IParamBlock2* const param_block = (element != nullptr) ? element->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case ParamID_Strength_Percentage:
            {
                // Convert strength from [0,1] to [0,100]
                const float strength = param_block->GetFloat(ParamID_Strength, t, valid);
                v.f = strength * 100.0f;
            }
            break;
        }
    }
}

void NoiseFilter_RenderElement::ParamBlockAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int /*tabIndex*/, TimeValue t) 
{
    NoiseFilter_RenderElement* const element = dynamic_cast<NoiseFilter_RenderElement*>(owner);
    IParamBlock2* const param_block = (element != nullptr) ? element->m_param_block : nullptr;
    if(DbgVerify(param_block != nullptr))
    {
        switch(id)
        {
        case ParamID_Strength_Percentage:
            {
                // Convert strength from [0,100] to [0,1]
                param_block->SetValue(ParamID_Strength, t, v.f * 0.01f);
            }
            break;
        }
    }
}


}}  // namespace