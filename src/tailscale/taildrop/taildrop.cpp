// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "taildrop.hpp"

#include <KLocalizedString>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QModelIndex>
#include <QMessageLogger>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QUrl>
#include <QtConcurrentRun>
#include <cstdlib>
#include <cstring>
#include <libktailctl_wrapper.h>
#include <memory>
#include <utility>

#include "ktailctl_config.h"
#include "logging_tailscale_taildrop.hpp"

namespace
{
class GoStringArg
{
    QByteArray mUtf8;
    GoString mValue{};

public:
    explicit GoStringArg(const QString &text)
        : mUtf8(text.toUtf8())
    {
        mValue.p = mUtf8.constData();
        mValue.n = static_cast<GoInt>(mUtf8.size());
    }

    [[nodiscard]] auto get() -> GoString *
    {
        return &mValue;
    }
};

[[nodiscard]] auto takeCString(char *raw) -> QByteArray
{
    if (raw == nullptr) {
        return {};
    }
    const std::unique_ptr<char, decltype(&free)> owned(raw, &free);
    return {owned.get(), static_cast<qsizetype>(std::strlen(owned.get()))};
}

[[nodiscard]] auto takeError(char *raw) -> QString
{
    const QByteArray bytes = takeCString(raw);
    if (bytes.isNull()) {
        return {};
    }
    return QString::fromUtf8(bytes);
}

[[nodiscard]] auto localPathFromVariant(const QVariant &value) -> QString
{
    const QUrl asUrl = value.canConvert<QUrl>() ? value.toUrl() : QUrl(value.toString());
    if (asUrl.isLocalFile()) {
        return asUrl.toLocalFile();
    }
    QString text = value.toString();
    if (text.startsWith(QStringLiteral("file:"))) {
        return {};
    }
    return text;
}

[[nodiscard]] auto regularFilesFromVariants(const QVariantList &values, QString *rejectedName) -> QStringList
{
    QStringList paths;
    for (const QVariant &value : values) {
        const QString path = localPathFromVariant(value);
        const QFileInfo info(path);
        if (path.isEmpty() || !info.exists() || !info.isFile()) {
            if (rejectedName != nullptr) {
                const QString label = info.fileName().isEmpty() ? path : info.fileName();
                *rejectedName = label.isEmpty() ? QStringLiteral("?") : label;
            }
            return {};
        }
        const QString absolute = info.absoluteFilePath();
        if (!paths.contains(absolute)) {
            paths.append(absolute);
        }
    }
    return paths;
}

} // namespace

auto Taildrop::sendBlocking(const QString &peerId, const QStringList &paths) -> QString
{
    for (const QString &path : paths) {
        GoStringArg target(peerId);
        GoStringArg file(path);
        const QString error = takeError(tailscale_push_file(target.get(), file.get()));
        if (!error.isEmpty()) {
            return error;
        }
    }
    return {};
}

auto Taildrop::saveBlocking(const QList<SaveItem> &items) -> QString
{
    for (const SaveItem &item : items) {
        GoStringArg name(item.name);
        GoStringArg destination(item.destination);
        const QString error = takeError(tailscale_save_waiting_file(name.get(), destination.get()));
        if (!error.isEmpty()) {
            return error;
        }
    }
    return {};
}

auto Taildrop::deleteBlocking(const QList<SaveItem> &items) -> QString
{
    for (const SaveItem &item : items) {
        GoStringArg name(item.name);
        const QString error = takeError(tailscale_delete_waiting_file(name.get()));
        if (!error.isEmpty()) {
            return error;
        }
    }
    return {};
}

Taildrop::Taildrop(QObject *parent)
    : QObject(parent)
    , mWaitingFiles(new WaitingFileModel(this))
    , mWatcher(new QFutureWatcher<QString>(this))
{
    connect(mWatcher, &QFutureWatcher<QString>::finished, this, &Taildrop::onJobFinished);
    connect(Config::self(), &Config::taildropDownloadDirectoryChanged, this, &Taildrop::downloadDirectoryChanged);
    refresh();
}

auto Taildrop::waitingFileCount() const noexcept -> int
{
    return mWaitingFiles->rowCount(QModelIndex{});
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto Taildrop::downloadDirectory() const -> QString
{
    QString configured = Config::taildropDownloadDirectory();
    if (!configured.isEmpty()) {
        return configured;
    }
    QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!downloads.isEmpty()) {
        return downloads;
    }
    return QDir::homePath();
}

