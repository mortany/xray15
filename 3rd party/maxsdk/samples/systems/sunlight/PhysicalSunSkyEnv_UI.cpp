//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "PhysicalSunSkyEnv_UI.h"

// local
#include "SunPositioner.h"

using namespace MaxSDK;

//==================================================================================================
// class PhysicalSunSkyEnv::MainPanelWidget
//==================================================================================================

PhysicalSunSkyEnv::MainPanelWidget::MainPanelWidget(IParamBlock2& param_block)
    : m_param_block(&param_block)
{
    // Setup the UI from the .ui file
    m_ui_builder.setupUi(this);

    // Connect signals
    DbgVerify(QObject::connect(m_ui_builder.create_sun_positioner_button, SIGNAL(clicked(bool)), this, SLOT(create_sun_positioner_button_clicked())));

    // Initialize lighting units
    m_ui_builder.night_intensity->setQuantityType(QmaxLightingSpinBox::Luminance);
    m_ui_builder.night_intensity->setInternalUnitSystem(ILightingUnits::DISPLAY_INTERNATIONAL);
    m_ui_builder.perez_diffuse_horizontal_illuminance->setQuantityType( QmaxLightingSpinBox::Illuminance);
    m_ui_builder.perez_diffuse_horizontal_illuminance->setInternalUnitSystem(ILightingUnits::DISPLAY_INTERNATIONAL);
    m_ui_builder.perez_direct_normal_illuminance->setQuantityType( QmaxLightingSpinBox::Illuminance);
    m_ui_builder.perez_direct_normal_illuminance->setInternalUnitSystem(ILightingUnits::DISPLAY_INTERNATIONAL);

    // Associate values with combo box entries
    m_ui_builder.illuminance_model->setItemData(0, kIlluminanceModel_Automatic);
    m_ui_builder.illuminance_model->setItemData(1, IlluminanceModel_Preetham);
    m_ui_builder.illuminance_model->setItemData(2, IlluminanceModel_Perez);
}

PhysicalSunSkyEnv::MainPanelWidget::~MainPanelWidget()
{

}

void PhysicalSunSkyEnv::MainPanelWidget::SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const param_block)
{
    m_param_block = param_block;
}

void PhysicalSunSkyEnv::MainPanelWidget::UpdateUI(const TimeValue t) 
{
    update_illuminance_model_controls(t);
}

void PhysicalSunSkyEnv::MainPanelWidget::UpdateParameterUI(const TimeValue t, const ParamID param_id, const int /*tab_index*/) 
{
    switch(param_id)
    {
    case kMainParamID_IlluminanceModel:
        update_illuminance_model_controls(t);
        break;
    }
}

void PhysicalSunSkyEnv::MainPanelWidget::create_sun_positioner_button_clicked()
{
    if(DbgVerify(m_param_block != nullptr))
    {
        // Create a new sun positioner object, at the origin and with default parameters
        SunPositionerObject* const new_sun_positioner = new SunPositionerObject(false);
        INode* const new_node = GetCOREInterface()->CreateObjectNode(new_sun_positioner);

        // Assign the new sun positioner object to the environment map
        m_param_block->SetValue(kMainParamID_SunPositionObject, 0, new_node);
    }
}

void PhysicalSunSkyEnv::MainPanelWidget::update_illuminance_model_controls(const TimeValue t)
{
    if(DbgVerify(m_param_block != nullptr))
    {
        PhysicalSunSkyEnv* const sky_env = dynamic_cast<PhysicalSunSkyEnv*>(m_param_block->GetOwner());
        if(DbgVerify(sky_env != nullptr)
            && DbgVerify((m_ui_builder.label_illum_using_weather_file != nullptr) && (m_ui_builder.label_illum_not_using_weather_file != nullptr)))
        {
            const TimeValue t = GetCOREInterface()->GetTime();
            Interval dummy_interval;

            // Determine whether to show the perez illuminance values 
            bool show_perez_illuminances = false;
            bool perez_illuminances_read_only = false;
            m_ui_builder.label_illum_using_weather_file->setVisible(false);
            m_ui_builder.label_illum_not_using_weather_file->setVisible(false);
            switch(m_param_block->GetInt(kMainParamID_IlluminanceModel, t, dummy_interval))
            {
            case kIlluminanceModel_Automatic:
                {
                    // Check if a weather data file is being used
                    ISunPositioner* const sun_positioner = sky_env->get_sun_positioner_object(t);
                    ISunPositioner::WeatherMeasurements weather_measurements;
                    show_perez_illuminances = (sun_positioner != nullptr) && sun_positioner->GetWeatherMeasurements(weather_measurements, t, dummy_interval);
                    perez_illuminances_read_only = true;

                    // Select the status message to display
                    m_ui_builder.label_illum_using_weather_file->setVisible(show_perez_illuminances);
                    m_ui_builder.label_illum_not_using_weather_file->setVisible(!show_perez_illuminances);
                    break;
                }
            case IlluminanceModel_Perez:
                show_perez_illuminances = true;
                break;
            }

            if((m_ui_builder.perez_diffuse_horizontal_illuminance != nullptr) && (m_ui_builder.perez_direct_normal_illuminance != nullptr)
                && (m_ui_builder.label_diffus_hor_ill != nullptr) && (m_ui_builder.label_dir_norm_ill != nullptr))
            {
                m_ui_builder.perez_diffuse_horizontal_illuminance->setVisible(show_perez_illuminances);
                m_ui_builder.label_diffus_hor_ill->setVisible(show_perez_illuminances);
                m_ui_builder.perez_direct_normal_illuminance->setVisible(show_perez_illuminances);
                m_ui_builder.label_dir_norm_ill->setVisible(show_perez_illuminances);

                // Set to read-only if needed
                m_ui_builder.perez_diffuse_horizontal_illuminance->setReadOnly(perez_illuminances_read_only);
                m_ui_builder.perez_diffuse_horizontal_illuminance->setButtonSymbols(!perez_illuminances_read_only ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
                m_ui_builder.perez_direct_normal_illuminance->setReadOnly(perez_illuminances_read_only);
                m_ui_builder.perez_direct_normal_illuminance->setButtonSymbols(!perez_illuminances_read_only ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
            }
        }
    }
}
