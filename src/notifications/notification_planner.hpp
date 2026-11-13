// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_NOTIFICATION_PLANNER_HPP
#define KTAILCTL_NOTIFICATION_PLANNER_HPP

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <stdint.h>

// A peer worth notifying about. Mullvad exit nodes are omitted by the caller.
struct NotificationPeer {
    QString hostName;
    bool online = false;
};

// One observation of the tailnet. waitingFilesKnown is false when the
// waiting-file query failed, so an empty list is not treated as "everything
// was saved".
struct TailscaleSnapshot {
    bool connected = false;
    QString selfName;
    QString exitNodeId;
    QString exitNodeName;
    QHash<QString, NotificationPeer> peers;
    bool waitingFilesKnown = false;
    QStringList waitingFiles;
};

enum class NoticeKind : uint8_t {
    Connected,
    Disconnected,
    ExitNodeEnabled,
    ExitNodeDisabled,
    ExitNodeChanged,
    PeerAdded,
    PeerRemoved,
    PeerOnline,
    PeerOffline,
    TaildropFile,
    TaildropFiles,
};

struct Notice {
    NoticeKind kind = NoticeKind::Connected;
    QString subject;
};

// Turns successive snapshots into notification events. The first observation,
// and the first time the tailnet is seen running after an account switch, do
// not announce peers that were already there.
class NotificationPlanner
{
public:
    void reset();
    [[nodiscard]] auto observe(const TailscaleSnapshot &current) -> QList<Notice>;

private:
    [[nodiscard]] auto diffConnected(const TailscaleSnapshot &current) const -> QList<Notice>;
    void adopt(const TailscaleSnapshot &current);

    bool mHasBaseline = false;
    bool mSeenConnected = false;
    TailscaleSnapshot mPrevious;
};

#endif // KTAILCTL_NOTIFICATION_PLANNER_HPP
