import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal showToast(string message)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("Einkaufsliste")
                    font.pixelSize: 24
                    font.bold: true
                    color: "#1B241B"
                }

                Text {
                    text: qsTr("%n Zutat(en) zu besorgen", "", shoppingModel.itemCount)
                    font.pixelSize: 14
                    color: "#5A6B5C"
                }
            }

            Button {
                text: qsTr("📋 Kopieren")
                font.bold: true
                onClicked: {
                    var listText = shoppingModel.formattedShoppingList();
                    // Copy to clipboard
                    clipboardHelper.copyText(listText);
                    root.showToast(qsTr("Einkaufsliste in die Zwischenablage kopiert!"));
                }
            }
        }

        // Filters and Controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Einkaufsliste filtern...")
                text: shoppingModel.searchQuery
                onTextChanged: shoppingModel.searchQuery = text
                selectByMouse: true

                background: Rectangle {
                    color: "#FFFFFF"
                    radius: 8
                    border.color: searchField.activeFocus ? "#2E7D32" : "#D0D7D0"
                    border.width: 1
                }
            }

            ComboBox {
                id: sortCombo
                model: [qsTr("A - Z"), qsTr("★ Favoriten"), qsTr("Häufigkeit")]
                currentIndex: shoppingModel.sortOrder
                onActivated: shoppingModel.sortOrder = currentIndex
                Layout.preferredWidth: 125
            }
        }

        // Starred Only Toggle Switch
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Switch {
                id: starredSwitch
                checked: shoppingModel.onlyStarred
                onToggled: shoppingModel.onlyStarred = checked
            }

            Text {
                text: qsTr("Nur Zutaten für Favoriten-Rezepte (★)")
                font.pixelSize: 13
                font.bold: shoppingModel.onlyStarred
                color: shoppingModel.onlyStarred ? "#B26A00" : "#333333"
                Layout.fillWidth: true
            }
        }

        // Shopping List Items
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: shoppingModel
            boundsBehavior: Flickable.StopAtBounds

            remove: Transition {
                NumberAnimation { property: "opacity"; to: 0.0; duration: 150 }
            }

            delegate: Rectangle {
                width: listView.width
                height: 64
                radius: 10
                color: "#FFFFFF"
                border.color: model.isUsedInStarred ? "#FFD54F" : "#E2E8E2"
                border.width: model.isUsedInStarred ? 1.5 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 12
                    spacing: 12

                    // Star / Favorite Badge
                    Rectangle {
                        width: 24
                        height: 24
                        radius: 12
                        color: model.isUsedInStarred ? "#FFF8E1" : "#F4F6F4"
                        border.color: model.isUsedInStarred ? "#FFA000" : "#D4DDD4"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: model.isUsedInStarred ? "★" : "•"
                            color: model.isUsedInStarred ? "#FFA000" : "#8A9A8A"
                            font.pixelSize: 14
                        }
                    }

                    // Ingredient & recipe usage
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: model.name
                            font.pixelSize: 15
                            font.bold: true
                            color: "#1B241B"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("Für: %1").arg(model.usedInRecipesSummary)
                            font.pixelSize: 12
                            color: "#6B7A6C"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    // Bought Button
                    Button {
                        text: qsTr("Gekauft ✓")
                        font.pixelSize: 12
                        onClicked: shoppingModel.markPurchased(index)
                    }
                }
            }

            // Empty state
            ColumnLayout {
                anchors.centerIn: parent
                visible: listView.count === 0
                spacing: 8

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🎉"
                    font.pixelSize: 40
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: shoppingModel.onlyStarred
                        ? qsTr("Keine fehlenden Zutaten für deine Favoriten!")
                        : qsTr("Alles vorrätig! Deine Vorratskammer ist voll.")
                    font.pixelSize: 16
                    font.bold: true
                    color: "#2E7D32"
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Du kannst alle passenden Rezepte sofort zubereiten.")
                    font.pixelSize: 13
                    color: "#5A6B5C"
                }
            }
        }
    }
}
