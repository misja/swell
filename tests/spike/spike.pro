# SPDX-License-Identifier: MIT
# Spike: do loopback TCP and UDP sockets work for QtTest in the SDK target?
TARGET = tst_spike
QT += testlib network
QT -= gui
CONFIG += testcase console
CONFIG -= app_bundle
SOURCES += tst_spike.cpp
