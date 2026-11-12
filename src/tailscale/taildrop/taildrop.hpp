// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_TAILDROP_HPP
#define KTAILCTL_TAILDROP_HPP

#include <QFutureWatcher>
#include <QObject>
#include <QObjectBindableProperty>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>
#include <qtmetamacros.h>
#include <stdint.h>

#include "paths.hpp"
#include "property_list_model.hpp"
#include "waiting_file.hpp"

using WaitingFileModelBase = PropertyListModel<WaitingFile, PropertyListModelOwnership::Owning>;

class WaitingFileModel : public WaitingFileModelBase
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    using WaitingFileModelBase::WaitingFileModelBase;
};

// Sends and receives Taildrop files through the Go wrapper around the Tailscale local API.
class Taildrop : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(WaitingFileModel *waitingFiles READ waitingFiles CONSTANT)
    Q_PROPERTY(int waitingFileCount READ waitingFileCount NOTIFY waitingFilesChanged)
    Q_PROPERTY(QStringList fileTargetIds READ fileTargetIds BINDABLE bindableFileTargetIds NOTIFY fileTargetIdsChanged)
    Q_PROPERTY(bool busy READ busy BINDABLE bindableBusy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage BINDABLE bindableStatusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastError READ lastError BINDABLE bindableLastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString activePeerId READ activePeerId BINDABLE bindableActivePeerId NOTIFY activePeerIdChanged)
    Q_PROPERTY(QString downloadDirectory READ downloadDirectory NOTIFY downloadDirectoryChanged)
    Q_PROPERTY(QUrl downloadDirectoryUrl READ downloadDirectoryUrl NOTIFY downloadDirectoryChanged)

public:
    explicit Taildrop(QObject *parent = nullptr);

    [[nodiscard]] auto waitingFiles() const noexcept -> WaitingFileModel *
    {
        return mWaitingFiles;
    }
    [[nodiscard]] auto waitingFileCount() const noexcept -> int;
    [[nodiscard]] auto fileTargetIds() const noexcept -> const QStringList &
    {
        return mFileTargetIds;
    }
    [[nodiscard]] auto busy() const noexcept -> bool
    {
        return mBusy;
    }
    [[nodiscard]] auto statusMessage() const noexcept -> const QString &
    {
        return mStatusMessage;
    }
    [[nodiscard]] auto lastError() const noexcept -> const QString &
    {
        return mLastError;
    }
    [[nodiscard]] auto activePeerId() const noexcept -> const QString &
    {
        return mActivePeerId;
    }
    [[nodiscard]] auto downloadDirectory() const -> QString;
    [[nodiscard]] auto downloadDirectoryUrl() const -> QUrl;

    [[nodiscard]] auto bindableFileTargetIds() -> QBindable<QStringList>
    {
        return {&mFileTargetIds};
    }
    [[nodiscard]] auto bindableBusy() -> QBindable<bool>
    {
        return {&mBusy};
    }
    [[nodiscard]] auto bindableStatusMessage() -> QBindable<QString>
    {
        return {&mStatusMessage};
    }
    [[nodiscard]] auto bindableLastError() -> QBindable<QString>
    {
        return {&mLastError};
    }
    [[nodiscard]] auto bindableActivePeerId() -> QBindable<QString>
    {
        return {&mActivePeerId};
    }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void sendFiles(const QString &peerId, const QVariantList &files);
    Q_INVOKABLE void saveFile(const QString &name);
    Q_INVOKABLE void saveFileTo(const QString &name, const QUrl &directory);
    Q_INVOKABLE void saveAll();
    Q_INVOKABLE void deleteFile(const QString &name);
    Q_INVOKABLE void setDownloadDirectory(const QUrl &directory);

    // Used by the tray, which already has local paths from QFileDialog.
    void sendLocalFiles(const QString &peerId, const QStringList &paths);

Q_SIGNALS:
    void waitingFilesChanged();
    void fileTargetIdsChanged();
    void busyChanged();
    void statusMessageChanged();
    void lastErrorChanged();
    void activePeerIdChanged();
    void downloadDirectoryChanged();

private:
    enum class JobKind : uint8_t {
        None,
        Send,
        Save,
        Delete,
    };

    struct SaveItem {
        QString name;
        QString destination;
    };

    [[nodiscard]] static auto sendBlocking(const QString &peerId, const QStringList &paths) -> QString;
    [[nodiscard]] static auto saveBlocking(const QList<SaveItem> &items) -> QString;
    [[nodiscard]] static auto deleteBlocking(const QList<SaveItem> &items) -> QString;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void startJob(JobKind kind, const QString &statusMessage, const QString &successMessage);
    void onJobFinished();
    void replaceWaitingFiles(const QList<ParsedWaitingFile> &files);
    void clearRemoteState();
    [[nodiscard]] static auto saveItemsFor(const QStringList &names, const QString &directory) -> QList<SaveItem>;

    WaitingFileModel *mWaitingFiles;
    QFutureWatcher<QString> *mWatcher;
    JobKind mJobKind = JobKind::None;
    QString mJobSuccessMessage;
    QList<SaveItem> mPendingSaves;
    QString mPendingPeerId;
    QStringList mPendingPaths;

    Q_OBJECT_BINDABLE_PROPERTY(Taildrop, QStringList, mFileTargetIds, &Taildrop::fileTargetIdsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Taildrop, bool, mBusy, &Taildrop::busyChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Taildrop, QString, mStatusMessage, &Taildrop::statusMessageChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Taildrop, QString, mLastError, &Taildrop::lastErrorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Taildrop, QString, mActivePeerId, &Taildrop::activePeerIdChanged)
};

#endif // KTAILCTL_TAILDROP_HPP
