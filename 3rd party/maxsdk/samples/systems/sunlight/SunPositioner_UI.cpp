//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#include "SunPositioner_UI.h"

// location
#include "sunlight.H"
#include "PhysicalSunSkyEnv.h"
#include "autovis.h"
// max sdk
#include <dllutilities.h>
#include <Qt/QtMax.h>

using namespace MaxSDK;
using namespace MaxSDK::NotificationAPI;

BOOL doLocationDialog(HWND hParent, float* latitude, float* longitude, MSTR& cityName);


namespace
{
	// local helper for setting the spinner range limits
	void updateSpinnerLimit( QSpinBox* spinner, int day, int max_day )
	{
		if ( max_day != spinner->maximum() )
		{
			spinner->blockSignals( true );
			spinner->setMaximum( max_day );
			spinner->setValue( day );
			spinner->blockSignals( false );
		}
	}
}


//==================================================================================================
// class SunPositionerObject::SunPositionRollupWidget
//==================================================================================================

SunPositionerObject::SunPositionRollupWidget::SunPositionRollupWidget(IParamBlock2& param_block)
    : m_param_block(&param_block),
    // Register notification callbacks, to update the state of the "install environment" button when the environment changes
    m_notification_client(INotificationManager::GetManager()->RegisterNewImmediateClient())
{
    // Setup the UI from the .ui file
    m_ui_builder.setupUi(this);

    // Setup the radio button IDs (I couldn't find a way to do this in the Qt designer)
    m_ui_builder.mode->setId(m_ui_builder.manual, kMode_Manual);
    m_ui_builder.mode->setId(m_ui_builder.datetime, kMode_SpaceTime);
    m_ui_builder.mode->setId(m_ui_builder.weatherfile, kMode_WeatherFile);

    // Initially toggle, to ensure proper initial state
    m_ui_builder.datetime->toggle();        
    m_ui_builder.manual->toggle();
    m_ui_builder.weatherfile->toggle();

    m_saved_weather_status_palette = m_ui_builder.label_weatherFileStatus->palette();

    // Setup the initial state for the "install environment" button
    update_environment_button_state();
    // Register notification callbacks, to update the state of the "install environment" button when the environment changes
    if(m_notification_client != nullptr)
    {
        m_notification_client->MonitorRenderEnvironment(~size_t(0), *this, nullptr);
    }
}

SunPositionerObject::SunPositionRollupWidget::~SunPositionRollupWidget()
{
    // get rid of the notification client interface
    if(m_notification_client != nullptr)
    {
        INotificationManager::GetManager()->RemoveClient(m_notification_client);
    }
}

void SunPositionerObject::SunPositionRollupWidget::SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const param_block)
{
    m_param_block = param_block;
}

void SunPositionerObject::SunPositionRollupWidget::updateSpinnerLimitsFromParam( const ParamID param_id, const TimeValue t )
{
	// update the spinner range limits for day spinners
	if ( m_param_block )
	{
		Interval dummy_validity;
		int day = 0, month = 0, year = 0;
		switch ( param_id )
		{
			case kMainParamID_JulianDay:
			{
				// date & time 
				// Convert julian day to d/m/y
				const int previous_julian_day = m_param_block->GetInt( kMainParamID_JulianDay, t, dummy_validity );
				julian2gregorian_int( previous_julian_day, &month, &day, &year );
				int max_day = GetMonthLengthInDays( month, year );
				updateSpinnerLimit( m_ui_builder.day, day, max_day );
				break;
			}
			case kMainParamID_DSTStartMonth:
			case kMainParamID_DSTEndMonth:
			{
				const MainParamID day_param_id = ((param_id == kMainParamID_DSTStartDay) || (param_id == kMainParamID_DSTStartMonth)) ? kMainParamID_DSTStartDay : kMainParamID_DSTEndDay;
				const MainParamID month_param_id = ((param_id == kMainParamID_DSTStartDay) || (param_id == kMainParamID_DSTStartMonth)) ? kMainParamID_DSTStartMonth : kMainParamID_DSTEndMonth;
				auto dstSpinner = (day_param_id == kMainParamID_DSTStartDay) ? m_ui_builder.dst_start_day : m_ui_builder.dst_end_day;

				// daylight saving time - start / end
				month = m_param_block->GetInt( month_param_id, t, dummy_validity );
				day = m_param_block->GetInt( day_param_id, t, dummy_validity );
				int max_day = GetMonthLengthInDays( month, true );     // assume leap year, since we don't have a year associated with this date
				updateSpinnerLimit( dstSpinner, day, max_day );
				break;
			}
		}
	}
}

