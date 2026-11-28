// SPDX-License-Identifier: MIT
#include "browsemodel.h"

#include "client.h"

#include <QTimer>

namespace BluOS {

const char *const BrowseModel::FavouritesKey = "LocalMusic:MG/LocalMusic-Favourites";

namespace {
// A page that failed is asked for again only after this pause.
const int RetryAfterMs = 3000;

// The leading number of a section's text2 ("65"), or -1.
int countOf(const QString &text2)
{
    int i = 0;
    while (i < text2.size() && text2.at(i).isDigit())
        ++i;
    bool ok = false;
    const int n = text2.left(i).toInt(&ok);
    return ok ? n : -1;
}
}

BrowseModel::BrowseModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(this, &QAbstractItemModel::modelReset, this, &BrowseModel::countChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &BrowseModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &BrowseModel::countChanged);
}

bool BrowseModel::knownAction(const QString &type)
{
    static const QStringList known = {
        QStringLiteral("add-now"), QStringLiteral("add-next"), QStringLiteral("add-last"),
        QStringLiteral("addAll-now"), QStringLiteral("addAll-next"), QStringLiteral("addAll-last"),
        QStringLiteral("add-shuffle"), QStringLiteral("favourite-add"), QStringLiteral("favourite-delete"),
        // Not in the API's list; the NAD sends it on stations (spec 008).
        QStringLiteral("preset"),
    };
    return known.contains(type);
}

bool BrowseModel::startsPlayback(const QString &type) const
{
    return type == QLatin1String("add-now") || type == QLatin1String("addAll-now")
            || type == QLatin1String("add-shuffle");
}

bool BrowseModel::changesFavourites(const QString &type) const
{
    return type == QLatin1String("favourite-add") || type == QLatin1String("favourite-delete");
}

bool BrowseModel::favouriteAlbums() const
{
    return m_favouritesKey.startsWith(QLatin1String("LocalMusic:Album/"));
}

void BrowseModel::setClient(Client *client)
{
    if (m_client == client)
        return;
    if (m_client)
        disconnect(m_client, nullptr, this, nullptr);
    m_client = client;
    // A read that failed while the player was away is tried again when it is
    // back; the page itself reloads only if its first read failed.
    if (m_client) {
        connect(m_client, &Client::availabilityChanged, this, [this]() {
            if (!m_client->online() || m_backoff.isEmpty())
                return;
            m_backoff.clear();
            if (m_sectioned)
                next();
            else if (canFetchMore(QModelIndex()))
                request(0);
        });
    }
    emit clientChanged();
}

void BrowseModel::setKey(const QString &key)
{
    if (m_key == key)
        return;
    m_key = key;
    emit keyChanged();
}

void BrowseModel::setQuery(const QString &query)
{
    if (m_query == query)
        return;
    m_query = query;
    emit keyChanged();
}

void BrowseModel::setRoot(bool root)
{
    if (m_root == root)
        return;
    m_root = root;
    emit keyChanged();
}

int BrowseModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    int n = m_favourites ? 1 : 0;
    for (const Segment &s : m_segments)
        n += s.rows.size();
    return n;
}

int BrowseModel::segmentOf(int row, int *offset) const
{
    for (int i = 0; i < m_segments.size(); ++i) {
        const int size = m_segments.at(i).rows.size();
        if (row < size) {
            *offset = row;
            return i;
        }
        row -= size;
    }
    return -1;
}

int BrowseModel::firstRow(int segment) const
{
    int row = m_favourites ? 1 : 0;
    for (int i = 0; i < segment && i < m_segments.size(); ++i)
        row += m_segments.at(i).rows.size();
    return row;
}

QList<BrowseAction> BrowseModel::actionsOf(const BrowseItem &item)
{
    QList<BrowseAction> list;
    for (const BrowseAction &a : item.actions) {
        if (knownAction(a.type))
            list.append(a);
    }
    return list;
}

QString BrowseModel::presetUrlOf(const QString &contextMenuKey)
{
    const int question = contextMenuKey.indexOf(QLatin1Char('?'));
    if (question < 0)
        return QString();
    const QString prefix = QStringLiteral("preset_url=");
    QString found;
    int count = 0;
    for (const QString &part : contextMenuKey.mid(question + 1).split(QLatin1Char('&'))) {
        if (part.startsWith(prefix)) {
            found = part.mid(prefix.size());
            ++count;
        }
    }
    return count == 1 ? found : QString();
}

