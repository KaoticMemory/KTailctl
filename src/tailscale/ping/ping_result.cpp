// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ping_result.hpp"

#include <KLocalizedString>
#include <QJsonValue>
#include <QLatin1Char>
#include <QStringLiteral>
#include <QtGlobal>

namespace
{
[[nodiscard]] auto latencyToMilliseconds(const double seconds) -> int
{
    if (seconds <= 0.0) {
        return 0;
    }
    const int rounded = qRound(seconds * 1000.0);
    return rounded > 0 ? rounded : 1;
}

[[nodiscard]] auto classifyPath(const QString &error, const bool isLocal, const int derpRegionId, const QString &derpRegionCode, const QString &peerRelay,
                                const QString &endpoint) -> PingResult::Path
{
    if (isLocal) {
        return PingResult::Path::Local;
    }
    if (!error.isEmpty()) {
        return PingResult::Path::Failed;
    }
    if ((derpRegionId != 0) || !derpRegionCode.isEmpty()) {
        return PingResult::Path::Derp;
    }
    if (!peerRelay.isEmpty()) {
        return PingResult::Path::PeerRelay;
    }
    if (!endpoint.isEmpty()) {
        return PingResult::Path::Direct;
    }
    return PingResult::Path::Unknown;
}
} // namespace

void PingResult::updateFromJson(const QJsonObject &json)
{
    mIp = json.value(QStringLiteral("IP")).toString();
    mNodeName = json.value(QStringLiteral("NodeName")).toString();
    mErrorString = json.value(QStringLiteral("Err")).toString();
    mEndpoint = json.value(QStringLiteral("Endpoint")).toString();
    mPeerRelay = json.value(QStringLiteral("PeerRelay")).toString();
    mDerpRegionCode = json.value(QStringLiteral("DERPRegionCode")).toString();
    mDerpRegionId = json.value(QStringLiteral("DERPRegionID")).toInt();
    mLatencyMilliseconds = latencyToMilliseconds(json.value(QStringLiteral("LatencySeconds")).toDouble());
    mPath = classifyPath(mErrorString,
                         json.value(QStringLiteral("IsLocalIP")).toBool(),
                         mDerpRegionId,
                         mDerpRegionCode,
                         mPeerRelay,
                         mEndpoint);
    refreshSummary();
}

void PingResult::setFailure(const QString &message)
{
    mIp = QString();
    mNodeName = QString();
    mErrorString = message;
    mEndpoint = QString();
    mPeerRelay = QString();
    mDerpRegionCode = QString();
    mDerpRegionId = 0;
    mLatencyMilliseconds = 0;
    mPath = Path::Failed;
    refreshSummary();
}

void PingResult::refreshSummary()
{
    switch (mPath.value()) {
    case Path::Failed:
        mSummary = mErrorString.value().isEmpty() ? i18n("Ping failed") : mErrorString.value();
        return;
    case Path::Local:
        mSummary = i18n("This is the local node");
        return;
    case Path::Direct:
        if (mEndpoint.value().isEmpty()) {
            mSummary = i18nc("@info ping result", "%1 ms, direct", mLatencyMilliseconds.value());
        } else {
            mSummary = i18nc("@info ping result", "%1 ms, direct (%2)", mLatencyMilliseconds.value(), mEndpoint.value());
        }
        return;
    case Path::Derp: {
        const QString region = mDerpRegionCode.value().isEmpty() ? QString::number(mDerpRegionId.value()) : mDerpRegionCode.value();
        mSummary = i18nc("@info ping result", "%1 ms, via DERP relay (%2)", mLatencyMilliseconds.value(), region);
        return;
    }
    case Path::PeerRelay:
        mSummary = i18nc("@info ping result", "%1 ms, via peer relay", mLatencyMilliseconds.value());
        return;
    case Path::Unknown:
        break;
    }

    if (mLatencyMilliseconds.value() > 0) {
        mSummary = i18nc("@info ping result", "%1 ms, path unknown", mLatencyMilliseconds.value());
        return;
    }
    mSummary = QString();
}

auto selectPingAddress(const QStringList &addresses) -> QString
{
    QString fallback;
    for (const QString &address : addresses) {
        if (address.isEmpty()) {
            continue;
        }
        if (!address.contains(QLatin1Char(':'))) {
            return address;
        }
        if (fallback.isEmpty()) {
            fallback = address;
        }
    }
    return fallback;
}

auto shouldRetryPing(const PingResult::Path path, const int attempt, const int maxAttempts) -> bool
{
    if (attempt >= maxAttempts) {
        return false;
    }
    switch (path) {
    case PingResult::Path::Derp:
    case PingResult::Path::PeerRelay:
    case PingResult::Path::Unknown:
        return true;
    case PingResult::Path::Direct:
    case PingResult::Path::Local:
    case PingResult::Path::Failed:
        return false;
    }
    return false;
}
