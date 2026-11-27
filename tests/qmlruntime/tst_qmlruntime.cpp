// SPDX-License-Identifier: MIT
// The values QML gets from the player and browse models, read with the
// expressions the pages use (Probe.qml), against the in-process FakePlayer.
// A key or list that does not reach JavaScript fails here instead of showing
// as a blank tile (spec 007, scope item 4). The MPRIS mapping is checked the
// same way, without a session bus (spec 003, AC5).
#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>

#include "browsemodel.h"
#include "client.h"
#include "fakeplayer.h"
#include "playermodel.h"
#include "qmlplayer.h"
#include "registration.h"

using namespace BluOS;

class TstQmlRuntime : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void presets();
    void inputs();
    void queue();
    void groupValues();
    void shuffleAndRepeat();
    void reorderViewAndModelMove();
    void reorderDropAndPutBack();
    void reorderCancelAndReload();
    void rowActions();
    void parentActionsArePassedOn();
    void favouritesAndUnreadRows();
    void letters();
    void moreLinks();
    void presetsFollowThePlayer();
    void mprisFollowsTheStatus();
    void mprisMetadataAndPosition();
    void mprisRequestsReachThePlayer();
    void mprisOffline();
    void mprisWithoutAPlayer();
    void firmwareValues();
    void upgradeStartFromQml();
    void upgradeEndState();

private:
    QString call(const char *function, const QVariant &arg = QVariant());
    QString image(const QString &path) const;

    FakePlayer *m_player = nullptr;
    PlayerModel *m_players = nullptr;
    QQmlEngine *m_engine = nullptr;
    QObject *m_probe = nullptr;
};

static QByteArray album(int i, const QByteArray &extra = QByteArray())
{
    const QByteArray n = QByteArray::number(i);
    return "<item browseKey=\"Lib:Album/" + n + "\" text=\"Album " + n + "\" text2=\"Nobody\" type=\"album\">"
           "<contextMenu>" + extra + "<item actionURL=\"/Add?album=" + n
           + "&amp;where=last\" text=\"Add last\" type=\"addAll-last\"/>"
             "<item actionURL=\"/Delete?x=1\" text=\"Delete\" type=\"delete\"/></contextMenu></item>";
}

static QByteArray section(const QByteArray &letter, const QByteArray &count)
{
    return "<item browseKey=\"Lib:S:" + letter + "\" text=\"" + letter + "\" text2=\"" + count
            + "\" type=\"section\"/>";
}

