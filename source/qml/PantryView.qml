import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

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
                    text: qsTr("Vorratskammer")
                    font.pixelSize: 24
                    font.bold: true
                    color: "#1B241B"
                }

                Text {
                    text: qsTr("%1 von %2 Zutaten vorrätig").arg(pantryModel.availableCount).arg(pantryModel.totalCount)
                    font.pixelSize: 14
                    color: "#5A6B5C"
                }
            }

            Button {
                text: qsTr("Alle an")
                flat: true
                onClicked: pantryModel.selectAll()
            }

            Button {
                text: qsTr("Alle aus")
                flat: true
                onClicked: pantryModel.clearAll()
            }
        }

        // Progress Bar
        Rectangle {
            Layout.fillWidth: true
            height: 6
            radius: 3
            color: "#E0E6E0"
            clip: true

            Rectangle {
                width: parent.width * (pantryModel.availableCount / Math.max(1, pantryModel.totalCount))
                height: parent.height
                radius: 3
                color: "#2E7D32"

                Behavior on width {
                    NumberAnimation { duration: 150 }
                }
            }
        }

        // Search & Filter
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Zutat suchen...")
                text: pantryModel.filterText
                onTextChanged: pantryModel.filterText = text
                selectByMouse: true

                background: Rectangle {
                    color: "#FFFFFF"
                    radius: 8
                    border.color: searchField.activeFocus ? "#2E7D32" : "#D0D7D0"
                    border.width: 1
                }
            }

            ComboBox {
                id: filterCombo
                model: [qsTr("Alle"), qsTr("Vorrätig"), qsTr("Fehlend")]
                currentIndex: pantryModel.filterMode
                onActivated: pantryModel.filterMode = currentIndex
                Layout.preferredWidth: 110
            }
        }

        // List View
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            model: pantryModel

            delegate: Rectangle {
                width: listView.width
                height: 52
                radius: 10
                color: model.available ? "#E8F5E9" : "#FFFFFF"
                border.color: model.available ? "#A5D6A7" : "#E2E8E2"
                border.width: 1

                MouseArea {
                    anchors.fill: parent
                    onClicked: pantryModel.toggleAvailable(index)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 12
                    spacing: 12

                    // Status Indicator
                    Rectangle {
                        width: 24
                        height: 24
                        radius: 12
                        color: model.available ? "#2E7D32" : "#F0F2F0"
                        border.color: model.available ? "#2E7D32" : "#B0BAB0"
                        border.width: 1.5

                        Text {
                            anchors.centerIn: parent
                            text: model.available ? "✓" : ""
                            color: "#FFFFFF"
                            font.bold: true
                            font.pixelSize: 14
                        }
                    }

                    // Ingredient Name
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: model.name
                            font.pixelSize: 15
                            font.bold: model.available
                            color: model.available ? "#1B5E20" : "#222222"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: qsTr("Wird in %n Rezept(en) gebraucht", "", model.recipeCount)
                            font.pixelSize: 11
                            color: "#6B7A6C"
                        }
                    }

                    // // Toggle Button
                    // Button {
                    //     text: model.available ? qsTr("Vorrätig") : qsTr("Hinzufügen")
                    //     flat: !model.available
                    //     font.pixelSize: 12
                    //     onClicked: pantryModel.toggleAvailable(index)
                    // }
                }
            }

            // Empty state
            Text {
                anchors.centerIn: parent
                visible: listView.count === 0
                text: qsTr("Keine Zutaten gefunden.")
                font.pixelSize: 16
                color: "#7E8C7E"
            }
        }
    }
}
