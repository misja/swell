// SPDX-License-Identifier: MIT
#include "covercache.h"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

namespace Swell {

const qint64 CoverCacheFactory::MaxCacheBytes;
const qint64 CoverCacheFactory::MaxAgeSecs;

CoverDiskCache::CoverDiskCache(qint64 maxAgeSecs, QObject *parent)
    : QNetworkDiskCache(parent), m_maxAgeSecs(maxAgeSecs)
{
}

QIODevice *CoverDiskCache::prepare(const QNetworkCacheMetaData &metaData)
{
    // Only a 200 or a permanent redirect (as the players send them) without
    // any cache header of its own; anything else keeps Qt's handling.
    const int status = metaData.attributes().value(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool headers = false;
    for (const QNetworkCacheMetaData::RawHeader &header : metaData.rawHeaders()) {
        const QByteArray name = header.first.toLower();
        headers = headers || name == "expires" || name == "etag" || name == "last-modified"
                || name == "cache-control";
    }
    if (headers || (status != 200 && status != 301))
        return QNetworkDiskCache::prepare(metaData);
    QNetworkCacheMetaData dated(metaData);
    dated.setExpirationDate(QDateTime::currentDateTimeUtc().addSecs(m_maxAgeSecs));
    return QNetworkDiskCache::prepare(dated);
}

CoverCacheFactory::CoverCacheFactory(const QString &directory, qint64 maxAgeSecs)
    : m_directory(directory), m_maxAgeSecs(maxAgeSecs)
{
}

QNetworkAccessManager *CoverCacheFactory::create(QObject *parent)
{
    auto *manager = new QNetworkAccessManager(parent);
    auto *cache = new CoverDiskCache(m_maxAgeSecs, manager);
    cache->setCacheDirectory(m_directory);
    cache->setMaximumCacheSize(MaxCacheBytes);
    manager->setCache(cache);
    return manager;
}

} // namespace Swell
