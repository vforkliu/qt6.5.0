# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_bench_qpixmapcache Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qpixmapcache
    SOURCES
        tst_qpixmapcache.cpp
    LIBRARIES
        Qt::Gui
        Qt::Test
)
