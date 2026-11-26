// SPDX-License-Identifier: MIT
// The player list as the pages show it (spec 005). PlayerListModel: one row
// per lone player or group, for Players; a group's secondaries are left out
// there, and Players draws them under their primary from its memberIds.
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

// The current player and the members of its group (Now Playing, Per
// player), in the order of the player model. The source is a
// BluOS::PlayerModel.
class CurrentGroupModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit CurrentGroupModel(QObject *parent = nullptr);
    void setSourceModel(QAbstractItemModel *model) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

} // namespace Swell

#endif
