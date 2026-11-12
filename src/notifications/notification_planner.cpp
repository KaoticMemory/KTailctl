// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notification_planner.hpp"

namespace
{
constexpr int maximum_individual_file_notices = 8;

void appendPeerNotices(const QHash<QString, NotificationPeer> &previous, const QHash<QString, NotificationPeer> &current, QList<Notice> *notices)
{
    for (auto it = current.cbegin(); it != current.cend(); ++it) {
        const auto previousIt = previous.constFind(it.key());
        if (previousIt == previous.cend()) {
            notices->append(Notice{NoticeKind::PeerAdded, it->hostName});
            continue;
        }
        if (previousIt->online == it->online) {
            continue;
        }
        notices->append(Notice{it->online ? NoticeKind::PeerOnline : NoticeKind::PeerOffline, it->hostName});
    }

    for (auto it = previous.cbegin(); it != previous.cend(); ++it) {
        if (!current.contains(it.key())) {
            notices->append(Notice{NoticeKind::PeerRemoved, it->hostName});
        }
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void appendFileNotices(const QStringList &previous, const QStringList &current, QList<Notice> *notices)
{
    QStringList arrived;
    for (const QString &name : current) {
        if (!previous.contains(name)) {
            arrived.append(name);
        }
    }
    if (arrived.isEmpty()) {
        return;
    }
    if (arrived.size() > maximum_individual_file_notices) {
        notices->append(Notice{NoticeKind::TaildropFiles, QString::number(arrived.size())});
        return;
    }
    for (const QString &name : arrived) {
        notices->append(Notice{NoticeKind::TaildropFile, name});
    }
}

void appendExitNodeNotice(const TailscaleSnapshot &previous, const TailscaleSnapshot &current, QList<Notice> *notices)
{
    if (previous.exitNodeId == current.exitNodeId) {
        return;
    }
    if (current.exitNodeId.isEmpty()) {
        const QString subject = previous.exitNodeName.isEmpty() ? previous.exitNodeId : previous.exitNodeName;
        notices->append(Notice{NoticeKind::ExitNodeDisabled, subject});
        return;
    }
    const QString subject = current.exitNodeName.isEmpty() ? current.exitNodeId : current.exitNodeName;
    if (previous.exitNodeId.isEmpty()) {
        notices->append(Notice{NoticeKind::ExitNodeEnabled, subject});
        return;
    }
    notices->append(Notice{NoticeKind::ExitNodeChanged, subject});
}
} // namespace

void NotificationPlanner::reset()
{
    mHasBaseline = false;
    mSeenConnected = false;
    mPrevious = {};
}

auto NotificationPlanner::observe(const TailscaleSnapshot &current) -> QList<Notice>
{
    if (!mHasBaseline) {
        mPrevious = current;
        mHasBaseline = true;
        mSeenConnected = current.connected;
        return {};
    }

    // The first time this account is actually up, peers and the exit node are
    // the baseline rather than a burst of "added" notifications. A transition
    // into the connected state is still announced.
    if (current.connected && !mSeenConnected) {
        QList<Notice> notices;
        if (!mPrevious.connected) {
            notices.append(Notice{NoticeKind::Connected, current.selfName});
        }
        if (mPrevious.waitingFilesKnown && current.waitingFilesKnown) {
            appendFileNotices(mPrevious.waitingFiles, current.waitingFiles, &notices);
        }
        adopt(current);
        mSeenConnected = true;
        return notices;
    }

    if (!current.connected) {
        QList<Notice> notices;
        if (mPrevious.connected) {
            notices.append(Notice{NoticeKind::Disconnected, current.selfName});
        }
        if (mPrevious.waitingFilesKnown && current.waitingFilesKnown) {
            appendFileNotices(mPrevious.waitingFiles, current.waitingFiles, &notices);
        }
        // Keep the last peer and exit-node sets so reconnecting does not look
        // like every peer was added again. Only the connection flag changes.
        mPrevious.connected = false;
        if (current.waitingFilesKnown) {
            mPrevious.waitingFiles = current.waitingFiles;
            mPrevious.waitingFilesKnown = true;
        }
        return notices;
    }

    const QList<Notice> notices = diffConnected(current);
    adopt(current);
    mSeenConnected = true;
    return notices;
}

void NotificationPlanner::adopt(const TailscaleSnapshot &current)
{
    // A failed waiting-file query must not look like the queue was emptied.
    const bool keepFiles = mPrevious.waitingFilesKnown && !current.waitingFilesKnown;
    const QStringList keptFiles = mPrevious.waitingFiles;
    mPrevious = current;
    if (keepFiles) {
        mPrevious.waitingFiles = keptFiles;
        mPrevious.waitingFilesKnown = true;
    }
}

auto NotificationPlanner::diffConnected(const TailscaleSnapshot &current) const -> QList<Notice>
{
    QList<Notice> notices;
    if (!mPrevious.connected) {
        notices.append(Notice{NoticeKind::Connected, current.selfName});
    }
    appendExitNodeNotice(mPrevious, current, &notices);
    appendPeerNotices(mPrevious.peers, current.peers, &notices);
    if (mPrevious.waitingFilesKnown && current.waitingFilesKnown) {
        appendFileNotices(mPrevious.waitingFiles, current.waitingFiles, &notices);
    }
    return notices;
}
