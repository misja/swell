// SPDX-License-Identifier: MIT
#ifndef TESTS_SENDLOG_H
#define TESTS_SENDLOG_H

#include <QElapsedTimer>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

// Records when the client hands each request to the network, by path. The
// interval rule is about sending; arrival at the fake player also includes
// transit, which varies (measured under valgrind on the device: 30 to 163 ms
// for the same kind of request, misja/swell#53).
class SendLog : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QList<qint64> sentAt(const QString &path) const { return m_sent.value(path); }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *data) override
    {
        if (!m_clock.isValid())
            m_clock.start();
        m_sent[request.url().path()].append(m_clock.elapsed());
        return QNetworkAccessManager::createRequest(op, request, data);
    }

private:
    QElapsedTimer m_clock;
    QHash<QString, QList<qint64>> m_sent;
};

#endif