void TstQmlRuntime::initTestCase()
{
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_player->rememberPort();
    m_player->serve(QStringLiteral("/SyncStatus"),
                    "<SyncStatus etag=\"s\" syncStat=\"s\" name=\"Den\" volume=\"20\" version=\"4.18.18\"/>");
    m_player->serve(QStringLiteral("/upgrade"),
                    "<upgrade inProgress=\"false\" version=\"4.18.20\" available=\"true\">check</upgrade>");
    m_player->setStatus("<status etag=\"e1\"><state>stop</state><prid>1</prid></status>");
    m_player->serve(QStringLiteral("/Presets"),
                    "<presets prid=\"1\">"
                    "<preset id=\"5\" name=\"North Sea FM\" url=\"TuneIn:s1\" image=\"/Artwork?id=5\"/>"
                    "<preset id=\"7\" name=\"Turntable\" url=\"Capture:hw:1?id=input1\" image=\"/images/in.png\"/>"
                    "</presets>");
    m_player->serve(QStringLiteral("/Repeat"), "<playlist length=\"0\" id=\"1\" shuffle=\"0\" repeat=\"0\"/>");
    m_player->serve(QStringLiteral("/RadioBrowse"),
                    "<radiotime service=\"Capture\">"
                    "<item text=\"Optical Input\" inputType=\"spdif\" id=\"input1\" URL=\"Capture%3Ahw%3A1\" image=\"/images/o.png\"/>"
                    "</radiotime>");
    m_player->handle(QStringLiteral("/Browse"), [](const QUrlQuery &q) -> QByteArray {
        const QString key = q.queryItemValue(QStringLiteral("key"), QUrl::FullyDecoded);
        if (key.isEmpty())
            return "<browse type=\"menu\">"
                   "<item browseKey=\"LocalMusic:\" text=\"Library\" type=\"link\"/>"
                   "<item browseKey=\"Airable:\" text=\"Radio\" type=\"link\"/></browse>";
        if (key == QLatin1String("LocalMusic:"))
            return "<browse serviceName=\"Library\" type=\"menu\">"
                   "<item browseKey=\"LocalMusic:MG/LocalMusic-Favourites\" text=\"Favoriten\" type=\"link\"/></browse>";
        if (key == QLatin1String("List"))
            return "<browse type=\"albums\">"
                   + album(0, "<item actionURL=\"/Add?album=0&amp;shuffle=1\" text=\"Shuffle\" type=\"add-shuffle\"/>")
                   + album(1) + "</browse>";
        if (key == QLatin1String("Index"))
            return "<browse type=\"sections\">" + section("#", "1") + section("A", "2") + section("B", "3")
                    + "</browse>";
        if (key.startsWith(QLatin1String("Lib:S:")))
            return "<browse type=\"albums\">" + album(0) + album(1) + album(2) + "</browse>";
        if (key == QLatin1String("Artist:X"))
            return "<browse serviceName=\"Library\" type=\"menu\">"
                   "<item browseKey=\"Lib:Albums?artist=X\" text=\"Albums\" type=\"link\"/>"
                   "<item browseKey=\"Lib:Songs?artist=X\" text=\"Songs\" type=\"link\"/></browse>";
        if (key == QLatin1String("Lib:Albums?artist=X"))
            return "<browse type=\"albums\">" + album(0) + "</browse>";
        return "<browse type=\"menu\"/>";
    });

    Swell::registerTypes();
    m_players = new Swell::QmlPlayerModel(this);
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.readTimeoutMs = 3000;
    t.upgradeTimeoutMs = 3000;
    t.minIntervalMs = 200;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    m_players->setClientTiming(t);
    const QString id = QStringLiteral("127.0.0.1:%1").arg(m_player->port());
    m_players->restoreCurrent(id, QString());
    QCOMPARE(m_players->currentId(), id);

    m_engine = new QQmlEngine(this);
    m_engine->rootContext()->setContextProperty(QStringLiteral("players"), m_players);
    QVariantMap roles;
    roles.insert(QStringLiteral("actions"), int(BrowseModel::ActionsRole));
    roles.insert(QStringLiteral("favourites"), int(BrowseModel::FavouritesRole));
    roles.insert(QStringLiteral("loaded"), int(BrowseModel::LoadedRole));
    m_engine->rootContext()->setContextProperty(QStringLiteral("browseRoles"), roles);
    QQmlComponent component(m_engine,
                            QUrl::fromLocalFile(QStringLiteral(SWELL_SOURCE_DIR "/tests/qmlruntime/Probe.qml")));
    m_probe = component.create();
    QVERIFY2(m_probe, qPrintable(component.errorString()));

    Client *client = m_players->client(id);
    QVERIFY(client);
    QTRY_VERIFY(client->online());
    QTRY_COMPARE(m_probe->property("presetCount").toInt(), 2);
    QTRY_COMPARE(m_probe->property("inputCount").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(m_probe, "loadAll"));
    QTRY_VERIFY(m_probe->property("ready").toBool());
}

void TstQmlRuntime::cleanupTestCase()
{
    delete m_probe;
    m_probe = nullptr;
}

QString TstQmlRuntime::call(const char *function, const QVariant &arg)
{
    QVariant result;
    const bool ok = arg.isValid()
            ? QMetaObject::invokeMethod(m_probe, function, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, arg))
            : QMetaObject::invokeMethod(m_probe, function, Q_RETURN_ARG(QVariant, result));
    if (!ok)
        return QStringLiteral("<no function %1>").arg(QLatin1String(function));
    return result.toString();
}

QString TstQmlRuntime::image(const QString &path) const
{
    return QStringLiteral("http://127.0.0.1:%1%2").arg(m_player->port()).arg(path);
}

