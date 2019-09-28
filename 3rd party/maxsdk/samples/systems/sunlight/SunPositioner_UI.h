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
#include "SunPositioner.h"
// max sdk
#include <NotificationAPI/NotificationAPI_Subscription.h>
#include <Qt/QMaxParamBlockWidget.h>
// Qt
#include "ui_SunPositioner_SunPosition.h"
#include <QtWidgets/QWidget>

//==================================================================================================
// class SunPositionerObject::SunPositionRollupWidget
//
// Qt widget that implements the UI for the "Sun Position" rollup
//
class SunPositionerObject::SunPositionRollupWidget : 
    public MaxSDK::QMaxParamBlockWidget,
    private MaxSDK::NotificationAPI::INotificationCallback
{
    Q_OBJECT
public:

    explicit SunPositionRollupWidget(IParamBlock2& param_block);
    ~SunPositionRollupWidget();

    // -- inherited from QMaxParamBlockWidget
    virtual void SetParamBlock(ReferenceMaker* owner, IParamBlock2* const param_block) override;
    virtual void UpdateUI(const TimeValue t) override;
    virtual void UpdateParameterUI(const TimeValue t, const ParamID param_id, const int tab_index) override;

protected slots:

    void on_manual_toggled(const bool checked);
    void on_location_clicked();
    void on_pushButton_weatherFile_clicked();
    void on_pushButton_installEnvironment_clicked();

private:

	void updateSpinnerLimitsFromParam( const ParamID param_id, const TimeValue t );
    void update_weather_file_status(const TimeValue t);
    // Updates the state of the "install environment map" button
    void update_environment_button_state();

    // -- inherited from INotificationCallback
    virtual void NotificationCallback_NotifyEvent(const MaxSDK::NotificationAPI::IGenericEvent& genericEvent, void* userData) override;

private:

    // UI designer object
    Ui_SunPositioner_SunPosition m_ui_builder;
    IParamBlock2* m_param_block;

    // Saved palette before overriding color for the weather status label
    QPalette m_saved_weather_status_palette;

    // Notification API interface used to listen for environment changes
    MaxSDK::NotificationAPI::IImmediateNotificationClient* m_notification_client;
};
