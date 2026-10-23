import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string recipeId: ""
    property var currentRecipe: ({})
    property int currentServings: 1

    signal backRequested()
    signal showToast(string message)

    onRecipeIdChanged: {
        loadRecipeData();
    }

    function loadRecipeData() {
        if (recipeId === "") return;
        currentRecipe = recipeModel.getRecipeById(recipeId);
        currentServings = currentRecipe.servings || 1;
        updateIngredients();
    }

    function updateIngredients() {
        if (recipeId === "") return;
        ingredientsRepeater.model = recipeModel.getScaledIngredients(recipeId, currentServings);
    }

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

                // Top Navigation & Action Bar
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: qsTr("← Zurück")
                        font.bold: true
                        onClicked: root.backRequested()
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: (currentRecipe.isStarred ? "★ " : "☆ ") + qsTr("Favorit")
                        font.bold: currentRecipe.isStarred
                        onClicked: {
                            recipeModel.toggleStarred(recipeId);
                            currentRecipe = recipeModel.getRecipeById(recipeId);
                        }
                    }
                }

                // Recipe Title Card
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: titleCol.implicitHeight + 24
                    radius: 12
                    color: "#FFFFFF"
                    border.color: "#E0E6E0"
                    border.width: 1

                    ColumnLayout {
                        id: titleCol
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Rectangle {
                                implicitHeight: 24
                                implicitWidth: catText.implicitWidth + 16
                                radius: 12
                                color: "#E8F5E9"

                                Text {
                                    id: catText
                                    anchors.centerIn: parent
                                    text: currentRecipe.category || ""
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#2E7D32"
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                implicitHeight: 24
                                implicitWidth: canMakeText.implicitWidth + 16
                                radius: 12
                                color: currentRecipe.canMake ? "#E8F5E9" : "#FFF3E0"

                                Text {
                                    id: canMakeText
                                    anchors.centerIn: parent
                                    text: currentRecipe.canMake ? qsTr("✓ Kochbereit") : qsTr("⚠ Zutaten fehlen")
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: currentRecipe.canMake ? "#2E7D32" : "#E65100"
                                }
                            }
                        }

                        Text {
                            text: currentRecipe.dishName || ""
                            font.pixelSize: 22
                            font.bold: true
                            color: "#1B241B"
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                }

                // Serving Size Recalculation Card
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: servingsCol.implicitHeight + 24
                    radius: 12
                    color: "#FFFFFF"
                    border.color: "#E0E6E0"
                    border.width: 1

                    ColumnLayout {
                        id: servingsCol
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: qsTr("Portionen anpassen:")
                                font.pixelSize: 16
                                font.bold: true
                                color: "#1B241B"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            // Minus button
                            Button {
                                text: "−"
                                font.pixelSize: 18
                                font.bold: true
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 38
                                enabled: root.currentServings > 1
                                onClicked: {
                                    if (root.currentServings > 1) {
                                        root.currentServings--;
                                        root.updateIngredients();
                                    }
                                }
                            }

                            // Serving display
                            Rectangle {
                                Layout.preferredWidth: 48
                                Layout.preferredHeight: 38
                                radius: 8
                                color: "#F4F6F4"
                                border.color: "#CCD4CC"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: root.currentServings.toString()
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#1B241B"
                                }
                            }

                            // Plus button
                            Button {
                                text: "+"
                                font.pixelSize: 18
                                font.bold: true
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 38
                                enabled: root.currentServings < 50
                                onClicked: {
                                    root.currentServings++;
                                    root.updateIngredients();
                                }
                            }
                        }

                        Text {
                            text: qsTr("Mengen werden automatisch für %1 Portion(en) umgerechnet (Basis: %2).")
                                .arg(root.currentServings)
                                .arg(currentRecipe.servings || 1)
                            font.pixelSize: 12
                            color: "#5A6B5C"
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }
                }

                // Ingredients List Card
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: ingCol.implicitHeight + 24
                    radius: 12
                    color: "#FFFFFF"
                    border.color: "#E0E6E0"
                    border.width: 1

                    ColumnLayout {
                        id: ingCol
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: qsTr("Zutaten")
                            font.pixelSize: 18
                            font.bold: true
                            color: "#1B241B"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                id: ingredientsRepeater

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 44
                                    radius: 8
                                    color: modelData.inPantry ? "#F1F8F1" : "#FFF8F6"
                                    border.color: modelData.inPantry ? "#C8E6C9" : "#FFCDD2"
                                    border.width: 1

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 8

                                        // Pantry badge
                                        Rectangle {
                                            implicitWidth: 20
                                            implicitHeight: 20
                                            radius: 10
                                            color: modelData.inPantry ? "#2E7D32" : "#D32F2F"

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData.inPantry ? "✓" : "✕"
                                                color: "#FFFFFF"
                                                font.pixelSize: 11
                                                font.bold: true
                                            }
                                        }

                                        // Name
                                        Text {
                                            text: modelData.name
                                            font.pixelSize: 14
                                            font.bold: true
                                            color: "#222222"
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        // Scaled Amount & Unit
                                        Text {
                                            text: (modelData.amount < 1 && modelData.amount > 0)
                                                ? Number(modelData.amount).toFixed(2) + " " + modelData.unit
                                                : Math.round(modelData.amount * 10) / 10 + " " + modelData.unit
                                            font.pixelSize: 14
                                            font.bold: true
                                            color: "#1B5E20"
                                        }

                                        // Direct Pantry Toggle
                                        Button {
                                            text: modelData.inPantry ? qsTr("Vorrätig") : qsTr("Fehlt")
                                            flat: true
                                            font.pixelSize: 11
                                            onClicked: {
                                                pantryModel.setItemAvailable(modelData.name, !modelData.inPantry);
                                                root.updateIngredients();
                                                currentRecipe = recipeModel.getRecipeById(recipeId);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Preparation Instructions Card (Markdown rendered)
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: prepCol.implicitHeight + 24
                    radius: 12
                    color: "#FFFFFF"
                    border.color: "#E0E6E0"
                    border.width: 1

                    ColumnLayout {
                        id: prepCol
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: qsTr("Zubereitung")
                                font.pixelSize: 18
                                font.bold: true
                                color: "#1B241B"
                                Layout.fillWidth: true
                            }

                            Button {
                                text: editInstructionsCol.visible ? qsTr("Fertig") : qsTr("✎ Bearbeiten")
                                flat: true
                                onClicked: editInstructionsCol.visible = !editInstructionsCol.visible
                            }
                        }

                        // Markdown instruction view
                        Text {
                            id: markdownView
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            textFormat: Text.MarkdownText
                            font.pixelSize: 14
                            lineHeight: 1.3
                            color: "#2B3A2C"
                            visible: currentRecipe.instructions && currentRecipe.instructions.trim().length > 0
                            text: currentRecipe.instructions || ""
                        }

                        // Placeholder if no instructions provided yet
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: !markdownView.visible && !editInstructionsCol.visible

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: placeholderCol.implicitHeight + 24
                                radius: 8
                                color: "#F8FBF8"
                                border.color: "#E0E6E0"
                                border.width: 1

                                ColumnLayout {
                                    id: placeholderCol
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4

                                    Text {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignHCenter
                                        wrapMode: Text.Wrap
                                        text: qsTr("📖 Noch keine Zubereitungsschritte hinterlegt.")
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: "#5A6B5C"
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        horizontalAlignment: Text.AlignHCenter
                                        wrapMode: Text.Wrap
                                        text: qsTr("Klicke auf 'Bearbeiten', um eine Markdown-Anleitung einzugeben.")
                                        font.pixelSize: 11
                                        color: "#7E8C7E"
                                    }
                                }
                            }
                        }

                        // Edit Instructions field
                        ColumnLayout {
                            id: editInstructionsCol
                            Layout.fillWidth: true
                            visible: false
                            spacing: 8

                            Text {
                                text: qsTr("Markdown-Anleitung für dieses Rezept bearbeiten:")
                                font.pixelSize: 12
                                color: "#5A6B5C"
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                                color: "#FAFAFA"
                                radius: 8
                                border.color: editArea.activeFocus ? "#2E7D32" : "#CCD4CC"
                                border.width: 1

                                TextArea {
                                    id: editArea
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    text: currentRecipe.instructions || ""
                                    placeholderText: qsTr("### 1. Vorbereitung\nAlle Zutaten bereitstellen...\n\n### 2. Kochen\nZusammen erhitzen und abschmecken.")
                                    wrapMode: TextEdit.Wrap
                                    selectByMouse: true
                                }
                            }

                            Button {
                                text: qsTr("Speichern")
                                onClicked: {
                                    recipeRepo.updateRecipeInstructions(recipeId, editArea.text);
                                    currentRecipe = recipeModel.getRecipeById(recipeId);
                                    editInstructionsCol.visible = false;
                                    root.showToast(qsTr("Zubereitungsanleitung gespeichert!"));
                                }
                            }
                        }
                    }
                }

                Item { Layout.preferredHeight: 24 }
            }
        }
    }
}
