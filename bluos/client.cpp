// SPDX-License-Identifier: MIT
#include "client.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>

namespace BluOS {

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
    , m_queue(new QueueModel(this))
{
    // The one-second rule is a "must" (API 2); coarse timers may fire up to 5%
    // early, so every rate-limiting timer is precise and rechecks before sending.
    m_pollTimer.setSingleShot(true);
    m_pollTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_pollTimer, &QTimer::timeout, this, &Client::pollStatus);
    m_positionTimer.setInterval(1000);
    connect(&m_positionTimer, &QTimer::timeout, this, &Client::positionChanged);
    connect(&m_upgradeTimer, &QTimer::timeout, this, &Client::checkUpgrade);
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
    if (resolved.scheme() != QLatin1String("http") && resolved.scheme() != QLatin1String("https"))
        return QUrl();
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
    m_syncBackoffMs = 0;
    if (m_statusPolling) {
        fetchSyncStatus();
        schedulePoll(0);
        m_upgradeTimer.start(m_timing.upgradeCheckIntervalMs);
    }
    watchSyncStatus();
}

void Client::setStatusPolling(bool polling)
{
    if (m_statusPolling == polling)
        return;
    m_statusPolling = polling;
    if (!m_running)
        return;
    if (polling) {
        schedulePoll(0);
        m_upgradeTimer.start(m_timing.upgradeCheckIntervalMs);
        checkUpgrade();
    } else {
        m_pollTimer.stop();
        m_upgradeTimer.stop();
        if (m_statusReply)
            m_statusReply->abort();
    }
}

void Client::stop()
{
    m_running = false;
    m_pollTimer.stop();
    m_positionTimer.stop();
    m_upgradeTimer.stop();
    if (m_statusReply)
        m_statusReply->abort();
    if (m_syncReply)
        m_syncReply->abort();
    if (m_syncWatchReply)
        m_syncWatchReply->abort();
    m_pendingAdds.clear();
    for (Gate &gate : m_gates) {
        if (gate.timer)
            gate.timer->stop();
        gate.hasPending = false;
    }
}

void Client::play() { send(QStringLiteral("/Play"), QString()); }
void Client::pause() { send(QStringLiteral("/Pause"), QString()); }
void Client::skip()
{
    if (!m_status.hasStreamUrl)
        send(QStringLiteral("/Skip"), QString());
    else if (!m_status.skipActionUrl.isEmpty())
        sendAction(m_status.skipActionUrl);
}

void Client::back()
{
    if (!m_status.hasStreamUrl)
        send(QStringLiteral("/Back"), QString());
    else if (!m_status.backActionUrl.isEmpty())
        sendAction(m_status.backActionUrl);
}

void Client::sendAction(const QString &actionUrl)
{
    // A path with its query, from the player itself; only player-relative paths are used.
    QString path;
    QString query;
    if (!splitPlayerPath(actionUrl, &path, &query)) {
        emit commandFailed(actionUrl, QStringLiteral("unsupported action URL"));
        return;
    }
    send(path, query);
}

bool Client::splitPlayerPath(const QString &playerPath, QString *path, QString *query) const
{
    if (!playerPath.startsWith(QLatin1Char('/')) || playerPath.startsWith(QLatin1String("//"))
            || playerPath.contains(QLatin1Char('#'))) {
        return false;
    }
    const int q = playerPath.indexOf(QLatin1Char('?'));
    *path = q < 0 ? playerPath : playerPath.left(q);
    *query = q < 0 ? QString() : playerPath.mid(q + 1);
    const QUrl check = url(*path, *query);
    // Sent byte for byte: anything Qt would rewrite is refused, not guessed.
    return check.isValid() && check.path(QUrl::FullyEncoded) == *path
            && check.query(QUrl::FullyEncoded) == *query;
}

void Client::playPath(const QString &playerPath)
{
    QString path;
    QString query;
    if (!splitPlayerPath(playerPath, &path, &query)) {
        emit commandFailed(playerPath, QStringLiteral("unsupported play URL"));
        return;
    }
    send(path, query);
}

void Client::runAction(const QString &playerPath, const QString &type)
{
    QString path;
    QString query;
    if (!splitPlayerPath(playerPath, &path, &query)) {
        emit commandFailed(playerPath, QStringLiteral("unsupported action URL"));
        return;
    }
    // Not through the merging gate: two adds are two adds.
    fetch(path, query, [this, path, type](const QByteArray &body) {
        const QString error = body.isEmpty() ? QStringLiteral("no answer") : errorIn(body);
        if (error.isEmpty()) {
            emit actionDone(type);
        } else {
            emit commandFailed(path, error);
        }
    });
}

