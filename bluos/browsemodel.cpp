// SPDX-License-Identifier: MIT
#include "browsemodel.h"

#include "client.h"

#include <QTimer>

namespace BluOS {

const char *const BrowseModel::FavouritesKey = "LocalMusic:MG/LocalMusic-Favourites";

namespace {
// A page that failed is asked for again only after this pause.
const int RetryAfterMs = 3000;
// In a plain list, the next page is read when a row this close to the end is shown.
const int ReadAhead = 10;

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
}

bool BrowseModel::knownAction(const QString &type)
{
    static const QStringList known = {
        QStringLiteral("add-now"), QStringLiteral("add-next"), QStringLiteral("add-last"),
        QStringLiteral("addAll-now"), QStringLiteral("addAll-next"), QStringLiteral("addAll-last"),
        QStringLiteral("add-shuffle"), QStringLiteral("favourite-add"), QStringLiteral("favourite-delete"),
    };
    return known.contains(type);
}

void BrowseModel::setClient(Client *client)
{
    if (m_client == client)
        return;
    m_client = client;
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

QVariant BrowseModel::actionsOf(const BrowseItem &item) const
{
    QVariantList list;
    for (const BrowseAction &a : item.actions) {
        if (!knownAction(a.type))
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("type"), a.type);
        m.insert(QStringLiteral("text"), a.text);
        m.insert(QStringLiteral("url"), a.actionUrl);
        list.append(m);
    }
    return list;
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
            case Text2Role: return m_favouritesName;
            case BrowseKeyRole: return QString::fromLatin1(FavouritesKey);
            case LoadedRole: return true;
            case FavouritesRole: return true;
            default: return QVariant();
            }
        }
        --row;
    }
    int offset = 0;
    const int segment = segmentOf(row, &offset);
    if (segment < 0)
        return QVariant();
    const Segment &s = m_segments.at(segment);
    const Row &r = s.rows.at(offset);
    if (!r.loaded) {
        want(segment);
    } else if (!m_sectioned && !s.done && !s.nextKey.isEmpty() && offset >= s.rows.size() - ReadAhead) {
        want(segment);
    }
    switch (role) {
    case TypeRole: return r.item.type;
    case TextRole: return r.item.text;
    case Text2Role: return r.item.text2;
    case ImageRole: return r.image;
    case BrowseKeyRole: return r.item.browseKey;
    case PlayUrlRole: return r.item.playUrl;
    case AutoplayUrlRole: return r.item.autoplayUrl;
    case DurationRole: return r.item.duration;
    case ActionsRole: return r.loaded ? actionsOf(r.item) : QVariant(QVariantList());
    case SectionRole: return s.title;
    case LoadedRole: return r.loaded;
    case FavouritesRole: return false;
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
        { FavouritesRole, "favourites" },
    };
}

QVariantList BrowseModel::letters() const
{
    QVariantList list;
    if (!m_sectioned)
        return list;
    for (int i = 0; i < m_segments.size(); ++i) {
        if (m_segments.at(i).rows.isEmpty())
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("text"), m_segments.at(i).title);
        m.insert(QStringLiteral("row"), firstRow(i));
        list.append(m);
    }
    return list;
}

void BrowseModel::load()
{
    if (!m_client)
        return;
    ++m_generation;
    m_inFlight = -2;  // the first read, not a segment
    m_wanted = -1;
    m_failed.clear();
    emit loadingChanged();
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    m_client->fetchBrowse(m_key, m_query, [self, generation](const Browse &reply) {
        if (!self || generation != self->m_generation)
            return;  // the page went away, or it was read again
        self->m_inFlight = -1;
        self->m_wanted = -1;
        self->onFirst(reply);
        emit self->loadingChanged();
    });
}

void BrowseModel::onFirst(const Browse &reply)
{
    m_reply = reply;
    m_reply.items.clear();
    m_error = reply.ok ? QString() : reply.error;
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
            for (const BrowseItem &item : reply.items) {
                Segment s;
                s.key = item.browseKey;
                s.title = item.text;
                s.rows = QVector<Row>(countOf(item.text2));
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
                    continue;  // inputs (on Home) and play-only hand-offs
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
    m_client->fetchBrowse(QStringLiteral("LocalMusic:"), QString(), [self, generation](const Browse &menu) {
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

void BrowseModel::want(int segment) const
{
    if (!m_client || segment < 0 || segment >= m_segments.size() || segment == m_inFlight)
        return;
    const Segment &s = m_segments.at(segment);
    if (s.done && s.loaded >= s.rows.size())
        return;
    if (m_failed.contains(segment) && m_failed.value(segment).elapsed() < RetryAfterMs)
        return;
    if (m_inFlight != -1) {
        m_wanted = segment;  // only the latest wish is kept
        return;
    }
    // Called from data() while the view builds delegates: send it later.
    m_inFlight = segment;
    BrowseModel *self = const_cast<BrowseModel *>(this);
    const int generation = m_generation;
    QTimer::singleShot(0, self, [self, generation, segment]() {
        if (generation == self->m_generation && self->m_inFlight == segment)
            self->request(segment);
    });
}

void BrowseModel::request(int segment)
{
    if (!m_client) {
        m_inFlight = -1;
        return;
    }
    emit loadingChanged();
    const Segment &s = m_segments.at(segment);
    const QString key = s.started ? s.nextKey : s.key;
    const int generation = m_generation;
    QPointer<BrowseModel> self(this);
    m_client->fetchBrowse(key, QString(), [self, generation, segment](const Browse &reply) {
        if (self)
            self->onPage(generation, segment, reply);
    });
}

void BrowseModel::onPage(int generation, int segment, const Browse &reply)
{
    if (generation != m_generation)
        return;
    m_inFlight = -1;
    if (!reply.ok) {
        m_failed[segment].start();
    } else {
        m_failed.remove(segment);
        Segment &s = m_segments[segment];
        s.started = true;
        s.nextKey = reply.nextKey;
        s.done = reply.nextKey.isEmpty();
        const int first = firstRow(segment);
        const int start = s.loaded;
        int filled = 0;
        for (const BrowseItem &item : reply.items) {
            Row r;
            r.loaded = true;
            r.item = item;
            r.image = m_client ? m_client->resolveImage(item.image) : QUrl();
            if (s.loaded < s.rows.size()) {
                s.rows[s.loaded] = r;
                ++filled;
            } else {
                // More than announced: trust the reply.
                beginInsertRows(QModelIndex(), first + s.loaded, first + s.loaded);
                s.rows.append(r);
                endInsertRows();
            }
            ++s.loaded;
        }
        if (filled > 0)
            emit dataChanged(index(first + start), index(first + start + filled - 1));
        if (s.done && s.loaded < s.rows.size()) {
            // Fewer than announced: drop the rest of this letter.
            beginRemoveRows(QModelIndex(), first + s.loaded, first + s.rows.size() - 1);
            s.rows.resize(s.loaded);
            endRemoveRows();
            emit lettersChanged();
        } else if (!s.done && reply.items.isEmpty()) {
            m_failed[segment].start();  // a page without rows: do not loop
        }
    }
    emit loadingChanged();
    if (m_wanted >= 0) {
        const int next = m_wanted;
        m_wanted = -1;
        want(next);
    }
}

} // namespace BluOS
