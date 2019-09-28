/********************************************************************************
** Form generated from reading UI file 'SunPositioner_Display.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SUNPOSITIONER_DISPLAY_H
#define UI_SUNPOSITIONER_DISPLAY_H

#include <Qt/QmaxSpinBox.h>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <qt/QmaxSpinBox.h>

QT_BEGIN_NAMESPACE

class Ui_SunPositioner_Display
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label_3;
    QLabel *label;
    MaxSDK::QmaxDoubleSpinBox *north_direction_deg;
    QCheckBox *show_compass;
    MaxSDK::QmaxWorldSpinBox *compass_radius;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *label_2;
    MaxSDK::QmaxWorldSpinBox *sun_distance;

    void setupUi(QWidget *SunPositioner_Display)
    {
        if (SunPositioner_Display->objectName().isEmpty())
            SunPositioner_Display->setObjectName(QStringLiteral("SunPositioner_Display"));
        SunPositioner_Display->resize(188, 209);
        verticalLayout = new QVBoxLayout(SunPositioner_Display);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        groupBox = new QGroupBox(SunPositioner_Display);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setMargin(0);

        gridLayout->addWidget(label_3, 4, 0, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName(QStringLiteral("label"));
        label->setMargin(0);

        gridLayout->addWidget(label, 2, 0, 1, 1);

        north_direction_deg = new MaxSDK::QmaxDoubleSpinBox(groupBox);
        north_direction_deg->setObjectName(QStringLiteral("north_direction_deg"));

        gridLayout->addWidget(north_direction_deg, 4, 1, 1, 1);

        show_compass = new QCheckBox(groupBox);
        show_compass->setObjectName(QStringLiteral("show_compass"));
        show_compass->setChecked(true);

        gridLayout->addWidget(show_compass, 0, 0, 1, 2);

        compass_radius = new MaxSDK::QmaxWorldSpinBox(groupBox);
        compass_radius->setObjectName(QStringLiteral("compass_radius"));

        gridLayout->addWidget(compass_radius, 2, 1, 1, 1);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(SunPositioner_Display);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout_2->addWidget(label_2, 0, 0, 1, 1);

        sun_distance = new MaxSDK::QmaxWorldSpinBox(groupBox_2);
        sun_distance->setObjectName(QStringLiteral("sun_distance"));

        gridLayout_2->addWidget(sun_distance, 0, 1, 1, 1);


        verticalLayout->addWidget(groupBox_2);


        retranslateUi(SunPositioner_Display);
        QObject::connect(show_compass, SIGNAL(toggled(bool)), label, SLOT(setEnabled(bool)));
        QObject::connect(show_compass, SIGNAL(toggled(bool)), compass_radius, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(SunPositioner_Display);
    } // setupUi

    void retranslateUi(QWidget *SunPositioner_Display)
    {
        SunPositioner_Display->setWindowTitle(QApplication::translate("SunPositioner_Display", "Form", nullptr));
        groupBox->setTitle(QApplication::translate("SunPositioner_Display", "Compass Rose", nullptr));
        label_3->setText(QApplication::translate("SunPositioner_Display", "North Offset:", nullptr));
        label->setText(QApplication::translate("SunPositioner_Display", "    Radius:", nullptr));
#ifndef QT_NO_TOOLTIP
        north_direction_deg->setToolTip(QApplication::translate("SunPositioner_Display", "Rotates the compass rose, changing the cardinal directions used for positioning the sun according to date and time.", nullptr));
#endif // QT_NO_TOOLTIP
        north_direction_deg->setSuffix(QApplication::translate("SunPositioner_Display", "\313\232", nullptr));
#ifndef QT_NO_TOOLTIP
        show_compass->setToolTip(QApplication::translate("SunPositioner_Display", "Shows or hides the compass rose from the viewport.", nullptr));
#endif // QT_NO_TOOLTIP
        show_compass->setText(QApplication::translate("SunPositioner_Display", "Show", nullptr));
#ifndef QT_NO_TOOLTIP
        compass_radius->setToolTip(QApplication::translate("SunPositioner_Display", "The size of the compass rose, as displayed in the viewport.", nullptr));
#endif // QT_NO_TOOLTIP
        groupBox_2->setTitle(QApplication::translate("SunPositioner_Display", "Sun", nullptr));
        label_2->setText(QApplication::translate("SunPositioner_Display", "Distance:", nullptr));
#ifndef QT_NO_TOOLTIP
        sun_distance->setToolTip(QApplication::translate("SunPositioner_Display", "The distance of the sun from the compass rose, as displayed in the viewport.", nullptr));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class SunPositioner_Display: public Ui_SunPositioner_Display {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SUNPOSITIONER_DISPLAY_H
