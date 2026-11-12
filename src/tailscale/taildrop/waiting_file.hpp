// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KTAILCTL_TAILDROP_WAITING_FILE_HPP
#define KTAILCTL_TAILDROP_WAITING_FILE_HPP

#include <QObject>
#include <QObjectBindableProperty>
#include <QString>
#include <qtmetamacros.h>
#include <qtypes.h>

class WaitingFile : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name BINDABLE bindableName NOTIFY nameChanged)
    Q_PROPERTY(qint64 size READ size BINDABLE bindableSize NOTIFY sizeChanged)
    Q_PROPERTY(QString formattedSize READ formattedSize BINDABLE bindableFormattedSize NOTIFY formattedSizeChanged)

public:
    explicit WaitingFile(const QString &name, qint64 size, QObject *parent = nullptr);

    [[nodiscard]] auto name() const noexcept -> const QString &
    {
        return mName;
    }
    [[nodiscard]] auto size() const noexcept -> qint64
    {
        return mSize;
    }
    [[nodiscard]] auto formattedSize() const noexcept -> const QString &
    {
        return mFormattedSize;
    }

    [[nodiscard]] auto bindableName() -> QBindable<QString>
    {
        return {&mName};
    }
    [[nodiscard]] auto bindableSize() -> QBindable<qint64>
    {
        return {&mSize};
    }
    [[nodiscard]] auto bindableFormattedSize() -> QBindable<QString>
    {
        return {&mFormattedSize};
    }

Q_SIGNALS:
    void nameChanged();
    void sizeChanged();
    void formattedSizeChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(WaitingFile, QString, mName, &WaitingFile::nameChanged)
    Q_OBJECT_BINDABLE_PROPERTY(WaitingFile, qint64, mSize, &WaitingFile::sizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(WaitingFile, QString, mFormattedSize, &WaitingFile::formattedSizeChanged)
};

#endif // KTAILCTL_TAILDROP_WAITING_FILE_HPP
