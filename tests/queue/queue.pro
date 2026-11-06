# SPDX-License-Identifier: MIT
TARGET = tst_queue
QT += testlib
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
INCLUDEPATH += ../client
HEADERS += ../client/fakeplayer.h
SOURCES += tst_queue.cpp
