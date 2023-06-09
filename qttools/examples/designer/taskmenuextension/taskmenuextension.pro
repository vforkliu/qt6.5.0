#! [1]
QT      += widgets designer
#! [1]

QTDIR_build {
# This is only for the Qt build. Do not use externally. We mean it.
PLUGIN_TYPE = designer
PLUGIN_CLASS_NAME = TicTacToePlugin
load(qt_plugin)
CONFIG += install_ok
} else {
# Public example:

#! [0]
TEMPLATE = lib
CONFIG  += plugin
#! [0]

TARGET = $$qtLibraryTarget($$TARGET)

#! [3]
target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target
#! [3]
}

#! [2]
HEADERS += tictactoe.h \
           tictactoedialog.h \
           tictactoeplugin.h \
           tictactoetaskmenu.h
SOURCES += tictactoe.cpp \
           tictactoedialog.cpp \
           tictactoeplugin.cpp \
           tictactoetaskmenu.cpp
OTHER_FILES += tictactoe.json
#! [2]
