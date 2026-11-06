// SPDX-License-Identifier: MIT
// Control API document tests. The documents are written for this project in
// the shape the API v1.7 describes; names, addresses and ids are invented
// (TEST-NET-1 addresses, locally administered MACs).
#include <QtTest>

#include "xml.h"

using namespace bluos;

class TstXml : public QObject
{
    Q_OBJECT

private slots:
    void status();
    void statusStreamAndFixedVolume();
    void statusIgnoresUnknownElements();
    void statusMalformed_data();
    void statusMalformed();
    void syncStatusPrimary();
    void syncStatusSecondary();
    void syncStatusWrongRoot();
    void volume();
};

void TstXml::status()
{
    const Status s = parseStatus(
        "<status etag=\"e1\">"
        "<album>Invented Album</album><artist>Nobody</artist><name>Track A</name>"
        "<title1>Track A</title1><title2>Nobody</title2><title3>Invented Album</title3>"
        "<twoline_title1>Track A</twoline_title1><twoline_title2>Nobody - Invented Album</twoline_title2>"
        "<image>/Artwork?service=LocalMusic&amp;album=Invented</image>"
        "<state>play</state><secs>42</secs><totlen>180</totlen>"
        "<volume>23</volume><mute>0</mute><canSeek>1</canSeek><syncStat>s7</syncStat>"
        "</status>");
    QVERIFY2(s.ok, qPrintable(s.error));
    QCOMPARE(s.etag, QStringLiteral("e1"));
    QCOMPARE(s.title1, QStringLiteral("Track A"));
    QCOMPARE(s.title2, QStringLiteral("Nobody"));
    QCOMPARE(s.title3, QStringLiteral("Invented Album"));
    QCOMPARE(s.twolineTitle2, QStringLiteral("Nobody - Invented Album"));
    QCOMPARE(s.image, QStringLiteral("/Artwork?service=LocalMusic&album=Invented"));
    QVERIFY(s.isPlaying());
    QCOMPARE(s.secs, 42);
    QCOMPARE(s.totlen, 180);
    QCOMPARE(s.volume, 23);
    QVERIFY(!s.mute);
    QVERIFY(s.canSeek);
    QVERIFY(!s.hasStreamUrl);
    QCOMPARE(s.syncStat, QStringLiteral("s7"));
}

void TstXml::statusStreamAndFixedVolume()
{
    const Status s = parseStatus(
        "<status etag=\"e2\"><state>stream</state><streamUrl>opaque:1</streamUrl>"
        "<volume>-1</volume><mute>1</mute></status>");
    QVERIFY(s.ok);
    QVERIFY(s.isPlaying());
    QVERIFY(s.hasStreamUrl);
    QCOMPARE(s.volume, -1);
    QVERIFY(s.mute);
    QCOMPARE(s.secs, -1);
}

void TstXml::statusIgnoresUnknownElements()
{
    const Status s = parseStatus(
        "<status etag=\"e3\" future=\"x\"><futureThing a=\"1\"><nested>y</nested></futureThing>"
        "<battery level=\"80\" charging=\"1\"/><title1>Still here</title1></status>");
    QVERIFY2(s.ok, qPrintable(s.error));
    QCOMPARE(s.title1, QStringLiteral("Still here"));
}

void TstXml::statusMalformed_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::newRow("empty") << QByteArray();
    QTest::newRow("html") << QByteArray("<html><body>Not a player</body></html>");
    QTest::newRow("truncated") << QByteArray("<status etag=\"e4\"><title1>Cut");
    QTest::newRow("mismatched") << QByteArray("<status><title1>x</title2></status>");
    QTest::newRow("garbage") << QByteArray("\x00\x01\x02 not xml", 12);
}

void TstXml::statusMalformed()
{
    QFETCH(QByteArray, xml);
    const Status s = parseStatus(xml);
    QVERIFY(!s.ok);
    QVERIFY(!s.error.isEmpty());
}

void TstXml::syncStatusPrimary()
{
    const SyncStatus s = parseSyncStatus(
        "<SyncStatus icon=\"/images/players/x.png\" volume=\"30\" modelName=\"Test Model\" "
        "name=\"Living Room\" model=\"T1\" brand=\"Testbrand\" etag=\"9\" schemaVersion=\"35\" "
        "initialized=\"true\" group=\"Living Room + 1\" syncStat=\"9\" id=\"192.0.2.10:11000\" "
        "mac=\"02:00:00:00:00:0A\">"
        "<slave port=\"11000\" id=\"192.0.2.11\"/><unknown/></SyncStatus>");
    QVERIFY2(s.ok, qPrintable(s.error));
    QCOMPARE(s.id, QStringLiteral("192.0.2.10:11000"));
    QCOMPARE(s.name, QStringLiteral("Living Room"));
    QCOMPARE(s.modelName, QStringLiteral("Test Model"));
    QCOMPARE(s.volume, 30);
    QCOMPARE(s.syncStat, QStringLiteral("9"));
    QVERIFY(s.initialized);
    QVERIFY(!s.isSecondary());
    QCOMPARE(s.slaves, QStringList({ QStringLiteral("192.0.2.11:11000") }));
}

void TstXml::syncStatusSecondary()
{
    const SyncStatus s = parseSyncStatus(
        "<SyncStatus volume=\"12\" mute=\"1\" name=\"Kitchen\" etag=\"4\" syncStat=\"4\" "
        "id=\"192.0.2.11:11000\" initialized=\"false\">"
        "<master port=\"11010\" reconnecting=\"true\">192.0.2.10</master></SyncStatus>");
    QVERIFY2(s.ok, qPrintable(s.error));
    QVERIFY(s.isSecondary());
    QCOMPARE(s.masterHost, QStringLiteral("192.0.2.10"));
    QCOMPARE(s.masterPort, 11010);
    QVERIFY(s.masterReconnecting);
    QVERIFY(s.mute);
    QCOMPARE(s.volume, 12);
    QVERIFY(!s.initialized);
}

void TstXml::syncStatusWrongRoot()
{
    const SyncStatus s = parseSyncStatus("<status etag=\"1\"/>");
    QVERIFY(!s.ok);
    QVERIFY(s.error.contains(QStringLiteral("status")));
}

void TstXml::volume()
{
    const Volume v = parseVolume("<volume db=\"-43.1\" mute=\"0\" offsetDb=\"0\" etag=\"v1\">11</volume>");
    QVERIFY2(v.ok, qPrintable(v.error));
    QCOMPARE(v.level, 11);
    QCOMPARE(v.db, -43.1);
    QVERIFY(!v.mute);
    QCOMPARE(v.etag, QStringLiteral("v1"));
}

QTEST_GUILESS_MAIN(TstXml)
#include "tst_xml.moc"
