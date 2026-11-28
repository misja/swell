// SPDX-License-Identifier: MIT
// The QML engine's cover cache (issue #67) against the in-process FakePlayer.
// The player serves its library covers without cache headers (measured on a
// NAD C 379, BluOS 4.18.26, 2026-11-28: Content-Type, Date, Vary only).
#include <QtTest>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QTemporaryDir>

#include "covercache.h"
#include "fakeplayer.h"

class TstCovers : public QObject
{
    Q_OBJECT

private slots:
    void aCoverIsReadOnce();
    void aRedirectedCoverIsReadOnce();
    void anErrorIsNotKept();
    void mustRevalidateGoesToThePlayer();
    void theCacheHasALimit();

private:
    static QNetworkReply *get(QNetworkAccessManager *nam, const QUrl &url);
};

QNetworkReply *TstCovers::get(QNetworkAccessManager *nam, const QUrl &url)
{
    QNetworkReply *reply = nam->get(QNetworkRequest(url));
    QSignalSpy finished(reply, &QNetworkReply::finished);
    finished.wait(5000);
    return reply;
}

void TstCovers::aCoverIsReadOnce()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FakePlayer player;
    QVERIFY(player.listen());
    player.serve(QStringLiteral("/Artwork"), QByteArray(4000, 'x'), "image/jpeg");
    Swell::CoverCacheFactory factory(dir.path());
    QScopedPointer<QNetworkAccessManager> nam(factory.create(nullptr));
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/Artwork?service=LocalMusic&album=A").arg(player.port()));

    QScopedPointer<QNetworkReply> first(get(nam.data(), url));
    QCOMPARE(first->error(), QNetworkReply::NoError);
    QCOMPARE(first->readAll().size(), 4000);
    QVERIFY(!first->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());

    // Shown again later: from the disk cache, the player is not asked.
    QScopedPointer<QNetworkReply> second(get(nam.data(), url));
    QCOMPARE(second->error(), QNetworkReply::NoError);
    QCOMPARE(second->readAll().size(), 4000);
    QVERIFY(second->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
    QCOMPARE(player.requests(QStringLiteral("/Artwork")).size(), 1);

    // Also for a new manager (the app started again).
    QScopedPointer<QNetworkAccessManager> later(factory.create(nullptr));
    QScopedPointer<QNetworkReply> third(get(later.data(), url));
    QVERIFY(third->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
    QCOMPARE(player.requests(QStringLiteral("/Artwork")).size(), 1);
}

void TstCovers::aRedirectedCoverIsReadOnce()
{
    // As measured: /Artwork answers 301 to the library on another port.
    QTemporaryDir dir;
    FakePlayer player;
    FakePlayer library;
    QVERIFY(player.listen());
    QVERIFY(library.listen());
    const QUrl target(QStringLiteral("http://127.0.0.1:%1/library/v1/Artwork?album=A").arg(library.port()));
    player.redirect(QStringLiteral("/Artwork"), target);
    library.serve(QStringLiteral("/library/v1/Artwork"), QByteArray(3000, 'y'), "image/jpeg");
    Swell::CoverCacheFactory factory(dir.path());
    QScopedPointer<QNetworkAccessManager> nam(factory.create(nullptr));
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/Artwork?album=A").arg(player.port()));
    for (int round = 0; round < 2; ++round) {
        // The image reader follows the redirect itself (qquickpixmapcache.cpp).
        QScopedPointer<QNetworkReply> hop(get(nam.data(), url));
        const QUrl next = hop->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        QCOMPARE(next, target);
        QScopedPointer<QNetworkReply> image(get(nam.data(), next));
        QCOMPARE(image->readAll().size(), 3000);
        QCOMPARE(hop->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool(), round == 1);
        QCOMPARE(image->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool(), round == 1);
    }
    QCOMPARE(player.requests(QStringLiteral("/Artwork")).size(), 1);
    QCOMPARE(library.requests(QStringLiteral("/library/v1/Artwork")).size(), 1);
}

void TstCovers::anErrorIsNotKept()
{
    QTemporaryDir dir;
    FakePlayer player;
    QVERIFY(player.listen());
    player.serve(QStringLiteral("/Artwork"), QByteArray(100, 'z'), "image/jpeg");
    player.failWith(QStringLiteral("/Artwork"), 404);
    Swell::CoverCacheFactory factory(dir.path());
    QScopedPointer<QNetworkAccessManager> nam(factory.create(nullptr));
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/Artwork?album=B").arg(player.port()));
    QScopedPointer<QNetworkReply> failed(get(nam.data(), url));
    QVERIFY(failed->error() != QNetworkReply::NoError);
    player.stopFailing(QStringLiteral("/Artwork"));
    QScopedPointer<QNetworkReply> fetched(get(nam.data(), url));
    QCOMPARE(fetched->error(), QNetworkReply::NoError);
    QVERIFY(!fetched->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
    QScopedPointer<QNetworkReply> cached(get(nam.data(), url));
    QVERIFY(cached->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
    QCOMPARE(player.requests(QStringLiteral("/Artwork")).size(), 2);
}

void TstCovers::mustRevalidateGoesToThePlayer()
{
    // Not seen on a server Swell uses; the cache backend refuses such
    // entries, so they are always read from the network.
    QTemporaryDir dir;
    FakePlayer player;
    QVERIFY(player.listen());
    player.serve(QStringLiteral("/Artwork"), QByteArray(100, 'w'), "image/jpeg");
    player.addHeaders(QStringLiteral("/Artwork"), "Cache-Control: max-age=0, must-revalidate\r\n");
    Swell::CoverCacheFactory factory(dir.path());
    QScopedPointer<QNetworkAccessManager> nam(factory.create(nullptr));
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/Artwork?album=C").arg(player.port()));
    for (int round = 0; round < 2; ++round) {
        QScopedPointer<QNetworkReply> reply(get(nam.data(), url));
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QCOMPARE(reply->readAll().size(), 100);
    }
    QCOMPARE(player.requests(QStringLiteral("/Artwork")).size(), 2);
}

// The setting only; eviction is QNetworkDiskCache's.
void TstCovers::theCacheHasALimit()
{
    QTemporaryDir dir;
    Swell::CoverCacheFactory factory(dir.path());
    QScopedPointer<QNetworkAccessManager> nam(factory.create(nullptr));
    QVERIFY(nam->cache());
    QCOMPARE(nam->cache()->metaObject()->className(), "QNetworkDiskCache");
    QCOMPARE(qobject_cast<QNetworkDiskCache *>(nam->cache())->maximumCacheSize(),
             Swell::CoverCacheFactory::MaxCacheBytes);
}

QTEST_GUILESS_MAIN(TstCovers)
#include "tst_covers.moc"