auto Taildrop::downloadDirectoryUrl() const -> QUrl
{
    return QUrl::fromLocalFile(downloadDirectory());
}

void Taildrop::setDownloadDirectory(const QUrl &directory)
{
    if (!directory.isLocalFile()) {
        mLastError = i18n("Choose a local folder");
        mStatusMessage = mLastError.value();
        return;
    }
    Config::setTaildropDownloadDirectory(directory.toLocalFile());
}

void Taildrop::refresh()
{
    if (mBusy) {
        return;
    }

    if (tailscale_daemon_running() == 0) {
        qCWarning(Logging::Tailscale::Taildrop) << "Tailscale daemon is not running";
        clearRemoteState();
        return;
    }

    const QByteArray targetJson = takeCString(tailscale_file_targets());
    const QByteArray waitingJson = takeCString(tailscale_waiting_files());
    if (targetJson.isNull() || waitingJson.isNull()) {
        qCWarning(Logging::Tailscale::Taildrop) << "Failed to refresh Taildrop state";
        return;
    }

    QStringList ids;
    if (!parseFileTargetIdsJson(targetJson, &ids)) {
        qCWarning(Logging::Tailscale::Taildrop) << "Failed to parse Taildrop targets";
        return;
    }
    mFileTargetIds = ids;

    QList<ParsedWaitingFile> files;
    if (!parseWaitingFilesJson(waitingJson, &files)) {
        qCWarning(Logging::Tailscale::Taildrop) << "Failed to parse waiting Taildrop files";
        return;
    }
    replaceWaitingFiles(files);
    qCDebug(Logging::Tailscale::Taildrop) << "Taildrop refreshed," << files.size() << "waiting files";
}

void Taildrop::sendFiles(const QString &peerId, const QVariantList &files)
{
    if (mBusy) {
        return;
    }
    QString rejected;
    const QStringList paths = regularFilesFromVariants(files, &rejected);
    if (paths.isEmpty()) {
        mActivePeerId = peerId;
        mLastError = rejected.isEmpty() ? i18n("No files to send") : i18n("%1 is not a file Taildrop can send", rejected);
        mStatusMessage = mLastError.value();
        return;
    }
    sendLocalFiles(peerId, paths);
}

void Taildrop::sendLocalFiles(const QString &peerId, const QStringList &paths)
{
    if (mBusy || peerId.isEmpty() || paths.isEmpty()) {
        return;
    }
    mPendingPeerId = peerId;
    mPendingPaths = paths;
    mPendingSaves.clear();
    mActivePeerId = peerId;
    const QString status = (paths.size() == 1) ? i18n("Sending %1…", QFileInfo(paths.constFirst()).fileName()) : i18n("Sending %1 files…", paths.size());
    const QString success = (paths.size() == 1) ? i18n("Sent %1", QFileInfo(paths.constFirst()).fileName()) : i18n("Sent %1 files", paths.size());
    startJob(JobKind::Send, status, success);
}

void Taildrop::saveFile(const QString &name)
{
    saveFileTo(name, QUrl::fromLocalFile(downloadDirectory()));
}

void Taildrop::saveFileTo(const QString &name, const QUrl &directory)
{
    if (mBusy) {
        return;
    }
    if (!directory.isLocalFile()) {
        mLastError = i18n("Choose a local folder");
        mStatusMessage = mLastError.value();
        return;
    }
    const QList<SaveItem> items = saveItemsFor({name}, directory.toLocalFile());
    if (items.isEmpty()) {
        mLastError = i18n("Cannot save %1", name);
        mStatusMessage = mLastError.value();
        return;
    }
    mPendingSaves = items;
    mPendingPaths.clear();
    mActivePeerId = QString();
    startJob(JobKind::Save, i18n("Saving %1…", name), i18n("Saved %1", items.constFirst().destination));
}

