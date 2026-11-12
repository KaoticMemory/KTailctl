// SPDX-FileCopyrightText: 2026 Nick
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KDialogs
import org.kde.kirigamiaddons.formcard as FormCard
import org.fkoehler.KTailctl as KTailctl
import "qrc:/ui/components"

FormCard.FormCardPage {
    id: page

    title: i18n("Taildrop")

    property string pendingDeleteName: ""

    function openFolderDialog(): void {
        folderDialog.currentFolder = KTailctl.Tailscale.taildrop.downloadDirectoryUrl;
        folderDialog.open();
    }

    actions: [
        Kirigami.Action {
            icon.name: "document-save"
            text: i18n("Save all")
            enabled: KTailctl.Tailscale.taildrop.waitingFileCount > 0 && !KTailctl.Tailscale.taildrop.busy
            onTriggered: KTailctl.Tailscale.taildrop.saveAll()
        },
        Kirigami.Action {
            icon.name: "folder-downloads"
            text: i18n("Download folder")
            onTriggered: page.openFolderDialog()
        }
    ]

    FolderDialog {
        id: folderDialog
        title: i18n("Taildrop download folder")
        currentFolder: KTailctl.Tailscale.taildrop.downloadDirectoryUrl
        onAccepted: KTailctl.Tailscale.taildrop.setDownloadDirectory(selectedFolder)
    }

    KDialogs.PromptDialog {
        id: deleteDialog
        dialogType: KDialogs.PromptDialog.Warning
        title: i18n("Delete file?")
        subtitle: i18n("Delete %1 without saving it?", page.pendingDeleteName)
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: KTailctl.Tailscale.taildrop.deleteFile(page.pendingDeleteName)
    }

    DaemonError {}

    OperatorWarning {}

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: KTailctl.Tailscale.taildrop.statusMessage !== ""
        type: KTailctl.Tailscale.taildrop.lastError !== "" ? Kirigami.MessageType.Error : Kirigami.MessageType.Positive
        text: KTailctl.Tailscale.taildrop.statusMessage
    }

    FormCard.FormHeader {
        title: i18n("Waiting files")
    }

    FormCard.FormCard {
        visible: KTailctl.Tailscale.taildrop.waitingFileCount === 0

        FormCard.FormTextDelegate {
            text: i18n("No files waiting")
            description: i18n("Files sent to this device show up here until you save them")
        }
    }

    FormCard.FormCard {
        visible: KTailctl.Tailscale.taildrop.waitingFileCount > 0

        Repeater {
            model: KTailctl.Tailscale.taildrop.waitingFiles

            delegate: FormCard.AbstractFormDelegate {
                id: fileDelegate

                required property string name
                required property string formattedSize

                background: null
                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Kirigami.Icon {
                        source: "text-plain"
                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: fileDelegate.name
                            elide: Text.ElideMiddle
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: fileDelegate.formattedSize
                            color: Kirigami.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }

                    QQC2.ToolButton {
                        icon.name: "document-save"
                        text: i18n("Save")
                        display: QQC2.AbstractButton.IconOnly
                        enabled: !KTailctl.Tailscale.taildrop.busy
                        onClicked: KTailctl.Tailscale.taildrop.saveFile(fileDelegate.name)

                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Save to %1", KTailctl.Tailscale.taildrop.downloadDirectory)
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    QQC2.ToolButton {
                        icon.name: "document-save-as"
                        display: QQC2.AbstractButton.IconOnly
                        enabled: !KTailctl.Tailscale.taildrop.busy
                        onClicked: {
                            saveAsDialog.fileName = fileDelegate.name;
                            saveAsDialog.open();
                        }

                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Save to…")
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    QQC2.ToolButton {
                        icon.name: "edit-delete"
                        display: QQC2.AbstractButton.IconOnly
                        enabled: !KTailctl.Tailscale.taildrop.busy
                        onClicked: {
                            page.pendingDeleteName = fileDelegate.name;
                            deleteDialog.open();
                        }

                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: i18n("Delete")
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }
        }
    }

    FolderDialog {
        id: saveAsDialog
        property string fileName: ""
        title: i18n("Save %1", fileName)
        currentFolder: KTailctl.Tailscale.taildrop.downloadDirectoryUrl
        onAccepted: KTailctl.Tailscale.taildrop.saveFileTo(fileName, selectedFolder)
    }

    FormCard.FormHeader {
        title: i18n("Download folder")
    }

    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: KTailctl.Tailscale.taildrop.downloadDirectory
            description: i18n("Accepted files are saved here")
            icon.name: "folder-downloads"
            onClicked: page.openFolderDialog()
        }
    }
}
