# SPDX-License-Identifier: MIT
TARGET = tst_xml
QT += testlib
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
include(../../bluos/bluos.pri)
SOURCES += tst_xml.cpp
