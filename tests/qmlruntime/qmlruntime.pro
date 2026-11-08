# SPDX-License-Identifier: MIT
# Runs the expressions the pages use on the player and browse models, against
# the in-process FakePlayer and the app's own QML registration (spec 007).
TARGET = tst_qmlruntime
QT += testlib qml quick
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
INCLUDEPATH += ../../src ../client
HEADERS += ../client/fakeplayer.h
SOURCES += tst_qmlruntime.cpp ../../src/registration.cpp
DEFINES += SWELL_SOURCE_DIR=\\\"$$PWD/../..\\\"
