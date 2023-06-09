# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfileinfo Test:
#####################################################################

qt_internal_add_test(tst_qfileinfo
    SOURCES
        tst_qfileinfo.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
set(qfileinfo_resource_files
    "resources/"
)

qt_internal_add_resource(tst_qfileinfo "qfileinfo"
    PREFIX
        "/tst_qfileinfo/"
    FILES
        ${qfileinfo_resource_files}
)
set(testdata_resource_files
    "resources/file1"
    "resources/file1.ext1"
    "resources/file1.ext1.ext2"
    "tst_qfileinfo.cpp"
)

qt_internal_add_resource(tst_qfileinfo "testdata"
    PREFIX
        "/testdata"
    FILES
        ${testdata_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfileinfo CONDITION WIN32
    LIBRARIES
        advapi32
        netapi32
)

if (APPLE)
    target_compile_options(tst_qfileinfo PRIVATE -x objective-c++)
endif()
