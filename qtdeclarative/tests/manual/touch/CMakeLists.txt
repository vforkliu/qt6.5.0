# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from touch.pro.

#####################################################################
## touch Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_touch
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "flicktext.qml"
    "main.qml"
    "mpta-crosshairs.qml"
)

qt_internal_add_resource(tst_manual_touch "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

set(quick_shared_resource_files
    "../../../examples/quick/shared/Button.qml"
    "../../../examples/quick/shared/CheckBox.qml"
    "../../../examples/quick/shared/Label.qml"
    "../../../examples/quick/shared/LauncherList.qml"
    "../../../examples/quick/shared/SimpleLauncherDelegate.qml"
    "../../../examples/quick/shared/Slider.qml"
    "../../../examples/quick/shared/TextField.qml"
    "../../../examples/quick/shared/images/back.png"
    "../../../examples/quick/shared/images/checkmark.png"
    "../../../examples/quick/shared/images/next.png"
    "../../../examples/quick/shared/images/slider_handle.png"
)

qt_internal_add_resource(tst_manual_touch "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../../../examples/quick/shared"
    FILES
        ${quick_shared_resource_files}
)


#### Keys ignored in scope 1:.:.:touch.pro:<TRUE>:
# TEMPLATE = "app"
