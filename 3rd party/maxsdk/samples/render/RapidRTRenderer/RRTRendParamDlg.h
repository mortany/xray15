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
#include <render.h>
#include <Noncopyable.h>
#include <iparamm2.h>
#include <Qt/QMaxParamBlockWidget.h>
// std
#include <vector>

namespace Max
{;
namespace RapidRTTranslator
{;

//==================================================================================================
// class RenderParamsQtDialog
//
// Qt implementation of "Rendering Parameters" rollup
//
class RenderParamsQtDialog :
    public MaxSDK::QMaxParamBlockWidget
{
    Q_OBJECT
public:
    explicit RenderParamsQtDialog(IParamBlock2& param_block);

    // -- inherited from QMaxParamBlockWidget
    virtual void SetParamBlock(ReferenceMaker* owner, IParamBlock2* const param_block) override;
    virtual void UpdateUI(const TimeValue t) override;
    virtual void UpdateParameterUI(const TimeValue t, const ParamID param_id, const int tab_index) override;

private slots:
    void render_method_combo_changed(int new_index);

private:
    IParamBlock2* m_param_block;
};

//==================================================================================================
// class ImageQualityQtDialog
//
// Qt implementation of "Image Quality" rollup
//
class ImageQualityQtDialog :
    public MaxSDK::QMaxParamBlockWidget
{
    Q_OBJECT
public:
    ImageQualityQtDialog();

    // -- inherited from QMaxParamBlockWidget
    virtual void SetParamBlock(ReferenceMaker* owner, IParamBlock2* const param_block) override;
    virtual void UpdateUI(const TimeValue t) override;
    virtual void UpdateParameterUI(const TimeValue t, const ParamID param_id, const int tab_index) override;
};

//==================================================================================================
// class AdvancedParamsQtDialog
//
// Qt implementation of "Advanced Parameters" rollup
//
class AdvancedParamsQtDialog :
    public MaxSDK::QMaxParamBlockWidget
{
    Q_OBJECT
public:
    AdvancedParamsQtDialog();

    // -- inherited from QMaxParamBlockWidget
    virtual void SetParamBlock(ReferenceMaker* owner, IParamBlock2* const param_block) override;
    virtual void UpdateUI(const TimeValue t) override;
    virtual void UpdateParameterUI(const TimeValue t, const ParamID param_id, const int tab_index) override;
};

}}  // namespace