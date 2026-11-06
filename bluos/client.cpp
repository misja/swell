// SPDX-License-Identifier: MIT
#include "client.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace bluos {

namespace {

QString describe(QNetworkReply *reply)
{
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::OperationCanceledError)
        return QStringLiteral("timed out");
    if (http && http != 200)
        return QStringLiteral("HTTP %1").arg(http);
    return reply->errorString();
}

} // namespace

Client::Client(QNetworkAccessManager *nam, const QString &host, int port, QObject *parent)
    : QObject(parent)
    , m_nam(nam)
    , m_host(host)
    , m_port(port > 0 ? port : 11000)
{
    m_pollTimer.setSingleShot(true);
    connect(&m_pollTimer, &QTimer::timeout, this, &Client::pollStatus);
    m_positionTimer.setInterval(1000);
    connect(&m_positionTimer, &QTimer::timeout, this, &Client::positionChanged);
}

Client::~Client()
{
    stop();
}

void Client::setTiming(const Timing &timing)
{
    m_timing = timing;
}

QString Client::id() const
{
    return QStringLiteral("%1:%2").arg(m_host).arg(m_port);
}

int Client::position() const
{
    if (!m_status.ok || m_status.secs < 0)
        return -1;
    // The API requires clients to advance the position themselves while playing.
    int secs = m_status.secs;
    if (m_status.isPlaying() && m_statusAge.isValid())
        secs += int(m_statusAge.elapsed() / 1000);
    if (m_status.totlen > 0 && secs > m_status.totlen)
        secs = m_status.totlen;
    return secs;
}

QUrl Client::resolveImage(const QString &image) const
{
    if (image.isEmpty())
        return QUrl();
    const QUrl base = url(QStringLiteral("/"));
    QUrl resolved = base.resolved(QUrl(image));
    if (resolved.host() == base.host() && resolved.port() == base.port()) {
        QUrlQuery query(resolved);
        if (!query.hasQueryItem(QStringLiteral("followRedirects")))
            query.addQueryItem(QStringLiteral("followRedirects"), QStringLiteral("1"));
        resolved.setQuery(query);
    }
    return resolved;
}

void Client::start()
{
    if (m_running)
        return;
    m_running = true;
    m_backoffMs = 0;
    fetchSyncStatus();
    schedulePoll(0);
}

void Client::stop()
{
    m_running = false;
    m_pollTimer.stop();
    m_positionTimer.stop();
    if (m_statusReply)
        m_statusReply->abort();
    if (m_syncReply)
        m_syncReply->abort();
    for (Gate &gate : m_gates) {
        if (gate.timer)
            gate.timer->stop();
        gate.hasPending = false;
    }
}

void Client::play() { send(QStringLiteral("/Play"), QUrlQuery()); }
void Client::pause() { send(QStringLiteral("/Pause"), QUrlQuery()); }
void Client::skip() { send(QStringLiteral("/Skip"), QUrlQuery()); }
void Client::back() { send(QStringLiteral("/Back"), QUrlQuery()); }

void Client::setVolume(int level)
{
    if (fixedVolume())
        return;
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("level"), QString::number(qBound(0, level, 100)));
    send(QStringLiteral("/Volume"), query);
}

void Client::setMuted(bool muted)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("mute"), muted ? QStringLiteral("1") : QStringLiteral("0"));
    send(QStringLiteral("/Volume"), query);
}

QUrl Client::url(const QString &path, const QUrlQuery &query) const
{
    QUrl u;
    u.setScheme(QStringLiteral("http"));
    u.setHost(m_host);
    u.setPort(m_port);
    u.setPath(path);
    if (!query.isEmpty())
        u.setQuery(query);
    return u;
}

int Client::msUntilAllowed(const QString &path)
{
    const Gate &gate = m_gates[path];
    if (!gate.lastSent.isValid())
        return 0;
    const qint64 wait = m_timing.minIntervalMs - gate.lastSent.elapsed();
    return wait > 0 ? int(wait) : 0;
}

// Commands go through a per-path gate: at most one request per path per
// minimum interval. While a command waits, a newer one for the same path
// replaces it, so a burst of volume changes ends at the last value.
void Client::send(const QString &path, const QUrlQuery &query)
{
    Gate &gate = m_gates[path];
    gate.pending = query;
    gate.hasPending = true;
    const int wait = msUntilAllowed(path);
    if (wait == 0) {
        dispatch(path);
        return;
    }
    if (!gate.timer) {
        gate.timer = new QTimer(this);
        gate.timer->setSingleShot(true);
        connect(gate.timer, &QTimer::timeout, this, [this, path]() { dispatch(path); });
    }
    if (!gate.timer->isActive())
        gate.timer->start(wait);
}

