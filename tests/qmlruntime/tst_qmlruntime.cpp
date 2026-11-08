// SPDX-License-Identifier: MIT
// The values QML gets from the player and browse models, read with the
// expressions the pages use (Probe.qml), against the in-process FakePlayer.
// A key or list that does not reach JavaScript fails here instead of showing
// as a blank tile (spec 007, scope item 4).
#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>

#include "browsemodel.h"
#include "client.h"
#include "fakeplayer.h"
#include "playermodel.h"
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
    void rowActions();
    void parentActionsArePassedOn();
    void favouritesAndUnreadRows();
    void letters();
    void moreLinks();
    void presetsFollowThePlayer();

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
    m_player->serve(QStringLiteral("/SyncStatus"), "<SyncStatus etag=\"s\" syncStat=\"s\" name=\"Den\" volume=\"20\"/>");
    m_player->setStatus("<status etag=\"e1\"><state>stop</state><prid>1</prid></status>");
    m_player->serve(QStringLiteral("/Presets"),
                    "<presets prid=\"1\">"
                    "<preset id=\"5\" name=\"North Sea FM\" url=\"TuneIn:s1\" image=\"/Artwork?id=5\"/>"
                    "<preset id=\"7\" name=\"Turntable\" url=\"Capture:hw:1?id=input1\" image=\"/images/in.png\"/>"
                    "</presets>");
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
    m_players = new PlayerModel(this);
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.readTimeoutMs = 3000;
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

    auto *client = qobject_cast<Client *>(m_players->client(id));
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
    // The favourites row has no actions at all; a row not read yet has none.
    QTRY_COMPARE(call("rootRow", 0), QStringLiteral("true undefined"));
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

int main(int argc, char *argv[])
{
    // No display in the build engine.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "minimal");
    QGuiApplication app(argc, argv);
    TstQmlRuntime test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_qmlruntime.moc"
