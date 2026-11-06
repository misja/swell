// SPDX-License-Identifier: MIT
// Control API document tests. The documents are written for this project in
// the shape the API v1.7 describes; names, addresses and ids are invented
// (TEST-NET-1 addresses, locally administered MACs).
#include <QtTest>

#include "xml.h"

using namespace BluOS;

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
    void statusPrid();
    void statusStreamActions();
    void statusQueuePosition();
    void playlistWindow();
    void playlistStatusForm();
    void playlistMalformed();
    void presets();
    void presetsEmptyAndMalformed();
    void captureInputs();
    void captureInputsMalformed();
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


void TstXml::statusQueuePosition()
{
    const Status s = parseStatus("<status etag=\"q\"><pid>692</pid><song>3</song></status>");
    QVERIFY(s.ok);
    QCOMPARE(s.pid, QStringLiteral("692"));
    QCOMPARE(s.song, 3);
}

void TstXml::playlistWindow()
{
    const Playlist p = parsePlaylist(
        "<playlist name=\"Mix\" modified=\"0\" length=\"160\" id=\"1054\">"
        "<song service=\"Svc\" songid=\"Svc:1\" id=\"5\"><title>Ebb</title><art>Nobody</art>"
        "<alb>Invented</alb><fn>Svc:1</fn><time>221</time><image>/Artwork?x=1</image><future/></song>"
        "<song id=\"6\"><title>Slack Water</title><art>Nobody</art><alb>Invented</alb><fn>/f/2.flac</fn></song>"
        "<song><title>No position</title></song></playlist>");
    QVERIFY2(p.ok, qPrintable(p.error));
    QCOMPARE(p.length, 160);
    QCOMPARE(p.id, QStringLiteral("1054"));
    QCOMPARE(p.songs.size(), 2);
    QCOMPARE(p.songs.at(0).position, 5);
    QCOMPARE(p.songs.at(0).duration, 221);
    QCOMPARE(p.songs.at(0).image, QStringLiteral("/Artwork?x=1"));
    QCOMPARE(p.songs.at(0).identity(), QStringLiteral("Svc:1|Ebb"));
    QCOMPARE(p.songs.at(1).duration, -1);
    QCOMPARE(p.songs.at(1).identity(), QStringLiteral("/f/2.flac|Slack Water"));
}

void TstXml::playlistStatusForm()
{
    const Playlist p = parsePlaylist("<playlist><length>13</length><id>243</id><name></name><modified>1</modified></playlist>");
    QVERIFY(p.ok);
    QCOMPARE(p.length, 13);
    QCOMPARE(p.id, QStringLiteral("243"));
    QVERIFY(p.songs.isEmpty());
    const Playlist cleared = parsePlaylist("<playlist modified=\"0\" length=\"0\" id=\"1056\"/>");
    QVERIFY(cleared.ok);
    QCOMPARE(cleared.length, 0);
    QCOMPARE(cleared.id, QStringLiteral("1056"));
}

void TstXml::playlistMalformed()
{
    QVERIFY(!parsePlaylist("").ok);
    QVERIFY(!parsePlaylist("<playlist length=\"3\"><song id=\"0\"><title>x").ok);
    QVERIFY(!parsePlaylist("<status/>").ok);
}

void TstXml::statusStreamActions()
{
    const Status s = parseStatus(
        "<status etag=\"e6\"><streamUrl>Svc:station</streamUrl><actions>"
        "<action name=\"back\"/>"
        "<action name=\"skip\" url=\"/Action?service=Svc&amp;skip=42\"/>"
        "<action name=\"love\" url=\"/Action?service=Svc&amp;love=42\" text=\"Love\"/>"
        "</actions><title1>After</title1></status>");
    QVERIFY2(s.ok, qPrintable(s.error));
    QVERIFY(s.hasStreamUrl);
    QCOMPARE(s.skipActionUrl, QStringLiteral("/Action?service=Svc&skip=42"));
    QVERIFY(s.backActionUrl.isEmpty());
    QCOMPARE(s.title1, QStringLiteral("After"));
}

