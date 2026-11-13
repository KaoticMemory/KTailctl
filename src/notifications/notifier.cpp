// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifier.hpp"

#include <KLocalizedString>
#include <KNotification>
#include <QModelIndex>
#include <QStringLiteral>

#include "exit_node_status.hpp"
#include "ktailctl_config.h"
#include "peer_status.hpp"
#include "status.hpp"
#include "taildrop.hpp"
#include "tailscale.hpp"
#include "waiting_file.hpp"

namespace
{
[[nodiscard]] auto displayName(const PeerStatus *peer) -> QString
{
    if (peer == nullptr) {
        return {};
    }
    if (!peer->hostName().isEmpty()) {
        return peer->hostName();
    }
    if (!peer->dnsName().isEmpty()) {
        return peer->dnsName();
    }
    return peer->id();
}

[[nodiscard]] auto eventIdFor(const NoticeKind kind) -> QString
{
    switch (kind) {
    case NoticeKind::Connected:
        return QStringLiteral("up");
    case NoticeKind::Disconnected:
        return QStringLiteral("down");
    case NoticeKind::ExitNodeEnabled:
    case NoticeKind::ExitNodeDisabled:
    case NoticeKind::ExitNodeChanged:
        return QStringLiteral("exitNode");
    case NoticeKind::PeerAdded:
        return QStringLiteral("peerAdded");
    case NoticeKind::PeerRemoved:
        return QStringLiteral("peerRemoved");
    case NoticeKind::PeerOnline:
        return QStringLiteral("peerOnline");
    case NoticeKind::PeerOffline:
        return QStringLiteral("peerOffline");
    case NoticeKind::TaildropFile:
    case NoticeKind::TaildropFiles:
        return QStringLiteral("taildropFile");
    }
    return QStringLiteral("up");
}

[[nodiscard]] auto titleFor(const NoticeKind kind) -> QString
{
    switch (kind) {
    case NoticeKind::Connected:
        return i18nc("@title:notification", "Tailscale connected");
    case NoticeKind::Disconnected:
        return i18nc("@title:notification", "Tailscale disconnected");
    case NoticeKind::ExitNodeEnabled:
    case NoticeKind::ExitNodeDisabled:
    case NoticeKind::ExitNodeChanged:
        return i18nc("@title:notification", "Exit node changed");
    case NoticeKind::PeerAdded:
        return i18nc("@title:notification", "Peer added");
    case NoticeKind::PeerRemoved:
        return i18nc("@title:notification", "Peer removed");
    case NoticeKind::PeerOnline:
        return i18nc("@title:notification", "Peer online");
    case NoticeKind::PeerOffline:
        return i18nc("@title:notification", "Peer offline");
    case NoticeKind::TaildropFile:
    case NoticeKind::TaildropFiles:
        return i18nc("@title:notification", "Taildrop");
    }
    return i18nc("@title:notification", "Tailscale");
}

[[nodiscard]] auto textFor(const Notice &notice) -> QString
{
    switch (notice.kind) {
    case NoticeKind::Connected:
        return notice.subject.isEmpty() ? i18n("Tailscale is connected") : i18n("Tailscale is connected as %1", notice.subject);
    case NoticeKind::Disconnected:
        return i18n("Tailscale is disconnected");
    case NoticeKind::ExitNodeEnabled:
        return i18n("Exit node is now %1", notice.subject);
    case NoticeKind::ExitNodeDisabled:
        return notice.subject.isEmpty() ? i18n("Exit node turned off") : i18n("Exit node %1 turned off", notice.subject);
    case NoticeKind::ExitNodeChanged:
        return i18n("Exit node is now %1", notice.subject);
    case NoticeKind::PeerAdded:
        return i18n("%1 was added to your tailnet", notice.subject);
    case NoticeKind::PeerRemoved:
        return i18n("%1 was removed from your tailnet", notice.subject);
    case NoticeKind::PeerOnline:
        return i18n("%1 is online", notice.subject);
    case NoticeKind::PeerOffline:
        return i18n("%1 is offline", notice.subject);
    case NoticeKind::TaildropFile:
        return i18n("%1 is ready to save", notice.subject);
    case NoticeKind::TaildropFiles:
        return i18n("%1 files are ready to save", notice.subject);
    }
    return {};
}

[[nodiscard]] auto iconFor(const NoticeKind kind) -> QString
{
    switch (kind) {
    case NoticeKind::Connected:
        return QStringLiteral("network-vpn");
    case NoticeKind::Disconnected:
        return QStringLiteral("network-disconnect");
    case NoticeKind::ExitNodeEnabled:
    case NoticeKind::ExitNodeDisabled:
    case NoticeKind::ExitNodeChanged:
        return QStringLiteral("globe");
    case NoticeKind::PeerAdded:
    case NoticeKind::PeerOnline:
        return QStringLiteral("network-connect");
    case NoticeKind::PeerRemoved:
    case NoticeKind::PeerOffline:
        return QStringLiteral("network-disconnect");
    case NoticeKind::TaildropFile:
    case NoticeKind::TaildropFiles:
        return QStringLiteral("document-send");
    }
    return QStringLiteral("network-vpn");
}
} // namespace

