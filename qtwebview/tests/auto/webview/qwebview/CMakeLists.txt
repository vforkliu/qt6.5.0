# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwebview Test:
#####################################################################

qt_internal_add_test(tst_qwebview
    SOURCES
        tst_qwebview.cpp
    LIBRARIES
        Qt::Gui
        Qt::WebViewPrivate
)

qt_internal_extend_target(tst_qwebview CONDITION TARGET Qt::WebViewQuick
    DEFINES
        QT_QQUICKWEBVIEW_TESTS
    LIBRARIES
        Qt::WebViewQuickPrivate
        Qt::Qml
)

qt_internal_extend_target(tst_qwebview CONDITION TARGET Qt::WebEngineQuick
    DEFINES
        QT_WEBVIEW_WEBENGINE_BACKEND
    LIBRARIES
        Qt::WebEngineQuickPrivate
        Qt::Qml
)
