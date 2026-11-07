// SPDX-License-Identifier: MIT
// One level of a player's /Browse hierarchy (BluOS Custom Integration API
// v1.7, sections 7.1 and 7.2) as a list model. The list is made of segments:
// a plain list is one segment that grows through nextKey; an A-Z "sections"
// reply gives one segment per letter whose length is known from its count, so
// the whole list has its length at once and only the part in view is read.
#ifndef BLUOS_BROWSEMODEL_H
#define BLUOS_BROWSEMODEL_H

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QPointer>
#include <QUrl>
#include <QVariantList>
#include <QVector>

#include "client.h"
#include "xml.h"

namespace BluOS {

class BrowseModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(BluOS::Client *client READ client WRITE setClient NOTIFY clientChanged)
    // The key this level was opened with; empty for the top level.
    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
    // Search text; with a key from a searchKey, or alone for a top-level search.
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY keyChanged)
    // The top level: only links, and Favourites first when the library has them.
    Q_PROPERTY(bool root READ root WRITE setRoot NOTIFY keyChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY readyChanged)
    // True when error is the player's own message (an <error> reply).
    Q_PROPERTY(bool playerError READ playerError NOTIFY readyChanged)
    Q_PROPERTY(QString listType READ listType NOTIFY readyChanged)
    Q_PROPERTY(QString serviceName READ serviceName NOTIFY readyChanged)
    Q_PROPERTY(QString searchKey READ searchKey NOTIFY readyChanged)
    Q_PROPERTY(QString playUrl READ playUrl NOTIFY readyChanged)
    Q_PROPERTY(bool sectioned READ sectioned NOTIFY readyChanged)
    // For a letter index: [{ "text": "A", "row": 17 }, ...].
    Q_PROPERTY(QVariantList letters READ letters NOTIFY lettersChanged)
    // Open a menu's first link straight away (an artist: its albums); the
    // other links of that menu are in moreLinks: [{ "text", "browseKey" }].
    Q_PROPERTY(bool expandFirst READ expandFirst WRITE setExpandFirst NOTIFY keyChanged)
    Q_PROPERTY(QVariantList moreLinks READ moreLinks NOTIFY readyChanged)
    // For a library letter list: the key of the matching favourites list
    // (favourite albums for Albums, …), once the library is seen to offer it.
    Q_PROPERTY(QString favouritesKey READ favouritesKey NOTIFY favouritesKeyChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        TextRole,
        Text2Role,
        ImageRole,
        BrowseKeyRole,
        PlayUrlRole,
        AutoplayUrlRole,
        DurationRole,
        ActionsRole,
        SectionRole,
        LoadedRole,
        FavouritesRole
    };

    // The library's favourites, as the Library menu names them (observed on
    // firmware in 2026; not a documented key). Only followed when offered.
    static const char *const FavouritesKey;

    explicit BrowseModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Client *client() const { return m_client; }
    void setClient(Client *client);
    QString key() const { return m_key; }
    void setKey(const QString &key);
    QString query() const { return m_query; }
    void setQuery(const QString &query);
    bool root() const { return m_root; }
    void setRoot(bool root);

    bool ready() const { return m_ready; }
    bool loading() const { return m_inFlight != -1; }
    QString error() const { return m_error; }
    bool playerError() const { return m_playerError; }
    QString listType() const { return m_reply.type; }
    QString serviceName() const { return m_reply.serviceName; }
    QString searchKey() const { return m_reply.searchKey; }
    QString playUrl() const { return m_reply.playUrl; }
    bool sectioned() const { return m_sectioned; }
    QVariantList letters() const;
    // The letter of a row, without reading it (for the letter index).
    Q_INVOKABLE QString sectionAt(int row) const;
    bool expandFirst() const { return m_expandFirst; }
    void setExpandFirst(bool expand);
    QVariantList moreLinks() const { return m_moreLinks; }
    QString favouritesKey() const { return m_favouritesKey; }
    int count() const { return rowCount(); }

    // The context-menu types Swell understands (section 7.1, plus the
    // observed add-shuffle); other actions are not offered.
    static bool knownAction(const QString &type);

public slots:
    // Reads the level (again). Call once the properties are set.
    void load();

signals:
    void clientChanged();
    void keyChanged();
    void readyChanged();
    void loadingChanged();
    void lettersChanged();
    void favouritesKeyChanged();
    void countChanged();

private:
    struct Row {
        bool loaded = false;
        BrowseItem item;
        QUrl image;
    };
    struct Segment {
        QString key;          // the first read
        QString nextKey;      // the next page, once started
        QString title;        // the letter, in a sectioned list
        bool started = false;
        bool done = false;
        int loaded = 0;       // rows filled so far
        QVector<Row> rows;
    };

    void onFirst(const Browse &reply);
    void setRows(const QList<BrowseItem> &items);
    void want(int segment) const;
    void request(int segment);
    void onPage(int generation, int segment, const QString &sentKey, const Browse &reply);
    void askAgain();
    void retryLater(int segment);
    void findFavourites();
    void findListFavourites(const QString &kind);
    void readFirst(const QString &key);
    int segmentOf(int row, int *offset) const;
    int firstRow(int segment) const;
    QVariant actionsOf(const BrowseItem &item) const;

    QPointer<Client> m_client;
    QString m_key;
    QString m_query;
    bool m_root = false;

    Browse m_reply;           // the first reply, without its items
    QVector<Segment> m_segments;
    bool m_sectioned = false;
    bool m_ready = false;
    bool m_favourites = false;
    QString m_favouritesName;
    bool m_expandFirst = false;
    bool m_expanded = false;
    QVariantList m_moreLinks;
    QString m_favouritesKey;
    QString m_error;
    bool m_playerError = false;
    int m_generation = 0;
    mutable int m_inFlight = -1;   // segment being read, or -1
    mutable int m_wanted = -1;     // the latest other wish
    mutable QHash<int, QElapsedTimer> m_failed;
};

} // namespace BluOS

#endif
