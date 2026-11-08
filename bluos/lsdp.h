// SPDX-License-Identifier: MIT
// LSDP (Lenbrook Service Discovery Protocol) packets, as specified in the
// appendix of the BluOS Custom Integration API v1.7.
#ifndef BLUOS_LSDP_H
#define BLUOS_LSDP_H

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QString>
#include <QVector>

namespace BluOS {
namespace Lsdp {

const quint16 Port = 11430;
const quint16 AllClasses = 0xFFFF;

// Class IDs that identify a controllable player.
const quint16 ClassPlayer = 0x0001;
const quint16 ClassPlayerSecondary = 0x0003;
const quint16 ClassPlayerPairSlave = 0x0006;
const quint16 ClassHub = 0x0008;

bool isPlayerClass(quint16 classId);

struct Record {
    quint16 classId = 0;
    QHash<QString, QString> txt;
};

struct Announce {
    QByteArray nodeId;
    QHostAddress address;
    QVector<Record> records;
};

struct Delete {
    QByteArray nodeId;
    QVector<quint16> classes;
};

struct Packet {
    bool valid = false;      // header recognised
    bool truncated = false;  // a message ran past the end of the datagram
    QVector<Announce> announces;
    QVector<Delete> deletes;
    int queries = 0;         // Q/R messages, e.g. our own echoed query
    int skipped = 0;         // unknown or malformed messages
};

Packet parse(const QByteArray &datagram);

QByteArray buildQuery(const QVector<quint16> &classes);

// "02:00:00:00:00:0A" style rendering of a node id, for display and logs.
QString nodeIdToString(const QByteArray &nodeId);

} // namespace Lsdp
} // namespace BluOS

#endif
