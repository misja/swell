// SPDX-License-Identifier: MIT
// PlayerModel with LSDP packets injected into its Discovery. Announced
// addresses are on the loopback, with ports where nothing listens, so the
// clients the model starts fail fast and never leave the machine.
#include <QtTest>

#include "playermodel.h"

using namespace BluOS;

namespace {

QByteArray lengthPrefixed(const QByteArray &bytes)
{
    return char(bytes.size()) + bytes;
}

QByteArray packet(const QByteArray &message)
{
    return QByteArray("\x06LSDP\x01", 6) + char(message.size() + 1) + message;
}

QByteArray announce(char node, char lastOctet, quint16 classId, const QByteArray &port,
                    const QByteArray &name)
{
    QByteArray m("A");
    m.append(lengthPrefixed(QByteArray("\x02\x00\x00\x00\x00", 5) + node));
    m.append(lengthPrefixed(QByteArray("\x7F\x00\x00", 3) + lastOctet));  // 127.0.0.x
    m.append(char(1));
    m.append(char(classId >> 8)).append(char(classId & 0xFF)).append(char(2));
    m.append(lengthPrefixed("port")).append(lengthPrefixed(port));
    m.append(lengthPrefixed("name")).append(lengthPrefixed(name));
    return packet(m);
}

QByteArray deletion(char node)
{
    QByteArray m("D");
    m.append(lengthPrefixed(QByteArray("\x02\x00\x00\x00\x00", 5) + node));
    m.append(char(1)).append('\x00').append('\x01');
    return packet(m);
}

QString idAt(const PlayerModel &model, int row)
{
    return model.data(model.index(row), PlayerModel::IdRole).toString();
}

} // namespace

class TstModel : public QObject
{
    Q_OBJECT

private slots:
    void parseAddress_data();
    void parseAddress();
    void announcedPlayerAppears();
    void nonPlayerClassIgnored();
    void duplicateAnnouncementKeepsOneRow();
    void addressChangeUpdatesRow();
    void multiZoneNodeGivesTwoPlayers();
    void deletionRemovesDiscovered();
    void manualMergesWithAnnouncement();
    void manualSurvivesDeletion();
    void manualHostsList();
    void pollingPolicy();
    void restoredPlayerAppearsAtOnce();
    void currentFollowsAddressChange();
    void isValidAddressMatchesParse();
    void removingCurrentClearsIt();
    void deletionKeepsCurrent();
};

void TstModel::parseAddress_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("ok");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::newRow("ip") << "192.0.2.10" << true << "192.0.2.10" << 11000;
    QTest::newRow("ip:port") << "192.0.2.10:11010" << true << "192.0.2.10" << 11010;
    QTest::newRow("name") << " player.local " << true << "player.local" << 11000;
    QTest::newRow("empty") << "" << false << "" << 0;
    QTest::newRow("path") << "192.0.2.10/Status" << false << "" << 0;
    QTest::newRow("port 0") << "192.0.2.10:0" << false << "" << 0;
    QTest::newRow("bad port") << "192.0.2.10:99999" << false << "" << 0;
    QTest::newRow("user") << "me@192.0.2.10" << false << "" << 0;
    QTest::newRow("space") << "192.0.2 .10" << false << "" << 0;
}

void TstModel::parseAddress()
{
    QFETCH(QString, input);
    QFETCH(bool, ok);
    QFETCH(QString, host);
    QFETCH(int, port);
    QString gotHost;
    int gotPort = 0;
    QCOMPARE(PlayerModel::parseAddress(input, &gotHost, &gotPort), ok);
    if (ok) {
        QCOMPARE(gotHost, host);
        QCOMPARE(gotPort, port);
    }
}

void TstModel::announcedPlayerAppears()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(idAt(model, 0), QStringLiteral("127.0.0.10:1"));
    QCOMPARE(model.data(model.index(0), PlayerModel::NameRole).toString(), QStringLiteral("Den"));
    QCOMPARE(model.idAt(0), QStringLiteral("127.0.0.10:1"));
    QCOMPARE(model.idAt(1), QString());
    QVERIFY(!model.data(model.index(0), PlayerModel::ManualRole).toBool());
}

void TstModel::nonPlayerClassIgnored()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0002, "1", "Server"));
    model.discovery()->handleDatagram(announce('\x0B', 11, 0x0007, "1", "Web UI"));
    QCOMPARE(model.rowCount(), 0);
}

void TstModel::duplicateAnnouncementKeepsOneRow()
{
    PlayerModel model;
    for (int i = 0; i < 3; ++i)
        model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    QCOMPARE(model.rowCount(), 1);
}

void TstModel::addressChangeUpdatesRow()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    model.discovery()->handleDatagram(announce('\x0A', 20, 0x0001, "1", "Den"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(idAt(model, 0), QStringLiteral("127.0.0.20:1"));
}

void TstModel::multiZoneNodeGivesTwoPlayers()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Zone A"));
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0003, "2", "Zone B"));
    QCOMPARE(model.rowCount(), 2);
}

void TstModel::deletionRemovesDiscovered()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    model.discovery()->handleDatagram(announce('\x0B', 11, 0x0001, "1", "Hall"));
    model.discovery()->handleDatagram(deletion('\x0A'));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(idAt(model, 0), QStringLiteral("127.0.0.11:1"));
}

