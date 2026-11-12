// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tray_menu_incoming.hpp"

#include <KLocalizedString>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QModelIndex>
#include <QStringLiteral>
#include <QTimer>
#include <QUrl>

#include "taildrop.hpp"
#include "waiting_file.hpp"

TrayMenuIncoming::TrayMenuIncoming(Tailscale *tailscale, QWidget *parent)
    : QMenu(parent)
    , mTailscale(tailscale)
{
    setIcon(QIcon::fromTheme(QStringLiteral("document-send")));
    connect(this, &QMenu::aboutToShow, this, &TrayMenuIncoming::rebuildMenu);
    connect(mTailscale->taildrop(), &Taildrop::waitingFilesChanged, this, [this] {
        updateTitle();
        if (!isVisible()) {
            rebuildMenu();
        }
    });
    rebuildMenu();
}

void TrayMenuIncoming::updateTitle()
{
    const int count = mTailscale->taildrop()->waitingFileCount();
    if (count > 0) {
        setTitle(i18ncp("@title:menu", "Incoming file", "Incoming files (%1)", count));
    } else {
        setTitle(i18nc("@title:menu", "Incoming files"));
    }
}

void TrayMenuIncoming::rebuildMenu()
{
    updateTitle();
    clear();

    WaitingFileModel *model = mTailscale->taildrop()->waitingFiles();
    if (model->rowCount(QModelIndex{}) == 0) {
        addAction(i18n("No files waiting"))->setEnabled(false);
        return;
    }

    for (int row = 0; row < model->rowCount(QModelIndex{}); ++row) {
        const WaitingFile *file = model->at(row);
        if (file == nullptr) {
            continue;
        }
        const QString name = file->name();
        QMenu *fileMenu = addMenu(QStringLiteral("%1 (%2)").arg(name, file->formattedSize()));
        fileMenu->setIcon(QIcon::fromTheme(QStringLiteral("text-plain")));

        fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("@action:inmenu", "Save"), this, [this, name] {
            QTimer::singleShot(0, this, [this, name] {
                mTailscale->taildrop()->saveFile(name);
            });
        });
        fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-save-as")), i18nc("@action:inmenu", "Save to…"), this, [this, name] {
            QTimer::singleShot(0, this, [this, name] {
                const QString directory = QFileDialog::getExistingDirectory(nullptr,
                                                                            i18nc("@title:window", "Save %1", name),
                                                                            mTailscale->taildrop()->downloadDirectory());
                if (!directory.isEmpty()) {
                    mTailscale->taildrop()->saveFileTo(name, QUrl::fromLocalFile(directory));
                }
            });
        });
        fileMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:inmenu", "Delete"), this, [this, name] {
            QTimer::singleShot(0, this, [this, name] {
                const auto answer = QMessageBox::question(nullptr,
                                                          i18nc("@title:window", "Delete file?"),
                                                          i18n("Delete %1 without saving it?", name));
                if (answer == QMessageBox::Yes) {
                    mTailscale->taildrop()->deleteFile(name);
                }
            });
        });
    }

    addSeparator();
    addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("@action:inmenu", "Save all"), this, [this] {
        QTimer::singleShot(0, this, [this] {
            mTailscale->taildrop()->saveAll();
        });
    });
}
