/********************************************************************************
** Form generated from reading UI file 'PhysSunSky.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PHYSSUNSKY_H
#define UI_PHYSSUNSKY_H

#include <Qt/QmaxSpinBox.h>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <qt/qmaxcolorswatch.h>

QT_BEGIN_NAMESPACE

class Ui_PhysSunSky
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label_15;
    QPushButton *sun_position_object;
    QPushButton *create_sun_positioner_button;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label;
    QLabel *label_4;
    MaxSDK::QmaxDoubleSpinBox *global_intensity;
    MaxSDK::QmaxDoubleSpinBox *haze;
    QGroupBox *sun_enabled;
    QGridLayout *gridLayout_2;
    MaxSDK::QmaxDoubleSpinBox *sun_disc_scale_percent;
    QLabel *label_2;
    QLabel *label_6;
    MaxSDK::QmaxDoubleSpinBox *sun_disc_intensity;
    QLabel *label_7;
    MaxSDK::QmaxDoubleSpinBox *sun_glow_intensity;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_3;
    QLabel *label_5;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QComboBox *illuminance_model;
    QLabel *label_illum_using_weather_file;
    QLabel *label_illum_not_using_weather_file;
    QGridLayout *gridLayout_6;
    MaxSDK::QmaxLightingSpinBox *perez_diffuse_horizontal_illuminance;
    QLabel *label_diffus_hor_ill;
    MaxSDK::QmaxLightingSpinBox *perez_direct_normal_illuminance;
    QLabel *label_dir_norm_ill;
    MaxSDK::QmaxDoubleSpinBox *sky_intensity;
    QGroupBox *groupBox_6;
    QGridLayout *gridLayout_7;
    QGridLayout *gridLayout_9;
    QGridLayout *gridLayout_8;
    MaxSDK::QmaxLightingSpinBox *night_intensity;
    MaxSDK::QMaxColorSwatch *night_color;
    QLabel *label_9;
    QGroupBox *groupBox_4;
    QGridLayout *gridLayout_4;
    QGridLayout *gridLayout_17;
    QGridLayout *gridLayout_15;
    MaxSDK::QmaxDoubleSpinBox *horizon_blur_deg;
    QLabel *label_13;
    QWidget *widget;
    QGridLayout *gridLayout_16;
    MaxSDK::QmaxDoubleSpinBox *horizon_height_deg;
    QLabel *label_10;
    QLabel *label_11;
    MaxSDK::QMaxColorSwatch *ground_color;
    QGroupBox *groupBox_5;
    QGridLayout *gridLayout_5;
    QGridLayout *gridLayout_14;
    QGridLayout *gridLayout_12;
    MaxSDK::QmaxDoubleSpinBox *saturation;
    QLabel *label_14;
    QGridLayout *gridLayout_13;
    QLabel *label_12;
    MaxSDK::QMaxColorSwatch *tint;

    void setupUi(QWidget *PhysSunSky)
    {
        if (PhysSunSky->objectName().isEmpty())
            PhysSunSky->setObjectName(QStringLiteral("PhysSunSky"));
        PhysSunSky->resize(621, 1023);
        verticalLayout = new QVBoxLayout(PhysSunSky);
        verticalLayout->setSpacing(2);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(2);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        label_15 = new QLabel(PhysSunSky);
        label_15->setObjectName(QStringLiteral("label_15"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_15->sizePolicy().hasHeightForWidth());
        label_15->setSizePolicy(sizePolicy);
        label_15->setWordWrap(true);

        horizontalLayout->addWidget(label_15);

        sun_position_object = new QPushButton(PhysSunSky);
        sun_position_object->setObjectName(QStringLiteral("sun_position_object"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sun_position_object->sizePolicy().hasHeightForWidth());
        sun_position_object->setSizePolicy(sizePolicy1);

        horizontalLayout->addWidget(sun_position_object);

        create_sun_positioner_button = new QPushButton(PhysSunSky);
        create_sun_positioner_button->setObjectName(QStringLiteral("create_sun_positioner_button"));
        QSizePolicy sizePolicy2(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(create_sun_positioner_button->sizePolicy().hasHeightForWidth());
        create_sun_positioner_button->setSizePolicy(sizePolicy2);

        horizontalLayout->addWidget(create_sun_positioner_button);


        verticalLayout->addLayout(horizontalLayout);

        groupBox = new QGroupBox(PhysSunSky);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setSpacing(2);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));
        label->setMargin(0);

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setMargin(0);

        gridLayout->addWidget(label_4, 0, 2, 1, 1);

        global_intensity = new MaxSDK::QmaxDoubleSpinBox(groupBox);
        global_intensity->setObjectName(QStringLiteral("global_intensity"));
        global_intensity->setSingleStep(0.1);
        global_intensity->setValue(1);

        gridLayout->addWidget(global_intensity, 0, 1, 1, 1);

        haze = new MaxSDK::QmaxDoubleSpinBox(groupBox);
        haze->setObjectName(QStringLiteral("haze"));
        haze->setSingleStep(0.1);

        gridLayout->addWidget(haze, 0, 3, 1, 1);


        verticalLayout->addWidget(groupBox);

        sun_enabled = new QGroupBox(PhysSunSky);
        sun_enabled->setObjectName(QStringLiteral("sun_enabled"));
        sun_enabled->setCheckable(true);
        gridLayout_2 = new QGridLayout(sun_enabled);
        gridLayout_2->setSpacing(2);
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        sun_disc_scale_percent = new MaxSDK::QmaxDoubleSpinBox(sun_enabled);
        sun_disc_scale_percent->setObjectName(QStringLiteral("sun_disc_scale_percent"));
        sun_disc_scale_percent->setDecimals(0);
        sun_disc_scale_percent->setMinimum(1);
        sun_disc_scale_percent->setMaximum(10000);
        sun_disc_scale_percent->setSingleStep(25);
        sun_disc_scale_percent->setValue(100);

        gridLayout_2->addWidget(sun_disc_scale_percent, 0, 3, 1, 1);

        label_2 = new QLabel(sun_enabled);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setWordWrap(true);
        label_2->setMargin(0);

        gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

        label_6 = new QLabel(sun_enabled);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setMargin(0);

        gridLayout_2->addWidget(label_6, 0, 2, 1, 1);

        sun_disc_intensity = new MaxSDK::QmaxDoubleSpinBox(sun_enabled);
        sun_disc_intensity->setObjectName(QStringLiteral("sun_disc_intensity"));
        sun_disc_intensity->setSingleStep(0.1);
        sun_disc_intensity->setValue(1);

        gridLayout_2->addWidget(sun_disc_intensity, 0, 1, 1, 1);

        label_7 = new QLabel(sun_enabled);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setWordWrap(true);
        label_7->setMargin(0);

        gridLayout_2->addWidget(label_7, 1, 0, 1, 1);

        sun_glow_intensity = new MaxSDK::QmaxDoubleSpinBox(sun_enabled);
        sun_glow_intensity->setObjectName(QStringLiteral("sun_glow_intensity"));
        sun_glow_intensity->setSingleStep(0.1);
        sun_glow_intensity->setValue(1);

        gridLayout_2->addWidget(sun_glow_intensity, 1, 1, 1, 1);


        verticalLayout->addWidget(sun_enabled);

        groupBox_3 = new QGroupBox(PhysSunSky);
        groupBox_3->setObjectName(QStringLiteral("groupBox_3"));
        groupBox_3->setCheckable(false);
        gridLayout_3 = new QGridLayout(groupBox_3);
        gridLayout_3->setSpacing(2);
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        label_5 = new QLabel(groupBox_3);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setWordWrap(true);
        label_5->setMargin(0);

        gridLayout_3->addWidget(label_5, 0, 0, 1, 1);

        groupBox_2 = new QGroupBox(groupBox_3);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setSpacing(2);
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        illuminance_model = new QComboBox(groupBox_2);
        illuminance_model->addItem(QString());
        illuminance_model->addItem(QString());
        illuminance_model->addItem(QString());
        illuminance_model->setObjectName(QStringLiteral("illuminance_model"));

        verticalLayout_2->addWidget(illuminance_model);

        label_illum_using_weather_file = new QLabel(groupBox_2);
        label_illum_using_weather_file->setObjectName(QStringLiteral("label_illum_using_weather_file"));
        label_illum_using_weather_file->setWordWrap(true);

        verticalLayout_2->addWidget(label_illum_using_weather_file);

        label_illum_not_using_weather_file = new QLabel(groupBox_2);
        label_illum_not_using_weather_file->setObjectName(QStringLiteral("label_illum_not_using_weather_file"));
        label_illum_not_using_weather_file->setWordWrap(true);

        verticalLayout_2->addWidget(label_illum_not_using_weather_file);

        gridLayout_6 = new QGridLayout();
        gridLayout_6->setSpacing(2);
        gridLayout_6->setObjectName(QStringLiteral("gridLayout_6"));
        perez_diffuse_horizontal_illuminance = new MaxSDK::QmaxLightingSpinBox(groupBox_2);
        perez_diffuse_horizontal_illuminance->setObjectName(QStringLiteral("perez_diffuse_horizontal_illuminance"));
        perez_diffuse_horizontal_illuminance->setSingleStep(1000);

        gridLayout_6->addWidget(perez_diffuse_horizontal_illuminance, 0, 1, 1, 1);

        label_diffus_hor_ill = new QLabel(groupBox_2);
        label_diffus_hor_ill->setObjectName(QStringLiteral("label_diffus_hor_ill"));
        label_diffus_hor_ill->setWordWrap(true);
        label_diffus_hor_ill->setMargin(0);

        gridLayout_6->addWidget(label_diffus_hor_ill, 0, 0, 1, 1);

        perez_direct_normal_illuminance = new MaxSDK::QmaxLightingSpinBox(groupBox_2);
        perez_direct_normal_illuminance->setObjectName(QStringLiteral("perez_direct_normal_illuminance"));
        perez_direct_normal_illuminance->setSingleStep(1000);

        gridLayout_6->addWidget(perez_direct_normal_illuminance, 1, 1, 1, 1);

        label_dir_norm_ill = new QLabel(groupBox_2);
        label_dir_norm_ill->setObjectName(QStringLiteral("label_dir_norm_ill"));
        label_dir_norm_ill->setWordWrap(true);
        label_dir_norm_ill->setMargin(0);

        gridLayout_6->addWidget(label_dir_norm_ill, 1, 0, 1, 1);


        verticalLayout_2->addLayout(gridLayout_6);


        gridLayout_3->addWidget(groupBox_2, 1, 0, 1, 4);

        sky_intensity = new MaxSDK::QmaxDoubleSpinBox(groupBox_3);
        sky_intensity->setObjectName(QStringLiteral("sky_intensity"));
        sky_intensity->setSingleStep(0.1);
        sky_intensity->setValue(1);

        gridLayout_3->addWidget(sky_intensity, 0, 1, 1, 1);

        groupBox_6 = new QGroupBox(groupBox_3);
        groupBox_6->setObjectName(QStringLiteral("groupBox_6"));
        gridLayout_7 = new QGridLayout(groupBox_6);
        gridLayout_7->setSpacing(2);
        gridLayout_7->setContentsMargins(0, 0, 0, 0);
        gridLayout_7->setObjectName(QStringLiteral("gridLayout_7"));
        gridLayout_9 = new QGridLayout();
        gridLayout_9->setSpacing(2);
        gridLayout_9->setObjectName(QStringLiteral("gridLayout_9"));
        gridLayout_8 = new QGridLayout();
        gridLayout_8->setSpacing(2);
        gridLayout_8->setObjectName(QStringLiteral("gridLayout_8"));
        night_intensity = new MaxSDK::QmaxLightingSpinBox(groupBox_6);
        night_intensity->setObjectName(QStringLiteral("night_intensity"));
        night_intensity->setMaximum(1e+09);
        night_intensity->setValue(1);

        gridLayout_8->addWidget(night_intensity, 0, 0, 1, 1);

        night_color = new MaxSDK::QMaxColorSwatch(groupBox_6);
        night_color->setObjectName(QStringLiteral("night_color"));

        gridLayout_8->addWidget(night_color, 0, 1, 1, 1);


        gridLayout_9->addLayout(gridLayout_8, 0, 1, 1, 1);

        label_9 = new QLabel(groupBox_6);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setWordWrap(true);
        label_9->setMargin(0);

        gridLayout_9->addWidget(label_9, 0, 0, 1, 1);


        gridLayout_7->addLayout(gridLayout_9, 0, 0, 1, 1);


        gridLayout_3->addWidget(groupBox_6, 2, 0, 1, 4);


        verticalLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(PhysSunSky);
        groupBox_4->setObjectName(QStringLiteral("groupBox_4"));
        gridLayout_4 = new QGridLayout(groupBox_4);
        gridLayout_4->setSpacing(2);
        gridLayout_4->setContentsMargins(0, 0, 0, 0);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        gridLayout_17 = new QGridLayout();
        gridLayout_17->setSpacing(2);
        gridLayout_17->setObjectName(QStringLiteral("gridLayout_17"));
        gridLayout_15 = new QGridLayout();
        gridLayout_15->setSpacing(2);
        gridLayout_15->setObjectName(QStringLiteral("gridLayout_15"));
        horizon_blur_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox_4);
        horizon_blur_deg->setObjectName(QStringLiteral("horizon_blur_deg"));
        horizon_blur_deg->setDecimals(1);
        horizon_blur_deg->setSingleStep(0.1);

        gridLayout_15->addWidget(horizon_blur_deg, 0, 1, 1, 1);

        label_13 = new QLabel(groupBox_4);
        label_13->setObjectName(QStringLiteral("label_13"));
        label_13->setWordWrap(true);
        label_13->setMargin(0);

        gridLayout_15->addWidget(label_13, 0, 0, 1, 1);

        widget = new QWidget(groupBox_4);
        widget->setObjectName(QStringLiteral("widget"));

        gridLayout_15->addWidget(widget, 1, 0, 1, 1);


        gridLayout_17->addLayout(gridLayout_15, 0, 0, 1, 1);

        gridLayout_16 = new QGridLayout();
        gridLayout_16->setSpacing(2);
        gridLayout_16->setObjectName(QStringLiteral("gridLayout_16"));
        horizon_height_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox_4);
        horizon_height_deg->setObjectName(QStringLiteral("horizon_height_deg"));
        horizon_height_deg->setDecimals(1);
        horizon_height_deg->setSingleStep(0.1);

        gridLayout_16->addWidget(horizon_height_deg, 0, 1, 1, 1);

        label_10 = new QLabel(groupBox_4);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setWordWrap(true);
        label_10->setMargin(0);

        gridLayout_16->addWidget(label_10, 0, 0, 1, 1);

        label_11 = new QLabel(groupBox_4);
        label_11->setObjectName(QStringLiteral("label_11"));
        label_11->setMargin(0);

        gridLayout_16->addWidget(label_11, 1, 0, 1, 1);

        ground_color = new MaxSDK::QMaxColorSwatch(groupBox_4);
        ground_color->setObjectName(QStringLiteral("ground_color"));

        gridLayout_16->addWidget(ground_color, 1, 1, 1, 1);


        gridLayout_17->addLayout(gridLayout_16, 0, 1, 1, 1);


        gridLayout_4->addLayout(gridLayout_17, 1, 1, 1, 1);


        verticalLayout->addWidget(groupBox_4);

        groupBox_5 = new QGroupBox(PhysSunSky);
        groupBox_5->setObjectName(QStringLiteral("groupBox_5"));
        gridLayout_5 = new QGridLayout(groupBox_5);
        gridLayout_5->setSpacing(2);
        gridLayout_5->setContentsMargins(0, 0, 0, 0);
        gridLayout_5->setObjectName(QStringLiteral("gridLayout_5"));
        gridLayout_14 = new QGridLayout();
        gridLayout_14->setSpacing(2);
        gridLayout_14->setObjectName(QStringLiteral("gridLayout_14"));
        gridLayout_12 = new QGridLayout();
        gridLayout_12->setSpacing(2);
        gridLayout_12->setObjectName(QStringLiteral("gridLayout_12"));
        saturation = new MaxSDK::QmaxDoubleSpinBox(groupBox_5);
        saturation->setObjectName(QStringLiteral("saturation"));
        saturation->setDecimals(1);
        saturation->setSingleStep(0.1);
        saturation->setValue(1);

        gridLayout_12->addWidget(saturation, 0, 1, 1, 1);

        label_14 = new QLabel(groupBox_5);
        label_14->setObjectName(QStringLiteral("label_14"));
        label_14->setMargin(0);

        gridLayout_12->addWidget(label_14, 0, 0, 1, 1);


        gridLayout_14->addLayout(gridLayout_12, 0, 0, 1, 1);

        gridLayout_13 = new QGridLayout();
        gridLayout_13->setSpacing(2);
        gridLayout_13->setObjectName(QStringLiteral("gridLayout_13"));
        label_12 = new QLabel(groupBox_5);
        label_12->setObjectName(QStringLiteral("label_12"));
        label_12->setMargin(0);

        gridLayout_13->addWidget(label_12, 0, 0, 1, 1);

        tint = new MaxSDK::QMaxColorSwatch(groupBox_5);
        tint->setObjectName(QStringLiteral("tint"));

        gridLayout_13->addWidget(tint, 0, 1, 1, 1);


        gridLayout_14->addLayout(gridLayout_13, 0, 1, 1, 1);


        gridLayout_5->addLayout(gridLayout_14, 0, 0, 1, 1);


        verticalLayout->addWidget(groupBox_5);


        retranslateUi(PhysSunSky);

        QMetaObject::connectSlotsByName(PhysSunSky);
    } // setupUi

    void retranslateUi(QWidget *PhysSunSky)
    {
        PhysSunSky->setWindowTitle(QApplication::translate("PhysSunSky", "Form", nullptr));
        label_15->setText(QApplication::translate("PhysSunSky", "Sun Position Widget:", nullptr));
#ifndef QT_NO_TOOLTIP
        sun_position_object->setToolTip(QApplication::translate("PhysSunSky", "Select the Sun Positioner object from which the direction of the sun is to be controlled.", nullptr));
#endif // QT_NO_TOOLTIP
        sun_position_object->setText(QApplication::translate("PhysSunSky", "Pick", nullptr));
#ifndef QT_NO_TOOLTIP
        create_sun_positioner_button->setToolTip(QApplication::translate("PhysSunSky", "Create a new Sun Positioner object with which to control the position of the sun.", nullptr));
#endif // QT_NO_TOOLTIP
        create_sun_positioner_button->setText(QApplication::translate("PhysSunSky", "Create", nullptr));
        groupBox->setTitle(QApplication::translate("PhysSunSky", "Global", nullptr));
        label->setText(QApplication::translate("PhysSunSky", "Intensity:", nullptr));
        label_4->setText(QApplication::translate("PhysSunSky", "Haze:", nullptr));
#ifndef QT_NO_TOOLTIP
        global_intensity->setToolTip(QApplication::translate("PhysSunSky", "Multiplier that affects both sun and sky intensities.", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        haze->setToolTip(QApplication::translate("PhysSunSky", "The amount of haze (airborne particles and pollution) present in the atmosphere.", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        sun_enabled->setToolTip(QApplication::translate("PhysSunSky", "Enables or disables the presence of the sun in the sky.", nullptr));
#endif // QT_NO_TOOLTIP
        sun_enabled->setTitle(QApplication::translate("PhysSunSky", "Sun", nullptr));
#ifndef QT_NO_TOOLTIP
        sun_disc_scale_percent->setToolTip(QApplication::translate("PhysSunSky", "Size of the sun disc, with 100% being the real-world size.", nullptr));
#endif // QT_NO_TOOLTIP
        sun_disc_scale_percent->setSuffix(QApplication::translate("PhysSunSky", "%", nullptr));
        label_2->setText(QApplication::translate("PhysSunSky", "Disc Intensity:", nullptr));
        label_6->setText(QApplication::translate("PhysSunSky", "Disc Size:", nullptr));
#ifndef QT_NO_TOOLTIP
        sun_disc_intensity->setToolTip(QApplication::translate("PhysSunSky", "Multiplier that affects the sun intensity, with 1.0 being the real-world value.", nullptr));
#endif // QT_NO_TOOLTIP
        label_7->setText(QApplication::translate("PhysSunSky", "Glow Intensity:", nullptr));
#ifndef QT_NO_TOOLTIP
        sun_glow_intensity->setToolTip(QApplication::translate("PhysSunSky", "<html><head/><body><p>Intensity of the glow around the sun disc. This effect isn't strictly physical. Changing this value doesn't make the sun brighter or darker overall; it merely displaces the intensity between the disc and glow areas.</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        groupBox_3->setTitle(QApplication::translate("PhysSunSky", "Sky", nullptr));
        label_5->setText(QApplication::translate("PhysSunSky", "Sky Intensity:", nullptr));
        groupBox_2->setTitle(QApplication::translate("PhysSunSky", "Illuminance Model", nullptr));
        illuminance_model->setItemText(0, QApplication::translate("PhysSunSky", "Automatic", nullptr));
        illuminance_model->setItemText(1, QApplication::translate("PhysSunSky", "Physical (Preetham et al.)", nullptr));
        illuminance_model->setItemText(2, QApplication::translate("PhysSunSky", "Measured (Perez All-Weather)", nullptr));

#ifndef QT_NO_TOOLTIP
        illuminance_model->setToolTip(QApplication::translate("PhysSunSky", "Select the mathematical model from which the sky illuminance is derived.", nullptr));
#endif // QT_NO_TOOLTIP
        label_illum_using_weather_file->setText(QApplication::translate("PhysSunSky", "Using weather data file from sun position widget with Perez All-Weather Model.", nullptr));
        label_illum_not_using_weather_file->setText(QApplication::translate("PhysSunSky", "No weather data file present on sun position widget. Using Physical model.", nullptr));
#ifndef QT_NO_TOOLTIP
        perez_diffuse_horizontal_illuminance->setToolTip(QApplication::translate("PhysSunSky", "Illuminance measured on the horizontal plane, excluding the sun and circumsolar area.", nullptr));
#endif // QT_NO_TOOLTIP
        label_diffus_hor_ill->setText(QApplication::translate("PhysSunSky", "Diffuse Horizontal Illuminance:", nullptr));
#ifndef QT_NO_TOOLTIP
        perez_direct_normal_illuminance->setToolTip(QApplication::translate("PhysSunSky", "Illuminance measured while facing the sun.", nullptr));
#endif // QT_NO_TOOLTIP
        label_dir_norm_ill->setText(QApplication::translate("PhysSunSky", "Direct Normal Illuminance:", nullptr));
#ifndef QT_NO_TOOLTIP
        sky_intensity->setToolTip(QApplication::translate("PhysSunSky", "Multiplier that affects the sky intensity, with 1.0 being the real-world value.", nullptr));
#endif // QT_NO_TOOLTIP
        groupBox_6->setTitle(QApplication::translate("PhysSunSky", "Night Sky", nullptr));
#ifndef QT_NO_TOOLTIP
        night_intensity->setToolTip(QApplication::translate("PhysSunSky", "The luminance of the night sky. This is used once the sun dips below the horizon.", nullptr));
#endif // QT_NO_TOOLTIP
        label_9->setText(QApplication::translate("PhysSunSky", "Luminance:", nullptr));
        groupBox_4->setTitle(QApplication::translate("PhysSunSky", "Horizon && Ground", nullptr));
#ifndef QT_NO_TOOLTIP
        horizon_blur_deg->setToolTip(QApplication::translate("PhysSunSky", "The extent of the area in which the ground and sky are blurred together to form a fuzzy horizon.", nullptr));
#endif // QT_NO_TOOLTIP
        horizon_blur_deg->setSuffix(QApplication::translate("PhysSunSky", "\313\232", nullptr));
        label_13->setText(QApplication::translate("PhysSunSky", "Horizon Blur:", nullptr));
#ifndef QT_NO_TOOLTIP
        horizon_height_deg->setToolTip(QApplication::translate("PhysSunSky", "The height of the horizon line, to be used for artificially raising or lowering the horizon line (along with the entire sky and sun).", nullptr));
#endif // QT_NO_TOOLTIP
        horizon_height_deg->setSuffix(QApplication::translate("PhysSunSky", "\313\232", nullptr));
        label_10->setText(QApplication::translate("PhysSunSky", "Horizon Height:", nullptr));
        label_11->setText(QApplication::translate("PhysSunSky", "Ground Color:", nullptr));
#ifndef QT_NO_TOOLTIP
        ground_color->setToolTip(QApplication::translate("PhysSunSky", "Diffuse reflectance of the simulated ground plane, rendered below the horizon and illuminated by the sun and sky.", nullptr));
#endif // QT_NO_TOOLTIP
        groupBox_5->setTitle(QApplication::translate("PhysSunSky", "Color Tuning", nullptr));
#ifndef QT_NO_TOOLTIP
        saturation->setToolTip(QApplication::translate("PhysSunSky", "The saturation multiplier for the resulting environnment color. A value of 0.0 may be useful to simulate an overcast sky (along with a high haze value).", nullptr));
#endif // QT_NO_TOOLTIP
        saturation->setSuffix(QString());
        label_14->setText(QApplication::translate("PhysSunSky", "Saturation:", nullptr));
        label_12->setText(QApplication::translate("PhysSunSky", "Tint:", nullptr));
#ifndef QT_NO_TOOLTIP
        tint->setToolTip(QApplication::translate("PhysSunSky", "A color multiplier for the entire sun & sky environment.", nullptr));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class PhysSunSky: public Ui_PhysSunSky {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PHYSSUNSKY_H
