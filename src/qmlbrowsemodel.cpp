// SPDX-License-Identifier: MIT
#include "qmlbrowsemodel.h"

#include <QVariantMap>

namespace Swell {

QmlBrowseModel::QmlBrowseModel(QObject *parent) : BluOS::BrowseModel(parent)
{
    connect(this, &BluOS::BrowseModel::lettersChanged, this, &QmlBrowseModel::qmlLettersChanged);
    connect(this, &BluOS::BrowseModel::readyChanged, this, &QmlBrowseModel::qmlMoreLinksChanged);
}

QVariant QmlBrowseModel::data(const QModelIndex &index, int role) const
{
    // The base also reads rows in view; keep its results, convert only the type.
    const QVariant value = BluOS::BrowseModel::data(index, role);
    if (role != ActionsRole || value.userType() != qMetaTypeId<QList<BluOS::BrowseAction>>())
        return value;
    QVariantList list;
    for (const BluOS::BrowseAction &a : value.value<QList<BluOS::BrowseAction>>()) {
        QVariantMap m;
        m.insert(QStringLiteral("type"), a.type);
        m.insert(QStringLiteral("text"), a.text);
        m.insert(QStringLiteral("url"), a.actionUrl);
        list.append(m);
    }
    return list;
}

QVariantList QmlBrowseModel::qmlLetters() const
{
    QVariantList list;
    for (const Letter &l : letters()) {
        QVariantMap m;
        m.insert(QStringLiteral("text"), l.text);
        m.insert(QStringLiteral("row"), l.row);
        list.append(m);
    }
    return list;
}

QVariantList QmlBrowseModel::qmlMoreLinks() const
{
    QVariantList list;
    for (const BluOS::BrowseItem &item : moreLinks()) {
        QVariantMap m;
        m.insert(QStringLiteral("text"), item.text);
        m.insert(QStringLiteral("browseKey"), item.browseKey);
        list.append(m);
    }
    return list;
}

} // namespace Swell
