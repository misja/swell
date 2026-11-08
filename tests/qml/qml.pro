# SPDX-License-Identifier: MIT
# Compiles every QML file of the app in the target (spec AC4), without a display.
TARGET = tst_qml
QT += testlib qml quick
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
INCLUDEPATH += ../../src
HEADERS += ../../src/qmlbrowsemodel.h ../../src/playerlistmodel.h
SOURCES += tst_qml.cpp ../../src/registration.cpp ../../src/qmlbrowsemodel.cpp \
           ../../src/playerlistmodel.cpp
DEFINES += SWELL_SOURCE_DIR=\\\"$$PWD/../..\\\"
