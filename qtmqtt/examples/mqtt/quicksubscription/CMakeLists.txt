# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(quicksubscription LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/mqtt/quicksubscription")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Mqtt Qml Quick)

qt_add_executable(quicksubscription
    main.cpp
    qmlmqttclient.cpp qmlmqttclient.h
)

set_target_properties(quicksubscription PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_compile_definitions(quicksubscription PUBLIC
    QT_DEPRECATED_WARNINGS
)

target_link_libraries(quicksubscription PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Mqtt
    Qt::Qml
    Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt6_add_resources(quicksubscription "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS quicksubscription
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