void Client::fetchBrowse(const QString &key, const QString &q, QObject *context,
                         const std::function<void(const Browse &)> &handler)
{
    // Encoded once with every reserved character, as in the section 7.1
    // examples (Tidal%3A, %2FAlbums%3F...); QUrlQuery would leave some alone.
    QStringList items;
    if (!key.isEmpty())
        items << QStringLiteral("key=") + QString::fromLatin1(QUrl::toPercentEncoding(key));
    if (!q.isEmpty())
        items << QStringLiteral("q=") + QString::fromLatin1(QUrl::toPercentEncoding(q));
    items << QStringLiteral("withContextMenuItems=1");
    const QPointer<QObject> guard(context);
    fetch(QStringLiteral("/Browse"), items.join(QLatin1Char('&')), [guard, handler](const QByteArray &body) {
        if (!guard)
            return;
        Browse b = parseBrowse(body);
        if (body.isEmpty()) {
            b.error = QStringLiteral("no answer");
        }
        handler(b);
    }, m_timing.readTimeoutMs, false);
}

void Client::setVolume(int level)
{
    if (fixedVolume())
        return;
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("level"), QString::number(qBound(0, level, 100)));
    send(QStringLiteral("/Volume"), encoded(query));
}

// The group's volume and mute have gates of their own, so they never replace
// (or are replaced by) the player's own volume and mute.
void Client::setGroupVolume(int level)
{
    send(QStringLiteral("/Volume"),
         QStringLiteral("level=%1&tell_slaves=1").arg(qBound(0, level, 100)),
         QStringLiteral("group-level"));
}

void Client::setGroupMuted(bool muted)
{
    send(QStringLiteral("/Volume"),
         muted ? QStringLiteral("mute=1&tell_slaves=1") : QStringLiteral("mute=0&tell_slaves=1"),
         QStringLiteral("group-mute"));
}

Client::GroupRole Client::groupRole() const
{
    if (!m_sync.ok)
        return Alone;
    if (!m_sync.primaryHost.isEmpty()) {
        const QString primary = QStringLiteral("%1:%2").arg(m_sync.primaryHost).arg(m_sync.primaryPort);
        if (primary != id() && primary != m_sync.id)
            return Secondary;
    }
    return m_sync.secondaries.isEmpty() ? Alone : Primary;
}

QString Client::primaryId() const
{
    if (groupRole() != Secondary)
        return QString();
    return QStringLiteral("%1:%2").arg(m_sync.primaryHost).arg(m_sync.primaryPort);
}

void Client::addSecondaries(const QStringList &ids)
{
    m_pendingAdds += ids;
    nextGroupCommand();
}

void Client::nextGroupCommand()
{
    if (m_groupCommandRunning)
        return;
    if (m_pendingAdds.isEmpty()) {
        setGroupBusy(false);
        return;
    }
    const QString target = m_pendingAdds.takeFirst();
    const int colon = target.lastIndexOf(QLatin1Char(':'));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("slave"), colon > 0 ? target.left(colon) : target);
    query.addQueryItem(QStringLiteral("port"), colon > 0 ? target.mid(colon + 1) : QStringLiteral("11000"));
    m_groupCommandRunning = true;
    setGroupBusy(true);
    fetch(QStringLiteral("/AddSlave"), encoded(query), [this, target](const QByteArray &body) {
        m_groupCommandRunning = false;
        const GroupAdd answer = parseGroupAdd(body);
        if (body.isEmpty())
            emit commandFailed(QStringLiteral("/AddSlave"), QStringLiteral("no answer"));
        else if (!answer.ok)
            emit commandFailed(QStringLiteral("/AddSlave"),
                               errorIn(body).isEmpty() ? answer.error : errorIn(body));
        else if (!answer.added.contains(target))
            emit commandFailed(QStringLiteral("/AddSlave"), QStringLiteral("%1 was not added").arg(target));
        fetchSyncStatus();
        nextGroupCommand();
    });
}

