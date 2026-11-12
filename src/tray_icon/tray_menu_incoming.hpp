// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_TRAY_MENU_INCOMING_HPP
#define KTAILCTL_TRAY_MENU_INCOMING_HPP

#include <QMenu>
#include <QWidget>
#include <qtmetamacros.h>

#include "tailscale/tailscale.hpp"

class TrayMenuIncoming : public QMenu
{
    Q_OBJECT

    Tailscale *mTailscale;

public:
    explicit TrayMenuIncoming(Tailscale *tailscale, QWidget *parent = nullptr);

    Q_INVOKABLE void rebuildMenu();

private:
    void updateTitle();
};

#endif // KTAILCTL_TRAY_MENU_INCOMING_HPP
