# SPDX-License-Identifier: MIT
TARGET = tst_browse
QT += testlib
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
INCLUDEPATH += ../client
HEADERS += ../client/fakeplayer.h
SOURCES += tst_browse.cpp
