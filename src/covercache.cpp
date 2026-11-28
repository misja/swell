// SPDX-License-Identifier: MIT
#include "covercache.h"

#include <QDateTime>
#include <QNetworkAccessManager>

namespace Swell {

const qint64 CoverCacheFactory::MaxCacheBytes;
const qint64 CoverCacheFactory::MaxAgeSecs;

CoverDiskCache::CoverDiskCache(qint64 maxAgeSecs, QObject *parent)
    : QNetworkDiskCache(parent), m_maxAgeSecs(maxAgeSecs)
{
}

QIODevice *CoverDiskCache::prepare(const QNetworkCacheMetaData &metaData)
{
    if (metaData.expirationDate().isValid() || metaData.lastModified().isValid())
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
