import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("Vorratskammer")
                    font.pixelSize: 22
                    font.bold: true
                    color: "#1B241B"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: qsTr("%1 von %2 Zutaten vorrätig").arg(pantryModel.availableCount).arg(pantryModel.totalCount)
                    font.pixelSize: 13
                    color: "#5A6B5C"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
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

        // Search Field
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Zutat oder Kategorie suchen...")
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

        // Category, Status & Sort Controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ComboBox {
                id: categoryCombo
                model: pantryModel.availableCategories
                Layout.fillWidth: true
                onActivated: pantryModel.categoryFilter = currentText
            }

            ComboBox {
                id: filterCombo
                model: [qsTr("Alle"), qsTr("Vorrätig"), qsTr("Fehlend")]
                currentIndex: pantryModel.filterMode
                onActivated: pantryModel.filterMode = currentIndex
                Layout.preferredWidth: 92
            }

            ComboBox {
                id: sortCombo
                model: [qsTr("A - Z"), qsTr("🛒 Gang")]
                currentIndex: pantryModel.sortOrder
                onActivated: pantryModel.sortOrder = currentIndex
                Layout.preferredWidth: 96
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

            section.property: pantryModel.sortOrder === 1 ? "category" : ""
            section.criteria: ViewSection.FullString
            section.delegate: Rectangle {
                width: listView.width
                height: 32
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 6

                    Rectangle {
                        implicitHeight: 22
                        implicitWidth: secLabel.implicitWidth + 16
                        radius: 6
                        color: "#E8F5E9"
                        border.color: "#C8E6C9"
                        border.width: 1

                        Text {
                            id: secLabel
                            anchors.centerIn: parent
                            text: section
                            font.pixelSize: 12
                            font.bold: true
                            color: "#1B5E20"
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#DCE4DC"
                    }
                }
            }

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
                        implicitWidth: 24
                        implicitHeight: 24
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

                    // Ingredient Name & Category
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: model.name
                                font.pixelSize: 15
                                font.bold: model.available
                                color: model.available ? "#1B5E20" : "#222222"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                visible: pantryModel.sortOrder === 0 && model.category.length > 0
                                implicitHeight: 18
                                implicitWidth: Math.min(130, catBadge.implicitWidth + 10)
                                radius: 9
                                color: model.available ? "#D7EED8" : "#F1F6F1"
                                border.color: "#C8DEC8"
                                border.width: 1

                                Text {
                                    id: catBadge
                                    anchors.fill: parent
                                    anchors.leftMargin: 5
                                    anchors.rightMargin: 5
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    text: model.category
                                    font.pixelSize: 10
                                    color: "#3E523F"
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        Text {
                            text: qsTr("Wird in %n Rezept(en) gebraucht", "", model.recipeCount)
                            font.pixelSize: 11
                            color: "#6B7A6C"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
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
