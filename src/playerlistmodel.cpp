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

} // namespace Swell
