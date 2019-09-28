/********************************************************************************
** Form generated from reading UI file 'SunPositioner_SunPosition.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SUNPOSITIONER_SUNPOSITION_H
#define UI_SUNPOSITIONER_SUNPOSITION_H

#include <Qt/QmaxSpinBox.h>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SunPositioner_SunPosition
{
public:
    QVBoxLayout *verticalLayout;
    QPushButton *pushButton_installEnvironment;
    QGroupBox *groupBox_5;
    QVBoxLayout *verticalLayout_7;
    QRadioButton *datetime;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QRadioButton *weatherfile;
    QToolButton *pushButton_weatherFile;
    QLabel *label_weatherFileStatus;
    QRadioButton *manual;
    QGroupBox *groupBox_dateTime;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_8;
    QLabel *label_15;
    MaxSDK::QmaxSpinBox *hours;
    MaxSDK::QmaxSpinBox *minutes;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_3;
    QLabel *label_17;
    QLabel *label_18;
    MaxSDK::QmaxSpinBox *month;
    QLabel *label_16;
    MaxSDK::QmaxSpinBox *day;
    MaxSDK::QmaxSpinBox *year;
    QGroupBox *dst;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *dst_use_date_range;
    QGridLayout *gridLayout;
    QLabel *label_26;
    QLabel *label_27;
    MaxSDK::QmaxSpinBox *dst_start_day;
    MaxSDK::QmaxSpinBox *dst_start_month;
    QLabel *label_28;
    MaxSDK::QmaxSpinBox *dst_end_day;
    MaxSDK::QmaxSpinBox *dst_end_month;
    QLabel *label_25;
    QGroupBox *groupBox_Location;
    QVBoxLayout *verticalLayout_3;
    QPushButton *location;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    MaxSDK::QmaxDoubleSpinBox *latitude_deg;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_9;
    MaxSDK::QmaxDoubleSpinBox *longitude_deg;
    QHBoxLayout *horizontalLayout_9;
    QLabel *label_19;
    MaxSDK::QmaxDoubleSpinBox *time_zone;
    QGroupBox *groupBox_horizontalCoords;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_5;
    MaxSDK::QmaxDoubleSpinBox *azimuth_deg;
    QHBoxLayout *horizontalLayout_10;
    QLabel *label_20;
    MaxSDK::QmaxDoubleSpinBox *altitude_deg;
    QButtonGroup *mode;

    void setupUi(QWidget *SunPositioner_SunPosition)
    {
        if (SunPositioner_SunPosition->objectName().isEmpty())
            SunPositioner_SunPosition->setObjectName(QStringLiteral("SunPositioner_SunPosition"));
        SunPositioner_SunPosition->resize(235, 1105);
        verticalLayout = new QVBoxLayout(SunPositioner_SunPosition);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        pushButton_installEnvironment = new QPushButton(SunPositioner_SunPosition);
        pushButton_installEnvironment->setObjectName(QStringLiteral("pushButton_installEnvironment"));

        verticalLayout->addWidget(pushButton_installEnvironment);

        groupBox_5 = new QGroupBox(SunPositioner_SunPosition);
        groupBox_5->setObjectName(QStringLiteral("groupBox_5"));
        verticalLayout_7 = new QVBoxLayout(groupBox_5);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        datetime = new QRadioButton(groupBox_5);
        mode = new QButtonGroup(SunPositioner_SunPosition);
        mode->setObjectName(QStringLiteral("mode"));
        mode->addButton(datetime);
        datetime->setObjectName(QStringLiteral("datetime"));

        verticalLayout_7->addWidget(datetime);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        weatherfile = new QRadioButton(groupBox_5);
        mode->addButton(weatherfile);
        weatherfile->setObjectName(QStringLiteral("weatherfile"));

        horizontalLayout->addWidget(weatherfile);

        pushButton_weatherFile = new QToolButton(groupBox_5);
        pushButton_weatherFile->setObjectName(QStringLiteral("pushButton_weatherFile"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pushButton_weatherFile->sizePolicy().hasHeightForWidth());
        pushButton_weatherFile->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(pushButton_weatherFile);


        verticalLayout_2->addLayout(horizontalLayout);

        label_weatherFileStatus = new QLabel(groupBox_5);
        label_weatherFileStatus->setObjectName(QStringLiteral("label_weatherFileStatus"));
        label_weatherFileStatus->setWordWrap(true);
        label_weatherFileStatus->setIndent(25);

        verticalLayout_2->addWidget(label_weatherFileStatus);


        verticalLayout_7->addLayout(verticalLayout_2);

        manual = new QRadioButton(groupBox_5);
        mode->addButton(manual);
        manual->setObjectName(QStringLiteral("manual"));

        verticalLayout_7->addWidget(manual);


        verticalLayout->addWidget(groupBox_5);

        groupBox_dateTime = new QGroupBox(SunPositioner_SunPosition);
        groupBox_dateTime->setObjectName(QStringLiteral("groupBox_dateTime"));
        verticalLayout_5 = new QVBoxLayout(groupBox_dateTime);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        label_15 = new QLabel(groupBox_dateTime);
        label_15->setObjectName(QStringLiteral("label_15"));

        horizontalLayout_8->addWidget(label_15);

        hours = new MaxSDK::QmaxSpinBox(groupBox_dateTime);
        hours->setObjectName(QStringLiteral("hours"));
        hours->setWrapping(true);

        horizontalLayout_8->addWidget(hours);

        minutes = new MaxSDK::QmaxSpinBox(groupBox_dateTime);
        minutes->setObjectName(QStringLiteral("minutes"));
        minutes->setWrapping(true);

        horizontalLayout_8->addWidget(minutes);


        verticalLayout_5->addLayout(horizontalLayout_8);

        groupBox = new QGroupBox(groupBox_dateTime);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout_3 = new QGridLayout(groupBox);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        label_17 = new QLabel(groupBox);
        label_17->setObjectName(QStringLiteral("label_17"));
        label_17->setAlignment(Qt::AlignCenter);

        gridLayout_3->addWidget(label_17, 0, 1, 1, 1);

        label_18 = new QLabel(groupBox);
        label_18->setObjectName(QStringLiteral("label_18"));
        label_18->setAlignment(Qt::AlignCenter);

        gridLayout_3->addWidget(label_18, 0, 2, 1, 1);

        month = new MaxSDK::QmaxSpinBox(groupBox);
        month->setObjectName(QStringLiteral("month"));
        month->setWrapping(true);

        gridLayout_3->addWidget(month, 1, 1, 1, 1);

        label_16 = new QLabel(groupBox);
        label_16->setObjectName(QStringLiteral("label_16"));
        label_16->setAlignment(Qt::AlignCenter);

        gridLayout_3->addWidget(label_16, 0, 0, 1, 1);

        day = new MaxSDK::QmaxSpinBox(groupBox);
        day->setObjectName(QStringLiteral("day"));
        day->setWrapping(true);

        gridLayout_3->addWidget(day, 1, 0, 1, 1);

        year = new MaxSDK::QmaxSpinBox(groupBox);
        year->setObjectName(QStringLiteral("year"));

        gridLayout_3->addWidget(year, 1, 2, 1, 1);


        verticalLayout_5->addWidget(groupBox);

        dst = new QGroupBox(groupBox_dateTime);
        dst->setObjectName(QStringLiteral("dst"));
        dst->setEnabled(true);
        dst->setCheckable(true);
        verticalLayout_4 = new QVBoxLayout(dst);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        dst_use_date_range = new QCheckBox(dst);
        dst_use_date_range->setObjectName(QStringLiteral("dst_use_date_range"));

        verticalLayout_4->addWidget(dst_use_date_range);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_26 = new QLabel(dst);
        label_26->setObjectName(QStringLiteral("label_26"));
        label_26->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label_26, 0, 1, 1, 1);

        label_27 = new QLabel(dst);
        label_27->setObjectName(QStringLiteral("label_27"));

        gridLayout->addWidget(label_27, 1, 0, 1, 1);

        dst_start_day = new MaxSDK::QmaxSpinBox(dst);
        dst_start_day->setObjectName(QStringLiteral("dst_start_day"));
        dst_start_day->setWrapping(true);
        dst_start_day->setValue(15);

        gridLayout->addWidget(dst_start_day, 1, 1, 1, 1);

        dst_start_month = new MaxSDK::QmaxSpinBox(dst);
        dst_start_month->setObjectName(QStringLiteral("dst_start_month"));
        dst_start_month->setWrapping(true);
        dst_start_month->setValue(3);

        gridLayout->addWidget(dst_start_month, 1, 2, 1, 1);

        label_28 = new QLabel(dst);
        label_28->setObjectName(QStringLiteral("label_28"));

        gridLayout->addWidget(label_28, 2, 0, 1, 1);

        dst_end_day = new MaxSDK::QmaxSpinBox(dst);
        dst_end_day->setObjectName(QStringLiteral("dst_end_day"));
        dst_end_day->setWrapping(true);
        dst_end_day->setValue(1);

        gridLayout->addWidget(dst_end_day, 2, 1, 1, 1);

        dst_end_month = new MaxSDK::QmaxSpinBox(dst);
        dst_end_month->setObjectName(QStringLiteral("dst_end_month"));
        dst_end_month->setWrapping(true);
        dst_end_month->setValue(11);

        gridLayout->addWidget(dst_end_month, 2, 2, 1, 1);

        label_25 = new QLabel(dst);
        label_25->setObjectName(QStringLiteral("label_25"));
        label_25->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label_25, 0, 2, 1, 1);


        verticalLayout_4->addLayout(gridLayout);


        verticalLayout_5->addWidget(dst);


        verticalLayout->addWidget(groupBox_dateTime);

        groupBox_Location = new QGroupBox(SunPositioner_SunPosition);
        groupBox_Location->setObjectName(QStringLiteral("groupBox_Location"));
        verticalLayout_3 = new QVBoxLayout(groupBox_Location);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        location = new QPushButton(groupBox_Location);
        location->setObjectName(QStringLiteral("location"));

        verticalLayout_3->addWidget(location);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_2 = new QLabel(groupBox_Location);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_2->addWidget(label_2);

        latitude_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox_Location);
        latitude_deg->setObjectName(QStringLiteral("latitude_deg"));

        horizontalLayout_2->addWidget(latitude_deg);


        verticalLayout_3->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        label_9 = new QLabel(groupBox_Location);
        label_9->setObjectName(QStringLiteral("label_9"));

        horizontalLayout_3->addWidget(label_9);

        longitude_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox_Location);
        longitude_deg->setObjectName(QStringLiteral("longitude_deg"));

        horizontalLayout_3->addWidget(longitude_deg);


        verticalLayout_3->addLayout(horizontalLayout_3);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QStringLiteral("horizontalLayout_9"));
        label_19 = new QLabel(groupBox_Location);
        label_19->setObjectName(QStringLiteral("label_19"));

        horizontalLayout_9->addWidget(label_19);

        time_zone = new MaxSDK::QmaxDoubleSpinBox(groupBox_Location);
        time_zone->setObjectName(QStringLiteral("time_zone"));
        time_zone->setDecimals(1);

        horizontalLayout_9->addWidget(time_zone);


        verticalLayout_3->addLayout(horizontalLayout_9);


        verticalLayout->addWidget(groupBox_Location);

        groupBox_horizontalCoords = new QGroupBox(SunPositioner_SunPosition);
        groupBox_horizontalCoords->setObjectName(QStringLiteral("groupBox_horizontalCoords"));
        verticalLayout_6 = new QVBoxLayout(groupBox_horizontalCoords);
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        label_5 = new QLabel(groupBox_horizontalCoords);
        label_5->setObjectName(QStringLiteral("label_5"));

        horizontalLayout_5->addWidget(label_5);

        azimuth_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox_horizontalCoords);
        azimuth_deg->setObjectName(QStringLiteral("azimuth_deg"));

        horizontalLayout_5->addWidget(azimuth_deg);


        verticalLayout_6->addLayout(horizontalLayout_5);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));
        label_20 = new QLabel(groupBox_horizontalCoords);
        label_20->setObjectName(QStringLiteral("label_20"));

        horizontalLayout_10->addWidget(label_20);

        altitude_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox_horizontalCoords);
        altitude_deg->setObjectName(QStringLiteral("altitude_deg"));

        horizontalLayout_10->addWidget(altitude_deg);


        verticalLayout_6->addLayout(horizontalLayout_10);


        verticalLayout->addWidget(groupBox_horizontalCoords);


        retranslateUi(SunPositioner_SunPosition);
        QObject::connect(datetime, SIGNAL(toggled(bool)), groupBox_dateTime, SLOT(setEnabled(bool)));
        QObject::connect(datetime, SIGNAL(toggled(bool)), groupBox_Location, SLOT(setEnabled(bool)));
        QObject::connect(weatherfile, SIGNAL(toggled(bool)), label_weatherFileStatus, SLOT(setVisible(bool)));
        QObject::connect(weatherfile, SIGNAL(toggled(bool)), pushButton_weatherFile, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), label_26, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), label_25, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), label_27, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), dst_start_day, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), dst_start_month, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), label_28, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), dst_end_day, SLOT(setEnabled(bool)));
        QObject::connect(dst_use_date_range, SIGNAL(toggled(bool)), dst_end_month, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(SunPositioner_SunPosition);
    } // setupUi

    void retranslateUi(QWidget *SunPositioner_SunPosition)
    {
        SunPositioner_SunPosition->setWindowTitle(QApplication::translate("SunPositioner_SunPosition", "Form", nullptr));
        pushButton_installEnvironment->setText(QApplication::translate("SunPositioner_SunPosition", "Install Sun && Sky Environment", nullptr));
        groupBox_5->setTitle(QApplication::translate("SunPositioner_SunPosition", "Date && Time Mode", nullptr));
#ifndef QT_NO_TOOLTIP
        datetime->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Position the sun according to date, time, and location information.", nullptr));
#endif // QT_NO_TOOLTIP
        datetime->setText(QApplication::translate("SunPositioner_SunPosition", "Date, Time && Location", nullptr));
#ifndef QT_NO_TOOLTIP
        weatherfile->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Use weather data to position the sun and define sky illuminance.", nullptr));
#endif // QT_NO_TOOLTIP
        weatherfile->setText(QApplication::translate("SunPositioner_SunPosition", "Weather Data File", nullptr));
        pushButton_weatherFile->setText(QApplication::translate("SunPositioner_SunPosition", "Setup", nullptr));
        label_weatherFileStatus->setText(QApplication::translate("SunPositioner_SunPosition", "Weather file status message", nullptr));
#ifndef QT_NO_TOOLTIP
        manual->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Manually position the sun by translation/rotation, or by azimuth/altitude.", nullptr));
#endif // QT_NO_TOOLTIP
        manual->setText(QApplication::translate("SunPositioner_SunPosition", "Manual", nullptr));
        groupBox_dateTime->setTitle(QApplication::translate("SunPositioner_SunPosition", "Date && Time", nullptr));
        label_15->setText(QApplication::translate("SunPositioner_SunPosition", "Time:", nullptr));
#ifndef QT_NO_TOOLTIP
        hours->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Hours", nullptr));
#endif // QT_NO_TOOLTIP
        hours->setSuffix(QApplication::translate("SunPositioner_SunPosition", " h", nullptr));
#ifndef QT_NO_TOOLTIP
        minutes->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Minutes", nullptr));
#endif // QT_NO_TOOLTIP
        minutes->setSuffix(QApplication::translate("SunPositioner_SunPosition", " min", nullptr));
        label_17->setText(QApplication::translate("SunPositioner_SunPosition", "Month", nullptr));
        label_18->setText(QApplication::translate("SunPositioner_SunPosition", "Year", nullptr));
#ifndef QT_NO_TOOLTIP
        month->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Month", nullptr));
#endif // QT_NO_TOOLTIP
        label_16->setText(QApplication::translate("SunPositioner_SunPosition", "Day", nullptr));
#ifndef QT_NO_TOOLTIP
        day->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Day of month", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        year->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Year", nullptr));
#endif // QT_NO_TOOLTIP
        dst->setTitle(QApplication::translate("SunPositioner_SunPosition", "Daylight Saving Time", nullptr));
        dst_use_date_range->setText(QApplication::translate("SunPositioner_SunPosition", "Use Date Range", nullptr));
        label_26->setText(QApplication::translate("SunPositioner_SunPosition", "Day", nullptr));
        label_27->setText(QApplication::translate("SunPositioner_SunPosition", "Start:", nullptr));
#ifndef QT_NO_TOOLTIP
        dst_start_day->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Day of month", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        dst_start_month->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Month", nullptr));
#endif // QT_NO_TOOLTIP
        label_28->setText(QApplication::translate("SunPositioner_SunPosition", "End:", nullptr));
#ifndef QT_NO_TOOLTIP
        dst_end_day->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Day of month", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        dst_end_month->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Month", nullptr));
#endif // QT_NO_TOOLTIP
        label_25->setText(QApplication::translate("SunPositioner_SunPosition", "Month", nullptr));
        groupBox_Location->setTitle(QApplication::translate("SunPositioner_SunPosition", "Location on Earth", nullptr));
#ifndef QT_NO_TOOLTIP
        location->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Select a location from a user-configurable database.", nullptr));
#endif // QT_NO_TOOLTIP
        location->setText(QApplication::translate("SunPositioner_SunPosition", "Choose Location", nullptr));
        label_2->setText(QApplication::translate("SunPositioner_SunPosition", "Latitude:", nullptr));
#ifndef QT_NO_TOOLTIP
        latitude_deg->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Latitude", nullptr));
#endif // QT_NO_TOOLTIP
        latitude_deg->setSuffix(QApplication::translate("SunPositioner_SunPosition", "\313\232", nullptr));
        label_9->setText(QApplication::translate("SunPositioner_SunPosition", "Longitude:", nullptr));
#ifndef QT_NO_TOOLTIP
        longitude_deg->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Longitude", nullptr));
#endif // QT_NO_TOOLTIP
        longitude_deg->setSuffix(QApplication::translate("SunPositioner_SunPosition", "\313\232", nullptr));
        label_19->setText(QApplication::translate("SunPositioner_SunPosition", "Time Zone (\302\261GMT):", nullptr));
#ifndef QT_NO_TOOLTIP
        time_zone->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Time zone, as the offset from GMT.", nullptr));
#endif // QT_NO_TOOLTIP
        time_zone->setSuffix(QApplication::translate("SunPositioner_SunPosition", " h", nullptr));
        groupBox_horizontalCoords->setTitle(QApplication::translate("SunPositioner_SunPosition", "Horizontal Coordinates", nullptr));
        label_5->setText(QApplication::translate("SunPositioner_SunPosition", "Azimuth:", nullptr));
#ifndef QT_NO_TOOLTIP
        azimuth_deg->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Azimuth of the sun position in the sky, relative to the North direction.", nullptr));
#endif // QT_NO_TOOLTIP
        azimuth_deg->setSuffix(QApplication::translate("SunPositioner_SunPosition", "\313\232", nullptr));
        label_20->setText(QApplication::translate("SunPositioner_SunPosition", "Altitude:", nullptr));
#ifndef QT_NO_TOOLTIP
        altitude_deg->setToolTip(QApplication::translate("SunPositioner_SunPosition", "Altitude of the sun in the sky, relative to the horizon.", nullptr));
#endif // QT_NO_TOOLTIP
        altitude_deg->setSuffix(QApplication::translate("SunPositioner_SunPosition", "\313\232", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SunPositioner_SunPosition: public Ui_SunPositioner_SunPosition {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SUNPOSITIONER_SUNPOSITION_H
