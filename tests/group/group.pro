# SPDX-License-Identifier: MIT
TARGET = tst_group
QT += testlib
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
INCLUDEPATH += ../client ../../src
HEADERS += ../client/fakeplayer.h ../client/fakehousehold.h ../../src/playerlistmodel.h
SOURCES += tst_group.cpp ../../src/playerlistmodel.cpp