void Client::removeSecondaries(const QStringList &ids)
{
    if (ids.isEmpty())
        return;
    QStringList hosts;
    QStringList ports;
    for (const QString &target : ids) {
        const int colon = target.lastIndexOf(QLatin1Char(':'));
        hosts << (colon > 0 ? target.left(colon) : target);
        ports << (colon > 0 ? target.mid(colon + 1) : QStringLiteral("11000"));
    }
    QUrlQuery query;
    const bool one = ids.size() == 1;
    const QLatin1Char comma(',');
    query.addQueryItem(one ? QStringLiteral("slave") : QStringLiteral("slaves"), hosts.join(comma));
    query.addQueryItem(one ? QStringLiteral("port") : QStringLiteral("ports"), ports.join(comma));
    setGroupBusy(true);
    fetch(QStringLiteral("/RemoveSlave"), encoded(query), [this](const QByteArray &body) {
        // The answer is the primary's /SyncStatus (section 8.3).
        const SyncStatus sync = parseSyncStatus(body);
        if (sync.ok)
            takeSyncStatus(sync);
        else
            emit commandFailed(QStringLiteral("/RemoveSlave"),
                               body.isEmpty() ? QStringLiteral("no answer") : errorIn(body));
        setGroupBusy(m_groupCommandRunning || !m_pendingAdds.isEmpty());
    });
}

void Client::setGroupBusy(bool busy)
{
    if (m_groupBusy == busy)
        return;
    m_groupBusy = busy;
    emit groupBusyChanged();
}

void Client::setSyncWatch(bool watch)
{
    if (m_syncWatch == watch)
        return;
    m_syncWatch = watch;
    if (watch)
        watchSyncStatus();
    else if (m_syncWatchReply)
        m_syncWatchReply->abort();
}

void Client::setMuted(bool muted)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("mute"), muted ? QStringLiteral("1") : QStringLiteral("0"));
    send(QStringLiteral("/Volume"), encoded(query));
}

void Client::playPreset(int id)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), QString::number(id));
    send(QStringLiteral("/Preset"), encoded(query));
}

void Client::playInput(const QString &encodedUrl)
{
    // The attribute is already percent-encoded (API 11.1); passing it through
    // QUrlQuery would decode %3A, %2F and friends, so it is sent as given.
    const QString query = QStringLiteral("url=") + encodedUrl;
    const QUrl check = url(QStringLiteral("/Play"), query);
    if (encodedUrl.isEmpty() || !check.isValid() || encodedUrl.contains(QLatin1Char('&'))
            || encodedUrl.contains(QLatin1Char('#')) || check.query(QUrl::FullyEncoded) != query) {
        emit commandFailed(QStringLiteral("/Play"), QStringLiteral("invalid input URL"));
        return;
    }
    send(QStringLiteral("/Play"), query);
}

void Client::refreshInputs()
{
    if (!m_running || m_inputsLoading)
        return;
    m_inputsLoading = true;
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("service"), QStringLiteral("Capture"));
    fetch(QStringLiteral("/RadioBrowse"), encoded(query), [this](const QByteArray &body) {
        m_inputsLoading = false;
        const Inputs inputs = parseCaptureInputs(body);
        if (!inputs.ok)
            return;
        m_inputsFetched = true;
        if (inputs.items != m_inputs) {
            m_inputs = inputs.items;
            emit inputsChanged();
        }
    });
}

void Client::fetchPresets()
{
    if (!m_running || m_presetsLoading)
        return;
    m_presetsLoading = true;
    const QString requestedFor = m_status.prid;
    fetch(QStringLiteral("/Presets"), QString(), [this, requestedFor](const QByteArray &body) {
        m_presetsLoading = false;
        const Presets presets = parsePresets(body);
        if (!presets.ok)
            return;
        m_presetsFetched = true;
        if (presets.items != m_presets) {
            m_presets = presets.items;
            emit presetsChanged();
        }
        // Remember the status prid this answer belongs to, so a /Presets prid
        // that never matches cannot cause endless refetching.
        m_presetsPrid = requestedFor.isEmpty() ? presets.prid : requestedFor;
        // The status may have moved on while this was in flight.
        if (m_status.ok && m_status.prid != requestedFor)
            fetchPresets();
    });
}

