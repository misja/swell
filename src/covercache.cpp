// SPDX-License-Identifier: MIT
#include "covercache.h"

#include <QAbstractNetworkCache>
#include <QNetworkDiskCache>
#include <QNetworkRequest>

namespace Swell {

QNetworkReply *CoverNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request,
                                                        QIODevice *outgoingData)
{
    if (op != GetOperation || !cache() || !cache()->metaData(request.url()).isValid())
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    QNetworkRequest cached(request);
    cached.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
    return QNetworkAccessManager::createRequest(op, cached, outgoingData);
}

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
