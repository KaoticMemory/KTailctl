import org.kde.kirigamiaddons.formcard as FormCard
import org.fkoehler.KTailctl as KTailctl
import org.kde.kirigami as Kirigami
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "qrc:/ui/components"

FormCard.FormCardPage {
    id: page
    property KTailctl.PeerStatus peer: KTailctl.Tailscale.status.self

    title: peer?.hostName ?? ""

    readonly property bool canOfferTaildrop: (peer?.id ?? "") !== "" && peer.id !== (KTailctl.Tailscale.status.self?.id ?? "")
    readonly property bool canSendTaildrop: canOfferTaildrop && KTailctl.Tailscale.taildrop.fileTargetIds.indexOf(peer.id) >= 0
    readonly property bool showTaildropStatus: canOfferTaildrop && KTailctl.Tailscale.taildrop.activePeerId === peer.id && KTailctl.Tailscale.taildrop.statusMessage !== ""

    signal closeRequested

    FileDialog {
        id: sendDialog
        title: i18n("Send files to %1", page.peer?.hostName ?? "")
        fileMode: FileDialog.OpenFiles
        currentFolder: KTailctl.Tailscale.taildrop.downloadDirectoryUrl
        onAccepted: KTailctl.Tailscale.taildrop.sendFiles(page.peer.id, selectedFiles)
    }

    actions: [
        Kirigami.Action {
            icon.name: "dialog-close"
            text: "Close"
            onTriggered: page.closeRequested()
        }
    ]

    DaemonError {}

    OperatorWarning {}

    AuthError {}

    FormCard.FormHeader {
        visible: page.canOfferTaildrop
        title: i18n("Taildrop")
    }

    FormCard.FormCard {
        visible: page.canOfferTaildrop

        DropArea {
            id: taildropDrop
            width: parent.width
            height: sendFilesButton.implicitHeight
            enabled: page.canSendTaildrop && !KTailctl.Tailscale.taildrop.busy
            onDropped: drop => {
                if (drop.hasUrls) {
                    KTailctl.Tailscale.taildrop.sendFiles(page.peer.id, drop.urls);
                }
            }

            FormCard.FormButtonDelegate {
                id: sendFilesButton
                width: parent.width
                enabled: page.canSendTaildrop && !KTailctl.Tailscale.taildrop.busy
                text: taildropDrop.containsDrag ? i18n("Drop to send") : i18n("Send files")
                description: page.canSendTaildrop ? i18n("Choose files or drop them here") : ((page.peer.noFileSharingReason ?? "") !== "" ? page.peer.noFileSharingReason : i18n("This peer cannot receive files right now"))
                icon.name: "document-send"
                onClicked: sendDialog.open()
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: page.showTaildropStatus
        type: KTailctl.Tailscale.taildrop.lastError !== "" ? Kirigami.MessageType.Error : Kirigami.MessageType.Information
        text: KTailctl.Tailscale.taildrop.statusMessage
    }

    FormCard.FormHeader {
        title: "Addresses"
    }

    FormCard.FormCard {
        Repeater {
            id: repeaterIps
            model: page.peer.tailscaleIps

            ColumnLayout {
                FormCard.FormButtonDelegate {
                    text: modelData
                    trailingLogo.source: "edit-copy"
                    onClicked: {
                        KTailctl.Util.setClipboardText(modelData);
                    }
                }
            }
        }
    }

    FormCard.FormHeader {
        visible: (peer?.tags?.length ?? 0) > 0
        title: "Tags"
    }

    FormCard.FormCard {

        visible: (peer?.tags?.length ?? 0) > 0

        FormCard.AbstractFormDelegate {
            background: null
            contentItem: Flow {
                spacing: Kirigami.Units.smallSpacing
                Repeater {
                    model: peer?.tags ?? []
                    Kirigami.Chip {
                        text: modelData
                        closable: false
                        onClicked: KTailctl.Util.setClipboardText(modelData)
                        checkable: false
                    }
                }
            }
        }
    }

    FormCard.FormHeader {
        title: "Identity"
    }

    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            id: peerId
            text: peer?.id ?? ""
            description: "Node ID"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.id);
            }
        }

        FormCard.FormButtonDelegate {
            id: publicKey
            text: peer?.publicKey ?? ""
            description: "Public Key"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.publicKey);
            }
        }

        FormCard.FormButtonDelegate {
            id: dnsName
            text: peer?.dnsName ?? ""
            description: "DNS Name"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.dnsName);
            }
        }

        FormCard.FormButtonDelegate {
            id: hostName
            text: peer?.hostName ?? ""
            description: "Host Name"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.hostName);
            }
        }

        FormCard.FormButtonDelegate {
            id: os
            visible: peer?.os
            text: peer?.os ?? ""
            description: "Operating System"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.os);
            }
        }

        FormCard.FormButtonDelegate {
            id: created
            visible: peer?.created && !isNaN(peer.created)
            text: KTailctl.Util.formatDurationHumanReadable(peer?.created)
            description: "Created"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.created.toISOString());
            }
        }

        FormCard.FormButtonDelegate {
            id: lastSeen
            visible: peer?.lastSeen && !isNaN(peer.lastSeen)
            text: KTailctl.Util.formatDurationHumanReadable(peer?.lastSeen)
            description: "Last Seen"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.lastSeen.toISOString());
            }
        }

        FormCard.FormLinkDelegate {
            id: adminPanelUrl
            url: (peer?.tailscaleIps?.length ?? 0) > 0 ? "https://login.tailscale.com/admin/machines/" + peer.tailscaleIps[0] : "https://login.tailscale.com/admin/machines/"
            text: adminPanelUrl.url
            description: "Admin Panel"
        }
    }

    FormCard.FormHeader {
        title: "Primary Routes"
    }

    FormCard.FormCard {
        visible: peer.primaryRoutes.length == 0
        FormCard.FormTextDelegate {
            text: "None"
        }
    }
    FormCard.FormCard {
        visible: peer.primaryRoutes.length > 0
        Repeater {
            id: repeaterRoutes
            model: peer.primaryRoutes

            ColumnLayout {
                FormCard.FormButtonDelegate {
                    text: modelData
                    trailingLogo.source: "edit-copy"
                    onClicked: {
                        KTailctl.Util.setClipboardText(modelData);
                    }
                }
            }
        }
    }

    FormCard.FormHeader {
        visible: peer.location !== null
        title: "Location"
    }

    FormCard.FormCard {
        visible: peer.location !== null
        FormCard.FormButtonDelegate {
            id: country
            text: peer.location?.country + " (" + peer.location?.countryCode + ")"
            description: "Country (Country Code)"
            leading: Kirigami.Icon {
                source: "qrc:/country-flags/country-flag-" + peer.location?.countryCode.toLowerCase()
            }
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.location.country);
            }
        }

        FormCard.FormButtonDelegate {
            id: city
            text: peer.location?.city + " (" + peer.location?.cityCode + ")"
            description: "City (City Code)"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.location.city);
            }
        }

        FormCard.FormButtonDelegate {
            id: latitude
            text: peer?.location?.latitude
            description: "Latitude"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.location.latitude);
            }
        }

        FormCard.FormButtonDelegate {
            id: longitude
            text: peer?.location?.longitude
            description: "Longitude"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.location.longitude);
            }
        }

        FormCard.FormButtonDelegate {
            id: priority
            text: peer?.location?.priority
            description: "Priority"
            trailingLogo.source: "edit-copy"
            onClicked: {
                KTailctl.Util.setClipboardText(peer.location.priority);
            }
        }
    }
}
