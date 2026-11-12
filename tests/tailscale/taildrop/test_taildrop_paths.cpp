// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "tailscale/taildrop/paths.hpp"

namespace
{

class TestTaildropPaths : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    static void acceptsPlainFileName()
    {
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral("notes.txt")), QStringLiteral("notes.txt"));
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral("Archive (1).tar.gz")), QStringLiteral("Archive (1).tar.gz"));
    }

    static void rejectsUnsafeFileNames()
    {
        QCOMPARE(sanitizedWaitingFileName(QString()), QString());
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral(".")), QString());
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral("..")), QString());
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral("../secret")), QString());
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral("a/b")), QString());
        QCOMPARE(sanitizedWaitingFileName(QStringLiteral("a\\b")), QString());
    }

    static void uniquePathUsesOriginalName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QCOMPARE(uniqueFilePath(dir.path(), QStringLiteral("photo.png")), QDir(dir.path()).filePath(QStringLiteral("photo.png")));
    }

    static void uniquePathSkipsExistingAndReservedNames()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile existing(QDir(dir.path()).filePath(QStringLiteral("photo.png")));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        existing.close();

        const QString reserved = QDir(dir.path()).filePath(QStringLiteral("photo (1).png"));
        const QString selected = uniqueFilePath(dir.path(), QStringLiteral("photo.png"), {reserved});
        QCOMPARE(selected, QDir(dir.path()).filePath(QStringLiteral("photo (2).png")));
    }

    static void uniquePathRejectsTraversal()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QCOMPARE(uniqueFilePath(dir.path(), QStringLiteral("../photo.png")), QString());
        QCOMPARE(uniqueFilePath(QString(), QStringLiteral("photo.png")), QString());
    }

    static void parseWaitingFiles()
    {
        const QByteArray json = QJsonDocument(QJsonArray{
                                                  QJsonObject{{QStringLiteral("Name"), QStringLiteral("a.txt")}, {QStringLiteral("Size"), 12}},
                                                  QJsonObject{{QStringLiteral("Name"), QStringLiteral("b.bin")}, {QStringLiteral("Size"), 99}},
                                              })
                                    .toJson(QJsonDocument::Compact);

        QList<ParsedWaitingFile> files;
        QVERIFY(parseWaitingFilesJson(json, &files));
        QCOMPARE(files.size(), 2);
        QCOMPARE(files.at(0).name, QStringLiteral("a.txt"));
        QCOMPARE(files.at(0).size, static_cast<qint64>(12));
        QCOMPARE(files.at(1).name, QStringLiteral("b.bin"));
    }

    static void parseWaitingFilesRejectsTraversal()
    {
        const QByteArray json = QJsonDocument(QJsonArray{QJsonObject{{QStringLiteral("Name"), QStringLiteral("../a.txt")}, {QStringLiteral("Size"), 1}}})
                                    .toJson(QJsonDocument::Compact);
        QList<ParsedWaitingFile> files;
        QVERIFY(!parseWaitingFilesJson(json, &files));
        QCOMPARE(files.size(), 0);
    }

    static void parseEmptyWaitingFiles()
    {
        QList<ParsedWaitingFile> files{ParsedWaitingFile{.name = QStringLiteral("stale"), .size = 1}};
        QVERIFY(parseWaitingFilesJson(QByteArrayLiteral("[]"), &files));
        QCOMPARE(files.size(), 0);
    }

    static void parseFileTargets()
    {
        const QByteArray json = QJsonDocument(QJsonArray{QStringLiteral("n123"), QStringLiteral("n456")}).toJson(QJsonDocument::Compact);
        QStringList ids;
        QVERIFY(parseFileTargetIdsJson(json, &ids));
        QCOMPARE(ids, QStringList({QStringLiteral("n123"), QStringLiteral("n456")}));
    }

    static void parseFileTargetsRejectsGarbage()
    {
        QStringList ids{QStringLiteral("stale")};
        QVERIFY(!parseFileTargetIdsJson(QByteArrayLiteral("{"), &ids));
        QCOMPARE(ids.size(), 0);
        QVERIFY(!parseFileTargetIdsJson(QByteArrayLiteral("[\"\"]"), &ids));
        QCOMPARE(ids.size(), 0);
    }
};

}

QTEST_GUILESS_MAIN(TestTaildropPaths)
#include "test_taildrop_paths.moc"
