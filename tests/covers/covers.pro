# SPDX-License-Identifier: MIT
TARGET = tst_covers
QT += testlib network qml
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
INCLUDEPATH += ../../src ../client
HEADERS += ../../src/covercache.h ../client/fakeplayer.h
SOURCES += tst_covers.cpp ../../src/covercache.cpp
