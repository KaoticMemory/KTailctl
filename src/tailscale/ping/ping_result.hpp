// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_PING_RESULT_HPP
#define KTAILCTL_PING_RESULT_HPP

#include <QJsonObject>
#include <QObject>
#include <QObjectBindableProperty>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>
#include <qtmetamacros.h>
#include <stdint.h>

// Parsed view of tailscale.com/ipn/ipnstate.PingResult.
class PingResult : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Path : uint8_t {
        Unknown = 0,
        Direct,
        Derp,
        PeerRelay,
        Local,
        Failed,
    };
    Q_ENUM(Path)

    Q_PROPERTY(QString ip READ ip BINDABLE bindableIp NOTIFY ipChanged)
    Q_PROPERTY(QString nodeName READ nodeName BINDABLE bindableNodeName NOTIFY nodeNameChanged)
    Q_PROPERTY(QString errorString READ errorString BINDABLE bindableErrorString NOTIFY errorStringChanged)
    Q_PROPERTY(QString endpoint READ endpoint BINDABLE bindableEndpoint NOTIFY endpointChanged)
    Q_PROPERTY(QString peerRelay READ peerRelay BINDABLE bindablePeerRelay NOTIFY peerRelayChanged)
    Q_PROPERTY(QString derpRegionCode READ derpRegionCode BINDABLE bindableDerpRegionCode NOTIFY derpRegionCodeChanged)
    Q_PROPERTY(int derpRegionId READ derpRegionId BINDABLE bindableDerpRegionId NOTIFY derpRegionIdChanged)
    Q_PROPERTY(int latencyMilliseconds READ latencyMilliseconds BINDABLE bindableLatencyMilliseconds NOTIFY latencyMillisecondsChanged)
    Q_PROPERTY(Path path READ path BINDABLE bindablePath NOTIFY pathChanged)
    Q_PROPERTY(QString summary READ summary BINDABLE bindableSummary NOTIFY summaryChanged)

    explicit PingResult(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void updateFromJson(const QJsonObject &json);
    void setFailure(const QString &message);

    [[nodiscard]] auto ip() const noexcept -> const QString &
    {
        return mIp;
    }
    [[nodiscard]] auto nodeName() const noexcept -> const QString &
    {
        return mNodeName;
    }
    [[nodiscard]] auto errorString() const noexcept -> const QString &
    {
        return mErrorString;
    }
    [[nodiscard]] auto endpoint() const noexcept -> const QString &
    {
        return mEndpoint;
    }
    [[nodiscard]] auto peerRelay() const noexcept -> const QString &
    {
        return mPeerRelay;
    }
    [[nodiscard]] auto derpRegionCode() const noexcept -> const QString &
    {
        return mDerpRegionCode;
    }
    [[nodiscard]] auto derpRegionId() const noexcept -> int
    {
        return mDerpRegionId;
    }
    [[nodiscard]] auto latencyMilliseconds() const noexcept -> int
    {
        return mLatencyMilliseconds;
    }
    [[nodiscard]] auto path() const noexcept -> Path
    {
        return mPath;
    }
    [[nodiscard]] auto summary() const noexcept -> const QString &
    {
        return mSummary;
    }

    [[nodiscard]] auto bindableIp() -> QBindable<QString>
    {
        return {&mIp};
    }
    [[nodiscard]] auto bindableNodeName() -> QBindable<QString>
    {
        return {&mNodeName};
    }
    [[nodiscard]] auto bindableErrorString() -> QBindable<QString>
    {
        return {&mErrorString};
    }
    [[nodiscard]] auto bindableEndpoint() -> QBindable<QString>
    {
        return {&mEndpoint};
    }
    [[nodiscard]] auto bindablePeerRelay() -> QBindable<QString>
    {
        return {&mPeerRelay};
    }
    [[nodiscard]] auto bindableDerpRegionCode() -> QBindable<QString>
    {
        return {&mDerpRegionCode};
    }
    [[nodiscard]] auto bindableDerpRegionId() -> QBindable<int>
    {
        return {&mDerpRegionId};
    }
    [[nodiscard]] auto bindableLatencyMilliseconds() -> QBindable<int>
    {
        return {&mLatencyMilliseconds};
    }
    [[nodiscard]] auto bindablePath() -> QBindable<Path>
    {
        return {&mPath};
    }
    [[nodiscard]] auto bindableSummary() -> QBindable<QString>
    {
        return {&mSummary};
    }

Q_SIGNALS:
    void ipChanged();
    void nodeNameChanged();
    void errorStringChanged();
    void endpointChanged();
    void peerRelayChanged();
    void derpRegionCodeChanged();
    void derpRegionIdChanged();
    void latencyMillisecondsChanged();
    void pathChanged();
    void summaryChanged();

private:
    void refreshSummary();

    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mIp, &PingResult::ipChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mNodeName, &PingResult::nodeNameChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mErrorString, &PingResult::errorStringChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mEndpoint, &PingResult::endpointChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mPeerRelay, &PingResult::peerRelayChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mDerpRegionCode, &PingResult::derpRegionCodeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, int, mDerpRegionId, &PingResult::derpRegionIdChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, int, mLatencyMilliseconds, &PingResult::latencyMillisecondsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, Path, mPath, &PingResult::pathChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PingResult, QString, mSummary, &PingResult::summaryChanged)
};

// Prefer a Tailscale IPv4 address. IPv6 is used only when the peer has no IPv4.
[[nodiscard]] auto selectPingAddress(const QStringList &addresses) -> QString;

// A DERP or peer-relay answer is worth another attempt: the first packets often
// wake a direct path that a later disco ping can take.
[[nodiscard]] auto shouldRetryPing(PingResult::Path path, int attempt, int maxAttempts) -> bool;

#endif // KTAILCTL_PING_RESULT_HPP
