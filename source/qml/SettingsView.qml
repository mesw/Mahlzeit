import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal showToast(string message)

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
        clip: true

        ColumnLayout {
            width: parent.width
            anchors.margins: 16
            spacing: 16

            // Header
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("Einstellungen")
                    font.pixelSize: 24
                    font.bold: true
                    color: "#1B241B"
                }

                Text {
                    text: qsTr("Kategorien, Essenszeiten, Cloudflare-Sync & Rezepte verwalten")
                    font.pixelSize: 13
                    color: "#5A6B5C"
                }
            }

            // 1. Time Slots & Categories Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: timeSlotsCol.implicitHeight + 24
                radius: 12
                color: "#FFFFFF"
                border.color: "#E0E6E0"
                border.width: 1

                ColumnLayout {
                    id: timeSlotsCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: qsTr("Zeitfenster für Rezeptkategorien")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#1B241B"
                            }

                            Text {
                                text: qsTr("Definiert, wann Rezepte einer Kategorie als 'Passend zur Tageszeit' gelten.")
                                font.pixelSize: 12
                                color: "#5A6B5C"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            text: qsTr("Standard")
                            flat: true
                            onClicked: {
                                settingsManager.resetTimeSlotsToDefault();
                                root.showToast(qsTr("Zeiten auf Standardwerte zurückgesetzt."));
                            }
                        }
                    }

                    // List of time slots
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: settingsManager.timeSlots

                            Rectangle {
                                Layout.fillWidth: true
                                height: 50
                                radius: 8
                                color: "#F7FAF7"
                                border.color: "#E2EBE2"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 8

                                    Text {
                                        text: modelData.category
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#2B3A2C"
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    // Display time range
                                    Rectangle {
                                        height: 28
                                        width: rangeText.implicitWidth + 14
                                        radius: 6
                                        color: "#E8F5E9"

                                        Text {
                                            id: rangeText
                                            anchors.centerIn: parent
                                            text: modelData.timeRangeString
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: "#2E7D32"
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Quick custom slot adder
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        ComboBox {
                            id: editCatCombo
                            Layout.fillWidth: true
                            model: [
                                "Frühstück & Breie",
                                "Shakes",
                                "Aufstriche & Salate",
                                "Suppen & Nudelgerichte",
                                "Hauptgerichte",
                                "Süßspeisen",
                                "Morning Snack",
                                "Lunch",
                                "Afternoon Snack",
                                "Evening Snack",
                                "Dinner"
                            ]
                        }

                        SpinBox {
                            id: startHourSpin
                            from: 0
                            to: 23
                            value: 12
                            Layout.preferredWidth: 65
                        }

                        Text { text: "bis"; font.pixelSize: 12; color: "#5A6B5C" }

                        SpinBox {
                            id: endHourSpin
                            from: 0
                            to: 23
                            value: 14
                            Layout.preferredWidth: 65
                        }

                        Button {
                            text: qsTr("Setzen")
                            onClicked: {
                                settingsManager.updateTimeSlot(editCatCombo.currentText, startHourSpin.value, 0, endHourSpin.value, 0);
                                root.showToast(qsTr("Zeitfenster aktualisiert!"));
                            }
                        }
                    }
                }
            }

            // 2. Cloudflare Durable Object Sync Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: syncCol.implicitHeight + 24
                radius: 12
                color: "#FFFFFF"
                border.color: "#E0E6E0"
                border.width: 1

                ColumnLayout {
                    id: syncCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        text: qsTr("Cloudflare Vorratskammer-Sync (Durable Objects)")
                        font.pixelSize: 16
                        font.bold: true
                        color: "#1B241B"
                    }

                    Text {
                        text: qsTr("Teile Vorratskammer & Einkaufsliste privat mit anderen. Benötigt einen eigenen Cloudflare Worker mit Durable Object (siehe Anleitung im Ordner cloudflare/).")
                        font.pixelSize: 12
                        color: "#5A6B5C"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    TextField {
                        id: syncUrlField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Worker URL (z.B. https://mein-worker.workers.dev)")
                        text: settingsManager.syncUrl
                        onEditingFinished: settingsManager.syncUrl = text
                        selectByMouse: true

                        background: Rectangle {
                            color: "#FAFAFA"
                            radius: 8
                            border.color: "#D0D7D0"
                            border.width: 1
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: syncRoomField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Gemeinsame Raum-ID (z.B. familie-meier)")
                            text: settingsManager.syncRoomId
                            onEditingFinished: settingsManager.syncRoomId = text
                            selectByMouse: true

                            background: Rectangle {
                                color: "#FAFAFA"
                                radius: 8
                                border.color: "#D0D7D0"
                                border.width: 1
                            }
                        }

                        TextField {
                            id: syncTokenField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Geheimer Token (optional)")
                            echoMode: TextInput.Password
                            text: settingsManager.syncToken
                            onEditingFinished: settingsManager.syncToken = text
                            selectByMouse: true

                            background: Rectangle {
                                color: "#FAFAFA"
                                radius: 8
                                border.color: "#D0D7D0"
                                border.width: 1
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Button {
                            text: syncManager.isSyncing ? qsTr("Synchronisiere...") : qsTr("🔄 Jetzt synchronisieren")
                            enabled: !syncManager.isSyncing
                            onClicked: syncManager.syncNow()
                        }

                        Text {
                            text: syncManager.lastSyncStatus
                            font.pixelSize: 12
                            color: "#5A6B5C"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // 3. Recipe Management & Remote Refresh Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: repoCol.implicitHeight + 24
                radius: 12
                color: "#FFFFFF"
                border.color: "#E0E6E0"
                border.width: 1

                ColumnLayout {
                    id: repoCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        text: qsTr("Rezepte-Katalog & Aktualisierung")
                        font.pixelSize: 16
                        font.bold: true
                        color: "#1B241B"
                    }

                    Text {
                        text: qsTr("Geladene Rezepte: %1. Neue oder geänderte Standardrezepte können online von mesw.github.io/Malzeit/res/recipies.json ohne Neukompilierung nachgeladen werden.")
                            .arg(recipeRepo.recipeCount)
                        font.pixelSize: 12
                        color: "#5A6B5C"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Button {
                            text: recipeRepo.isRefreshing ? qsTr("Lade...") : qsTr("🌐 Von Webseite aktualisieren")
                            enabled: !recipeRepo.isRefreshing
                            onClicked: recipeRepo.refreshFromWeb()
                        }

                        Button {
                            text: qsTr("Auf Werksrezepte zurücksetzen")
                            flat: true
                            onClicked: recipeRepo.resetToBundledDefaults()
                        }
                    }

                    Text {
                        visible: recipeRepo.lastRefreshStatus.length > 0
                        text: recipeRepo.lastRefreshStatus
                        font.pixelSize: 12
                        color: "#2E7D32"
                        Layout.fillWidth: true
                    }
                }
            }

            // 4. Privacy & Local Storage Card
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: privCol.implicitHeight + 24
                radius: 12
                color: "#F4F7F4"
                border.color: "#C8D6C8"
                border.width: 1

                ColumnLayout {
                    id: privCol
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    Text {
                        text: qsTr("🔒 Datenschutz & Lokale Datenspeicherung")
                        font.pixelSize: 15
                        font.bold: true
                        color: "#1B5E20"
                    }

                    Text {
                        text: qsTr("1. Alle Vorratsdaten, Einkaufslisten und Favoriten werden ausschließlich lokal auf deinem Gerät gespeichert (IndexedDB / QSettings).\n2. Es erfolgt kein Tracking, keine Analyse und kein Phoning-Home.\n3. Eine Cloud-Verbindung erfolgt nur dann, wenn du deinen eigenen Cloudflare-Worker eingetragen hast.")
                        font.pixelSize: 12
                        color: "#37474F"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    Button {
                        text: qsTr("Alle lokalen Daten löschen")
                        flat: true
                        onClicked: {
                            settingsManager.clearAllUserData();
                            root.showToast(qsTr("Alle lokalen Benutzerdaten wurden gelöscht."));
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 32 }
        }
    }
}
