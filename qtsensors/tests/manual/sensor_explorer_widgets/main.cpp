// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "explorer.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Explorer explorer;
    explorer.show();
    return app.exec();
}

