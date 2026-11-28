// SPDX-License-Identifier: MIT
// The QML engine's network access: covers and icons from the players and
// from the services' servers (the engine fetches nothing else over the
// network; the players' API has its own QNetworkAccessManager). Covers are
// kept in a bounded disk cache and read from it once there: the players serve
// their library covers without cache headers, and Qt reuses a cached reply
// only while it is fresh (qnetworkreplyhttpimpl.cpp:579, Qt 5.6.3), which
// such a reply never is, also with PreferCache (issue #67).
#ifndef SWELL_COVERCACHE_H
#define SWELL_COVERCACHE_H

#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>
#include <QString>

namespace Swell {

// Reads a GET request from the cache when the cache has its URL
// (AlwaysCache, which skips the freshness check: qnetworkaccessmanager.cpp
// 1138-1154), from the network otherwise. Qt 5.6 has no such setting for a
// manager, only the per-request attribute. A changed cover is read again only
// once it has dropped out of the bounded cache.
class CoverNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *outgoingData) override;
};

class CoverCacheFactory : public QQmlNetworkAccessManagerFactory
{
public:
    static const qint64 MaxCacheBytes = 20 * 1024 * 1024;

    // directory: where the cache lives (the app's cache location in the app).
    explicit CoverCacheFactory(const QString &directory);

    // Called by the engine, possibly from its image reader thread.
    QNetworkAccessManager *create(QObject *parent) override;

private:
    const QString m_directory;
};

} // namespace Swell

#endif
