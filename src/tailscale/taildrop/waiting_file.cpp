// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "waiting_file.hpp"

#include <KFormat>

WaitingFile::WaitingFile(const QString &name, const qint64 size, QObject *parent)
    : QObject(parent)
{
    // Bindable properties are assigned after the QObject base is constructed.
    mName = name; // NOLINT(cppcoreguidelines-prefer-member-initializer)
    mSize = size; // NOLINT(cppcoreguidelines-prefer-member-initializer)
    mFormattedSize = KFormat().formatByteSize(static_cast<double>(size));
}