void Client::fetch(const QString &path, const QString &encodedQuery,
                   const std::function<void(const QByteArray &)> &handler, int deadlineMs, bool paced)
{
    if (!m_running) {
        // On the next turn of the event loop, so a handler that sends again
        // does not re-enter this function.
        QTimer::singleShot(0, this, [handler]() { handler(QByteArray()); });
        return;
    }
    const int wait = paced ? msUntilAllowed(path) : 0;
    if (wait > 0) {
        QTimer::singleShot(wait, Qt::PreciseTimer, this, [this, path, encodedQuery, handler, deadlineMs, paced]() {
            fetch(path, encodedQuery, handler, deadlineMs, paced);
        });
        return;
    }
    QNetworkReply *reply = get(path, encodedQuery, deadlineMs > 0 ? deadlineMs : m_timing.commandTimeoutMs);
    connect(reply, &QNetworkReply::finished, this, [reply, handler]() {
        reply->deleteLater();
        handler(reply->error() == QNetworkReply::NoError ? reply->readAll() : QByteArray());
    });
}

void Client::fetchQueueWindow(int start, int end, QObject *context,
                              const std::function<void(const Playlist &)> &handler)
{
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("start"), QString::number(start));
    query.addQueryItem(QStringLiteral("end"), QString::number(end));
    const QPointer<QObject> guard(context);
    fetch(QStringLiteral("/Playlist"), encoded(query), [guard, handler](const QByteArray &body) {
        if (guard)
            handler(parsePlaylist(body));
    }, m_timing.readTimeoutMs);
}

void Client::playQueueEntry(int position)
{
    if (position < 0) {
        return;
    }
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), QString::number(position));
    send(QStringLiteral("/Play"), encoded(query));
}

void Client::removeQueueEntry(int position, const QString &pid, const QString &identity)
{
    if (position < 0 || pid.isEmpty() || identity.isEmpty()) {
        emit removeRefused();
        return;
    }
    m_removals.append({ position, pid, identity });
    processRemovals();
}

void Client::processRemovals()
{
    if (m_removing || m_removals.isEmpty()) {
        return;
    }
    m_removing = true;
    const Removal removal = m_removals.first();
    const int generation = m_queue->generation();
    // Positions shift on every delete, and another controller may have changed
    // the queue: ask the player what is at that position right now. The queue
    // must be the one the user saw, or that one changed only by Swell's own
    // deletes (the id changes on every modification, API section 5.1).
    fetchQueueWindow(removal.position, removal.position, this, [this, removal, generation](const Playlist &now) {
        const bool same = now.ok && m_queue->knowsPid(removal.pid) && now.id == m_queue->pid()
                && now.songs.size() == 1 && now.songs.first().position == removal.position
                && now.songs.first().identity() == removal.identity;
        if (!same) {
            m_removals.removeFirst();
            m_removing = false;
            emit removeRefused();
            processRemovals();
            return;
        }
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("id"), QString::number(removal.position));
        m_queue->setHoldPid(true);
        // Not through the merging gate: removals are not idempotent.
        fetch(QStringLiteral("/Delete"), encoded(query), [this, removal, generation](const QByteArray &body) {
            const QString error = body.isEmpty() ? QStringLiteral("no answer") : errorIn(body);
            m_removals.removeFirst();
            if (!error.isEmpty()) {
                emit commandFailed(QStringLiteral("/Delete"), error);
                finishRemoval();
                return;
            }
            if (m_queue->generation() != generation) {
                m_queue->reload();  // read again meanwhile; it may or may not show this
                finishRemoval();
                return;
            }
            m_queue->removeEntry(removal.position);
            for (Removal &later : m_removals) {
                if (later.position > removal.position) {
                    --later.position;
                }
            }
            adoptQueueId(removal.position);
        });
    });
}

void Client::adoptQueueId(int position)
{
    // /Delete answers only <deleted>; read the new id. Take it only if the
    // player's queue is exactly ours minus the deleted row.
    const int length = m_queue->length();
    const int at = qMax(0, qMin(position, length - 1));
    const int generation = m_queue->generation();
    fetchQueueWindow(at, at, this, [this, at, length, generation](const Playlist &now) {
        bool matches = now.ok && !now.id.isEmpty() && now.length == length
                && m_queue->generation() == generation;
        if (matches && length > 0) {
            const QString identity = m_queue->identityAt(at);
            matches = now.songs.size() == 1 && now.songs.first().position == at
                    && !identity.isEmpty() && now.songs.first().identity() == identity;
        }
        if (matches) {
            m_queue->adopt(now.id);
        } else {
            m_queue->reload();
        }
        finishRemoval();
    });
}

void Client::finishRemoval()
{
    m_queue->setHoldPid(false);
    m_removing = false;
    processRemovals();
}