void Taildrop::saveAll()
{
    if (mBusy) {
        return;
    }
    QStringList names;
    names.reserve(waitingFileCount());
    for (int row = 0; row < waitingFileCount(); ++row) {
        names.append(mWaitingFiles->at(row)->name());
    }
    const QList<SaveItem> items = saveItemsFor(names, downloadDirectory());
    if (items.isEmpty()) {
        mLastError = i18n("No files to save");
        mStatusMessage = mLastError.value();
        return;
    }
    mPendingSaves = items;
    mPendingPaths.clear();
    mActivePeerId = QString();
    const QString status = (items.size() == 1) ? i18n("Saving %1…", items.constFirst().name) : i18n("Saving %1 files…", items.size());
    const QString success = (items.size() == 1) ? i18n("Saved %1", items.constFirst().destination) : i18n("Saved %1 files", items.size());
    startJob(JobKind::Save, status, success);
}

void Taildrop::deleteFile(const QString &name)
{
    if (mBusy) {
        return;
    }
    if (sanitizedWaitingFileName(name).isEmpty()) {
        mLastError = i18n("Cannot delete %1", name);
        mStatusMessage = mLastError.value();
        return;
    }
    mPendingSaves = {SaveItem{.name = name, .destination = QString()}};
    mPendingPaths.clear();
    mActivePeerId = QString();
    startJob(JobKind::Delete, i18n("Deleting %1…", name), i18n("Deleted %1", name));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Taildrop::startJob(const JobKind kind, const QString &statusMessage, const QString &successMessage)
{
    mJobKind = kind;
    mJobSuccessMessage = successMessage;
    mLastError = QString();
    mStatusMessage = statusMessage;
    mBusy = true;

    const JobKind capturedKind = kind;
    const QString peerId = mPendingPeerId;
    const QStringList paths = mPendingPaths;
    const QList<SaveItem> saves = mPendingSaves;
    qCInfo(Logging::Tailscale::Taildrop) << "Starting Taildrop job" << static_cast<int>(kind) << successMessage;
    mWatcher->setFuture(QtConcurrent::run([capturedKind, peerId, paths, saves]() -> QString {
        switch (capturedKind) {
        case JobKind::Send:
            return Taildrop::sendBlocking(peerId, paths);
        case JobKind::Save:
            return Taildrop::saveBlocking(saves);
        case JobKind::Delete:
            return Taildrop::deleteBlocking(saves);
        case JobKind::None:
            break;
        }
        return {};
    }));
}

void Taildrop::onJobFinished()
{
    const QString error = mWatcher->result();
    mBusy = false;
    mJobKind = JobKind::None;
    if (error.isEmpty()) {
        mLastError = QString();
        mStatusMessage = mJobSuccessMessage;
    } else {
        qCWarning(Logging::Tailscale::Taildrop) << error;
        mLastError = error;
        mStatusMessage = error;
    }
    refresh();
}

void Taildrop::replaceWaitingFiles(const QList<ParsedWaitingFile> &files)
{
    if (files.size() == waitingFileCount()) {
        bool unchanged = true;
        for (int row = 0; row < files.size(); ++row) {
            const WaitingFile *current = mWaitingFiles->at(row);
            if ((current == nullptr) || (current->name() != files.at(row).name) || (current->size() != files.at(row).size)) {
                unchanged = false;
                break;
            }
        }
        if (unchanged) {
            return;
        }
    }

    mWaitingFiles->clear();
    for (const ParsedWaitingFile &file : files) {
        mWaitingFiles->addItem(new WaitingFile(file.name, file.size, mWaitingFiles));
    }
    Q_EMIT waitingFilesChanged();
}

void Taildrop::clearRemoteState()
{
    if (!mFileTargetIds.value().isEmpty()) {
        mFileTargetIds = QStringList();
    }
    if (waitingFileCount() > 0) {
        mWaitingFiles->clear();
        Q_EMIT waitingFilesChanged();
    }
}

auto Taildrop::saveItemsFor(const QStringList &names, const QString &directory) -> QList<SaveItem>
{
    QList<SaveItem> items;
    QStringList reserved;
    for (const QString &name : names) {
        const QString safeName = sanitizedWaitingFileName(name);
        if (safeName.isEmpty()) {
            return {};
        }
        const QString destination = uniqueFilePath(directory, safeName, reserved);
        if (destination.isEmpty()) {
            return {};
        }
        reserved.append(destination);
        items.append(SaveItem{.name = safeName, .destination = destination});
    }
    return items;
}