void SunPositionerObject::SunPositionRollupWidget::UpdateUI(const TimeValue t)
{
	// init ui day min/max limits according to the param month/year settings
	updateSpinnerLimitsFromParam( kMainParamID_JulianDay, t );
	updateSpinnerLimitsFromParam( kMainParamID_DSTStartMonth, t );
	updateSpinnerLimitsFromParam( kMainParamID_DSTEndMonth, t );
}

void SunPositionerObject::SunPositionRollupWidget::UpdateParameterUI(const TimeValue t, const ParamID param_id, const int /*tab_index*/)
{
    update_weather_file_status(t);

	// update the spinner range limits for day spinners
	updateSpinnerLimitsFromParam( param_id, t );


    ParamBlockDesc2* const pb_desc = (m_param_block != nullptr) ? m_param_block->GetDesc() : nullptr;
    if(pb_desc != nullptr)
    {
        switch(param_id)
        {
        case kMainParamID_JulianDay:
            // Date changed: update the date controls
            pb_desc->InvalidateUI(kMainParamID_Day);
            pb_desc->InvalidateUI(kMainParamID_Month);
            pb_desc->InvalidateUI(kMainParamID_Year);
            break;
        case kMainParamID_TimeInSeconds:
            // Time changed: update the time controls
            pb_desc->InvalidateUI(kMainParamID_Hours);
            pb_desc->InvalidateUI(kMainParamID_Minutes);
            break;
        case kMainParamID_WeatherFile:
            update_weather_file_status(t);
            // Invalidate entire param map (most UI controls need to be updated)
            pb_desc->InvalidateUI();
            break;
        }
    }
}

