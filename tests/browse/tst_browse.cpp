// SPDX-License-Identifier: MIT
// BrowseModel and the client's browse requests against the in-process
// FakePlayer. The /Browse replies are synthetic, shaped like those recorded
// from a player (spec 004, "Facts").
#include <QtTest>
#include <QNetworkAccessManager>

#include "browsemodel.h"
#include "client.h"
#include "fakeplayer.h"

using namespace BluOS;

class TstBrowse : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void keysAndSearchTextAreEncodedOnce();
    void rootKeepsLinksAndOffersFavourites();
    void rootWithoutLibraryFavourites();
    void sectionsHaveTheirLengthAtOnce();
    void theLetterInViewLoadsAndOnlyTheLatestWishIsKept();
    void pagingContinuesWithinALetter();
    void countMismatchesFollowTheReply();
    void unusableCountsFallBackToLetterRows();
    void plainListGrowsThroughNextKey();
    void lettersPointAtTheirFirstRows();
    void errorReplyIsShown();
    void modelGoneMidRequest();
    void additiveActionsAreNeverMerged();
    void playerPathsAreSentAsGivenOrRefused();
    void severalVisibleLettersAllLoad();
    void lettersFollowInsertedRows();
    void aFailedLetterIsAskedForAgain();
    void aRepeatedNextKeyEndsTheList();
    void playPathsGoThroughTheGateOrAreRefused();
    void searchEverywhereListsSources();
    void anArtistOpensOnItsAlbums();
    void aLetterListFindsItsFavourites();
    void noFavouritesWhenTheLibraryHasNone();
    void songListsGetNoFavourites();

private:
    void startClient();
    BrowseModel *open(const QString &key, bool root = false, const QString &query = QString());
    QString text(BrowseModel *m, int row);
    QStringList browseKeys();

    QNetworkAccessManager *m_nam = nullptr;
    FakePlayer *m_player = nullptr;
    Client *m_client = nullptr;
    bool m_libraryFavourites = true;
};

static QByteArray section(const QByteArray &letter, const QByteArray &count)
{
    return "<item browseKey=\"Lib:S:" + letter + "\" text=\"" + letter + "\" text2=\"" + count
            + "\" type=\"section\"/>";
}

static QByteArray albums(const QString &letter, int from, int to, const QByteArray &next = QByteArray())
{
    QByteArray xml = "<browse type=\"albums\"" + (next.isEmpty() ? QByteArray() : " nextKey=\"" + next + "\"") + ">";
    for (int i = from; i < to; ++i) {
        const QByteArray name = letter.toUtf8() + " album " + QByteArray::number(i);
        xml += "<item browseKey=\"Lib:Album/" + name + "\" text=\"" + name + "\" text2=\"Nobody\" type=\"album\">"
               "<contextMenu><item actionURL=\"/Add?album=" + QByteArray::number(i)
               + "&amp;where=last\" text=\"Add last\" type=\"addAll-last\"/>"
                 "<item actionURL=\"/Delete?x=1\" text=\"Delete\" type=\"delete\"/></contextMenu></item>";
    }
    return xml + "</browse>";
}

