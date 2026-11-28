// SPDX-License-Identifier: MIT
#include "covercache.h"

#include <QAbstractNetworkCache>
#include <QNetworkDiskCache>
#include <QNetworkRequest>

namespace Swell {

bool CoverNetworkAccessManager::servable(const QNetworkCacheMetaData &metaData)
{
    if (!metaData.isValid())
        return false;
    // The cache backend refuses these entries (qnetworkaccesscachebackend.cpp
    // 83-87, Qt 5.6.3): reading one with AlwaysCache would fail every time.
    for (const QNetworkCacheMetaData::RawHeader &header : metaData.rawHeaders()) {
        if (header.first.toLower() == "cache-control" && header.second.contains("must-revalidate"))
            return false;
    }
    return true;
}

QNetworkReply *CoverNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request,
                                                        QIODevice *outgoingData)
{
    if (op != GetOperation || !cache() || !servable(cache()->metaData(request.url())))
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    QNetworkRequest cached(request);
    cached.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
    return QNetworkAccessManager::createRequest(op, cached, outgoingData);
}

const qint64 CoverCacheFactory::MaxCacheBytes;

CoverCacheFactory::CoverCacheFactory(const QString &directory)
    : m_directory(directory)
{
}

QNetworkAccessManager *CoverCacheFactory::create(QObject *parent)
{
    auto *manager = new CoverNetworkAccessManager(parent);
    auto *cache = new QNetworkDiskCache(manager);
    cache->setCacheDirectory(m_directory);
    cache->setMaximumCacheSize(MaxCacheBytes);
    manager->setCache(cache);
    return manager;
}

} // namespace Swell
