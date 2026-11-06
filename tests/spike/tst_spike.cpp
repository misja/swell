// SPDX-License-Identifier: MIT
#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class TstSpike : public QObject
{
    Q_OBJECT

private slots:
    void tcpLoopbackHttp();
    void udpLoopback();
    void timerPrecision();
};

void TstSpike::tcpLoopbackHttp()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QTcpServer::newConnection, [&server]() {
        QTcpSocket *socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, [socket]() {
            socket->readAll();
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\n"
                          "Content-Length: 9\r\nConnection: close\r\n\r\n<status/>");
            socket->disconnectFromHost();
        });
    });

    QNetworkAccessManager nam;
    QUrl url(QStringLiteral("http://127.0.0.1:%1/Status").arg(server.serverPort()));
    QNetworkReply *reply = nam.get(QNetworkRequest(url));
    QSignalSpy finished(reply, &QNetworkReply::finished);
    QVERIFY(finished.wait(5000));
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QCOMPARE(reply->readAll(), QByteArray("<status/>"));
    reply->deleteLater();
}

void TstSpike::udpLoopback()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    QUdpSocket sender;
    const QByteArray datagram("\x06LSDP\x01", 6);
    QCOMPARE(sender.writeDatagram(datagram, QHostAddress::LocalHost, receiver.localPort()),
             qint64(datagram.size()));
    QSignalSpy ready(&receiver, &QUdpSocket::readyRead);
    QVERIFY(ready.wait(5000));
    QByteArray received(int(receiver.pendingDatagramSize()), '\0');
    receiver.readDatagram(received.data(), received.size());
    QCOMPARE(received, datagram);
}

void TstSpike::timerPrecision()
{
    QElapsedTimer elapsed;
    elapsed.start();
    QTest::qWait(200);
    const qint64 ms = elapsed.elapsed();
    qInfo("qWait(200) took %lld ms", ms);
    QVERIFY(ms >= 190 && ms < 1000);
}

QTEST_GUILESS_MAIN(TstSpike)
#include "tst_spike.moc"
