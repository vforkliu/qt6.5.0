# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(WrapProtobuf
    PROVIDED_TARGETS
        WrapProtobuf::WrapLibProtoc
        WrapProtobuf::WrapLibProtobuf
    MODULE_NAME global
)

# WrapProtoc::WrapProtoc could come from top-level CMakeLists.txt so avoid promoting it to GLOBAL
# here in this case.
if(TARGET WrapProtoc::WrapProtoc)
    qt_internal_disable_find_package_global_promotion(WrapProtoc::WrapProtoc)
endif()
qt_find_package(WrapProtoc
    PROVIDED_TARGETS WrapProtoc::WrapProtoc
    MODULE_NAME global
)

qt_feature("qtprotobufgen" PRIVATE
    SECTION "Utilities"
    LABEL "Qt Protobuf generator"
    PURPOSE "Provides support for generating Qt-based classes for use with Protocol Buffers."
    CONDITION TARGET WrapProtobuf::WrapLibProtoc AND TARGET WrapProtobuf::WrapLibProtobuf AND
        TARGET WrapProtoc::WrapProtoc
)

qt_configure_add_summary_section(NAME "Qt Protobuf tools")
qt_configure_add_summary_entry(ARGS "qtprotobufgen")
qt_configure_end_summary_section()
