// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_TAILDROP_PATHS_HPP
#define KTAILCTL_TAILDROP_PATHS_HPP

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <qtypes.h>

struct ParsedWaitingFile {
    QString name;
    qint64 size = 0;
};

// Empty when name is not a single path segment (no slashes, not "." or "..").
[[nodiscard]] auto sanitizedWaitingFileName(const QString &name) -> QString;

// A path inside directory that does not overwrite an existing file or a path
// already reserved for another file in the same batch. Empty when the file
// name is unsafe or no free name could be found.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
[[nodiscard]] auto uniqueFilePath(const QString &directory, const QString &fileName, const QStringList &reserved = {}) -> QString;

[[nodiscard]] auto parseWaitingFilesJson(const QByteArray &json, QList<ParsedWaitingFile> *files) -> bool;

[[nodiscard]] auto parseFileTargetIdsJson(const QByteArray &json, QStringList *ids) -> bool;

#endif // KTAILCTL_TAILDROP_PATHS_HPP
