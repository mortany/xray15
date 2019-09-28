//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license
//  agreement provided at the time of installation or download, or which
//  otherwise accompanies this software in either electronic or hard copy form.
//
//////////////////////////////////////////////////////////////////////////////
#include "RRTRendParamDlg.h"

// local
#include "resource.h"
#include "RapidRTRendererPlugin.h"
#include "Util.h"
// max sdk
#include <iparamm2.h>
#include <Qt/QmaxFloatSlider.h>
#include <Qt/QmaxSpinBox.h>
// rti
#include <rti/scene/renderoptions.h>

#undef min
#undef max

using namespace MaxSDK;

namespace Max
{;
namespace RapidRTTranslator
{;

namespace
{
    // Connects the toggled state of the first button to the enabled state of the second button
    void connect_enabled_toggle(QAbstractButton& toggle, QWidget& enable)
    {
        DbgVerify(QObject::connect(&toggle, SIGNAL(toggled(bool)), &enable, SLOT(setEnabled(bool))));

        // Initially toggle the button to force the state to be changed and the toggle signal to be triggered.
        toggle.toggle();
    }
}

//==================================================================================================
// class RenderParamsQtDialog
//==================================================================================================

RenderParamsQtDialog::RenderParamsQtDialog(IParamBlock2& param_block)
    : m_param_block(&param_block)
{
    QVBoxLayout* dialog_layout = new QVBoxLayout(this);

    // Termination
    {
        // Group box
        QGroupBox* group = new QGroupBox(QStringFromID(IDS_TITLE_TERMINATION_GROUP));

        // Quality slider
        QmaxFloatSlider* quality_slider = nullptr;
        {
            // Setup tick mark presets
            std::vector<std::pair<QString, float>> tick_marks;
            const int num_quality_resets = static_cast<int>(RapidRTRendererPlugin::get_num_quality_presets());
            tick_marks.reserve(num_quality_resets);
            for(int i = 0; i < num_quality_resets; ++i)
            {
                const RapidRTRendererPlugin::QualityPreset& preset = RapidRTRendererPlugin::get_quality_preset(i);
                tick_marks.push_back(std::pair<QString, float>(QStringFromID(preset.resource_string_id), preset.quality_db));
            }

            quality_slider = new QmaxFloatSlider(QmaxFloatSlider::TickMarkPosition::Above, tick_marks);
            quality_slider->setSpinboxLabel(QStringFromID(IDS_QUALITY_SPINBOX_LABEL));
            quality_slider->setSpinboxDecimals(1);
            quality_slider->setSpinboxSingleStep(0.1f);
            quality_slider->setSpinboxSuffix(QStringFromID(IDS_DECIBELS_UNIT_SUFFIX));
        }

        // Time
        QCheckBox* time_checkbox = new QCheckBox(QStringFromID(IDS_TITLE_TIME_CHECKBOX));
        QSpinBox* hours_spinbox = new QmaxSpinBox();
        hours_spinbox->setSuffix(QStringFromID(IDS_HOURS_UNIT_SUFFIX));
        QSpinBox* minutes_spinbox = new QmaxSpinBox();
        minutes_spinbox->setSuffix(QStringFromID(IDS_MINUTES_UNIT_SUFFIX));
        QSpinBox* seconds_spinbox = new QmaxSpinBox();
        seconds_spinbox->setSuffix(QStringFromID(IDS_SECONDS_UNIT_SUFFIX));
        // Fit hours/minutes/seconds into their own layout
        QGridLayout* hms_layout = new QGridLayout();
        hms_layout->addWidget(hours_spinbox, 0, 0);
        hms_layout->addWidget(minutes_spinbox, 0, 1);
        hms_layout->addWidget(seconds_spinbox, 0, 2);

        // Iterations
        QCheckBox* iterations_checkbox = new QCheckBox(QStringFromID(IDS_TITLE_ITERATIONS_CHECKBOX));
        QSpinBox* iterations_spinbox = new QmaxSpinBox();

        // Build group box layout
        {
            const int grid_width = 100;
            const int grid_one_third = grid_width * 1 / 3;
            const int grid_two_third = grid_width * 2 / 3; 

            int row = 0;
            QGridLayout* group_layout = new QGridLayout(group);

            // Quality controls
            group_layout->addWidget(quality_slider, row++, 0, 1, grid_width);

            // Empty row
            group_layout->addWidget(new QLabel(), row++, 0);

            // Time & iteration controls
            group_layout->addWidget(new QLabel(QStringFromID(IDS_OTHER_STOPPING_CRITERIA)), row++, 0, 1, grid_width);
            const int time_and_iterations_left = grid_width * 1 / 20;        // indents the rows
            group_layout->addWidget(time_checkbox, row, time_and_iterations_left, 1, grid_one_third - time_and_iterations_left);
            group_layout->addLayout(hms_layout, row++, grid_one_third, 1, grid_width - grid_one_third);
            group_layout->addWidget(iterations_checkbox, row, time_and_iterations_left, 1, grid_two_third - time_and_iterations_left);
            group_layout->addWidget(iterations_spinbox, row++, grid_two_third, 1, grid_width - grid_two_third);
        }

        // Initialize control names
        quality_slider->setObjectName("quality_db");
        time_checkbox->setObjectName("enable_time");
        hours_spinbox->setObjectName("time_split_hours");
        minutes_spinbox->setObjectName("time_split_minutes");
        seconds_spinbox->setObjectName("time_split_seconds");
        iterations_checkbox->setObjectName("enable_iterations");
        iterations_spinbox->setObjectName("iterations");

        // Tooltips
        quality_slider->setToolTip(QStringFromID(IDS_TOOLTIP_QUALITY));
        time_checkbox->setToolTip(QStringFromID(IDS_TOOLTIP_TIME));
        hours_spinbox->setToolTip(QStringFromID(IDS_TOOLTIP_TIME));
        minutes_spinbox->setToolTip(QStringFromID(IDS_TOOLTIP_TIME));
        seconds_spinbox->setToolTip(QStringFromID(IDS_TOOLTIP_TIME));
        iterations_checkbox->setToolTip(QStringFromID(IDS_TOOLTIP_ITERATIONS));
        iterations_spinbox->setToolTip(QStringFromID(IDS_TOOLTIP_ITERATIONS));

        // Setup enable/disable logic
        connect_enabled_toggle(*time_checkbox, *hours_spinbox);
        connect_enabled_toggle(*time_checkbox, *minutes_spinbox);
        connect_enabled_toggle(*time_checkbox, *seconds_spinbox);
        connect_enabled_toggle(*iterations_checkbox, *iterations_spinbox);

        dialog_layout->addWidget(group);
    }

    // Rendering method
    {
        // Group box
        QGroupBox* group = new QGroupBox(QStringFromID(IDS_TITLE_RENDERING_METHOD_GROUP));

        QComboBox* render_method_combo = new QComboBox();
        render_method_combo->addItem(QStringFromID(IDS_RENDERMETHOD_PATH_TRACING), rti::KERNEL_PATH_TRACE);
        render_method_combo->addItem(QStringFromID(IDS_RENDERMETHOD_FAST_PATH_TRACING), rti::KERNEL_PATH_TRACE_FAST);
        //render_method_combo->addItem(QStringFromID(IDS_RENDERMETHOD_LOW_NOISE), rti::KERNEL_LOW_NOISE);
        //render_method_combo->addItem(QStringFromID(IDS_RENDERMETHOD_LOW_NOISE_DIRECT), rti::KERNEL_LOW_NOISE_DIRECT);
        QLabel* render_method_description_label = new QLabel();
        render_method_description_label->setWordWrap(true);
        render_method_description_label->setFrameStyle(QFrame::NoFrame | QFrame::Plain);

        // Build layout
        QFormLayout* group_layout = new QFormLayout(group);
        group_layout->addRow(QStringFromID(IDS_TITLE_RENDERING_METHOD), render_method_combo);
        group_layout->addRow(render_method_description_label);

        // Initialize control names, to be recognized by ParamMap
        render_method_combo->setObjectName("render_method");
        render_method_description_label->setObjectName("render_method_text_label");

        // Tooltips
        render_method_combo->setToolTip(QStringFromID(IDS_TOOLTIP_RENDER_MODE));

        // Setup the signal to change the text box whenever the render mode is changed
        connect(render_method_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(render_method_combo_changed(int)));
        // Force a toggle, to make sure the callback is first called
        render_method_combo->setCurrentIndex(0);
        render_method_combo->setCurrentIndex(1);

        dialog_layout->addWidget(group);
    }

    dialog_layout->addStretch(1000);
}

void RenderParamsQtDialog::SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const param_block)
{
    m_param_block = param_block;
}

void RenderParamsQtDialog::UpdateUI(const TimeValue /*t*/) 
{
    // nothing
}

void RenderParamsQtDialog::UpdateParameterUI(const TimeValue /*t*/, const ParamID /*param_id*/, const int /*tab_index*/) 
{
    // nothing
}

void RenderParamsQtDialog::render_method_combo_changed(int new_index)
{
    QComboBox* render_method_combo = dynamic_cast<QComboBox*>(sender());
    if(DbgVerify(render_method_combo != nullptr))
    {
        // Find the text label by name
        QObject* parent = render_method_combo->parent();
        QLabel* render_method_text_label = (parent != nullptr) ? parent->findChild<QLabel*>("render_method_text_label") : nullptr;
        if(DbgVerify(render_method_text_label != nullptr))
        {
            const int render_method = render_method_combo->itemData(new_index).value<int>();
            QString text;
            switch(render_method)
            {
            case rti::KERNEL_PATH_TRACE:
                text = QStringFromID(IDS_TOOLTIP_PATH_TRACING);
                break;
            case rti::KERNEL_PATH_TRACE_FAST:
                text = QStringFromID(IDS_TOOLTIP_FAST_PATH_TRACING);
                break;
            case rti::KERNEL_LOW_NOISE:
                text = QStringFromID(IDS_TOOLTIP_LOW_NOISE);
                break;
            case rti::KERNEL_LOW_NOISE_DIRECT:
                text = QStringFromID(IDS_TOOLTIP_LOW_NOISE_DIRECT);
                break;
            default:
                // nothing
                break;
            }

            render_method_text_label->setText(text);
        }
    }
}

//==================================================================================================
// class ImageQualityQtDialog
//==================================================================================================

ImageQualityQtDialog::ImageQualityQtDialog()
{
    QVBoxLayout* dialog_layout = new QVBoxLayout(this);

    // Noise filter
    {
        // Strength slider
        QGroupBox* strength_group = new QGroupBox(QStringFromID(IDS_NOISE_FILTERING));
        {
            // Checkbox
            QCheckBox* noise_filter_checkbox = new QCheckBox(QStringFromID(IDS_ENABLE));
            noise_filter_checkbox->setObjectName("enable_noise_filter");

            // Label
            QLabel* noise_filter_label = new QLabel(QStringFromID(IDS_FILTER_STRENGTH_LABEL));

            // Slider
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
                strength_slider->setObjectName("noise_filter_strength_percentage");
            }

            // Build group box layout
            QVBoxLayout* group_layout = new QVBoxLayout(strength_group);
            group_layout->addWidget(noise_filter_checkbox);
            group_layout->addWidget(noise_filter_label);
            group_layout->addWidget(strength_slider);

            // Tooltips
            strength_slider->setToolTip(QStringFromID(IDS_NOISEFITLER_ELEMENT_STRENGTH_TOOLTIP));
            noise_filter_label->setToolTip(QStringFromID(IDS_NOISEFITLER_ELEMENT_STRENGTH_TOOLTIP));
            noise_filter_checkbox->setToolTip(QStringFromID(IDS_NOISEFITLER_ELEMENT_STRENGTH_TOOLTIP));
            strength_group->setToolTip(QStringFromID(IDS_NOISEFITLER_ELEMENT_STRENGTH_TOOLTIP));

            // Setup behaviour
            connect_enabled_toggle(*noise_filter_checkbox, *noise_filter_label);
            // Need to connect the slider twice for the initial enabled state to be correct. I don't understand why.
            connect_enabled_toggle(*noise_filter_checkbox, *strength_slider);
            connect_enabled_toggle(*noise_filter_checkbox, *strength_slider);
        }
        dialog_layout->addWidget(strength_group);
    }

