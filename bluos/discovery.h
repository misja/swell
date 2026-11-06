// SPDX-License-Identifier: MIT
// LSDP discovery of BluOS players (BluOS Custom Integration API v1.7, appendix):
// a burst of seven queries, then listening for announcements and deletions.
#ifndef BLUOS_DISCOVERY_H
#define BLUOS_DISCOVERY_H

#include <QHostAddress>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>

#include "lsdp.h"

namespace bluos {

class Discovery : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)

public:
    struct Found {
        QString nodeId;   // stable node key, e.g. "02:00:00:00:00:0A"; with port it identifies a player
        QHostAddress address;
        int port = 11000;
        QString name;
        QString model;
        quint16 classId = 0;
    };

    explicit Discovery(QObject *parent = nullptr);

    bool searching() const { return m_burst.isActive(); }
    bool listening() const { return m_socket.state() == QAbstractSocket::BoundState; }

    // Handles one received datagram; public so tests can inject packets.
    void handleDatagram(const QByteArray &datagram);

public slots:
    // Binds the LSDP port (shared) if needed and sends the query burst.
    void search();
    void stop();

signals:
    void found(const bluos::Discovery::Found &player);
    void lost(const QString &nodeId);   // every player of that node
    void searchingChanged();
    void error(const QString &message);

private:
    void sendQuery();
    void readPending();

    QUdpSocket m_socket;
    QTimer m_burst;
    int m_sent = 0;
};

} // namespace bluos

Q_DECLARE_METATYPE(bluos::Discovery::Found)

#endif
