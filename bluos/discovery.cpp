// SPDX-License-Identifier: MIT
#include "discovery.h"

#include <random>

#include <QNetworkInterface>

namespace BluOS {

namespace {

// Absolute send times of the startup burst, in ms (API appendix, timing),
// each with up to 250 ms of random delay.
const int BurstMs[] = { 0, 1000, 2000, 3000, 5000, 7000, 10000 };
const int BurstCount = int(sizeof(BurstMs) / sizeof(BurstMs[0]));

int jitter()
{
    // Seeded once per process: an unseeded qrand() gives every device the
    // same delays, so devices that start together would send together.
    static std::minstd_rand engine(std::random_device{}());
    return std::uniform_int_distribution<int>(0, 250)(engine);
}

} // namespace

Discovery::Discovery(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<BluOS::Discovery::Found>();
    m_burst.setSingleShot(true);
    connect(&m_burst, &QTimer::timeout, this, &Discovery::sendQuery);
    connect(&m_socket, &QUdpSocket::readyRead, this, &Discovery::readPending);
}

void Discovery::search()
{
    if (!listening()) {
        // Players answer by broadcast to the LSDP port, so listen there; other
        // LSDP users on this device may share it.
        if (!m_socket.bind(QHostAddress(QHostAddress::AnyIPv4), Lsdp::Port,
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            emit error(m_socket.errorString());
            return;
        }
    }
    const bool wasSearching = searching();
    m_sent = 0;
    m_burst.start(jitter());
    if (!wasSearching)
        emit searchingChanged();
}


void Discovery::sendQuery()
{
    const QByteArray query = Lsdp::buildQuery({ Lsdp::AllClasses });
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
                || !(flags & QNetworkInterface::CanBroadcast) || (flags & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.broadcast().isNull())
                m_socket.writeDatagram(query, entry.broadcast(), Lsdp::Port);
        }
    }
    ++m_sent;
    if (m_sent < BurstCount) {
        m_burst.start(BurstMs[m_sent] - BurstMs[m_sent - 1] + jitter());
    } else {
        emit searchingChanged();
    }
}

void Discovery::readPending()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram(int(m_socket.pendingDatagramSize()), '\0');
        m_socket.readDatagram(datagram.data(), datagram.size());
        handleDatagram(datagram);
    }
}

void Discovery::handleDatagram(const QByteArray &datagram)
{
    const Lsdp::Packet packet = Lsdp::parse(datagram);
    if (!packet.valid)
        return;
    for (const Lsdp::Announce &announce : packet.announces) {
        for (const Lsdp::Record &record : announce.records) {
            if (!Lsdp::isPlayerClass(record.classId))
                continue;
            Found player;
            player.nodeId = Lsdp::nodeIdToString(announce.nodeId);
            player.address = announce.address;
            bool ok = false;
            const int port = record.txt.value(QStringLiteral("port")).toInt(&ok);
            player.port = ok && port > 0 && port < 65536 ? port : 11000;
            player.name = record.txt.value(QStringLiteral("name"));
            player.model = record.txt.value(QStringLiteral("model"));
            player.classId = record.classId;
            // A multi-zone player may announce several player classes on
            // different ports under one node id; each is its own player.
            emit found(player);
        }
    }
    for (const Lsdp::Delete &del : packet.deletes) {
        for (quint16 classId : del.classes) {
            if (Lsdp::isPlayerClass(classId)) {
                emit lost(Lsdp::nodeIdToString(del.nodeId));
                break;
            }
        }
    }
}

} // namespace BluOS