void TstXml::statusPrid()
{
    const Status s = parseStatus("<status etag=\"e5\"><prid>4</prid></status>");
    QVERIFY(s.ok);
    QCOMPARE(s.prid, QStringLiteral("4"));
}

void TstXml::presets()
{
    const Presets p = parsePresets(
        "<presets prid=\"3\">"
        "<preset id=\"1\" name=\"CD Player\" url=\"Capture:hw:x,0/1/25/2?id=xdynamic-Source3\" image=\"/images/capture/o.png\"/>"
        "<preset id=\"7\" name=\"North Sea FM\" url=\"Radio:/1/north\" image=\"https://img.example/n.jpg\"/>"
        "<preset id=\"2\" name=\"Morning Mix\" url=\"/Load?service=X&amp;id=1\" image=\"/Artwork?service=X&amp;id=1\"/>"
        "<preset name=\"No id\"/><future/></presets>");
    QVERIFY2(p.ok, qPrintable(p.error));
    QCOMPARE(p.prid, QStringLiteral("3"));
    QCOMPARE(p.items.size(), 3);
    QCOMPARE(p.items.at(0).inputId(), QStringLiteral("xdynamic-Source3"));
    QCOMPARE(p.items.at(1).id, 7);
    QCOMPARE(p.items.at(1).name, QStringLiteral("North Sea FM"));
    QCOMPARE(p.items.at(1).inputId(), QString());
    QCOMPARE(p.items.at(2).id, 2);
    QCOMPARE(p.items.at(2).image, QStringLiteral("/Artwork?service=X&id=1"));
    QCOMPARE(p.items.at(2).inputId(), QString());
}

void TstXml::presetsEmptyAndMalformed()
{
    const Presets empty = parsePresets("<presets prid=\"1\"/>");
    QVERIFY(empty.ok);
    QVERIFY(empty.items.isEmpty());
    QVERIFY(!parsePresets("<presets><preset id=\"1\"").ok);
    QVERIFY(!parsePresets("<status/>").ok);
}

void TstXml::captureInputs()
{
    const Inputs in = parseCaptureInputs(
        "<radiotime service=\"Capture\">"
        "<item playerName=\"Den\" text=\"Bluetooth\" inputType=\"bluetooth\" id=\"input2\""
        " URL=\"Capture%3Abluez%3Abluetooth\" image=\"/images/BluetoothIcon.png\" type=\"audio\"/>"
        "<item playerName=\"Den\" text=\"Optical Input\" inputType=\"spdif\" id=\"input1\""
        " URL=\"Capture%3Ahw%3Aimxspdif%2C0%2F1%2F25%2F2%3Fid%3Dinput1\" image=\"/images/o.png\" type=\"audio\"/>"
        "<item playerName=\"Den\" text=\"Cloud Thing\" id=\"Cloud\" URL=\"Cloud%3Aplay\""
        " serviceType=\"CloudService\" type=\"audio\"/>"
        "<category text=\"Hub\"><remoteitem text=\"Analog\" inputType=\"analog\" URL=\"Hub%3A%2F%2Fx\"/></category>"
        "</radiotime>");
    QVERIFY2(in.ok, qPrintable(in.error));
    QCOMPARE(in.items.size(), 2);
    QCOMPARE(in.items.at(0).text, QStringLiteral("Bluetooth"));
    QCOMPARE(in.items.at(0).inputType, QStringLiteral("bluetooth"));
    QCOMPARE(in.items.at(1).url, QStringLiteral("Capture%3Ahw%3Aimxspdif%2C0%2F1%2F25%2F2%3Fid%3Dinput1"));
}

void TstXml::captureInputsMalformed()
{
    QVERIFY(!parseCaptureInputs("").ok);
    QVERIFY(!parseCaptureInputs("<radiotime><item").ok);
    QVERIFY(!parseCaptureInputs("<presets/>").ok);
    const Inputs none = parseCaptureInputs("<radiotime service=\"Capture\"/>");
    QVERIFY(none.ok);
    QVERIFY(none.items.isEmpty());
}

QTEST_GUILESS_MAIN(TstXml)
#include "tst_xml.moc"
