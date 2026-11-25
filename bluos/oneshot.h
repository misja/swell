// SPDX-License-Identifier: MIT
// One HTTP GET that is written once and never sent again (spec 006, revision
// 2026-11-25). QNetworkAccessManager in Qt 5.6 resends a request by itself
// when the server closes the connection before a complete status line
// (qhttpnetworkconnectionchannel.cpp, reconnectAttemptsDefault), and has no
// attribute to turn that off. For a request that must reach the player at
// most once, this writes it on its own socket and does not reconnect.
#ifndef BLUOS_ONESHOT_H
#define BLUOS_ONESHOT_H

#include <QByteArray>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>

namespace BluOS {

class OneShotGet : public QObject
{
    Q_OBJECT

public:
    // Connects and writes "GET target" once; target is the encoded path and
    // query. The answer is read and dropped; deletes itself when the
    // connection ends or the deadline passes.
    OneShotGet(const QString &host, int port, const QByteArray &target, int deadlineMs,
               QObject *parent = nullptr);

private:
    void finish();

    QTcpSocket m_socket;
    QTimer m_deadline;
    QByteArray m_request;
    bool m_done = false;
};

} // namespace BluOS

#endif
