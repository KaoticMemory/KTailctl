// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_NOTIFIER_HPP
#define KTAILCTL_NOTIFIER_HPP

#include <QObject>
#include <qtmetamacros.h>

#include "notification_planner.hpp"

class Tailscale;

// Watches Tailscale status and posts KNotifications. Event ids match
// KTailctl.notifyrc so they can be configured in Plasma's notification settings.
class Notifier : public QObject
{
    Q_OBJECT

public:
    explicit Notifier(Tailscale *tailscale, QObject *parent = nullptr);

Q_SIGNALS:
    void openRequested();

private:
    void check();
    [[nodiscard]] auto capture() const -> TailscaleSnapshot;
    void deliver(const Notice &notice);

    Tailscale *mTailscale;
    NotificationPlanner mPlanner;
};

#endif // KTAILCTL_NOTIFIER_HPP
