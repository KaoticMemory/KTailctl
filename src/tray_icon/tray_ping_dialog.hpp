// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_TRAY_PING_DIALOG_HPP
#define KTAILCTL_TRAY_PING_DIALOG_HPP

#include <QDialog>
#include <QString>

class PeerPing;
class QLabel;

// Identifies which peer a tray ping dialog is talking about. Grouped so the
// three strings cannot be passed in the wrong order.
struct TrayPingRequest {
    QString peerId;
    QString hostName;
    QString address;
};

// Small non-modal dialog used when a ping is started from the tray menu.
// The peer details page shows the same PeerPing result inline.
class TrayPingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrayPingDialog(PeerPing *ping, TrayPingRequest request, QWidget *parent = nullptr);

    void start();

private:
    void refresh();

    PeerPing *mPing;
    QString mPeerId;
    QString mHostName;
    QString mAddress;
    QLabel *mLabel;
};

#endif // KTAILCTL_TRAY_PING_DIALOG_HPP
