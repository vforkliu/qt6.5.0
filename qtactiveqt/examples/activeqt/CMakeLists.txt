# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(MSVC)
    qt_internal_add_example(comapp)
    qt_internal_add_example(hierarchy)
    qt_internal_add_example(menus)
    qt_internal_add_example(multiple)
    qt_internal_add_example(simple)
    qt_internal_add_example(wrapper)
    if(TARGET Qt6::OpenGLWidgets AND QT_FEATURE_opengl AND NOT QT_FEATURE_opengles2)
        qt_internal_add_example(opengl)
    endif()
    if(TARGET Qt::QuickControls2)
        qt_internal_add_example(simpleqml)
    endif()
endif()
if(MINGW OR QT_BUILD_SHARED_LIBS)
    qt_internal_add_example(mediaplayer)
endif()
