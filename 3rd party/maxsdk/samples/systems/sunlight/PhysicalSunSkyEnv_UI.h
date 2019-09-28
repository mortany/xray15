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

// local
#include "PhysicalSunSkyEnv.h"

// Max SDK
#include <Qt/QMaxParamBlockWidget.h>

// Qt
#include "ui_PhysSunSky.h"
#include <QtWidgets/QWidget>

//==================================================================================================
// class PhysicalSunSkyEnv::MainPanelWidget
//
// Qt widget that implements the UI for the main panel of the physical sun & sky environment.
//
class PhysicalSunSkyEnv::MainPanelWidget : 
    public MaxSDK::QMaxParamBlockWidget
{
    Q_OBJECT
public:

    explicit MainPanelWidget(IParamBlock2& param_block);
    ~MainPanelWidget();

    // -- inherited from QMaxParamBlockWidget
    virtual void SetParamBlock(ReferenceMaker* owner, IParamBlock2* const param_block) override;
    virtual void UpdateUI(const TimeValue t) override;
    virtual void UpdateParameterUI(const TimeValue t, const ParamID param_id, const int tab_index) override;

protected slots:

    void create_sun_positioner_button_clicked();

private:

    void update_illuminance_model_controls(const TimeValue t);

private:

    // UI designer object
    Ui_PhysSunSky m_ui_builder;
    IParamBlock2* m_param_block;
};
