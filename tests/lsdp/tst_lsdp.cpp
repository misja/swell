// SPDX-License-Identifier: MIT
// LSDP packet tests on synthetic packets; node ids are locally administered
// MACs (02:...) and addresses are from TEST-NET-1 (192.0.2.0/24).
#include <QtTest>

#include "lsdp.h"

using namespace BluOS;

namespace {

QByteArray header()
{
    return QByteArray("\x06LSDP\x01", 6);
}

QByteArray withLength(const QByteArray &body)
{
    return char(body.size() + 1) + body;
}

QByteArray lengthPrefixed(const QByteArray &bytes)
{
    return char(bytes.size()) + bytes;
}

QByteArray record(quint16 classId, const QList<QPair<QByteArray, QByteArray>> &txt)
{
    QByteArray r;
    r.append(char(classId >> 8)).append(char(classId & 0xFF)).append(char(txt.size()));
    for (const auto &kv : txt)
        r.append(lengthPrefixed(kv.first)).append(lengthPrefixed(kv.second));
    return r;
}

QByteArray announce(const QByteArray &nodeId, const QByteArray &ipv4, const QList<QByteArray> &records)
{
    QByteArray body("A");
    body.append(lengthPrefixed(nodeId)).append(lengthPrefixed(ipv4)).append(char(records.size()));
    for (const QByteArray &r : records)
        body.append(r);
    return withLength(body);
}

const QByteArray NodeA("\x02\x00\x00\x00\x00\x0A", 6);
const QByteArray NodeB("\x02\x00\x00\x00\x00\x0B", 6);
const QByteArray Ip10("\xC0\x00\x02\x0A", 4);   // 192.0.2.10
const QByteArray Ip11("\xC0\x00\x02\x0B", 4);   // 192.0.2.11

QByteArray playerRecord()
{
    return record(Lsdp::ClassPlayer, { { "name", "Test Room" }, { "port", "11000" }, { "model", "T1" } });
}

} // namespace

class TstLsdp : public QObject
{
    Q_OBJECT

private slots:
    void announceWithTxt();
    void playerClasses();
    void multipleAnnouncesInOnePacket();
    void addressChangeKeepsNodeId();
    void deleteMessage();
    void ownQueryIsRecognised();
    void queryLayout();
    void unknownTypeIsSkipped();
    void truncatedMessage();
    void fieldOverrunsMessage();
    void rejectsBadHeader_data();
    void rejectsBadHeader();
    void nodeIdFormatting();
};

void TstLsdp::announceWithTxt()
{
    const Lsdp::Packet p = Lsdp::parse(header() + announce(NodeA, Ip10, { playerRecord() }));
    QVERIFY(p.valid);
    QVERIFY(!p.truncated);
    QCOMPARE(p.announces.size(), 1);
    const Lsdp::Announce &a = p.announces.first();
    QCOMPARE(a.nodeId, NodeA);
    QCOMPARE(a.address, QHostAddress(QStringLiteral("192.0.2.10")));
    QCOMPARE(a.records.size(), 1);
    QCOMPARE(a.records.first().classId, Lsdp::ClassPlayer);
    QCOMPARE(a.records.first().txt.value(QStringLiteral("name")), QStringLiteral("Test Room"));
    QCOMPARE(a.records.first().txt.value(QStringLiteral("port")), QStringLiteral("11000"));
}

void TstLsdp::playerClasses()
{
    for (quint16 c : { quint16(0x0001), quint16(0x0003), quint16(0x0006), quint16(0x0008) })
        QVERIFY2(Lsdp::isPlayerClass(c), qPrintable(QString::number(c, 16)));
    for (quint16 c : { quint16(0x0002), quint16(0x0004), quint16(0x0005), quint16(0x0007), quint16(0xFFFF) })
        QVERIFY2(!Lsdp::isPlayerClass(c), qPrintable(QString::number(c, 16)));

    // A node that only advertises a non-player class still parses; filtering is the caller's.
    const Lsdp::Packet p = Lsdp::parse(header() + announce(NodeA, Ip10, { record(0x0004, {}) }));
    QCOMPARE(p.announces.size(), 1);
    QVERIFY(!Lsdp::isPlayerClass(p.announces.first().records.first().classId));
}