Notifier::Notifier(Tailscale *tailscale, QObject *parent)
    : QObject(parent)
    , mTailscale(tailscale)
{
    connect(mTailscale->status(), &Status::refreshed, this, &Notifier::check);
    connect(mTailscale, &Tailscale::currentLoginProfileIdChanged, this, [this] {
        mPlanner.reset();
    });
    // The status object has already been refreshed once. Record that as the
    // baseline so existing peers are not announced at startup.
    check();
}

void Notifier::check()
{
    const QList<Notice> notices = mPlanner.observe(capture());
    if (!Config::enableNotifications()) {
        return;
    }
    for (const Notice &notice : notices) {
        deliver(notice);
    }
}

auto Notifier::capture() const -> TailscaleSnapshot
{
    TailscaleSnapshot snapshot;
    const Status *status = mTailscale->status();
    snapshot.connected = status->daemonRunning() && (status->backendState() == Status::BackendState::Running);
    snapshot.selfName = displayName(status->self());

    if (const ExitNodeStatus *exitNode = status->exitNodeStatus()) {
        snapshot.exitNodeId = exitNode->id();
        if (const PeerStatus *peer = status->peerWithId(snapshot.exitNodeId)) {
            snapshot.exitNodeName = displayName(peer);
        }
    }

    const auto *peers = status->peerModel();
    const int rows = peers->rowCount(QModelIndex{});
    for (int row = 0; row < rows; ++row) {
        const PeerStatus *peer = peers->at(row);
        if ((peer == nullptr) || peer->mullvadNode() || peer->id().isEmpty()) {
            continue;
        }
        snapshot.peers.insert(peer->id(), NotificationPeer{displayName(peer), peer->online()});
    }

    // Taildrop already lists waiting files. Skip the snapshot when the daemon
    // is down so an empty model is not treated as "every file was saved".
    if (status->daemonRunning()) {
        snapshot.waitingFilesKnown = true;
        const WaitingFileModel *files = mTailscale->taildrop()->waitingFiles();
        const int fileRows = files->rowCount(QModelIndex{});
        snapshot.waitingFiles.reserve(fileRows);
        for (int row = 0; row < fileRows; ++row) {
            const WaitingFile *file = files->at(row);
            if ((file != nullptr) && !file->name().isEmpty()) {
                snapshot.waitingFiles.append(file->name());
            }
        }
    }

    return snapshot;
}

void Notifier::deliver(const Notice &notice)
{
    const QString text = textFor(notice);
    if (text.isEmpty()) {
        return;
    }

    auto *notification = new KNotification(eventIdFor(notice.kind), KNotification::CloseOnTimeout, this);
    notification->setTitle(titleFor(notice.kind));
    notification->setText(text);
    notification->setIconName(iconFor(notice.kind));
    auto *show = notification->addDefaultAction(i18nc("@action:button", "Show"));
    connect(show, &KNotificationAction::activated, this, &Notifier::openRequested);
    notification->sendEvent();
}
