// SPDX-License-Identifier: MIT
#include "playerlistmodel.h"

#include "playermodel.h"

namespace Swell {

PlayerListModel::PlayerListModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    // Rows are filtered again whenever a player's group changes.
    setDynamicSortFilter(true);
}

bool PlayerListModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    return sourceModel()->data(index, BluOS::PlayerModel::LeaderIdRole).toString().isEmpty();
}

CurrentGroupModel::CurrentGroupModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

void CurrentGroupModel::setSourceModel(QAbstractItemModel *model)
{
    if (auto *old = qobject_cast<BluOS::PlayerModel *>(sourceModel()))
        disconnect(old, nullptr, this, nullptr);
    QSortFilterProxyModel::setSourceModel(model);
    if (auto *players = qobject_cast<BluOS::PlayerModel *>(model)) {
        connect(players, &BluOS::PlayerModel::currentChanged, this, &CurrentGroupModel::invalidateFilter);
        connect(players, &BluOS::PlayerModel::currentGroupChanged, this, &CurrentGroupModel::invalidateFilter);
    }
}

bool CurrentGroupModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto *players = qobject_cast<const BluOS::PlayerModel *>(sourceModel());
    if (!players)
        return false;
    const QString id = players->data(players->index(sourceRow, 0, sourceParent), BluOS::PlayerModel::IdRole).toString();
    return id == players->currentId() || players->currentMemberIds().contains(id);
}

} // namespace Swell