void Client::clearQueue()
{
    fetch(QStringLiteral("/Clear"), QString(), [this](const QByteArray &body) {
        const Playlist cleared = parsePlaylist(body);
        if (cleared.ok) {
            m_queue->resetTo(cleared.id, cleared.length);
        } else {
            emit commandFailed(QStringLiteral("/Clear"), body.isEmpty() ? QStringLiteral("no answer") : errorIn(body));
        }
    });
}

QString Client::errorIn(const QByteArray &body)
{
    QXmlStreamReader xml(body);
    if (!xml.readNextStartElement() || xml.name() != QLatin1String("error")) {
        return QString();
    }
    QString message;
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("message")) {
            message = xml.readElementText();
        } else {
            xml.skipCurrentElement();
        }
    }
    return message.isEmpty() ? QStringLiteral("error") : message;
}

QString Client::encoded(const QUrlQuery &query)
{
    return query.query(QUrl::FullyEncoded);
}

QUrl Client::url(const QString &path, const QString &encodedQuery) const
{
    QUrl u;
    u.setScheme(QStringLiteral("http"));
    u.setHost(m_host);
    u.setPort(m_port);
    u.setPath(path);
    if (!encodedQuery.isEmpty())
        u.setQuery(encodedQuery, QUrl::StrictMode);
    return u;
}

int Client::msUntilAllowed(const QString &path) const
{
    const QElapsedTimer sent = m_lastSent.value(path);
    if (!sent.isValid())
        return 0;
    const qint64 wait = m_timing.minIntervalMs - sent.elapsed();
    return wait > 0 ? int(wait) : 0;
}

// Commands go through a gate. The rate limit is per path (API 2: one request
// per resource per second). Collapsing is per path and parameter, so a burst of
// volume changes ends at the last level, but a mute in between is not lost.
void Client::send(const QString &path, const QString &encodedQuery, const QString &gateName)
{
    const QString name = gateName.isEmpty() ? encodedQuery.section(QLatin1Char('='), 0, 0)
                                            : gateName;
    const QString key = path + QLatin1Char('?') + name;
    Gate &gate = m_gates[key];
    gate.path = path;
    gate.pending = encodedQuery;
    gate.hasPending = true;
    dispatch(key);
}

void Client::dispatch(const QString &key)
{
    Gate &gate = m_gates[key];
    if (!gate.hasPending)
        return;
    const int wait = msUntilAllowed(gate.path);
    if (wait > 0) {
        if (!gate.timer) {
            gate.timer = new QTimer(this);
            gate.timer->setSingleShot(true);
            gate.timer->setTimerType(Qt::PreciseTimer);
            connect(gate.timer, &QTimer::timeout, this, [this, key]() { dispatch(key); });
        }
        if (!gate.timer->isActive())
            gate.timer->start(wait);
        return;
    }
    const QString path = gate.path;
    const QString query = gate.pending;
    gate.hasPending = false;
    QNetworkReply *reply = get(path, query, m_timing.commandTimeoutMs);
    connect(reply, &QNetworkReply::finished, this, [this, reply, path]() { onCommandReply(reply, path); });
}

QNetworkReply *Client::get(const QString &path, const QString &encodedQuery, int deadlineMs)
{
    m_lastSent[path].start();
    QNetworkRequest request(url(path, encodedQuery));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = m_nam->get(request);
    // Owned by this client, so replacing or deleting it aborts what is in flight.
    reply->setParent(this);
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
    if (!m_running || m_statusReply || m_probing)
        return;
    if (m_availability == Updating) {
        probeSyncStatus(Updating);
        return;
    }
    const int wait = msUntilAllowed(QStringLiteral("/Status"));
    if (wait > 0) {
        m_pollTimer.start(wait);
        return;
    }
    QUrlQuery query;
    int deadline = m_timing.commandTimeoutMs;
    // Long-poll only once there is an etag to wait on.
    if (m_status.ok && !m_status.etag.isEmpty()) {
        query.addQueryItem(QStringLiteral("timeout"), QString::number(m_timing.pollTimeoutSecs));
        query.addQueryItem(QStringLiteral("etag"), m_status.etag);
        deadline = m_timing.pollTimeoutSecs * 1000 + m_timing.deadlineGraceMs;
    }
    QNetworkReply *reply = get(QStringLiteral("/Status"), encoded(query), deadline);
    m_statusReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onStatusReply(reply); });
}