QVariant BrowseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();
    int row = index.row();
    if (m_favourites) {
        if (row == 0) {
            switch (role) {
            case TypeRole: return QStringLiteral("link");
            case TextRole: return QString();
            case Text2Role: return m_favouritesName;
            case ImageRole: return QUrl();
            case BrowseKeyRole: return QString::fromLatin1(FavouritesKey);
            case PlayUrlRole: return QString();
            case AutoplayUrlRole: return QString();
            case DurationRole: return -1;
            case ActionsRole: return QVariant::fromValue(QList<BrowseAction>());
            case SectionRole: return QString();
            case LoadedRole: return true;
            case FavouritesRole: return true;
            }
            return QVariant();
        }
        --row;
    }
    int offset = 0;
    const int segment = segmentOf(row, &offset);
    if (segment < 0)
        return QVariant();
    const Segment &s = m_segments.at(segment);
    // Known without reading the row: the ListView reads the section of rows
    // beside the ones it shows (the letter index's currentSection).
    if (role == SectionRole)
        return s.title;
    const Row &r = s.rows.at(offset);
    switch (role) {
    case TypeRole: return r.item.type;
    case TextRole: return r.item.text;
    case Text2Role: return r.item.text2;
    case ImageRole: return r.image;
    case BrowseKeyRole: return r.item.browseKey;
    case PlayUrlRole: return r.item.playUrl;
    case AutoplayUrlRole: return r.item.autoplayUrl;
    case DurationRole: return r.item.duration;
    case ActionsRole: return QVariant::fromValue(r.loaded ? actionsOf(r.item) : QList<BrowseAction>());
    case LoadedRole: return r.loaded;
    case FavouritesRole: return false;
    case PresetRole: {
        if (!r.loaded)
            return QVariant();
        for (const BrowseAction &a : actionsOf(r.item)) {
            if (a.type == QLatin1String("preset"))
                return QVariant();   // the player's own action is offered
        }
        const QString url = presetUrlOf(r.item.contextMenuKey);
        if (url.isEmpty())
            return QVariant();
        return QVariantMap { { QStringLiteral("url"), url }, { QStringLiteral("name"), r.item.text },
                             { QStringLiteral("image"), r.item.image } };
    }
    }
    return QVariant();
}

QHash<int, QByteArray> BrowseModel::roleNames() const
{
    return {
        { TypeRole, "type" },
        { TextRole, "text" },
        { Text2Role, "text2" },
        { ImageRole, "image" },
        { BrowseKeyRole, "browseKey" },
        { PlayUrlRole, "playUrl" },
        { AutoplayUrlRole, "autoplayUrl" },
        { DurationRole, "duration" },
        { ActionsRole, "actions" },
        { SectionRole, "section" },
        { LoadedRole, "loaded" },
        { PresetRole, "preset" },
        { FavouritesRole, "favourites" },
    };
}

QList<BrowseModel::Letter> BrowseModel::letters() const
{
    QList<Letter> list;
    if (!m_sectioned)
        return list;
    for (int i = 0; i < m_segments.size(); ++i) {
        if (!m_segments.at(i).rows.isEmpty())
            list.append({ m_segments.at(i).title, firstRow(i) });
    }
    return list;
}

void BrowseModel::setExpandFirst(bool expand)
{
    if (m_expandFirst == expand)
        return;
    m_expandFirst = expand;
    emit keyChanged();
}

void BrowseModel::load()
{
    if (!m_client)
        return;
    m_expanded = false;
    m_moreLinks.clear();
    readFirst(m_key);
}

void BrowseModel::readFirst(const QString &key)
{
    ++m_generation;
    m_inFlight = -2;  // the first read, not a segment
    m_backoff.clear();
    emit loadingChanged();
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    m_client->fetchBrowse(key, m_expanded ? QString() : m_query, this, [self, generation](const Browse &reply) {
        if (!self || generation != self->m_generation)
            return;  // the page went away, or it was read again
        self->m_inFlight = -1;
        self->onFirst(reply);
        emit self->loadingChanged();
    });
}

