// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

#include "notifications/notification_planner.hpp"

namespace
{

[[nodiscard]] auto hasNotice(const QList<Notice> &notices, const NoticeKind kind, const QString &subject) -> bool
{
    for (const Notice &notice : notices) {
        if ((notice.kind == kind) && (notice.subject == subject)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] auto countKind(const QList<Notice> &notices, const NoticeKind kind) -> int
{
    int count = 0;
    for (const Notice &notice : notices) {
        if (notice.kind == kind) {
            count += 1;
        }
    }
    return count;
}

class TestNotificationPlanner : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    static void firstSnapshotIsSilent()
    {
        NotificationPlanner planner;
        TailscaleSnapshot snapshot;
        snapshot.connected = true;
        snapshot.selfName = QStringLiteral("laptop");
        snapshot.peers.insert(QStringLiteral("n1"), NotificationPeer{QStringLiteral("phone"), true});
        snapshot.waitingFilesKnown = true;
        snapshot.waitingFiles = {QStringLiteral("already-here.txt")};
        QCOMPARE(planner.observe(snapshot).size(), 0);
    }

    static void connectAndDisconnect()
    {
        NotificationPlanner planner;
        TailscaleSnapshot down;
        QCOMPARE(planner.observe(down).size(), 0);

        TailscaleSnapshot up;
        up.connected = true;
        up.selfName = QStringLiteral("laptop");
        up.peers.insert(QStringLiteral("n1"), NotificationPeer{QStringLiteral("phone"), true});
        const QList<Notice> connected = planner.observe(up);
        QCOMPARE(connected.size(), 1);
        QVERIFY(hasNotice(connected, NoticeKind::Connected, QStringLiteral("laptop")));

        TailscaleSnapshot stopped;
        stopped.selfName = QStringLiteral("laptop");
        const QList<Notice> disconnected = planner.observe(stopped);
        QCOMPARE(disconnected.size(), 1);
        QVERIFY(hasNotice(disconnected, NoticeKind::Disconnected, QStringLiteral("laptop")));
    }

    static void reconnectDoesNotRepeatExistingPeers()
    {
        NotificationPlanner planner;
        TailscaleSnapshot up;
        up.connected = true;
        up.peers.insert(QStringLiteral("n1"), NotificationPeer{QStringLiteral("phone"), true});
        QCOMPARE(planner.observe(up).size(), 0);

        TailscaleSnapshot down;
        QCOMPARE(planner.observe(down).size(), 1);

        up.peers.insert(QStringLiteral("n2"), NotificationPeer{QStringLiteral("nas"), true});
        const QList<Notice> again = planner.observe(up);
        QVERIFY(hasNotice(again, NoticeKind::Connected, QString()));
        QVERIFY(hasNotice(again, NoticeKind::PeerAdded, QStringLiteral("nas")));
        QVERIFY(!hasNotice(again, NoticeKind::PeerAdded, QStringLiteral("phone")));
    }

    static void peerOnlineOfflineAndRemoved()
    {
        NotificationPlanner planner;
        TailscaleSnapshot snapshot;
        snapshot.connected = true;
        snapshot.peers.insert(QStringLiteral("n1"), NotificationPeer{QStringLiteral("phone"), true});
        snapshot.peers.insert(QStringLiteral("n2"), NotificationPeer{QStringLiteral("nas"), true});
        QCOMPARE(planner.observe(snapshot).size(), 0);

        snapshot.peers[QStringLiteral("n1")].online = false;
        snapshot.peers.remove(QStringLiteral("n2"));
        snapshot.peers.insert(QStringLiteral("n3"), NotificationPeer{QStringLiteral("tablet"), false});
        const QList<Notice> notices = planner.observe(snapshot);
        QVERIFY(hasNotice(notices, NoticeKind::PeerOffline, QStringLiteral("phone")));
        QVERIFY(hasNotice(notices, NoticeKind::PeerRemoved, QStringLiteral("nas")));
        QVERIFY(hasNotice(notices, NoticeKind::PeerAdded, QStringLiteral("tablet")));
        QCOMPARE(countKind(notices, NoticeKind::PeerOnline), 0);
    }

    static void exitNodeChanges()
    {
        NotificationPlanner planner;
        TailscaleSnapshot snapshot;
        snapshot.connected = true;
        QCOMPARE(planner.observe(snapshot).size(), 0);

        snapshot.exitNodeId = QStringLiteral("n9");
        snapshot.exitNodeName = QStringLiteral("exit");
        const QList<Notice> enabled = planner.observe(snapshot);
        QVERIFY(hasNotice(enabled, NoticeKind::ExitNodeEnabled, QStringLiteral("exit")));

        snapshot.exitNodeId = QStringLiteral("n10");
        snapshot.exitNodeName = QStringLiteral("other");
        const QList<Notice> changed = planner.observe(snapshot);
        QVERIFY(hasNotice(changed, NoticeKind::ExitNodeChanged, QStringLiteral("other")));

        snapshot.exitNodeId.clear();
        snapshot.exitNodeName.clear();
        const QList<Notice> disabled = planner.observe(snapshot);
        QVERIFY(hasNotice(disabled, NoticeKind::ExitNodeDisabled, QStringLiteral("other")));
    }

    static void incomingFilesAndReset()
    {
        NotificationPlanner planner;
        TailscaleSnapshot snapshot;
        snapshot.connected = true;
        snapshot.waitingFilesKnown = true;
        QCOMPARE(planner.observe(snapshot).size(), 0);

        snapshot.waitingFiles = {QStringLiteral("a.txt"), QStringLiteral("b.txt")};
        const QList<Notice> arrived = planner.observe(snapshot);
        QVERIFY(hasNotice(arrived, NoticeKind::TaildropFile, QStringLiteral("a.txt")));
        QVERIFY(hasNotice(arrived, NoticeKind::TaildropFile, QStringLiteral("b.txt")));

        snapshot.waitingFiles = {QStringLiteral("b.txt")};
        QCOMPARE(planner.observe(snapshot).size(), 0);

        QStringList many;
        for (int index = 0; index < 9; ++index) {
            many.append(QStringLiteral("file-%1").arg(index));
        }
        snapshot.waitingFiles = many;
        const QList<Notice> burst = planner.observe(snapshot);
        QCOMPARE(burst.size(), 1);
        QVERIFY(hasNotice(burst, NoticeKind::TaildropFiles, QStringLiteral("9")));

        planner.reset();
        snapshot.waitingFiles = {QStringLiteral("new.txt")};
        snapshot.peers.insert(QStringLiteral("n1"), NotificationPeer{QStringLiteral("phone"), true});
        QCOMPARE(planner.observe(snapshot).size(), 0);
    }

    static void unknownFileListIsNotCleared()
    {
        NotificationPlanner planner;
        TailscaleSnapshot snapshot;
        snapshot.connected = true;
        snapshot.waitingFilesKnown = true;
        snapshot.waitingFiles = {QStringLiteral("kept.txt")};
        QCOMPARE(planner.observe(snapshot).size(), 0);

        snapshot.waitingFilesKnown = false;
        snapshot.waitingFiles.clear();
        QCOMPARE(planner.observe(snapshot).size(), 0);

        snapshot.waitingFilesKnown = true;
        snapshot.waitingFiles = {QStringLiteral("kept.txt"), QStringLiteral("extra.txt")};
        const QList<Notice> notices = planner.observe(snapshot);
        QVERIFY(hasNotice(notices, NoticeKind::TaildropFile, QStringLiteral("extra.txt")));
        QVERIFY(!hasNotice(notices, NoticeKind::TaildropFile, QStringLiteral("kept.txt")));
    }
};

}

QTEST_GUILESS_MAIN(TestNotificationPlanner)
#include "test_notification_planner.moc"