void TstLsdp::multipleAnnouncesInOnePacket()
{
    // A multi-zone player splits its nodes over several announce messages (API Appendix, note 2).
    const Lsdp::Packet p = Lsdp::parse(header() + announce(NodeA, Ip10, { playerRecord() })
                                       + announce(NodeB, Ip11, { record(Lsdp::ClassPlayerSecondary, {}) }));
    QCOMPARE(p.announces.size(), 2);
    QCOMPARE(p.announces.at(0).nodeId, NodeA);
    QCOMPARE(p.announces.at(1).nodeId, NodeB);
    QCOMPARE(p.announces.at(1).records.first().classId, Lsdp::ClassPlayerSecondary);
}

void TstLsdp::addressChangeKeepsNodeId()
{
    const Lsdp::Packet first = Lsdp::parse(header() + announce(NodeA, Ip10, { playerRecord() }));
    const Lsdp::Packet second = Lsdp::parse(header() + announce(NodeA, Ip11, { playerRecord() }));
    QCOMPARE(first.announces.first().nodeId, second.announces.first().nodeId);
    QVERIFY(first.announces.first().address != second.announces.first().address);
}

void TstLsdp::deleteMessage()
{
    QByteArray body("D");
    body.append(lengthPrefixed(NodeA)).append(char(1)).append('\x00').append('\x01');
    const Lsdp::Packet p = Lsdp::parse(header() + withLength(body));
    QCOMPARE(p.deletes.size(), 1);
    QCOMPARE(p.deletes.first().nodeId, NodeA);
    QCOMPARE(p.deletes.first().classes, QVector<quint16>({ Lsdp::ClassPlayer }));
}

void TstLsdp::ownQueryIsRecognised()
{
    const Lsdp::Packet p = Lsdp::parse(Lsdp::buildQuery({ Lsdp::AllClasses }));
    QVERIFY(p.valid);
    QCOMPARE(p.queries, 1);
    QVERIFY(p.announces.isEmpty());
    QCOMPARE(p.skipped, 0);
}

void TstLsdp::queryLayout()
{
    // Header, then one 'Q' message with one class: FFFF (all classes).
    QCOMPARE(Lsdp::buildQuery({ Lsdp::AllClasses }),
             QByteArray("\x06LSDP\x01\x05Q\x01\xFF\xFF", 11));
}

void TstLsdp::unknownTypeIsSkipped()
{
    const Lsdp::Packet p = Lsdp::parse(header() + withLength(QByteArray("Zfuture", 7))
                                       + announce(NodeA, Ip10, { playerRecord() }));
    QCOMPARE(p.skipped, 1);
    QCOMPARE(p.announces.size(), 1);
}

void TstLsdp::truncatedMessage()
{
    QByteArray packet = header() + announce(NodeA, Ip10, { playerRecord() });
    packet.chop(3);
    const Lsdp::Packet p = Lsdp::parse(packet);
    QVERIFY(p.valid);
    QVERIFY(p.truncated);
    QVERIFY(p.announces.isEmpty());
}

void TstLsdp::fieldOverrunsMessage()
{
    // The message length is consistent with the packet, but the node id length
    // claims more bytes than the message holds.
    QByteArray body("A");
    body.append(char(40)).append(NodeA);
    const Lsdp::Packet p = Lsdp::parse(header() + withLength(body) + announce(NodeB, Ip11, { playerRecord() }));
    QCOMPARE(p.skipped, 1);
    QCOMPARE(p.announces.size(), 1);
    QCOMPARE(p.announces.first().nodeId, NodeB);
}

void TstLsdp::rejectsBadHeader_data()
{
    QTest::addColumn<QByteArray>("packet");
    QTest::newRow("empty") << QByteArray();
    QTest::newRow("short") << QByteArray("\x06LSD", 4);
    QTest::newRow("magic") << QByteArray("\x06LSDQ\x01", 6);
    QTest::newRow("version") << QByteArray("\x06LSDP\x02", 6);
    QTest::newRow("header length") << QByteArray("\x09LSDP\x01", 6);
}

void TstLsdp::rejectsBadHeader()
{
    QFETCH(QByteArray, packet);
    const Lsdp::Packet p = Lsdp::parse(packet);
    QVERIFY(!p.valid);
    QVERIFY(p.announces.isEmpty());
}

void TstLsdp::nodeIdFormatting()
{
    QCOMPARE(Lsdp::nodeIdToString(NodeA), QStringLiteral("02:00:00:00:00:0A"));
}

QTEST_GUILESS_MAIN(TstLsdp)
#include "tst_lsdp.moc"