void TstQmlRuntime::presets()
{
    const QString radio = QStringLiteral("5|North Sea FM|%1&followRedirects=1|").arg(image(QStringLiteral("/Artwork?id=5")));
    const QString input = QStringLiteral("7|Turntable|%1?followRedirects=1|input1").arg(image(QStringLiteral("/images/in.png")));
    QCOMPARE(call("presetAt", 0), radio);
    QCOMPARE(call("presetAt", 1), input);
    QCOMPARE(call("presetTile", 0), radio);
    QCOMPARE(call("presetTile", 1), input);
}

void TstQmlRuntime::inputs()
{
    const QString optical = QStringLiteral("input1|Optical Input|%1?followRedirects=1|Capture%3Ahw%3A1")
            .arg(image(QStringLiteral("/images/o.png")));
    QCOMPARE(call("inputAt", 0), optical);
    QCOMPARE(call("inputTile", 0), optical);
    QCOMPARE(call("inputImage", QStringLiteral("input1")),
             QStringLiteral("%1?followRedirects=1").arg(image(QStringLiteral("/images/o.png"))));
}

void TstQmlRuntime::queue()
{
    QCOMPARE(call("queue"), QStringLiteral("true boolean number string boolean number"));
}

void TstQmlRuntime::firmwareValues()
{
    // What HomePage and PlayerSettingsPage read for firmware (spec 006). An
    // earlier test takes the player offline; the values stay as last checked.
    QTRY_COMPARE(call("firmware"), QStringLiteral("true 4.18.20 true 4.18.18 true false number"));
}

void TstQmlRuntime::upgradeStartFromQml()
{
    // UpgradeDialog and the start button (revision 2026-11-25, AC8). Last:
    // the player is updating afterwards. An earlier test took it offline.
    QCOMPARE(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("false"));
    QVERIFY(m_player->listen(m_player->port()));
    m_player->setStatus("<status etag=\"u1\"><state>stop</state><prid>1</prid></status>");
    QTRY_COMPARE_WITH_TIMEOUT(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("true"), 10000);
    QCOMPARE(call("upgradeCanAccept", QStringLiteral("4.18.19")), QStringLiteral("false"));
    QCOMPARE(call("upgradeButton"), QStringLiteral("true true"));
    // A dialog opened for this player cannot accept after a switch.
    call("holdPlayer");
    const QString id = m_players->currentId();
    QVERIFY(m_players->addManual(QStringLiteral("127.0.0.30:1")));
    m_players->setCurrentId(QStringLiteral("127.0.0.30:1"));
    QCOMPARE(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("false"));
    m_players->setCurrentId(id);
    QTRY_COMPARE_WITH_TIMEOUT(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("true"), 10000);
    call("startUpgrade", QStringLiteral("4.18.20"));
    QCOMPARE(call("upgradePhase"), QStringLiteral("checking"));
    QCOMPARE(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("false"));
    QCOMPARE(call("upgradeButton"), QStringLiteral("false false"));
    call("startUpgrade", QStringLiteral("4.18.20"));
    QTRY_COMPARE(call("upgradePhase"), QStringLiteral("starting"));
    QCOMPARE(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("false"));
    QCOMPARE(call("upgradeButton"), QStringLiteral("false false"));
    QTest::qWait(1000);
    int starts = 0;
    for (const FakePlayer::Request &r : m_player->requests(QStringLiteral("/upgrade")))
        starts += r.target == QByteArray("/upgrade?upgrade=this") ? 1 : 0;
    QCOMPARE(starts, 1);
}

