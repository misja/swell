// SPDX-License-Identifier: MIT
// A minimal in-process stand-in for a player's control port, for tests only.
// It serves fixed documents per path, holds /Status long polls until the
// status changes or the requested timeout passes, can redirect a path to
// another server, and logs every request it receives.
#ifndef FAKEPLAYER_H
#define FAKEPLAYER_H

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

class FakePlayer : public QObject
{
    Q_OBJECT

public:
    struct Request {
        QString path;
        QUrlQuery query;
        qint64 atMs;
        QByteArray target;   // the request target exactly as sent
    };

    explicit FakePlayer(QObject *parent = nullptr) : QObject(parent)
    {
        m_clock.start();
        connect(&m_server, &QTcpServer::newConnection, this, &FakePlayer::accept);
    }

    bool listen(quint16 port = 0)
    {
        return m_server.listen(QHostAddress(QHostAddress::LocalHost), port);
    }

    // Stops answering and drops every open connection, like a player going away.
    void close()
    {
        m_server.close();
        for (const Held &held : m_held)
            if (held.socket)
                held.socket->abort();
        m_held.clear();
    }

    quint16 port() const { return m_port ? m_port : m_server.serverPort(); }
    void rememberPort() { m_port = m_server.serverPort(); }

    // The <status> document; its etag attribute must change when it changes.
    void setStatus(const QByteArray &xml)
    {
        m_routes.insert(QStringLiteral("/Status"), xml);
        const QList<Held> held = m_held;
        m_held.clear();
        for (const Held &h : held)
            if (h.socket)
                respond(h.socket, QStringLiteral("/Status"));
    }

    void serve(const QString &path, const QByteArray &body,
               const QByteArray &contentType = "text/xml")
    {
        m_routes.insert(path, body);
        m_types.insert(path, contentType);
    }

    void redirect(const QString &path, const QUrl &to) { m_redirects.insert(path, to); }
    // Answers this path with an HTTP error status instead of its document.
    void failWith(const QString &path, int status) { m_failures.insert(path, status); }

    QList<Request> requests() const { return m_requests; }
    QList<Request> requests(const QString &path) const
    {
        QList<Request> result;
        for (const Request &r : m_requests)
            if (r.path == path)
                result.append(r);
        return result;
    }

private:
    struct Held {
        QPointer<QTcpSocket> socket;
    };

    static QString etagOf(const QByteArray &xml)
    {
        const int at = xml.indexOf("etag=\"");
        if (at < 0)
            return QString();
        const int end = xml.indexOf('"', at + 6);
        return QString::fromUtf8(xml.mid(at + 6, end - at - 6));
    }

    void accept()
    {
        while (QTcpSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { read(socket); });
        }
    }

    void read(QTcpSocket *socket)
    {
        QByteArray &buffer = m_buffers[socket];
        buffer += socket->readAll();
        const int end = buffer.indexOf("\r\n\r\n");
        if (end < 0)
            return;
        const QList<QByteArray> requestLine = buffer.left(buffer.indexOf("\r\n")).split(' ');
        m_buffers.remove(socket);
        if (requestLine.size() < 2)
            return;
        const QByteArray raw = requestLine.at(1);
        const QUrl target = QUrl::fromEncoded(raw);
        const QUrlQuery query(target);
        const QString path = target.path();
        m_requests.append({ path, query, m_clock.elapsed(), raw });

        if (path == QLatin1String("/Status") && query.hasQueryItem(QStringLiteral("etag"))
                && query.queryItemValue(QStringLiteral("etag")) == etagOf(m_routes.value(path))) {
            const int timeout = query.queryItemValue(QStringLiteral("timeout")).toInt();
            m_held.append({ socket });
            QPointer<QTcpSocket> guarded(socket);
            QTimer::singleShot(timeout * 1000, this, [this, guarded]() {
                for (int i = 0; i < m_held.size(); ++i) {
                    if (m_held.at(i).socket == guarded) {
                        m_held.removeAt(i);
                        if (guarded)
                            respond(guarded, QStringLiteral("/Status"));
                        return;
                    }
                }
            });
            return;
        }
        respond(socket, path);
    }

    void respond(QTcpSocket *socket, const QString &path)
    {
        QByteArray head;
        QByteArray body;
        if (m_failures.contains(path)) {
            body = "error";
            head = "HTTP/1.1 " + QByteArray::number(m_failures.value(path)) + " Error\r\nContent-Type: text/plain\r\n";
        } else if (m_redirects.contains(path)) {
            head = "HTTP/1.1 302 Found\r\nLocation: " + m_redirects.value(path).toEncoded() + "\r\n";
        } else if (m_routes.contains(path)) {
            body = m_routes.value(path);
            head = "HTTP/1.1 200 OK\r\nContent-Type: " + m_types.value(path, "text/xml") + "\r\n";
        } else {
            body = "<html><body>Not found</body></html>";
            head = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n";
        }
        head += "Content-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n";
        socket->write(head + body);
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    quint16 m_port = 0;
    QElapsedTimer m_clock;
    QHash<QString, QByteArray> m_routes;
    QHash<QString, QByteArray> m_types;
    QHash<QString, QUrl> m_redirects;
    QHash<QString, int> m_failures;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QList<Held> m_held;
    QList<Request> m_requests;
};

#endif