    // Anti-aliasing
    {
        // Group box
        QGroupBox* group = new QGroupBox(QStringFromID(IDS_TITLE_ANTI_ALIASING_GROUP));

        // Anti-aliasing filter
        QLabel* filter_label = new QLabel(QStringFromID(IDS_TITLE_FILTER_LABEL));
        QmaxDoubleSpinBox* filter_spinbox = new QmaxDoubleSpinBox();
        filter_spinbox->setSuffix(QStringFromID(IDS_PIXELS_UNIT_SUFFIX));
        filter_spinbox->setDecimals(1);
        filter_spinbox->setSingleStep(0.1);

        // Build the layout
        QGridLayout* group_layout = new QGridLayout(group);
        group_layout->addWidget(filter_label, 0, 0, 1, 2);
        group_layout->addWidget(filter_spinbox, 0, 2);

        // Initialize control names
        filter_spinbox->setObjectName("anti_aliasing_filter_diameter");

        // Tooltips
        group->setToolTip(QStringFromID(IDS_TOOLTIP_FILTER_DIAMETER));
        filter_label->setToolTip(QStringFromID(IDS_TOOLTIP_FILTER_DIAMETER));
        filter_spinbox->setToolTip(QStringFromID(IDS_TOOLTIP_FILTER_DIAMETER));

        dialog_layout->addWidget(group);
    }