void TstQmlRuntime::upgradeEndState()
{
    // Continues the update upgradeStartFromQml started. Once the player runs
    // the new firmware: no start button and no notice, while the check does
    // not answer (a NAD just back, evidence.md; held here, which is
    // invented) and after it says nothing is on offer; a later offer brings
    // them back.
    m_player->serve(QStringLiteral("/SyncStatus"),
                    "<UpgradeStatusStage2 version=\"1.97\" name=\"Den\" model=\"C379\"><total>3</total>"
                    "<step>2</step><percent>1</percent><error>0</error></UpgradeStatusStage2>");
    QTRY_COMPARE_WITH_TIMEOUT(call("upgradePhase"), QStringLiteral("installing"), 10000);
    m_player->silenceIf(QStringLiteral("/upgrade"), [](const QUrlQuery &) { return true; },
                        FakePlayer::Hold);
    m_player->serve(QStringLiteral("/SyncStatus"),
                    "<SyncStatus etag=\"t\" syncStat=\"s\" name=\"Den\" volume=\"20\" version=\"4.18.20\"/>");
    QTRY_COMPARE_WITH_TIMEOUT(call("upgradePhase"), QStringLiteral("done"), 10000);
    QCOMPARE(call("upgradeButton"), QStringLiteral("false false"));
    QCOMPARE(call("upgradeNotice"), QStringLiteral("false false"));
    QCOMPARE(call("upgradeCanAccept", QStringLiteral("4.18.20")), QStringLiteral("false"));

    m_player->stopSilence(QStringLiteral("/upgrade"));
    m_player->serve(QStringLiteral("/upgrade"),
                    "<upgrade inProgress=\"false\" available=\"false\">check</upgrade>");
    Client *client = m_players->client(m_players->currentId());
    const int checks = m_player->requests(QStringLiteral("/upgrade")).size();
    QTRY_VERIFY_WITH_TIMEOUT((client->checkUpgrade(),
                              m_player->requests(QStringLiteral("/upgrade")).size() > checks), 5000);
    QTest::qWait(500);  // for the answer; the state asserted does not change
    QCOMPARE(call("upgradePhase"), QStringLiteral("done"));
    QCOMPARE(call("upgradeButton"), QStringLiteral("false false"));
    QCOMPARE(call("upgradeNotice"), QStringLiteral("false false"));

    m_player->serve(QStringLiteral("/upgrade"),
                    "<upgrade inProgress=\"false\" version=\"4.18.30\" available=\"true\">check</upgrade>");
    QTRY_COMPARE_WITH_TIMEOUT((client->checkUpgrade(), call("upgradeButton")), QStringLiteral("true true"), 5000);
    QCOMPARE(call("upgradePhase"), QStringLiteral("none"));
    QCOMPARE(call("upgradeNotice"), QStringLiteral("true true"));
}

void TstQmlRuntime::groupValues()
{
    // A lone player: what the pages read for a group reaches JavaScript.
    QCOMPARE(call("group"), QStringLiteral("true 0 false -1 false"));
    QTRY_COMPARE(call("playerRow"), QStringLiteral("0 true 0  20 0 false false"));
}

