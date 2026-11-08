// SPDX-License-Identifier: MIT
// The player list as Players shows it (spec 005): one row per lone player or
// group. A group's secondaries are left out here; Players draws them under
// their primary from its memberIds.
#ifndef SWELL_PLAYERLISTMODEL_H
#define SWELL_PLAYERLISTMODEL_H

#include <QSortFilterProxyModel>

namespace Swell {

class PlayerListModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit PlayerListModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

} // namespace Swell

#endif