void Client::dispatch(const QString &path)
{
    Gate &gate = m_gates[path];
    if (!gate.hasPending)
        return;
    const QUrlQuery query = gate.pending;
    gate.hasPending = false;
    QNetworkReply *reply = get(path, query, m_timing.commandTimeoutMs);
    connect(reply, &QNetworkReply::finished, this, [this, reply, path]() { onCommandReply(reply, path); });
}

QNetworkReply *Client::get(const QString &path, const QUrlQuery &query, int deadlineMs)
{
    m_gates[path].lastSent.start();
    QNetworkRequest request(url(path, query));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = m_nam->get(request);
    // Qt 5.6 has no transfer timeout; abort() at the deadline instead.
    QTimer *deadline = new QTimer(reply);
    deadline->setSingleShot(true);
    connect(deadline, &QTimer::timeout, reply, &QNetworkReply::abort);
    deadline->start(deadlineMs);
    return reply;
}

void Client::schedulePoll(int delayMs)
{
    if (!m_running)
        return;
    const int wait = qMax(delayMs, msUntilAllowed(QStringLiteral("/Status")));
    m_pollTimer.start(wait);
}

void Client::pollStatus()
{
    if (!m_running || m_statusReply)
        return;
    QUrlQuery query;
    int deadline = m_timing.commandTimeoutMs;
    // Long-poll only once there is an etag to wait on.
    if (m_status.ok && !m_status.etag.isEmpty()) {
        query.addQueryItem(QStringLiteral("timeout"), QString::number(m_timing.pollTimeoutSecs));
        query.addQueryItem(QStringLiteral("etag"), m_status.etag);
        deadline = m_timing.pollTimeoutSecs * 1000 + m_timing.deadlineGraceMs;
    }
    QNetworkReply *reply = get(QStringLiteral("/Status"), query, deadline);
    m_statusReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onStatusReply(reply); });
}

void Client::onStatusReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (m_statusReply == reply)
        m_statusReply = nullptr;
    if (!m_running)
        return;

    if (reply->error() != QNetworkReply::NoError) {
        const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        failed(http ? NotAPlayer : Offline);
        return;
    }
    const Status status = parseStatus(reply->readAll());
    if (!status.ok) {
        failed(NotAPlayer);
        return;
    }
    succeeded();

    const bool syncChanged = status.syncStat != m_status.syncStat;
    const bool changed = !m_status.ok || status.etag != m_status.etag;
    // secs is outside the etag; keep the position fresh either way.
    m_status = status;
    m_statusAge.start();
    if (m_status.isPlaying())
        m_positionTimer.start();
    else
        m_positionTimer.stop();
    if (changed)
        emit statusChanged();
    emit positionChanged();
    if (syncChanged && !m_syncReply)
        fetchSyncStatus();
    schedulePoll(0);
}

void Client::fetchSyncStatus()
{
    if (!m_running || m_syncReply)
        return;
    const int wait = msUntilAllowed(QStringLiteral("/SyncStatus"));
    if (wait > 0) {
        QTimer::singleShot(wait, this, &Client::fetchSyncStatus);
        return;
    }
    QNetworkReply *reply = get(QStringLiteral("/SyncStatus"), QUrlQuery(), m_timing.commandTimeoutMs);
    m_syncReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onSyncStatusReply(reply); });
}

void Client::onSyncStatusReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (m_syncReply == reply)
        m_syncReply = nullptr;
    if (!m_running || reply->error() != QNetworkReply::NoError)
        return;  // the status poll owns availability and retries
    const SyncStatus sync = parseSyncStatus(reply->readAll());
    if (!sync.ok)
        return;
    m_sync = sync;
    emit syncStatusChanged();
    // The status may have moved on while this was in flight.
    if (m_status.ok && !m_status.syncStat.isEmpty() && m_status.syncStat != m_sync.syncStat)
        fetchSyncStatus();
}

void Client::onCommandReply(QNetworkReply *reply, const QString &path)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
        emit commandFailed(path, describe(reply));
    // A queued newer value for this path is sent once the gate allows it.
    Gate &gate = m_gates[path];
    if (gate.hasPending && !(gate.timer && gate.timer->isActive()))
        send(path, gate.pending);
}

void Client::succeeded()
{
    m_backoffMs = 0;
    setAvailability(Online);
    if (!m_sync.ok)
        fetchSyncStatus();
}

void Client::failed(Availability availability)
{
    setAvailability(availability);
    m_positionTimer.stop();
    m_backoffMs = m_backoffMs ? qMin(m_backoffMs * 2, m_timing.backoffMaxMs) : m_timing.backoffInitialMs;
    // The etag of a player that went away is meaningless after it returns.
    const bool hadStatus = m_status.ok;
    m_status = Status();
    if (hadStatus)
        emit statusChanged();
    schedulePoll(m_backoffMs);
}

void Client::setAvailability(Availability availability)
{
    if (m_availability == availability)
        return;
    m_availability = availability;
    emit availabilityChanged();
}

} // namespace bluos
