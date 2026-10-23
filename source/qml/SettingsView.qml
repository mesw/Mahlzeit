import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal showToast(string message)

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        contentHeight: contentContainer.implicitHeight
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Item {
            id: contentContainer
            width: scrollView.availableWidth
            implicitHeight: mainCol.implicitHeight + 32

            ColumnLayout {
                id: mainCol
                anchors.fill: parent
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
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Text {
                        text: qsTr("Kategorien, Supermärkte, Essenszeiten, Cloudflare-Sync & Rezepte verwalten")
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                        color: "#5A6B5C"
                        Layout.fillWidth: true
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
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: qsTr("Zeitfenster für Rezeptkategorien")
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#1B241B"
                                    Layout.fillWidth: true
                                    wrapMode: Text.Wrap
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
                                            implicitHeight: 28
                                            implicitWidth: rangeText.implicitWidth + 14
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
                        ColumnLayout {
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

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                SpinBox {
                                    id: startHourSpin
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 23
                                    value: 12
                                }

                                Text {
                                    text: qsTr("bis")
                                    font.pixelSize: 12
                                    color: "#5A6B5C"
                                }

                                SpinBox {
                                    id: endHourSpin
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 23
                                    value: 14
                                }
                            }

                            Button {
                                text: qsTr("Setzen")
                                Layout.fillWidth: true
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
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("Teile Vorratskammer & Einkaufsliste privat im Haushalt. Änderungen werden automatisch im Hintergrund per Delta-Sync übertragen.")
                            font.pixelSize: 12
                            color: "#5A6B5C"
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: syncUrlField
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            placeholderText: qsTr("Worker URL (z.B. https://mein-worker.workers.dev)")
                            text: settingsManager.syncUrl
                            onTextEdited: settingsManager.syncUrl = text
                            onEditingFinished: settingsManager.syncUrl = text
                            selectByMouse: true

                            background: Rectangle {
                                color: "#FAFAFA"
                                radius: 8
                                border.color: "#D0D7D0"
                                border.width: 1
                            }
                        }

                        TextField {
                            id: syncRoomField
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            placeholderText: qsTr("Gemeinsame Raum-ID (z.B. familie-meier)")
                            text: settingsManager.syncRoomId
                            onTextEdited: settingsManager.syncRoomId = text
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
                            Layout.minimumWidth: 0
                            placeholderText: qsTr("Geheimer Token (optional)")
                            echoMode: TextInput.Password
                            text: settingsManager.syncToken
                            onTextEdited: settingsManager.syncToken = text
                            onEditingFinished: settingsManager.syncToken = text
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

                            Switch {
                                id: autoSyncSwitch
                                checked: settingsManager.autoSync
                                onToggled: settingsManager.autoSync = checked
                            }

                            Text {
                                text: qsTr("Automatisch im Hintergrund synchronisieren")
                                font.pixelSize: 12
                                color: "#2B3A2C"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Button {
                                Layout.fillWidth: true
                                text: syncManager.isSyncing ? qsTr("Synchronisiere...") : qsTr("🔄 Jetzt synchronisieren")
                                enabled: !syncManager.isSyncing
                                onClicked: {
                                    settingsManager.syncUrl = syncUrlField.text;
                                    settingsManager.syncRoomId = syncRoomField.text;
                                    settingsManager.syncToken = syncTokenField.text;
                                    syncManager.syncNow();
                                }
                            }

                            Text {
                                visible: syncManager.lastSyncStatus.length > 0
                                text: syncManager.lastSyncStatus
                                font.pixelSize: 12
                                color: "#5A6B5C"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                // 3. Recipe Management, Store Layouts & Remote Refresh Card
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
                            text: qsTr("Rezepte-Katalog, Supermarkt-Gänge & Aktualisierung")
                            font.pixelSize: 16
                            font.bold: true
                            color: "#1B241B"
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            visible: recipeRepo.stores.length > 0

                            Text {
                                text: qsTr("Aktiver Supermarkt:")
                                font.pixelSize: 13
                                font.bold: true
                                color: "#2B3A2C"
                                Layout.fillWidth: true
                            }

                            ComboBox {
                                id: settingsStoreCombo
                                Layout.fillWidth: true
                                model: recipeRepo.stores
                                textRole: "name"
                                valueRole: "id"
                                currentIndex: {
                                    var stores = recipeRepo.stores;
                                    for (var i = 0; i < stores.length; ++i) {
                                        if (stores[i].id === settingsManager.activeStoreId) {
                                            return i;
                                        }
                                    }
                                    return 0;
                                }
                                onActivated: function(index) {
                                    var stores = recipeRepo.stores;
                                    if (index >= 0 && index < stores.length) {
                                        settingsManager.activeStoreId = stores[index].id;
                                    }
                                }
                            }
                        }

                        Text {
                            text: qsTr("Geladene Rezepte: %1. Du kannst das Repository forken, deine eigene recipes.json (inkl. Supermarkt-Gängen & Alias-Regeln) anpassen und die URL deines Forks hier eintragen:")
                                .arg(recipeRepo.recipeCount)
                            font.pixelSize: 12
                            color: "#5A6B5C"
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: recipesUrlField
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            placeholderText: qsTr("https://mesw.github.io/Malzeit/resources/recipes.json")
                            text: settingsManager.recipesUrl
                            onTextEdited: settingsManager.recipesUrl = text
                            onEditingFinished: settingsManager.recipesUrl = text
                            selectByMouse: true

                            background: Rectangle {
                                color: "#FAFAFA"
                                radius: 8
                                border.color: "#D0D7D0"
                                border.width: 1
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Button {
                                Layout.fillWidth: true
                                text: recipeRepo.isRefreshing ? qsTr("Lade...") : qsTr("🌐 Von URL aktualisieren")
                                enabled: !recipeRepo.isRefreshing
                                onClicked: {
                                    settingsManager.recipesUrl = recipesUrlField.text;
                                    recipeRepo.refreshFromWeb();
                                }
                            }

                            Button {
                                Layout.fillWidth: true
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
                            wrapMode: Text.Wrap
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
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("1. Alle Vorratsdaten, Einkaufslisten und Favoriten werden ausschließlich lokal auf deinem Gerät gespeichert (IndexedDB / QSettings).\n2. Es erfolgt kein Tracking, keine Analyse und kein Phoning-Home.\n3. Eine Cloud-Verbindung erfolgt nur dann, wenn du deinen eigenen Cloudflare-Worker eingetragen hast.")
                            font.pixelSize: 12
                            color: "#37474F"
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Alle lokalen Daten löschen")
                            flat: true

                            onClicked: confirmDeleteDialog.open()
                        }

                        Dialog {
                            id: confirmDeleteDialog

                            title: qsTr("Bestätigung")
                            modal: true
                            anchors.centerIn: parent
                            width: Math.min(280, root.width - 32)

                            standardButtons: Dialog.Yes | Dialog.No

                            Text {
                                width: parent.width
                                text: qsTr("Möchten Sie wirklich alle lokalen Benutzerdaten löschen?")
                                wrapMode: Text.Wrap
                            }

                            onAccepted: {
                                settingsManager.clearAllUserData()
                                root.showToast(qsTr("Alle lokalen Benutzerdaten wurden gelöscht."))
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 32 }
            }
        }
    }
}