void Client::onStatusReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (m_statusReply == reply)
        m_statusReply = nullptr;
    // While the player updates, /SyncStatus decides (probeSyncStatus).
    if (!m_running || m_availability == Updating)
        return;

    if (reply->error() != QNetworkReply::NoError) {
        // A 404 means no control API here; other HTTP errors (a proxy) and
        // network errors are transient.
        const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        probeSyncStatus(http == 404 ? NotAPlayer : Offline);
        return;
    }
    const Status status = parseStatus(reply->readAll());
    if (!status.ok) {
        probeSyncStatus(NotAPlayer);
        return;
    }
    succeeded();

    const bool syncChanged = status.syncStat != m_status.syncStat;
    const bool needPresets = !m_presetsFetched
            || (!status.prid.isEmpty() && status.prid != m_presetsPrid);
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
    if (needPresets)
        fetchPresets();
    m_queue->onStatus(m_status.pid, m_status.song, m_status.hasStreamUrl);
    if (!m_inputsFetched)
        refreshInputs();
    schedulePoll(0);
}

void Client::fetchSyncStatus()
{
    if (!m_running || m_syncReply)
        return;
    const int wait = msUntilAllowed(QStringLiteral("/SyncStatus"));
    if (wait > 0) {
        QTimer::singleShot(wait, Qt::PreciseTimer, this, &Client::fetchSyncStatus);
        return;
    }
    QNetworkReply *reply = get(QStringLiteral("/SyncStatus"), QString(), m_timing.commandTimeoutMs);
    m_syncRequestedFor = m_status.syncStat;
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
    if (!sync.ok) {
        if (sync.upgradeStage > 0)
            enterUpdating();
        return;
    }
    takeSyncStatus(sync);
    // The status may have moved on while this was in flight; compare with what
    // was asked for, not with the answer, which need not echo it.
    if (m_status.ok && m_status.syncStat != m_syncRequestedFor)
        fetchSyncStatus();
}

void Client::takeSyncStatus(const SyncStatus &sync)
{
    m_sync = sync;
    emit syncStatusChanged();
}

void Client::watchSyncStatus()
{
    if (!m_running || !m_syncWatch || m_syncWatchReply)
        return;
    const int wait = msUntilAllowed(QStringLiteral("/SyncStatus"));
    if (wait > 0) {
        QTimer::singleShot(wait, Qt::PreciseTimer, this, &Client::watchSyncStatus);
        return;
    }
    QUrlQuery query;
    int deadline = m_timing.commandTimeoutMs;
    // No long poll across a firmware update's restart.
    if (m_sync.ok && !m_sync.etag.isEmpty() && m_availability != Updating) {
        query.addQueryItem(QStringLiteral("timeout"), QString::number(m_timing.syncPollTimeoutSecs));
        query.addQueryItem(QStringLiteral("etag"), m_sync.etag);
        deadline = m_timing.syncPollTimeoutSecs * 1000 + m_timing.deadlineGraceMs;
    }
    QNetworkReply *reply = get(QStringLiteral("/SyncStatus"), encoded(query), deadline);
    m_syncWatchReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onSyncWatchReply(reply); });
}

void Client::onSyncWatchReply(QNetworkReply *reply)
{
    reply->deleteLater();
    if (m_syncWatchReply == reply)
        m_syncWatchReply = nullptr;
    if (!m_running || !m_syncWatch)
        return;
    const SyncStatus sync = reply->error() == QNetworkReply::NoError ? parseSyncStatus(reply->readAll())
                                                                     : SyncStatus();
    if (!sync.ok) {
        // Without a status poll, this read decides availability.
        if (!m_statusPolling)
            setAvailability(sync.upgradeStage > 0 ? Updating : Offline);
        else if (sync.upgradeStage > 0)
            enterUpdating();
        m_syncBackoffMs = m_syncBackoffMs ? qMin(m_syncBackoffMs * 2, m_timing.backoffMaxMs)
                                          : m_timing.backoffInitialMs;
        QTimer::singleShot(m_syncBackoffMs, this, &Client::watchSyncStatus);
        return;
    }
    m_syncBackoffMs = 0;
    if (!m_statusPolling)
        setAvailability(Online);
    if (!m_sync.ok || sync.etag != m_sync.etag)
        takeSyncStatus(sync);
    watchSyncStatus();
}

