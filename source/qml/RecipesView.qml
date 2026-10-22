import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal recipeSelected(string recipeId)

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
                    text: qsTr("Rezepte")
                    font.pixelSize: 24
                    font.bold: true
                    color: "#1B241B"
                }

                Text {
                    text: qsTr("%n Rezept(e) gefunden", "", recipeModel.count)
                    font.pixelSize: 13
                    color: "#5A6B5C"
                }
            }

            ComboBox {
                id: sortCombo
                model: [qsTr("💡 Vorschlag"), qsTr("Name (A-Z)"), qsTr("Kategorie"), qsTr("Fehlende Zutaten")]
                currentIndex: recipeModel.sortMode
                onActivated: recipeModel.sortMode = currentIndex
                Layout.preferredWidth: 140
            }
        }

        // Search Bar
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Rezept oder Zutat suchen...")
            text: recipeModel.searchQuery
            onTextChanged: recipeModel.searchQuery = text
            selectByMouse: true

            background: Rectangle {
                color: "#FFFFFF"
                radius: 8
                border.color: searchField.activeFocus ? "#2E7D32" : "#D0D7D0"
                border.width: 1
            }
        }

        // Suggestion System Filter Switches (Default ON as requested)
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Ready to cook switch
            RowLayout {
                spacing: 4
                Switch {
                    id: readySwitch
                    checked: recipeModel.onlyReadyToCook
                    onToggled: recipeModel.onlyReadyToCook = checked
                }
                Text {
                    text: qsTr("Kochbereit")
                    font.pixelSize: 12
                    font.bold: recipeModel.onlyReadyToCook
                    color: recipeModel.onlyReadyToCook ? "#2E7D32" : "#555555"
                }
            }

            // Time slot switch
            RowLayout {
                spacing: 4
                Switch {
                    id: timeSwitch
                    checked: recipeModel.onlyTimeMatch
                    onToggled: recipeModel.onlyTimeMatch = checked
                }
                Text {
                    text: qsTr("Passende Uhrzeit")
                    font.pixelSize: 12
                    font.bold: recipeModel.onlyTimeMatch
                    color: recipeModel.onlyTimeMatch ? "#1565C0" : "#555555"
                }
            }

            // Starred switch
            RowLayout {
                spacing: 4
                Switch {
                    id: starSwitch
                    checked: recipeModel.onlyStarred
                    onToggled: recipeModel.onlyStarred = checked
                }
                Text {
                    text: qsTr("★ Favoriten")
                    font.pixelSize: 12
                    font.bold: recipeModel.onlyStarred
                    color: recipeModel.onlyStarred ? "#FF8F00" : "#555555"
                }
            }
        }

        // Category Pills (Horizontal Scroll)
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            Row {
                spacing: 6

                Repeater {
                    model: recipeModel.availableCategories

                    Rectangle {
                        id: catPill
                        height: 32
                        width: catText.implicitWidth + 24
                        radius: 16
                        color: (recipeModel.categoryFilter === modelData || (modelData === "Alle" && recipeModel.categoryFilter === ""))
                            ? "#2E7D32" : "#EAEFEA"

                        Text {
                            id: catText
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 12
                            font.bold: (recipeModel.categoryFilter === modelData || (modelData === "Alle" && recipeModel.categoryFilter === ""))
                            color: (recipeModel.categoryFilter === modelData || (modelData === "Alle" && recipeModel.categoryFilter === ""))
                                ? "#FFFFFF" : "#37474F"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (modelData === "Alle") {
                                    recipeModel.categoryFilter = "";
                                } else {
                                    recipeModel.categoryFilter = modelData;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Recipes ListView
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            model: recipeModel

            delegate: Rectangle {
                width: listView.width
                implicitHeight: cardCol.implicitHeight + 24
                radius: 12
                color: "#FFFFFF"
                border.color: model.isStarred ? "#FFD54F" : "#E2E8E2"
                border.width: model.isStarred ? 1.5 : 1

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.recipeSelected(model.id)
                }

                ColumnLayout {
                    id: cardCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    // Top Row: Category + Star Button
                    RowLayout {
                        Layout.fillWidth: true

                        Rectangle {
                            height: 22
                            width: catBadgeText.implicitWidth + 14
                            radius: 11
                            color: "#E8F5E9"

                            Text {
                                id: catBadgeText
                                anchors.centerIn: parent
                                text: model.category
                                font.pixelSize: 11
                                font.bold: true
                                color: "#2E7D32"
                            }
                        }

                        // Time match badge
                        Rectangle {
                            visible: model.isTimeMatch
                            height: 22
                            width: timeBadgeText.implicitWidth + 14
                            radius: 11
                            color: "#E3F2FD"

                            Text {
                                id: timeBadgeText
                                anchors.centerIn: parent
                                text: qsTr("🕒 Jetzt passend")
                                font.pixelSize: 11
                                color: "#1976D2"
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Star Button
                        Rectangle {
                            width: 32
                            height: 32
                            radius: 16
                            color: model.isStarred ? "#FFF8E1" : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: model.isStarred ? "★" : "☆"
                                font.pixelSize: 20
                                color: model.isStarred ? "#FFA000" : "#9E9E9E"
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: recipeModel.toggleStarred(model.id)
                            }
                        }
                    }

                    // Recipe Title
                    Text {
                        text: model.dishName
                        font.pixelSize: 17
                        font.bold: true
                        color: "#1B241B"
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    // Ingredient Status Banner
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        radius: 6
                        color: model.canMake ? "#E8F5E9" : "#FFF3E0"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 6

                            Text {
                                text: model.canMake ? "✓" : "⚠"
                                font.bold: true
                                font.pixelSize: 12
                                color: model.canMake ? "#2E7D32" : "#E65100"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: model.canMake
                                    ? qsTr("Alle Zutaten im Vorrat vorhanden!")
                                    : qsTr("Es fehlen %1 Zutaten (%2)").arg(model.missingCount).arg(model.missingSummary)
                                font.pixelSize: 12
                                color: model.canMake ? "#2E7D32" : "#B26A00"
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // Bottom info: Servings
                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: qsTr("Basisrezept: %n Portion(en)", "", model.servings)
                            font.pixelSize: 12
                            color: "#7E8C7E"
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: qsTr("Details ansehen →")
                            font.pixelSize: 12
                            font.bold: true
                            color: "#2E7D32"
                        }
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
                    text: "🔍"
                    font.pixelSize: 36
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Keine Rezepte für die aktuellen Filterkriterien.")
                    font.pixelSize: 15
                    font.bold: true
                    color: "#5A6B5C"
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Tipp: Schalte 'Kochbereit' oder 'Passende Uhrzeit' oben aus, um alle Rezepte zu sehen.")
                    font.pixelSize: 12
                    color: "#7E8C7E"
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Filter zurücksetzen")
                    onClicked: {
                        recipeModel.onlyReadyToCook = false;
                        recipeModel.onlyTimeMatch = false;
                        recipeModel.onlyStarred = false;
                        recipeModel.categoryFilter = "";
                        recipeModel.searchQuery = "";
                    }
                }
            }
        }
    }
}