void TstBrowse::init()
{
    m_nam = new QNetworkAccessManager(this);
    m_player = new FakePlayer(this);
    QVERIFY(m_player->listen());
    m_libraryFavourites = true;
    m_player->serve(QStringLiteral("/SyncStatus"), "<SyncStatus etag=\"s\" syncStat=\"s\" name=\"Den\" volume=\"20\"/>");
    m_player->setStatus("<status etag=\"e1\"><state>stop</state></status>");
    m_player->serve(QStringLiteral("/Add"), "<addsong length=\"3\"/>");
    m_player->handle(QStringLiteral("/Browse"), [this](const QUrlQuery &q) -> QByteArray {
        const QString key = q.queryItemValue(QStringLiteral("key"), QUrl::FullyDecoded);
        if (q.hasQueryItem(QStringLiteral("q")))
            return "<browse type=\"menu\"><item browseKey=\"Lib:Search/x\" text=\"Library\" type=\"link\"/></browse>";
        if (key.isEmpty())
            return "<browse type=\"menu\">"
                   "<item browseKey=\"BluOS:\" text=\"Playlists\" type=\"link\"/>"
                   "<item playURL=\"/Play?url=Capture%3Abluez%3Abluetooth\" text=\"Bluetooth\" type=\"audio\" inputType=\"bluetooth\"/>"
                   "<item browseKey=\"LocalMusic:\" text=\"Library\" type=\"link\"/>"
                   "<item playURL=\"/Play?url=Spotify%3Aplay\" text=\"Spotify\" type=\"audio\"/>"
                   "<item browseKey=\"Airable:\" text=\"Radio\" type=\"link\"/></browse>";
        if (key == QLatin1String("LocalMusic:"))
            return QByteArray("<browse serviceName=\"Library\" type=\"menu\">"
                              "<item browseKey=\"LocalMusic:bySection:Album\" text=\"Albums\" type=\"link\"/>")
                    + (m_libraryFavourites ? "<item browseKey=\"LocalMusic:MG/LocalMusic-Favourites\" text=\"Favoriten\" type=\"link\"/>" : "")
                    + "</browse>";
        if (key == QLatin1String("Index"))
            return "<browse type=\"sections\">" + section("A", "3") + section("B", "60") + section("C", "2")
                    + section("D", "4") + "</browse>";
        if (key == QLatin1String("BadIndex"))
            return "<browse type=\"sections\">" + section("A", "3") + section("B", "many") + "</browse>";
        if (key == QLatin1String("Lib:S:A"))
            return albums(QStringLiteral("A"), 0, 3);
        if (key == QLatin1String("Lib:S:B"))
            return albums(QStringLiteral("B"), 0, 50, "Lib:S:B?start=50");
        if (key == QLatin1String("Lib:S:B?start=50"))
            return albums(QStringLiteral("B"), 50, 60);
        if (key == QLatin1String("Lib:S:C"))
            return albums(QStringLiteral("C"), 0, 3);   // announced 2
        if (key == QLatin1String("Lib:S:D"))
            return albums(QStringLiteral("D"), 0, 1);   // announced 4
        if (key == QLatin1String("List"))
            return albums(QStringLiteral("L"), 0, 30, "List?start=30");
        if (key == QLatin1String("List?start=30"))
            return albums(QStringLiteral("L"), 30, 45);
        if (key == QLatin1String("Small"))
            return "<browse type=\"sections\">" + section("X", "1") + section("Y", "1") + section("Z", "1") + "</browse>";
        if (key == QLatin1String("Lib:S:X") || key == QLatin1String("Lib:S:Y") || key == QLatin1String("Lib:S:Z"))
            return albums(key.right(1), 0, 1);
        if (key == QLatin1String("Loop"))
            return albums(QStringLiteral("O"), 0, 20, "Loop");
        if (key == QLatin1String("Artist:X"))
            return "<browse serviceName=\"Library\" type=\"menu\">"
                   "<item browseKey=\"Lib:Albums?artist=X\" text=\"Albums\" type=\"link\"/>"
                   "<item browseKey=\"Lib:Songs?artist=X\" text=\"Songs\" type=\"link\"/></browse>";
        if (key == QLatin1String("Lib:Albums?artist=X"))
            return albums(QStringLiteral("X"), 0, 2);
        if (key == QLatin1String("LocalMusic:MG/LocalMusic-Favourites"))
            return "<browse type=\"menu\">"
                   "<item browseKey=\"LocalMusic:Artist/%2Flibrary%2Fv1%2FArtists%3Fcategory=FAVOURITES&amp;service=LocalMusic\" text=\"Artists\" type=\"link\"/>"
                   "<item browseKey=\"LocalMusic:Album/%2Flibrary%2Fv1%2FAlbums%3Fcategory=FAVOURITES&amp;service=LocalMusic\" text=\"Albums\" type=\"link\"/></browse>";
        if (key == QLatin1String("LocalMusic:bySection:Album/%2Flibrary%2Fv1%2FAlbums%3Fservice=LocalMusic"))
            return "<browse type=\"sections\">" + section("A", "3") + "</browse>";
        if (key == QLatin1String("LocalMusic:bySection:Song/%2Flibrary%2Fv1%2FSongs%3Fall=1&service=LocalMusic"))
            return "<browse type=\"sections\">" + section("A", "3") + "</browse>";
        if (key == QLatin1String("Broken"))
            return "<error><message>Service unavailable</message></error>";
        return "<browse type=\"menu\"/>";
    });
}

void TstBrowse::cleanup()
{
    delete m_client;
    m_client = nullptr;
    delete m_player;
    m_player = nullptr;
    delete m_nam;
    m_nam = nullptr;
}

