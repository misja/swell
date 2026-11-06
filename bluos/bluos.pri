# SPDX-License-Identifier: MIT
# BluOS protocol library: LSDP discovery and the control API (port 11000),
# following the BluOS Custom Integration API v1.7. Qt Core/Network only, no UI.
QT += network
INCLUDEPATH += $$PWD
HEADERS += \
    $$PWD/client.h \
    $$PWD/lsdp.h \
    $$PWD/xml.h
SOURCES += \
    $$PWD/client.cpp \
    $$PWD/lsdp.cpp \
    $$PWD/xml.cpp
