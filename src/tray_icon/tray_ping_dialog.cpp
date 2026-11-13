// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tray_ping_dialog.hpp"

#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLatin1Char>
#include <QStringLiteral>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

#include "peer_ping.hpp"
#include "ping_result.hpp"

TrayPingDialog::TrayPingDialog(PeerPing *ping, TrayPingRequest request, QWidget *parent)
    : QDialog(parent)
    , mPing(ping)
    , mPeerId(std::move(request.peerId))
    , mHostName(std::move(request.hostName))
    , mAddress(std::move(request.address))
    , mLabel(new QLabel(this))
{
    setWindowTitle(i18nc("@title:window", "Ping %1", mHostName));
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(false);

    mLabel->setWordWrap(true);
    mLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mLabel->setText(i18nc("@info", "Pinging %1…", mHostName));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(mLabel);
    layout->addWidget(buttons);

    connect(mPing, &PeerPing::runningChanged, this, &TrayPingDialog::refresh);
    connect(mPing, &PeerPing::attemptChanged, this, &TrayPingDialog::refresh);
    connect(mPing, &PeerPing::hasResultChanged, this, &TrayPingDialog::refresh);
    connect(mPing->result(), &PingResult::summaryChanged, this, &TrayPingDialog::refresh);
}

void TrayPingDialog::start()
{
    if ((mPing == nullptr) || !mPing->pingAddress(mPeerId, mAddress)) {
        if ((mPing != nullptr) && mPing->running() && (mPing->address() != mAddress)) {
            mLabel->setText(i18n("A ping is already in progress."));
            return;
        }
    }
    refresh();
}

void TrayPingDialog::refresh()
{
    if (mPing == nullptr) {
        return;
    }

    const bool forThisPeer = (mPing->peerId() == mPeerId) && (mPing->address() == mAddress);
    if (!forThisPeer) {
        return;
    }

    if (mPing->running()) {
        QString text = i18nc("@info", "Pinging %1… (attempt %2 of %3)", mHostName, mPing->attempt(), mPing->maxAttempts());
        if (mPing->hasResult() && !mPing->result()->summary().isEmpty()) {
            text += QLatin1Char('\n');
            text += mPing->result()->summary();
        }
        mLabel->setText(text);
        return;
    }

    if (mPing->hasResult()) {
        mLabel->setText(mPing->result()->summary());
    }
}