void TstQmlRuntime::shuffleAndRepeat()
{
    // shown (queue is the source), shuffled, repeat on, repeat one
    m_player->setStatus("<status etag=\"r1\"><state>play</state><prid>1</prid>"
                        "<shuffle>1</shuffle><repeat>1</repeat></status>");
    QTRY_COMPARE(call("shuffleRepeat"), QStringLiteral("true true true true"));
    m_player->setStatus("<status etag=\"r2\"><state>play</state><prid>1</prid>"
                        "<shuffle>0</shuffle><repeat>0</repeat></status>");
    QTRY_COMPARE(call("shuffleRepeat"), QStringLiteral("true false true false"));
    // An input plays: hidden (API 2.1, streamUrl).
    m_player->setStatus("<status etag=\"r3\"><state>stream</state><prid>1</prid>"
                        "<streamUrl>Capture:x</streamUrl><repeat>2</repeat></status>");
    QTRY_COMPARE(call("shuffleRepeat"), QStringLiteral("false false false false"));
    // Stopped without a stream: only !streaming is checked here; the page
    // also hides the row with the transport row (_empty).
    m_player->setStatus("<status etag=\"r4\"><state>stop</state><prid>1</prid></status>");
    QTRY_COMPARE(call("shuffleRepeat"), QStringLiteral("true false false false"));

    // The enum reaches the slot from QML (Player.RepeatQueue is 0, RepeatTrack 1).
    call("setRepeatTo", QStringLiteral("RepeatQueue"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Repeat")).size(), 1);
    call("setRepeatTo", QStringLiteral("RepeatTrack"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Repeat")).size(), 2);
    QCOMPARE(m_player->requests(QStringLiteral("/Repeat")).at(0).target, QByteArray("/Repeat?state=0"));
    QCOMPARE(m_player->requests(QStringLiteral("/Repeat")).at(1).target, QByteArray("/Repeat?state=1"));
}

void TstQmlRuntime::reorderViewAndModelMove()
{
    // Qt 5.6 DelegateModel: after items.move(1, 5) the model's own move 1 -> 5
    // leaves the view as it is and keeps the delegate (spec 009, D).
    QCOMPARE(call("compositor"), QStringLiteral("acdefbg acdefbg true"));
}

void TstQmlRuntime::reorderDropAndPutBack()
{
    // lifted, the move requested, the view after the drop, after putBack, ended
    QCOMPARE(call("reorderDropForward"), QStringLiteral("true 1>4 acdebfg abcdefg true"));
    QCOMPARE(call("reorderDropBackward"), QStringLiteral("true 5>2 abfcdeg abcdefg true"));
}

void TstQmlRuntime::reorderCancelAndReload()
{
    QCOMPARE(call("reorderCancel"), QStringLiteral("abdefgc abcdefg true true"));
    QCOMPARE(call("reorderEnd"), QStringLiteral("true false true"));
    // proxy at x,y: a grid follows the finger, a list only up and down
    QCOMPARE(call("reorderFollow"), QStringLiteral("30,35 0,35"));
}

void TstQmlRuntime::rowActions()
{
    // Unknown types (delete) are not offered.
    QCOMPARE(call("rowActions", 0),
             QStringLiteral("2:add-shuffle Shuffle /Add?album=0&shuffle=1|addAll-last Add last /Add?album=0&where=last"));
    QCOMPARE(call("rowActions", 1), QStringLiteral("1:addAll-last Add last /Add?album=1&where=last"));
}

void TstQmlRuntime::parentActionsArePassedOn()
{
    QVERIFY(QMetaObject::invokeMethod(m_probe, "openRow", Q_ARG(QVariant, 0)));
    QCOMPARE(call("shuffleUrl"), QStringLiteral("/Add?album=0&shuffle=1"));
    QVERIFY(QMetaObject::invokeMethod(m_probe, "openRow", Q_ARG(QVariant, 1)));
    QCOMPARE(call("shuffleUrl"), QString());
}

void TstQmlRuntime::favouritesAndUnreadRows()
{
    // The favourites row and a row not read yet both have an empty list of
    // actions, which BrowsePage reads as model.actions.length.
    QTRY_COMPARE(call("rootRow", 0), QStringLiteral("true 0"));
    QCOMPARE(call("unreadActions", 5), QStringLiteral("false 0"));
}

void TstQmlRuntime::letters()
{
    // LetterIndex leaves the "#" section out.
    QCOMPARE(call("letters"), QStringLiteral("A1 B3"));
}

void TstQmlRuntime::moreLinks()
{
    QTRY_COMPARE(call("moreLinks"),
                 QStringLiteral("1:Songs Lib:Songs?artist=X:Songs Lib:Songs?artist=X"));
}

void TstQmlRuntime::presetsFollowThePlayer()
{
    m_player->serve(QStringLiteral("/Presets"),
                    "<presets prid=\"2\"><preset id=\"9\" name=\"Dinner\" url=\"TuneIn:s9\"/></presets>");
    m_player->setStatus("<status etag=\"e2\"><state>stop</state><prid>2</prid></status>");
    QTRY_COMPARE(m_probe->property("presetCount").toInt(), 1);
    QCOMPARE(call("presetTile", 0), QStringLiteral("9|Dinner||"));
}

void TstQmlRuntime::mprisFollowsTheStatus()
{
    // status, canControl, canPlay, canPause, canGoNext, canGoPrevious, hasShuffle, hasLoopStatus
    m_player->setStatus("<status etag=\"m1\"><state>play</state><title1>A</title1></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("1 true true true true true false false"));
    // A stream that offers skip only (API 4.8).
    m_player->setStatus("<status etag=\"m2\"><state>stream</state><streamUrl>Svc:x</streamUrl>"
                        "<actions><action name=\"skip\" url=\"/Action?service=Svc&amp;skip=1\"/></actions></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("1 true true true true false false false"));
    m_player->setStatus("<status etag=\"m3\"><state>stream</state><streamUrl>Radio:x</streamUrl></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("1 true true true false false false false"));
    m_player->setStatus("<status etag=\"m4\"><state>pause</state></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("2 true true true true true false false"));
    m_player->setStatus("<status etag=\"m5\"><state>stop</state><title1>A</title1></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("0 true true true true true false false"));
    // Nothing to play (an emptied queue): nothing to offer, so the system
    // controls hide.
    m_player->setStatus("<status etag=\"m7\"><state>stop</state></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("0 true false false false false false false"));
    QCOMPARE(call("mprisMeta"), QStringLiteral("||||"));
    m_player->setStatus("<status etag=\"m6\"><state>connecting</state></status>");
    QTRY_COMPARE(call("mprisState"), QStringLiteral("0 true true true true true false false"));
}

void TstQmlRuntime::mprisMetadataAndPosition()
{
    m_player->setStatus("<status etag=\"p1\"><state>pause</state><title1>Track A</title1><title2>Nobody</title2>"
                        "<title3>Invented</title3><image>/Artwork?id=1</image><secs>10</secs>"
                        "<totlen>200</totlen></status>");
    QTRY_COMPARE(call("mprisMeta"), QStringLiteral("Track A|Nobody|Invented|%1&followRedirects=1|200000")
                 .arg(image(QStringLiteral("/Artwork?id=1"))));
    QCOMPARE(call("mprisPosition"), QStringLiteral("10000"));
    // No position and no length: 0, not -1000; amber drops a duration of 0.
    m_player->setStatus("<status etag=\"p2\"><state>stop</state><title1>B</title1></status>");
    QTRY_COMPARE(call("mprisMeta"), QStringLiteral("B||||"));
    QCOMPARE(call("mprisPosition"), QStringLiteral("0"));
}

void TstQmlRuntime::mprisRequestsReachThePlayer()
{
    QObject *mpris = m_probe->findChild<QObject *>(QStringLiteral("mpris"));
    QVERIFY(mpris);
    m_player->setStatus("<status etag=\"r1\"><state>play</state></status>");
    QTRY_COMPARE(call("mprisState").left(1), QStringLiteral("1"));
    const int pauses = m_player->requests(QStringLiteral("/Pause")).size();
    const int skips = m_player->requests(QStringLiteral("/Skip")).size();
    const int backs = m_player->requests(QStringLiteral("/Back")).size();
    QVERIFY(QMetaObject::invokeMethod(mpris, "playPauseRequested"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Pause")).size(), pauses + 1);
    QVERIFY(QMetaObject::invokeMethod(mpris, "nextRequested"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Skip")).size(), skips + 1);
    QVERIFY(QMetaObject::invokeMethod(mpris, "previousRequested"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Back")).size(), backs + 1);

    m_player->setStatus("<status etag=\"r2\"><state>pause</state></status>");
    QTRY_COMPARE(call("mprisState").left(1), QStringLiteral("2"));
    const int plays = m_player->requests(QStringLiteral("/Play")).size();
    QVERIFY(QMetaObject::invokeMethod(mpris, "playPauseRequested"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Play")).size(), plays + 1);
}

void TstQmlRuntime::mprisOffline()
{
    m_player->setStatus("<status etag=\"o1\"><state>play</state><title1>Gone</title1></status>");
    QTRY_COMPARE(call("mprisMeta").left(4), QStringLiteral("Gone"));
    m_player->close();
    QTRY_COMPARE_WITH_TIMEOUT(call("mprisState"), QStringLiteral("0 true false false false false false false"),
                              10000);
    QCOMPARE(call("mprisMeta"), QStringLiteral("||||"));
    QCOMPARE(call("mprisPosition"), QStringLiteral("0"));
}

void TstQmlRuntime::mprisWithoutAPlayer()
{
    // Stopped, nothing to do, nothing shown but the name.
    QCOMPARE(call("mprisNone"), QStringLiteral("0 true false false false false false false:||||"));
}

int main(int argc, char *argv[])
{
    // No display in the build engine.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "minimal");
    // No session bus: the MPRIS player must never register on a real one.
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent");
    QGuiApplication app(argc, argv);
    TstQmlRuntime test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_qmlruntime.moc"
