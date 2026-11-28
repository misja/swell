// SPDX-License-Identifier: MIT
// The QML engine's network access: covers and icons from the players and
// from the services' servers (the engine fetches nothing else over the
// network; the players' API has its own QNetworkAccessManager). Covers are
// kept in a bounded disk cache (issue #67).
#ifndef SWELL_COVERCACHE_H
#define SWELL_COVERCACHE_H

#include <QNetworkDiskCache>
#include <QQmlNetworkAccessManagerFactory>
#include <QString>

namespace Swell {

// A disk cache that gives a reply without freshness information of its own
// (no Expires, no Last-Modified) an expiry of maxAge from the moment it is
// stored. The players serve their library covers that way, and Qt reuses a
// cached reply only while it is fresh (qnetworkreplyhttpimpl.cpp:564-580,
// Qt 5.6.3). Until then the cover comes from the cache; after that Qt reads it
// from the player again and stores it anew. Replies with Last-Modified or
// ETag (station images) keep Qt's own freshness and revalidation.
class CoverDiskCache : public QNetworkDiskCache
{
    Q_OBJECT

public:
    CoverDiskCache(qint64 maxAgeSecs, QObject *parent = nullptr);

    QIODevice *prepare(const QNetworkCacheMetaData &metaData) override;

private:
    const qint64 m_maxAgeSecs;
};

class CoverCacheFactory : public QQmlNetworkAccessManagerFactory
{
public:
    static const qint64 MaxCacheBytes = 20 * 1024 * 1024;
    // Covers rarely change (the maintainer, 2026-11-28).
    static const qint64 MaxAgeSecs = 30 * 24 * 3600;

    // directory: where the cache lives (the app's cache location in the app).
    explicit CoverCacheFactory(const QString &directory, qint64 maxAgeSecs = MaxAgeSecs);

    // Called by the engine, serialized (qqmlengine.cpp:1106-1115), possibly
    // for its image reader thread.
    QNetworkAccessManager *create(QObject *parent) override;

private:
    const QString m_directory;
    const qint64 m_maxAgeSecs;
};

} // namespace Swell

#endif