void TstBrowse::startClient()
{
    m_client = new Client(m_nam, QStringLiteral("127.0.0.1"), m_player->port());
    Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.readTimeoutMs = 3000;
    t.minIntervalMs = 200;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    m_client->setTiming(t);
    m_client->start();
    QTRY_VERIFY(m_client->online());
}

BrowseModel *TstBrowse::open(const QString &key, bool root, const QString &query)
{
    auto *m = new BrowseModel(this);
    m->setClient(m_client);
    m->setKey(key);
    m->setRoot(root);
    m->setQuery(query);
    m->load();
    return m;
}

QString TstBrowse::text(BrowseModel *m, int row)
{
    return m->data(m->index(row), BrowseModel::TextRole).toString();
}

QStringList TstBrowse::browseKeys()
{
    QStringList keys;
    for (const auto &r : m_player->requests(QStringLiteral("/Browse")))
        keys << r.query.queryItemValue(QStringLiteral("key"), QUrl::FullyDecoded);
    return keys;
}

void TstBrowse::keysAndSearchTextAreEncodedOnce()
{
    startClient();
    const QString key = QString::fromUtf8("Lib:S:x/%2Fa%3Fb=1&c=d+e \xc3\xa9#");
    QScopedPointer<BrowseModel> m(open(key, false, QStringLiteral("C++ & co")));
    QTRY_VERIFY(m->ready());
    const QByteArray target = m_player->requests(QStringLiteral("/Browse")).last().target;
    QCOMPARE(target, QByteArray("/Browse?key=Lib%3AS%3Ax%2F%252Fa%253Fb%3D1%26c%3Dd%2Be%20%C3%A9%23"
                                "&q=C%2B%2B%20%26%20co&withContextMenuItems=1"));
    // The player sees exactly the key it handed out.
    QCOMPARE(browseKeys().last(), key);
}

void TstBrowse::rootKeepsLinksAndOffersFavourites()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QString(), true));
    QTRY_VERIFY(m->ready());
    QTRY_COMPARE(m->rowCount(), 4);
    QCOMPARE(m->data(m->index(0), BrowseModel::FavouritesRole).toBool(), true);
    QCOMPARE(m->data(m->index(0), BrowseModel::BrowseKeyRole).toString(),
             QStringLiteral("LocalMusic:MG/LocalMusic-Favourites"));
    QCOMPARE(m->data(m->index(0), BrowseModel::Text2Role).toString(), QStringLiteral("Library"));
    QCOMPARE(m->data(m->index(0), BrowseModel::DurationRole).toInt(), -1);   // no NaN:NaN
    QCOMPARE(text(m.data(), 1), QStringLiteral("Playlists"));
    QCOMPARE(text(m.data(), 2), QStringLiteral("Library"));
    QCOMPARE(text(m.data(), 3), QStringLiteral("Radio"));
}

void TstBrowse::rootWithoutLibraryFavourites()
{
    m_libraryFavourites = false;
    startClient();
    QScopedPointer<BrowseModel> m(open(QString(), true));
    QTRY_VERIFY(m->ready());
    QTRY_COMPARE(browseKeys().size(), 2);   // the root, then the Library menu
    QTest::qWait(200);
    QCOMPARE(m->rowCount(), 3);
    QCOMPARE(m->data(m->index(0), BrowseModel::FavouritesRole).toBool(), false);
    // The favourites key itself is never read blind.
    QVERIFY(!browseKeys().contains(QStringLiteral("LocalMusic:MG/LocalMusic-Favourites")));
}

void TstBrowse::sectionsHaveTheirLengthAtOnce()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Index")));
    QTRY_VERIFY(m->ready());
    QVERIFY(m->sectioned());
    QCOMPARE(m->rowCount(), 3 + 60 + 2 + 4);
    QTest::qWait(200);
    QCOMPARE(browseKeys(), QStringList({ QStringLiteral("Index") }));   // nothing read until shown
    // A row of B is a placeholder with its letter; showing it reads B only.
    QCOMPARE(m->sectionAt(64), QStringLiteral("C"));   // without reading C
    QTest::qWait(100);
    QVERIFY(!browseKeys().contains(QStringLiteral("Lib:S:C")));
    QCOMPARE(m->data(m->index(3), BrowseModel::SectionRole).toString(), QStringLiteral("B"));
    QCOMPARE(m->data(m->index(3), BrowseModel::LoadedRole).toBool(), false);
    QTRY_COMPARE(text(m.data(), 3), QStringLiteral("B album 0"));
    QCOMPARE(browseKeys(), QStringList({ QStringLiteral("Index"), QStringLiteral("Lib:S:B") }));
}

