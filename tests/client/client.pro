# SPDX-License-Identifier: MIT
TARGET = tst_client
QT += testlib
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
HEADERS += fakeplayer.h
SOURCES += tst_client.cpp
