# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from benchmarks.pro.

add_subdirectory(qml)
add_subdirectory(quick)
if(QT_FEATURE_private_tests)
    add_subdirectory(particles)
endif()
add_subdirectory(quickcontrols)
