# SPDX-License-Identifier: MIT
# BluOS protocol library: LSDP discovery and the control API (port 11000),
# following the BluOS Custom Integration API v1.7. Qt Core/Network only, no UI.
QT += network
INCLUDEPATH += $$PWD
HEADERS += \
    $$PWD/client.h \
    $$PWD/discovery.h \
    $$PWD/lsdp.h \
    $$PWD/playermodel.h \
    $$PWD/queuemodel.h \
    $$PWD/browsemodel.h \
    $$PWD/xml.h
SOURCES += \
    $$PWD/client.cpp \
    $$PWD/discovery.cpp \
    $$PWD/lsdp.cpp \
    $$PWD/playermodel.cpp \
    $$PWD/queuemodel.cpp \
    $$PWD/browsemodel.cpp \
    $$PWD/xml.cpp