void BrowseModel::onFirst(const Browse &reply)
{
    if (m_expandFirst && !m_expanded && reply.ok && reply.type == QLatin1String("menu")
            && !reply.items.isEmpty() && reply.items.first().type == QLatin1String("link")
            && !reply.items.first().browseKey.isEmpty()) {
        // An artist: show its first link (albums) at once, offer the rest.
        m_expanded = true;
        for (int i = 1; i < reply.items.size(); ++i) {
            const BrowseItem &item = reply.items.at(i);
            if (item.type == QLatin1String("link") && !item.browseKey.isEmpty())
                m_moreLinks.append(item);
        }
        readFirst(reply.items.first().browseKey);
        return;
    }
    m_reply = reply;
    m_reply.items.clear();
    m_error = reply.ok ? QString() : reply.error;
    m_playerError = reply.playerError;
    m_sectioned = false;
    beginResetModel();
    m_segments.clear();
    m_favourites = false;
    if (reply.ok) {
        bool sections = reply.type == QLatin1String("sections") && !reply.items.isEmpty();
        for (const BrowseItem &item : reply.items) {
            if (item.type != QLatin1String("section") || item.browseKey.isEmpty() || countOf(item.text2) < 0)
                sections = false;
        }
        if (sections) {
            // Letters with counts: the length is known; the rows load per letter.
            m_sectioned = true;
            int left = MaxListLength;   // for all letters together
            for (const BrowseItem &item : reply.items) {
                Segment s;
                s.key = item.browseKey;
                s.title = item.text;
                const int n = qMin(countOf(item.text2), left);
                left -= n;
                s.rows = QVector<Row>(n);
                m_segments.append(s);
            }
        } else {
            // A plain list (or letters without usable counts, shown as rows).
            Segment s;
            s.started = true;
            s.nextKey = reply.nextKey;
            s.done = reply.nextKey.isEmpty();
            m_segments.append(s);
            QList<BrowseItem> items;
            for (const BrowseItem &item : reply.items) {
                if (m_root && item.type != QLatin1String("link"))
                    continue;  // inputs (see Client::inputs) and play-only hand-offs
                items.append(item);
            }
            setRows(items);
        }
    }
    endResetModel();
    m_ready = true;
    emit readyChanged();
    emit lettersChanged();
    if (m_root && reply.ok)
        findFavourites();
    const QString bySection = QStringLiteral("LocalMusic:bySection:");
    if (m_sectioned && m_favouritesKey.isEmpty() && m_key.startsWith(bySection)) {
        const int slash = m_key.indexOf(QLatin1Char('/'), bySection.size());
        const QString kind = slash > bySection.size() ? m_key.mid(bySection.size(), slash - bySection.size()) : QString();
        // Agreed for Albums (a cover grid) and Artists (rows) only.
        if (kind == QLatin1String("Album") || kind == QLatin1String("Artist"))
            findListFavourites(kind);
    }
}

void BrowseModel::findListFavourites(const QString &kind)
{
    // Observed library structure (not documented): the Library menu offers
    // Favourites, whose links are "LocalMusic:<Kind>/…category=FAVOURITES…".
    // Each step is only followed when the player offers it.
    if (!m_client)
        return;
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    m_client->fetchBrowse(QStringLiteral("LocalMusic:"), QString(), this, [self, generation, kind](const Browse &menu) {
        if (!self || generation != self->m_generation || !menu.ok || !self->m_client)
            return;
        bool offered = false;
        for (const BrowseItem &item : menu.items)
            offered = offered || item.browseKey == QLatin1String(FavouritesKey);
        if (!offered)
            return;
        self->m_client->fetchBrowse(QString::fromLatin1(FavouritesKey), QString(), self,
                                    [self, generation, kind](const Browse &favourites) {
            if (!self || generation != self->m_generation || !favourites.ok)
                return;
            const QString prefix = QStringLiteral("LocalMusic:") + kind + QLatin1Char('/');
            for (const BrowseItem &item : favourites.items) {
                if (item.browseKey.startsWith(prefix) && item.browseKey.contains(QLatin1String("category=FAVOURITES"))) {
                    self->m_favouritesKey = item.browseKey;
                    emit self->favouritesKeyChanged();
                    return;
                }
            }
        });
    });
}

void BrowseModel::setRows(const QList<BrowseItem> &items)
{
    Segment &s = m_segments.last();
    for (const BrowseItem &item : items) {
        Row r;
        r.loaded = true;
        r.item = item;
        r.image = m_client ? m_client->resolveImage(item.image) : QUrl();
        s.rows.append(r);
    }
    s.loaded = s.rows.size();
}

void BrowseModel::findFavourites()
{
    bool library = false;
    for (const Segment &s : m_segments) {
        for (const Row &r : s.rows)
            library = library || r.item.browseKey == QLatin1String("LocalMusic:");
    }
    if (!library || !m_client)
        return;
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    m_client->fetchBrowse(QStringLiteral("LocalMusic:"), QString(), this, [self, generation](const Browse &menu) {
        if (!self || generation != self->m_generation || !menu.ok || self->m_favourites)
            return;
        for (const BrowseItem &item : menu.items) {
            if (item.browseKey == QLatin1String(FavouritesKey)) {
                self->beginInsertRows(QModelIndex(), 0, 0);
                self->m_favourites = true;
                self->m_favouritesName = menu.serviceName;
                self->endInsertRows();
                emit self->lettersChanged();
                return;
            }
        }
    });
}