void SunPositionerObject::SunPositionRollupWidget::on_manual_toggled(const bool checked)
{
    // In non-manual mode, make altitude/azimuth spinners read-only, spin-less
    if((m_ui_builder.azimuth_deg != nullptr) && (m_ui_builder.altitude_deg != nullptr))
    {
        m_ui_builder.azimuth_deg->setReadOnly(!checked);
        m_ui_builder.azimuth_deg->setButtonSymbols(checked ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
        m_ui_builder.altitude_deg->setReadOnly(!checked);
        m_ui_builder.altitude_deg->setButtonSymbols(checked ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
    }
}

void SunPositionerObject::SunPositionRollupWidget::on_location_clicked()
{
    SunPositionerObject* const sun_positioner = (m_param_block != nullptr) ? dynamic_cast<SunPositionerObject*>(m_param_block->GetOwner()) : nullptr;
    if(DbgVerify(sun_positioner != nullptr))
    {
        const TimeValue t = GetCOREInterface()->GetTime();
        Interval dummy_interval;

        // Fetch existing parameters
        float latitude = m_param_block->GetFloat(kMainParamID_LatitudeDeg, t, dummy_interval);
        float longitude = m_param_block->GetFloat(kMainParamID_LongitudeDeg, t, dummy_interval);
        MSTR city = m_param_block->GetStr(kMainParamID_Location, t, dummy_interval);

        // Launch the "choose location" dialog
        const HWND hwnd = reinterpret_cast<HWND>( effectiveWinId() );
        if (doLocationDialog(hwnd, &latitude, &longitude, city)) 
        {
            theHold.Begin();
            sun_positioner->set_city_location(t, latitude, longitude, city);
            theHold.Accept(MaxSDK::GetResourceStringAsMSTR(IDS_SUNPOS_UNDO_CITY_CHANGE));
        }
    }
}

void SunPositionerObject::SunPositionRollupWidget::on_pushButton_weatherFile_clicked()
{
    SunPositionerObject* const sun_positioner = (m_param_block != nullptr) ? dynamic_cast<SunPositionerObject*>(m_param_block->GetOwner()) : nullptr;
    if(DbgVerify(sun_positioner != nullptr))
    {
        sun_positioner->do_setup_weather_file_dialog( reinterpret_cast<HWND>( effectiveWinId() ) );
    }    
}

void SunPositionerObject::SunPositionRollupWidget::update_weather_file_status(const TimeValue t)
{
    SunPositionerObject* const sun_positioner = (m_param_block != nullptr) ? dynamic_cast<SunPositionerObject*>(m_param_block->GetOwner()) : nullptr;
    DbgAssert((m_param_block == nullptr) || (sun_positioner != nullptr));   // param block owner must be sun positioner
    if((sun_positioner != nullptr) && (m_ui_builder.label_weatherFileStatus != nullptr))
    {
        // Report the status of the weather file to the user
        bool error = false;
        switch(sun_positioner->m_weather_file_status)
        {
        case WeatherFileStatus::NotLoaded:
            m_ui_builder.label_weatherFileStatus->setText(QStringFromID(IDS_SUNPOS_WEATHER_FILE_NOTLOADED));
            break;
        case WeatherFileStatus::LoadedSuccessfully:
            m_ui_builder.label_weatherFileStatus->setText(QStringFromID(IDS_SUNPOS_WEATHER_FILE_LOADED));
            break;
        case WeatherFileStatus::FileMissing:
            m_ui_builder.label_weatherFileStatus->setText(QStringFromID(IDS_SUNPOS_WEATHER_FILE_NOT_FOUND));
            error = true;
            break;
        default:
            DbgAssert(false);
            // Fall into...
        case WeatherFileStatus::LoadError:
            m_ui_builder.label_weatherFileStatus->setText(QStringFromID(IDS_SUNPOS_WEATHER_FILE_ERROR));
            error = true;
            break;
        }

        if(error)
        {
            // Make text red on error
            QPalette palette = m_ui_builder.label_weatherFileStatus->palette();
            palette.setColor(QPalette::WindowText, QColor(200, 0, 0));
            m_ui_builder.label_weatherFileStatus->setPalette(palette);
        }
        else
        {
            // Reset palette to system defaults
            m_ui_builder.label_weatherFileStatus->setPalette(m_saved_weather_status_palette);
        }

        // Set filename as tooltip
        const MSTR filename = m_param_block->GetStr(kMainParamID_WeatherFile, t);
        m_ui_builder.label_weatherFileStatus->setToolTip(filename);
        if(m_ui_builder.pushButton_weatherFile != nullptr)
        {
            m_ui_builder.pushButton_weatherFile->setToolTip(filename);
        }
    }
}

void SunPositionerObject::SunPositionRollupWidget::on_pushButton_installEnvironment_clicked()
{
    SunPositionerObject* const sun_positioner = (m_param_block != nullptr) ? dynamic_cast<SunPositionerObject*>(m_param_block->GetOwner()) : nullptr;
    if(DbgVerify(sun_positioner != nullptr))
    {
        sun_positioner->install_sunsky_environment(true, true);

        // Update the button state
        update_environment_button_state();
    }
}

void SunPositionerObject::SunPositionRollupWidget::update_environment_button_state()
{
    if(m_ui_builder.pushButton_installEnvironment != nullptr)
    {
        // Get the current environment map
        Texmap* const env_map = GetCOREInterface()->GetEnvironmentMap();
        PhysicalSunSkyEnv* const sunsky_env = dynamic_cast<PhysicalSunSkyEnv*>(env_map);
        if(sunsky_env == nullptr)
        {
            // Sun & sky environment is not present: let the user install it
            m_ui_builder.pushButton_installEnvironment->setEnabled(true);
            m_ui_builder.pushButton_installEnvironment->setText(QStringFromID(IDS_SUNPOS_INSTALL_SUNSKY_ENV_BUTTON));
            m_ui_builder.pushButton_installEnvironment->setToolTip(QStringFromID(IDS_SUNPOS_INSTALL_SUNSKY_ENV_BUTTON_TOOLTIP));
        }
        else
        {
            SunPositionerObject* const sun_positioner = (m_param_block != nullptr) ? dynamic_cast<SunPositionerObject*>(m_param_block->GetOwner()) : nullptr;
            if(DbgVerify(sun_positioner != nullptr))
            {
                // Check whether the sun & sky environment points to our sun positioner
                bool is_our_sun_positioner_used = false;
                ISunPositioner* const sunsky_sunpos_used = sunsky_env->get_sun_positioner_object(GetCOREInterface()->GetTime());
                is_our_sun_positioner_used = (sunsky_sunpos_used == sun_positioner);

                if(is_our_sun_positioner_used)
                {
                    // Our sun positioner is being used - disable the button as there's nothing to install
                    m_ui_builder.pushButton_installEnvironment->setEnabled(false);
                    m_ui_builder.pushButton_installEnvironment->setText(QStringFromID(IDS_SUNPOS_ALREADYINSTALLED_SUNSKY_ENV_BUTTON));
                    m_ui_builder.pushButton_installEnvironment->setToolTip(QStringFromID(IDS_SUNPOS_ALREADYINSTALLED_SUNSKY_ENV_BUTTON_TOOLTIP));
                }
                else
                {
                    // Our sun positioner isn't used - let the user re-direct the current sun & sky to it
                    m_ui_builder.pushButton_installEnvironment->setEnabled(true);
                    m_ui_builder.pushButton_installEnvironment->setText(QStringFromID(IDS_SUNPOS_REDIRECT_SUNSKY_ENV_BUTTON));
                    m_ui_builder.pushButton_installEnvironment->setToolTip(QStringFromID(IDS_SUNPOS_REDIRECT_SUNSKY_ENV_BUTTON_TOOLTIP));
                }
            }
        }
    }
}

void SunPositionerObject::SunPositionRollupWidget::NotificationCallback_NotifyEvent(const IGenericEvent& /*genericEvent*/, void* /*userData*/)
{
    // Update the "install environment" button
    update_environment_button_state();
}