    dialog_layout->addStretch(1000);
}

void ImageQualityQtDialog::SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const /*param_block*/)
{
    // nothing
}

void ImageQualityQtDialog::UpdateUI(const TimeValue /*t*/)
{
    // nothing
}

void ImageQualityQtDialog::UpdateParameterUI(const TimeValue /*t*/, const ParamID /*param_id*/, const int /*tab_index*/)
{
    // nothing
}

//==================================================================================================
// class AdvancedParamsQtDialog
//==================================================================================================

AdvancedParamsQtDialog::AdvancedParamsQtDialog()
{
    QVBoxLayout* dialog_layout = new QVBoxLayout(this);

    // Scene options
    {
        // Group box
        QGroupBox* group = new QGroupBox(QStringFromID(IDS_TITLE_SCENE_OPTIONS_GROUP));

        // Point light diameter label and spinner
        QLabel* light_label = new QLabel(QStringFromID(IDS_TITLE_POINT_LIGHT_DIAM_LABEL));
		MaxSDK::QmaxWorldSpinBox* light_spinbox = new MaxSDK::QmaxWorldSpinBox();
        light_label->setToolTip(QStringFromID(IDS_TOOLTIP_POINT_LIGHT_DIAMETER));
        light_spinbox->setToolTip(QStringFromID(IDS_TOOLTIP_POINT_LIGHT_DIAMETER));
        light_spinbox->setObjectName("point_light_diameter");

        // Motion blur
        QCheckBox* motion_checkbox = new QCheckBox(QStringFromID(IDS_TITLE_MOTION_BLUR_ALL_OBJECTS_CHECKBOX));
        motion_checkbox->setToolTip(QStringFromID(IDS_TOOLTIP_MOTION_BLUR_ALL_OBJECTS));
        motion_checkbox->setObjectName("motion_blur_all_objects");

        // Build the layout
        QGridLayout* group_layout = new QGridLayout(group);
        group_layout->addWidget(light_label, 0, 0, 1, 2);
        group_layout->addWidget(light_spinbox, 0, 2);
        group_layout->addWidget(motion_checkbox, 1, 0, 1, 3);

        dialog_layout->addWidget(group);
    }

    // Noise pattern
    {
        // Group box
        QGroupBox* group = new QGroupBox(QStringFromID(IDS_NOISE_PATTERN));

        // Animate noise checkbox
        QCheckBox* animated_noise_checkbox = new QCheckBox(QStringFromID(IDS_ANIMATE_NOISE_PATTERN));
        animated_noise_checkbox->setToolTip(QStringFromID(IDS_ANIMATE_NOISE_PATTERN_TOOLTIP));
        animated_noise_checkbox->setObjectName("enable_animated_noise");

        // Build the layout
        QGridLayout* group_layout = new QGridLayout(group);
        group_layout->addWidget(animated_noise_checkbox);

        dialog_layout->addWidget(group);
    }

    dialog_layout->addStretch(1000);
}

void AdvancedParamsQtDialog::SetParamBlock(ReferenceMaker* /*owner*/, IParamBlock2* const /*param_block*/)
{
    // nothing
}

void AdvancedParamsQtDialog::UpdateUI(const TimeValue /*t*/)
{
    // nothing
}

void AdvancedParamsQtDialog::UpdateParameterUI(const TimeValue /*t*/, const ParamID /*param_id*/, const int /*tab_index*/)
{
    // nothing
}

}}  // namespace