// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonObject>
#include <QStringList>
#include <QTest>

#include "tailscale/ping/ping_result.hpp"

namespace
{

class TestPingResult : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    static void parseDirectPath()
    {
        QJsonObject json{
            {QStringLiteral("IP"), QStringLiteral("100.64.0.8")},
            {QStringLiteral("NodeName"), QStringLiteral("laptop")},
            {QStringLiteral("LatencySeconds"), 0.0124},
            {QStringLiteral("Endpoint"), QStringLiteral("203.0.113.9:41641")},
            {QStringLiteral("DERPRegionID"), 0},
            {QStringLiteral("DERPRegionCode"), QStringLiteral("")},
            {QStringLiteral("PeerRelay"), QStringLiteral("")},
            {QStringLiteral("Err"), QStringLiteral("")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::Direct);
        QCOMPARE(result.latencyMilliseconds(), 12);
        QCOMPARE(result.endpoint(), QStringLiteral("203.0.113.9:41641"));
        QCOMPARE(result.nodeName(), QStringLiteral("laptop"));
        QCOMPARE(result.summary(), QStringLiteral("12 ms, direct (203.0.113.9:41641)"));
    }

    static void parseDerpPath()
    {
        QJsonObject json{
            {QStringLiteral("IP"), QStringLiteral("100.64.0.8")},
            {QStringLiteral("LatencySeconds"), 0.0456},
            {QStringLiteral("DERPRegionID"), 12},
            {QStringLiteral("DERPRegionCode"), QStringLiteral("fra")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::Derp);
        QCOMPARE(result.derpRegionId(), 12);
        QCOMPARE(result.derpRegionCode(), QStringLiteral("fra"));
        QCOMPARE(result.latencyMilliseconds(), 46);
        QCOMPARE(result.summary(), QStringLiteral("46 ms, via DERP relay (fra)"));
    }

    static void parseDerpPathWithoutRegionCodeUsesId()
    {
        QJsonObject json{
            {QStringLiteral("LatencySeconds"), 0.020},
            {QStringLiteral("DERPRegionID"), 4},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::Derp);
        QCOMPARE(result.summary(), QStringLiteral("20 ms, via DERP relay (4)"));
    }

    static void parsePeerRelayPath()
    {
        QJsonObject json{
            {QStringLiteral("LatencySeconds"), 0.008},
            {QStringLiteral("PeerRelay"), QStringLiteral("100.64.0.3:1234:vni:1")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::PeerRelay);
        QCOMPARE(result.latencyMilliseconds(), 8);
        QCOMPARE(result.summary(), QStringLiteral("8 ms, via peer relay"));
    }

    static void derpTakesPriorityOverEndpoint()
    {
        QJsonObject json{
            {QStringLiteral("LatencySeconds"), 0.030},
            {QStringLiteral("Endpoint"), QStringLiteral("203.0.113.9:41641")},
            {QStringLiteral("DERPRegionID"), 2},
            {QStringLiteral("DERPRegionCode"), QStringLiteral("nyc")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::Derp);
    }

    static void parseError()
    {
        QJsonObject json{
            {QStringLiteral("Err"), QStringLiteral("timeout talking to peer")},
            {QStringLiteral("IP"), QStringLiteral("100.64.0.8")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::Failed);
        QCOMPARE(result.errorString(), QStringLiteral("timeout talking to peer"));
        QCOMPARE(result.summary(), QStringLiteral("timeout talking to peer"));
    }

    static void parseLocalNode()
    {
        QJsonObject json{
            {QStringLiteral("IsLocalIP"), true},
            {QStringLiteral("Err"), QStringLiteral("local node")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.path(), PingResult::Path::Local);
        QCOMPARE(result.summary(), QStringLiteral("This is the local node"));
    }

    static void subMillisecondLatencyRoundsUpToOne()
    {
        QJsonObject json{
            {QStringLiteral("LatencySeconds"), 0.0004},
            {QStringLiteral("Endpoint"), QStringLiteral("203.0.113.9:41641")},
        };

        PingResult result;
        result.updateFromJson(json);

        QCOMPARE(result.latencyMilliseconds(), 1);
        QCOMPARE(result.path(), PingResult::Path::Direct);
    }

    static void setFailureMarksResultFailed()
    {
        PingResult result;
        result.setFailure(QStringLiteral("Peer has no Tailscale address"));

        QCOMPARE(result.path(), PingResult::Path::Failed);
        QCOMPARE(result.summary(), QStringLiteral("Peer has no Tailscale address"));
        QCOMPARE(result.latencyMilliseconds(), 0);
    }

    static void selectPrefersIpv4()
    {
        const QString selected = selectPingAddress({
            QStringLiteral("fd7a:115c:a1e0::12"),
            QStringLiteral("100.64.0.12"),
        });
        QCOMPARE(selected, QStringLiteral("100.64.0.12"));
    }

    static void selectFallsBackToIpv6()
    {
        const QString selected = selectPingAddress({
            QString(),
            QStringLiteral("fd7a:115c:a1e0::12"),
        });
        QCOMPARE(selected, QStringLiteral("fd7a:115c:a1e0::12"));
    }

    static void selectEmptyWhenNoAddresses()
    {
        QCOMPARE(selectPingAddress({}), QString());
        QCOMPARE(selectPingAddress({QString()}), QString());
    }

    static void retryWhileRelayOrUnknown()
    {
        static constexpr int maxAttempts = 3;
        QVERIFY(shouldRetryPing(PingResult::Path::Derp, 1, maxAttempts));
        QVERIFY(shouldRetryPing(PingResult::Path::PeerRelay, 2, maxAttempts));
        QVERIFY(shouldRetryPing(PingResult::Path::Unknown, 1, maxAttempts));
        QVERIFY(!shouldRetryPing(PingResult::Path::Derp, 3, maxAttempts));
        QVERIFY(!shouldRetryPing(PingResult::Path::Direct, 1, maxAttempts));
        QVERIFY(!shouldRetryPing(PingResult::Path::Failed, 1, maxAttempts));
        QVERIFY(!shouldRetryPing(PingResult::Path::Local, 1, maxAttempts));
    }
};

}

QTEST_GUILESS_MAIN(TestPingResult)
#include "test_ping_result.moc"