void TstModel::manualMergesWithAnnouncement()
{
    PlayerModel model;
    QVERIFY(model.addManual(QStringLiteral("127.0.0.10:1")));
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.data(model.index(0), PlayerModel::ManualRole).toBool());
    QCOMPARE(model.data(model.index(0), PlayerModel::NameRole).toString(), QStringLiteral("Den"));
    QVERIFY(model.addManual(QStringLiteral("127.0.0.10:1")));
    QCOMPARE(model.rowCount(), 1);
}

void TstModel::manualSurvivesDeletion()
{
    PlayerModel model;
    QVERIFY(model.addManual(QStringLiteral("127.0.0.10:1")));
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    model.discovery()->handleDatagram(deletion('\x0A'));
    QCOMPARE(model.rowCount(), 1);
    model.removeManual(QStringLiteral("127.0.0.10:1"));
    QCOMPARE(model.rowCount(), 0);
}

void TstModel::manualHostsList()
{
    PlayerModel model;
    QSignalSpy changed(&model, &PlayerModel::manualHostsChanged);
    QVERIFY(model.addManual(QStringLiteral("127.0.0.10:1")));
    QVERIFY(model.addManual(QStringLiteral("127.0.0.11")));
    QVERIFY(!model.addManual(QStringLiteral("not an/address")));
    QCOMPARE(model.manualHosts(),
             QStringList({ QStringLiteral("127.0.0.10:1"), QStringLiteral("127.0.0.11:11000") }));
    QCOMPARE(changed.size(), 2);
}

void TstModel::pollingPolicy()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    model.discovery()->handleDatagram(announce('\x0B', 11, 0x0001, "1", "Hall"));
    auto running = [&model](const QString &id) {
        return qobject_cast<Client *>(model.client(id))->running();
    };
    QVERIFY(!running(QStringLiteral("127.0.0.10:1")));
    QVERIFY(!running(QStringLiteral("127.0.0.11:1")));

    model.setCurrentId(QStringLiteral("127.0.0.10:1"));
    QVERIFY(running(QStringLiteral("127.0.0.10:1")));
    QVERIFY(!running(QStringLiteral("127.0.0.11:1")));

    model.setPollAll(true);
    QVERIFY(running(QStringLiteral("127.0.0.11:1")));
    // A player found while all are polled starts too.
    model.discovery()->handleDatagram(announce('\x0C', 12, 0x0001, "1", "Attic"));
    QVERIFY(running(QStringLiteral("127.0.0.12:1")));

    model.setPollAll(false);
    QVERIFY(running(QStringLiteral("127.0.0.10:1")));
    QVERIFY(!running(QStringLiteral("127.0.0.11:1")));
    QVERIFY(!running(QStringLiteral("127.0.0.12:1")));
}

void TstModel::restoredPlayerAppearsAtOnce()
{
    PlayerModel model;
    QSignalSpy current(&model, &PlayerModel::currentChanged);
    model.restoreCurrent(QStringLiteral("127.0.0.10:1"), QStringLiteral("02:00:00:00:00:0A"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.currentId(), QStringLiteral("127.0.0.10:1"));
    QVERIFY(model.current());
    QCOMPARE(model.current()->availability(), Client::Connecting);
    QVERIFY(current.size() >= 1);
    // Its announcement merges into the same row.
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    QCOMPARE(model.rowCount(), 1);
}

void TstModel::currentFollowsAddressChange()
{
    PlayerModel model;
    model.restoreCurrent(QStringLiteral("127.0.0.10:1"), QStringLiteral("02:00:00:00:00:0A"));
    Client *before = model.current();
    QSignalSpy current(&model, &PlayerModel::currentChanged);
    model.discovery()->handleDatagram(announce('\x0A', 20, 0x0001, "1", "Den"));
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.currentId(), QStringLiteral("127.0.0.20:1"));
    QVERIFY(model.current() != before);
    QVERIFY(model.current()->running());
    QCOMPARE(current.size(), 1);
    QCOMPARE(model.currentNodeId(), QStringLiteral("02:00:00:00:00:0A"));
}

void TstModel::isValidAddressMatchesParse()
{
    PlayerModel model;
    QVERIFY(model.isValidAddress(QStringLiteral("192.0.2.10:11010")));
    QVERIFY(!model.isValidAddress(QStringLiteral("not an/address")));
    QVERIFY(!model.isValidAddress(QString()));
}

void TstModel::removingCurrentClearsIt()
{
    PlayerModel model;
    QVERIFY(model.addManual(QStringLiteral("127.0.0.10:1")));
    model.setCurrentId(QStringLiteral("127.0.0.10:1"));
    QSignalSpy current(&model, &PlayerModel::currentChanged);
    model.removeManual(QStringLiteral("127.0.0.10:1"));
    QCOMPARE(model.currentId(), QString());
    QCOMPARE(model.current(), static_cast<Client *>(nullptr));
    QCOMPARE(current.size(), 1);
}

void TstModel::deletionKeepsCurrent()
{
    PlayerModel model;
    model.discovery()->handleDatagram(announce('\x0A', 10, 0x0001, "1", "Den"));
    model.setCurrentId(QStringLiteral("127.0.0.10:1"));
    model.discovery()->handleDatagram(deletion('\x0A'));
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.current());
}

QTEST_GUILESS_MAIN(TstModel)
#include "tst_model.moc"