void Client::onCommandReply(QNetworkReply *reply, const QString &path)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit commandFailed(path, describe(reply));
        return;
    }
    const QByteArray body = reply->readAll();
    // A command can also fail with an <error> document (API 7.1).
    const QString error = errorIn(body);
    if (!error.isEmpty()) {
        emit commandFailed(path, error);
        return;
    }
    // /Volume answers with the new level: show it before /SyncStatus does.
    if (path == QLatin1String("/Volume") && m_sync.ok) {
        const VolumeReply volume = parseVolume(body);
        if (volume.ok && (volume.level != m_sync.volume || volume.mute != m_sync.mute)) {
            m_sync.volume = volume.level;
            m_sync.mute = volume.mute;
            emit syncStatusChanged();
        }
    }
}

// Not in the API: an updating player answers /SyncStatus with an
// UpgradeStatusStage root, and is silent while it restarts (observed by
// azzurro; /Status is not observed then). So a plain /SyncStatus read decides
// before a failed /Status counts as `otherwise`, and while the player updates;
// no long poll is held across the restart, as in the official app.
void Client::probeSyncStatus(Availability otherwise)
{
    m_probing = true;
    fetch(QStringLiteral("/SyncStatus"), QString(), [this, otherwise](const QByteArray &body) {
        m_probing = false;
        if (!m_running)
            return;
        const SyncStatus sync = parseSyncStatus(body);
        if (sync.upgradeStage > 0 || (m_availability == Updating && !sync.ok)) {
            failed(Updating);
        } else if (m_availability != Updating) {
            failed(otherwise);
        } else if (!m_upgrade.inProgress) {
            leaveUpdating(sync);
        } else {
            // While the update downloads, the player still answers as a
            // player: it is back only once the check no longer says so.
            m_probing = true;
            fetchUpgradeCheck([this, sync](const UpgradeCheck &check) {
                m_probing = false;
                if (check.ok && check.inProgress) {
                    failed(Updating);
                } else {
                    m_upgrade.inProgress = false;  // or no answer: the player is back
                    leaveUpdating(sync);
                }
            });
        }
    });
}

// Back as a player, with its new firmware.
void Client::leaveUpdating(const SyncStatus &sync)
{
    takeSyncStatus(sync);
    succeeded();
    schedulePoll(0);
}

void Client::enterUpdating()
{
    if (m_availability == Updating)
        return;
    failed(Updating);
    if (m_statusReply)
        m_statusReply->abort();
    if (m_syncWatchReply)
        m_syncWatchReply->abort();
}

void Client::setUpgradeChecks(bool checks)
{
    if (m_upgradeChecks == checks)
        return;
    m_upgradeChecks = checks;
    checkUpgrade();
}

QUrl Client::upgradePage() const
{
    QUrl u;
    u.setScheme(QStringLiteral("http"));
    u.setHost(m_host);
    u.setPath(QStringLiteral("/upgrade"));
    // Without the player's own header, as the official app shows it (azzurro).
    u.setQuery(QStringLiteral("noheader=1"));
    return u;
}

void Client::checkUpgrade()
{
    if (!m_running || !m_statusPolling || !m_upgradeChecks || !online() || m_upgradeChecking
            || m_sync.fixedSecondary)
        return;
    fetchUpgradeCheck([this](const UpgradeCheck &check) {
        if (check.ok && check.inProgress)
            enterUpdating();
    });
}

// The one request to /upgrade. Only ever "check": "this" or "all" would
// start an update. No answer, or not one, keeps what was known.
void Client::fetchUpgradeCheck(const std::function<void(const UpgradeCheck &)> &done)
{
    m_upgradeChecking = true;
    fetch(QStringLiteral("/upgrade"), QStringLiteral("upgrade=check"), [this, done](const QByteArray &body) {
        m_upgradeChecking = false;
        if (!m_running)
            return;
        const UpgradeCheck check = parseUpgradeCheck(body);
        if (check.ok) {
            const bool changed = !m_upgrade.ok || check.available != m_upgrade.available
                    || check.version != m_upgrade.version;
            m_upgrade = check;
            if (changed)
                emit upgradeChanged();
        }
        done(check);
    }, m_timing.upgradeTimeoutMs);
}

void Client::succeeded()
{
    const bool wasOnline = online();
    m_backoffMs = 0;
    setAvailability(Online);
    if (!m_sync.ok)
        fetchSyncStatus();
    if (!wasOnline)
        checkUpgrade();
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

} // namespace BluOS