void TstBrowse::theLetterInViewLoadsAndOnlyTheLatestWishIsKept()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Index")));
    QTRY_VERIFY(m->ready());
    m_player->delay(QStringLiteral("/Browse"), 300);
    text(m.data(), 64);                 // C, in view after a jump
    QTRY_VERIFY(m->loading());
    text(m.data(), 0);                  // A passed on the way back…
    text(m.data(), 4);                  // …then B: only B is still wanted
    QTRY_COMPARE(text(m.data(), 4), QStringLiteral("B album 1"));
    QTest::qWait(400);
    QCOMPARE(browseKeys(), QStringList({ QStringLiteral("Index"), QStringLiteral("Lib:S:C"), QStringLiteral("Lib:S:B") }));
    QCOMPARE(m->data(m->index(0), BrowseModel::LoadedRole).toBool(), false);
}

void TstBrowse::pagingContinuesWithinALetter()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Index")));
    QTRY_VERIFY(m->ready());
    QTRY_COMPARE(text(m.data(), 3), QStringLiteral("B album 0"));
    QCOMPARE(m->data(m->index(3 + 55), BrowseModel::LoadedRole).toBool(), false);
    QTRY_COMPARE(text(m.data(), 3 + 55), QStringLiteral("B album 55"));
    QVERIFY(browseKeys().contains(QStringLiteral("Lib:S:B?start=50")));
    QCOMPARE(m->rowCount(), 69);
}

void TstBrowse::countMismatchesFollowTheReply()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Index")));
    QTRY_VERIFY(m->ready());
    text(m.data(), 63);                 // C: announced 2, three arrive
    QTRY_COMPARE(m->rowCount(), 70);
    QCOMPARE(text(m.data(), 65), QStringLiteral("C album 2"));
    QCOMPARE(m->data(m->index(66), BrowseModel::SectionRole).toString(), QStringLiteral("D"));
    text(m.data(), 66);                 // D: announced 4, one arrives
    QTRY_COMPARE(m->rowCount(), 67);
    QCOMPARE(text(m.data(), 66), QStringLiteral("D album 0"));
}

void TstBrowse::unusableCountsFallBackToLetterRows()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("BadIndex")));
    QTRY_VERIFY(m->ready());
    QVERIFY(!m->sectioned());
    QCOMPARE(m->rowCount(), 2);
    QCOMPARE(m->data(m->index(1), BrowseModel::BrowseKeyRole).toString(), QStringLiteral("Lib:S:B"));
}

void TstBrowse::plainListGrowsThroughNextKey()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("List")));
    QTRY_VERIFY(m->ready());
    QCOMPARE(m->rowCount(), 30);
    text(m.data(), 5);
    QTest::qWait(300);
    QCOMPARE(m->rowCount(), 30);        // not near the end yet
    text(m.data(), 25);
    QTRY_COMPARE(m->rowCount(), 45);
    QCOMPARE(text(m.data(), 44), QStringLiteral("L album 44"));
    // Only the actions Swell understands are offered.
    const auto actions = m->data(m->index(0), BrowseModel::ActionsRole).value<QList<BrowseAction>>();
    QCOMPARE(actions.size(), 1);
    QCOMPARE(actions.first().type, QStringLiteral("addAll-last"));
}

void TstBrowse::lettersPointAtTheirFirstRows()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Index")));
    QTRY_VERIFY(m->ready());
    const QList<BrowseModel::Letter> letters = m->letters();
    QCOMPARE(letters.size(), 4);
    QCOMPARE(letters.at(2).text, QStringLiteral("C"));
    QCOMPARE(letters.at(2).row, 63);
}

void TstBrowse::errorReplyIsShown()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Broken")));
    QTRY_VERIFY(m->ready());
    QCOMPARE(m->error(), QStringLiteral("Service unavailable"));
    QCOMPARE(m->rowCount(), 0);
}

void TstBrowse::modelGoneMidRequest()
{
    startClient();
    m_player->delay(QStringLiteral("/Browse"), 300);
    BrowseModel *m = open(QStringLiteral("List"));
    QTRY_VERIFY(m->loading());
    delete m;   // the page was popped
    QTest::qWait(500);
    QVERIFY(m_client->online());
}