bool BrowseModel::canFetchMore(const QModelIndex &parent) const
{
    // Not before the first read (no segment) nor during a read (in flight):
    // the view asks once inside endResetModel, before ready is set.
    if (parent.isValid() || !m_client || m_sectioned || m_segments.size() != 1)
        return false;
    return !m_segments.first().done && m_inFlight == -1 && !m_backoff.contains(0);
}

void BrowseModel::fetchMore(const QModelIndex &parent)
{
    // The view calls this without asking canFetchMore again
    // (qqmldelegatemodel.cpp:1104, Qt 5.6.3).
    if (canFetchMore(parent))
        request(0);
}

void BrowseModel::show(int first, int last)
{
    m_showFirst = qMin(first, last);
    m_showLast = qMax(first, last);
    next();
}

void BrowseModel::next()
{
    if (!m_client || !m_sectioned || m_inFlight != -1 || m_showFirst < 0)
        return;
    const int offsetRows = m_favourites ? 1 : 0;
    int offset = 0;
    const int from = segmentOf(qMax(0, m_showFirst - offsetRows), &offset);
    const int to = segmentOf(qMin(rowCount() - 1, m_showLast) - offsetRows, &offset);
    if (from < 0)
        return;
    const int end = to < 0 ? m_segments.size() - 1 : to;
    // The first letter on screen with rows still to load, from where its
    // loading stands (a big letter continues page by page).
    for (int segment = from; segment <= end; ++segment) {
        const Segment &s = m_segments.at(segment);
        const bool more = s.loaded < s.rows.size() || (!s.done && s.started);
        const int unloaded = firstRow(segment) + s.loaded;   // firstRow counts the favourites row
        if (more && unloaded <= m_showLast && !m_backoff.contains(segment)) {
            request(segment);
            return;
        }
    }
}

void BrowseModel::request(int segment)
{
    if (!m_client)
        return;
    m_inFlight = segment;
    emit loadingChanged();
    const Segment &s = m_segments.at(segment);
    const QString key = s.started ? s.nextKey : s.key;
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    m_client->fetchBrowse(key, QString(), this, [self, generation, segment, key](const Browse &reply) {
        if (self)
            self->onPage(generation, segment, key, reply);
    });
}

void BrowseModel::onPage(int generation, int segment, const QString &sentKey, const Browse &reply)
{
    if (generation != m_generation)
        return;
    m_inFlight = -1;
    if (!reply.ok) {
        backOff(segment);
    } else {
        Segment &s = m_segments[segment];
        s.started = true;
        // A reply that points at itself again would only repeat its rows.
        s.nextKey = reply.nextKey == sentKey ? QString() : reply.nextKey;
        s.done = s.nextKey.isEmpty();
        const int first = firstRow(segment);
        const int start = s.loaded;
        QVector<Row> extra;
        for (const BrowseItem &item : reply.items) {
            Row r;
            r.loaded = true;
            r.item = item;
            r.image = m_client ? m_client->resolveImage(item.image) : QUrl();
            if (s.loaded < s.rows.size())
                s.rows[s.loaded] = r;
            else
                extra.append(r);  // more than announced: trust the reply
            ++s.loaded;
        }
        const int filled = reply.items.size() - extra.size();
        if (filled > 0)
            emit dataChanged(index(first + start), index(first + start + filled - 1));
        if (!extra.isEmpty()) {
            const int at = first + s.rows.size();
            beginInsertRows(QModelIndex(), at, at + extra.size() - 1);
            s.rows += extra;
            endInsertRows();
            emit lettersChanged();
        }
        if (s.done && s.loaded < s.rows.size()) {
            // Fewer than announced: drop the rest of this letter.
            beginRemoveRows(QModelIndex(), first + s.loaded, first + s.rows.size() - 1);
            s.rows.resize(s.loaded);
            endRemoveRows();
            emit lettersChanged();
        } else if (!s.done && reply.items.isEmpty()) {
            backOff(segment);  // a page without rows: do not loop
        }
    }
    emit loadingChanged();
    next();
}

void BrowseModel::backOff(int segment)
{
    m_backoff.insert(segment);
    if (!m_client || !m_client->online())
        return;  // the page reloads when the player is back
    // The retry interval itself, not a guess: then the letter (or the next
    // page of a plain list) is read again if it is still wanted.
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    QTimer::singleShot(RetryAfterMs, this, [self, generation, segment]() {
        if (!self || generation != self->m_generation)
            return;
        self->m_backoff.remove(segment);
        if (self->m_sectioned)
            self->next();
        else if (self->canFetchMore(QModelIndex()))
            self->request(segment);
    });
}

} // namespace BluOS
