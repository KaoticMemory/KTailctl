// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "peer_ping.hpp"

#include <KLocalizedString>
#include <QByteArray>
#include <QFuture>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QMessageLogger>
#include <QStringLiteral>
#include <QtConcurrentRun>
#include <cstdlib>
#include <cstring>
#include <libktailctl_wrapper.h>
#include <memory>

#include "logging_tailscale_ping.hpp"
#include "peer_status.hpp"
#include "status.hpp"
#include "tailscale.hpp"

namespace
{
[[nodiscard]] auto runPing(const QString &address) -> QString
{
    const QByteArray utf8 = address.toUtf8();
    GoString goAddress;
    goAddress.p = utf8.constData();
    goAddress.n = static_cast<GoInt>(utf8.size());

    const std::unique_ptr<char, decltype(&free)> payload(tailscale_ping(&goAddress), &free);
    if (!payload) {
        return {};
    }
    return QString::fromUtf8(payload.get());
}
} // namespace

PeerPing::PeerPing(Tailscale *tailscale, QObject *parent)
    : QObject(parent)
    , mTailscale(tailscale)
    , mResult(new PingResult(this))
    , mWatcher(new QFutureWatcher<QString>(this))
{
    connect(mWatcher, &QFutureWatcher<QString>::finished, this, &PeerPing::onAttemptFinished);
}

auto PeerPing::pingPeer(const QString &peerId) -> bool
{
    if (mRunning) {
        return false;
    }
    if (mTailscale == nullptr || mTailscale->status() == nullptr) {
        mPeerId = peerId;
        mAddress = QString();
        mResult->setFailure(i18n("Tailscale status is not available"));
        mHasResult = true;
        Q_EMIT finished(false);
        return false;
    }

    const PeerStatus *peer = mTailscale->status()->peerWithId(peerId);
    if (peer == nullptr) {
        mPeerId = peerId;
        mAddress = QString();
        mResult->setFailure(i18n("Peer is no longer in the tailnet"));
        mHasResult = true;
        Q_EMIT finished(false);
        return false;
    }

    const QString address = selectPingAddress(peer->tailscaleIps());
    if (address.isEmpty()) {
        mPeerId = peerId;
        mAddress = QString();
        mResult->setFailure(i18n("Peer has no Tailscale address"));
        mHasResult = true;
        Q_EMIT finished(false);
        return false;
    }
    return pingAddress(peerId, address);
}

auto PeerPing::pingAddress(const QString &peerId, const QString &address) -> bool
{
    if (mRunning) {
        return false;
    }
    if (address.isEmpty()) {
        mPeerId = peerId;
        mAddress = QString();
        mResult->setFailure(i18n("Peer has no Tailscale address"));
        mHasResult = true;
        Q_EMIT finished(false);
        return false;
    }
    begin(peerId, address);
    return true;
}

void PeerPing::begin(const QString &peerId, const QString &address)
{
    mPeerId = peerId;
    mAddress = address;
    mAttempt = 1;
    mHasResult = false;
    mRunning = true;
    qCInfo(Logging::Tailscale::Ping) << "Pinging" << address << "for peer" << peerId;
    startAttempt();
}

void PeerPing::startAttempt()
{
    const QString address = mAddress.value();
    mWatcher->setFuture(QtConcurrent::run([address]() -> QString {
        return runPing(address);
    }));
}

void PeerPing::onAttemptFinished()
{
    const QString payload = mWatcher->result();
    if (payload.isEmpty()) {
        mResult->setFailure(i18n("Ping failed"));
    } else {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8(), &error);
        if ((error.error != QJsonParseError::NoError) || !document.isObject()) {
            qCWarning(Logging::Tailscale::Ping) << "Failed to parse ping response:" << error.errorString();
            mResult->setFailure(i18n("Ping failed"));
        } else {
            mResult->updateFromJson(document.object());
        }
    }
    mHasResult = true;

    if (shouldRetryPing(mResult->path(), mAttempt.value(), maxAttemptCount)) {
        const int nextAttempt = mAttempt.value() + 1;
        qCInfo(Logging::Tailscale::Ping) << "Ping via relay, retrying attempt" << nextAttempt;
        mAttempt = nextAttempt;
        // Queue the next attempt so a ping that returns immediately cannot re-enter this slot.
        QMetaObject::invokeMethod(
            this,
            [this] {
                startAttempt();
            },
            Qt::QueuedConnection);
        return;
    }

    const bool success = (mResult->path() == PingResult::Path::Direct) || (mResult->path() == PingResult::Path::Derp)
        || (mResult->path() == PingResult::Path::PeerRelay);
    qCInfo(Logging::Tailscale::Ping) << "Ping finished:" << mResult->summary();
    finish(success);
}

void PeerPing::finish(const bool success)
{
    mRunning = false;
    Q_EMIT finished(success);
}