void TstBrowse::additiveActionsAreNeverMerged()
{
    startClient();
    QSignalSpy done(m_client, &Client::actionDone);
    m_client->runAction(QStringLiteral("/Add?service=LocalMusic&album=A&where=next"), QStringLiteral("addAll-next"));
    m_client->runAction(QStringLiteral("/Add?service=LocalMusic&album=B&where=last"), QStringLiteral("addAll-last"));
    QTRY_COMPARE(done.size(), 2);
    const auto adds = m_player->requests(QStringLiteral("/Add"));
    QCOMPARE(adds.size(), 2);
    QCOMPARE(adds.at(0).target, QByteArray("/Add?service=LocalMusic&album=A&where=next"));
    QCOMPARE(adds.at(1).target, QByteArray("/Add?service=LocalMusic&album=B&where=last"));
    QCOMPARE(done.at(1).at(0).toString(), QStringLiteral("addAll-last"));
}

void TstBrowse::playerPathsAreSentAsGivenOrRefused()
{
    startClient();
    QSignalSpy failed(m_client, &Client::commandFailed);
    const QStringList refused = {
        QStringLiteral("http://elsewhere/Add?x=1"), QStringLiteral("//elsewhere/Add"),
        QStringLiteral("Add?x=1"), QStringLiteral("/Add?album=Low Tide"), QStringLiteral("/Add?x=1#frag"),
        QString::fromUtf8("/Add?artist=\xc3\xa9"),
    };
    for (const QString &path : refused)
        m_client->runAction(path, QStringLiteral("add-last"));
    QCOMPARE(failed.size(), refused.size());
    const QString exact = QStringLiteral("/Add?album=Low+Tide&artist=%C3%A9&file=%2Fm%2F1.flac");
    m_client->runAction(exact, QStringLiteral("add-last"));
    QTRY_COMPARE(m_player->requests(QStringLiteral("/Add")).size(), 1);
    QCOMPARE(m_player->requests(QStringLiteral("/Add")).first().target, exact.toLatin1());
}

// Re-reads rows the way a view does on dataChanged: only the changed rows
// that are "on screen".
static void watch(BrowseModel *m, int first, int last)
{
    QObject::connect(m, &QAbstractItemModel::dataChanged, m,
                     [m, first, last](const QModelIndex &from, const QModelIndex &to) {
        for (int row = qMax(first, from.row()); row <= qMin(last, to.row()) && row < m->rowCount(); ++row)
            m->data(m->index(row), BrowseModel::LoadedRole);
    });
}

void TstBrowse::severalVisibleLettersAllLoad()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Small")));
    QTRY_VERIFY(m->ready());
    watch(m.data(), 0, 2);
    // X, Y and Z are on screen together; only one wish is kept.
    text(m.data(), 0);
    text(m.data(), 1);
    text(m.data(), 2);
    // Without touching the rows again (that is the view's job), all three
    // letters are read.
    QTRY_VERIFY(browseKeys().contains(QStringLiteral("Lib:S:Y")));
    QTRY_VERIFY(!m->loading());
    QVERIFY(browseKeys().contains(QStringLiteral("Lib:S:X")));
    QVERIFY(browseKeys().contains(QStringLiteral("Lib:S:Z")));
    QCOMPARE(text(m.data(), 1), QStringLiteral("Y album 0"));
}

void TstBrowse::lettersFollowInsertedRows()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Index")));
    QTRY_VERIFY(m->ready());
    QSignalSpy changed(m.data(), &BrowseModel::lettersChanged);
    text(m.data(), 63);   // C: announced 2, three arrive
    QTRY_COMPARE(m->rowCount(), 70);
    QVERIFY(changed.size() >= 1);
    QCOMPARE(m->letters().at(3).row, 66);
}

void TstBrowse::aFailedLetterIsAskedForAgain()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Small")));
    QTRY_VERIFY(m->ready());
    watch(m.data(), 0, 0);
    m_player->failWith(QStringLiteral("/Browse"), 503);
    text(m.data(), 0);
    QTRY_VERIFY(!m->loading());
    QCOMPARE(m->data(m->index(0), BrowseModel::LoadedRole).toBool(), false);
    m_player->stopFailing(QStringLiteral("/Browse"));
    // Nothing touches the row: the model asks the view again by itself.
    QTRY_COMPARE_WITH_TIMEOUT(text(m.data(), 0), QStringLiteral("X album 0"), 8000);
}

void TstBrowse::aRepeatedNextKeyEndsTheList()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("Loop")));
    QTRY_VERIFY(m->ready());
    QCOMPARE(m->rowCount(), 20);
    text(m.data(), 15);
    QTRY_VERIFY(browseKeys().size() >= 2);
    QTRY_VERIFY(!m->loading());
    text(m.data(), 35);
    text(m.data(), 15);
    QTest::qWait(300);
    QCOMPARE(browseKeys().size(), 2);   // the repeat was not followed again
}

