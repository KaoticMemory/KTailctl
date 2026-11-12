// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "paths.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLatin1Char>
#include <QStringLiteral>
#include <utility>

namespace
{
constexpr int maximum_unique_name_attempts = 1000;

[[nodiscard]] auto splitFileName(const QString &fileName) -> std::pair<QString, QString>
{
    const qsizetype dot = fileName.lastIndexOf(QLatin1Char('.'));
    if (dot <= 0) {
        return {fileName, QString()};
    }
    return {fileName.left(dot), fileName.mid(dot)};
}
} // namespace

auto sanitizedWaitingFileName(const QString &name) -> QString
{
    if (name.isEmpty() || (name == QStringLiteral(".")) || (name == QStringLiteral(".."))) {
        return {};
    }
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) || name.contains(QChar(u'\0'))) {
        return {};
    }
    if (QFileInfo(name).fileName() != name) {
        return {};
    }
    return name;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto uniqueFilePath(const QString &directory, const QString &fileName, const QStringList &reserved) -> QString
{
    const QString safeName = sanitizedWaitingFileName(fileName);
    if (safeName.isEmpty() || directory.isEmpty()) {
        return {};
    }

    const auto available = [&reserved](const QString &candidate) -> bool {
        return !QFileInfo::exists(candidate) && !reserved.contains(candidate);
    };

    const QDir dir(directory);
    QString first = dir.filePath(safeName);
    if (available(first)) {
        return first;
    }

    const auto [stem, extension] = splitFileName(safeName);
    for (int attempt = 1; attempt <= maximum_unique_name_attempts; ++attempt) {
        const QString candidate = dir.filePath(QStringLiteral("%1 (%2)%3").arg(stem).arg(attempt).arg(extension));
        if (available(candidate)) {
            return candidate;
        }
    }
    return {};
}

auto parseWaitingFilesJson(const QByteArray &json, QList<ParsedWaitingFile> *files) -> bool
{
    if (files == nullptr) {
        return false;
    }
    files->clear();

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if ((error.error != QJsonParseError::NoError) || !document.isArray()) {
        return false;
    }

    const QJsonArray array = document.array();
    files->reserve(array.size());
    for (const auto &value : array) {
        if (!value.isObject()) {
            files->clear();
            return false;
        }
        const QJsonObject object = value.toObject();
        const QString name = sanitizedWaitingFileName(object.value(QStringLiteral("Name")).toString());
        if (name.isEmpty()) {
            files->clear();
            return false;
        }
        files->append(ParsedWaitingFile{.name = name, .size = object.value(QStringLiteral("Size")).toInteger()});
    }
    return true;
}

auto parseFileTargetIdsJson(const QByteArray &json, QStringList *ids) -> bool
{
    if (ids == nullptr) {
        return false;
    }
    ids->clear();

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if ((error.error != QJsonParseError::NoError) || !document.isArray()) {
        return false;
    }

    const QJsonArray array = document.array();
    ids->reserve(array.size());
    for (const auto &value : array) {
        const QString targetId = value.toString();
        if (targetId.isEmpty()) {
            ids->clear();
            return false;
        }
        ids->append(targetId);
    }
    return true;
}
