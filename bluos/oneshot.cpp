// SPDX-License-Identifier: MIT
#include "oneshot.h"

namespace BluOS {

OneShotGet::OneShotGet(const QString &host, int port, const QByteArray &target, int deadlineMs,
                       QObject *parent)
    : QObject(parent)
{
    const QByteArray hostHeader = host.contains(QLatin1Char(':'))
            ? "[" + host.toUtf8() + "]:" + QByteArray::number(port)
            : host.toUtf8() + ":" + QByteArray::number(port);
    m_request = "GET " + target + " HTTP/1.1\r\nHost: " + hostHeader
            + "\r\nConnection: close\r\n\r\n";

    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        // Written once; nothing below writes again.
        m_socket.write(m_request);
    });
    connect(&m_socket, &QTcpSocket::readyRead, this, [this]() { m_socket.readAll(); });
    connect(&m_socket, &QTcpSocket::disconnected, this, &OneShotGet::finish);
    connect(&m_socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &OneShotGet::finish);

    m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, &OneShotGet::finish);
    m_deadline.start(deadlineMs);
    m_socket.connectToHost(host, quint16(port));
}

void OneShotGet::finish()
{
    if (m_done)
        return;
    m_done = true;
    m_deadline.stop();
    m_socket.abort();
    deleteLater();
}

} // namespace BluOS