void TstBrowse::playPathsGoThroughTheGateOrAreRefused()
{
    startClient();
    m_player->serve(QStringLiteral("/Play"), "<state>play</state>");
    QSignalSpy failed(m_client, &Client::commandFailed);
    m_client->playPath(QStringLiteral("http://elsewhere/Play?url=x"));
    QCOMPARE(failed.size(), 1);
    // Two quick taps on different albums: the last one wins.
    m_client->playPath(QStringLiteral("/Add?playnow=1&service=LocalMusic&album=A"));
    m_client->playPath(QStringLiteral("/Add?playnow=1&service=LocalMusic&album=B"));
    QTRY_VERIFY(m_player->requests(QStringLiteral("/Add")).size() >= 1);
    QTest::qWait(400);
    QCOMPARE(m_player->requests(QStringLiteral("/Add")).last().target,
             QByteArray("/Add?playnow=1&service=LocalMusic&album=B"));
}

void TstBrowse::searchEverywhereListsSources()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QString(), false, QStringLiteral("bodega")));
    QTRY_VERIFY(m->ready());
    QCOMPARE(m_player->requests(QStringLiteral("/Browse")).last().target,
             QByteArray("/Browse?q=bodega&withContextMenuItems=1"));
    QCOMPARE(m->rowCount(), 1);
    QCOMPARE(m->data(m->index(0), BrowseModel::BrowseKeyRole).toString(), QStringLiteral("Lib:Search/x"));
}

void TstBrowse::anArtistOpensOnItsAlbums()
{
    startClient();
    auto *m = new BrowseModel(this);
    m->setClient(m_client);
    m->setKey(QStringLiteral("Artist:X"));
    m->setExpandFirst(true);
    m->load();
    QTRY_VERIFY(m->ready());
    QTRY_COMPARE(m->rowCount(), 2);
    QCOMPARE(text(m, 0), QStringLiteral("X album 0"));
    QCOMPARE(m->listType(), QStringLiteral("albums"));
    QCOMPARE(m->moreLinks().size(), 1);
    QCOMPARE(m->moreLinks().first().browseKey, QStringLiteral("Lib:Songs?artist=X"));
    delete m;
}

void TstBrowse::aLetterListFindsItsFavourites()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("LocalMusic:bySection:Album/%2Flibrary%2Fv1%2FAlbums%3Fservice=LocalMusic")));
    QTRY_VERIFY(m->ready());
    QVERIFY(m->sectioned());
    QTRY_COMPARE(m->favouritesKey(),
                 QStringLiteral("LocalMusic:Album/%2Flibrary%2Fv1%2FAlbums%3Fcategory=FAVOURITES&service=LocalMusic"));
    // Only after the Library menu offered Favourites.
    const QStringList keys = browseKeys();
    QVERIFY(keys.indexOf(QStringLiteral("LocalMusic:")) < keys.indexOf(QStringLiteral("LocalMusic:MG/LocalMusic-Favourites")));
}

void TstBrowse::noFavouritesWhenTheLibraryHasNone()
{
    m_libraryFavourites = false;
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("LocalMusic:bySection:Album/%2Flibrary%2Fv1%2FAlbums%3Fservice=LocalMusic")));
    QTRY_VERIFY(m->ready());
    QTRY_VERIFY(browseKeys().contains(QStringLiteral("LocalMusic:")));
    QTest::qWait(300);
    QCOMPARE(m->favouritesKey(), QString());
    QVERIFY(!browseKeys().contains(QStringLiteral("LocalMusic:MG/LocalMusic-Favourites")));
}

void TstBrowse::songListsGetNoFavourites()
{
    startClient();
    QScopedPointer<BrowseModel> m(open(QStringLiteral("LocalMusic:bySection:Song/%2Flibrary%2Fv1%2FSongs%3Fall=1&service=LocalMusic")));
    QTRY_VERIFY(m->ready());
    QVERIFY(m->sectioned());
    // Agreed for Albums and Artists only: the Library is not even asked.
    QTest::qWait(300);
    QVERIFY(!browseKeys().contains(QStringLiteral("LocalMusic:")));
    QCOMPARE(m->favouritesKey(), QString());
}

QTEST_GUILESS_MAIN(TstBrowse)
#include "tst_browse.moc"
