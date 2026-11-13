// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_PEER_PING_HPP
#define KTAILCTL_PEER_PING_HPP

#include <QFutureWatcher>
#include <QObject>
#include <QObjectBindableProperty>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <qtmetamacros.h>

#include "ping_result.hpp"

class Tailscale;

// One in-flight ping at a time. Disco pings are repeated a few times so a
// direct path that comes up after the first relayed reply can be reported.
class PeerPing : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool running READ running BINDABLE bindableRunning NOTIFY runningChanged)
    Q_PROPERTY(int attempt READ attempt BINDABLE bindableAttempt NOTIFY attemptChanged)
    Q_PROPERTY(int maxAttempts READ maxAttempts CONSTANT)
    Q_PROPERTY(QString peerId READ peerId BINDABLE bindablePeerId NOTIFY peerIdChanged)
    Q_PROPERTY(QString address READ address BINDABLE bindableAddress NOTIFY addressChanged)
    Q_PROPERTY(bool hasResult READ hasResult BINDABLE bindableHasResult NOTIFY hasResultChanged)
    Q_PROPERTY(PingResult *result READ result CONSTANT)

public:
    static constexpr int maxAttemptCount = 3;

    explicit PeerPing(Tailscale *tailscale, QObject *parent = nullptr);

    [[nodiscard]] auto running() const noexcept -> bool
    {
        return mRunning;
    }
    [[nodiscard]] auto attempt() const noexcept -> int
    {
        return mAttempt;
    }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    [[nodiscard]] auto maxAttempts() const noexcept -> int
    {
        return maxAttemptCount;
    }
    [[nodiscard]] auto peerId() const noexcept -> const QString &
    {
        return mPeerId;
    }
    [[nodiscard]] auto address() const noexcept -> const QString &
    {
        return mAddress;
    }
    [[nodiscard]] auto hasResult() const noexcept -> bool
    {
        return mHasResult;
    }
    [[nodiscard]] auto result() const noexcept -> PingResult *
    {
        return mResult;
    }

    [[nodiscard]] auto bindableRunning() -> QBindable<bool>
    {
        return {&mRunning};
    }
    [[nodiscard]] auto bindableAttempt() -> QBindable<int>
    {
        return {&mAttempt};
    }
    [[nodiscard]] auto bindablePeerId() -> QBindable<QString>
    {
        return {&mPeerId};
    }
    [[nodiscard]] auto bindableAddress() -> QBindable<QString>
    {
        return {&mAddress};
    }
    [[nodiscard]] auto bindableHasResult() -> QBindable<bool>
    {
        return {&mHasResult};
    }

    // Returns false when a ping is already running or the peer cannot be pinged.
    Q_INVOKABLE bool pingPeer(const QString &peerId);                                 // NOLINT(modernize-use-trailing-return-type)
    Q_INVOKABLE bool pingAddress(const QString &peerId, const QString &address);       // NOLINT(modernize-use-trailing-return-type)

Q_SIGNALS:
    void runningChanged();
    void attemptChanged();
    void peerIdChanged();
    void addressChanged();
    void hasResultChanged();
    void finished(bool success);

private:
    void begin(const QString &peerId, const QString &address);
    void startAttempt();
    void onAttemptFinished();
    void finish(bool success);

    Tailscale *mTailscale;
    PingResult *mResult;
    QFutureWatcher<QString> *mWatcher;
    Q_OBJECT_BINDABLE_PROPERTY(PeerPing, bool, mRunning, &PeerPing::runningChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PeerPing, int, mAttempt, &PeerPing::attemptChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PeerPing, QString, mPeerId, &PeerPing::peerIdChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PeerPing, QString, mAddress, &PeerPing::addressChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PeerPing, bool, mHasResult, &PeerPing::hasResultChanged)
};

#endif // KTAILCTL_PEER_PING_HPP
