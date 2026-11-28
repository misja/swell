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
    void theCacheIsBounded();

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

void TstCovers::theCacheIsBounded()
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
