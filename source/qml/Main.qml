import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: appWindow
    width: 480
    height: 800
    minimumWidth: 320
    minimumHeight: 520
    visible: true
    title: qsTr("Mahlzeit - Smarte Vorrats- & Rezeptplanung")
    color: "#F6F8F6"

    property int activeTab: 2 // Default to Recipes tab
    property string activeRecipeId: ""
    property bool inDetailView: false

    // Toast helper
    function showNotification(msg) {
        toastText.text = msg;
        toastAnimation.stop();
        toastBox.opacity = 1.0;
        toastAnimation.start();
    }

    // Main Layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // App Header (Desktop / Mobile)
        Rectangle {
            Layout.fillWidth: true
            height: 52
            color: "#2E7D32"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "🥗 Mahlzeit"
                    font.pixelSize: 20
                    font.bold: true
                    color: "#FFFFFF"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: inDetailView ? qsTr("Rezeptdetails") :
                          (activeTab === 0 ? qsTr("Vorratskammer") :
                           activeTab === 1 ? qsTr("Einkaufsliste") :
                           activeTab === 2 ? qsTr("Rezepte") : qsTr("Einstellungen"))
                    font.pixelSize: 14
                    color: "#E8F5E9"
                }
            }
        }

        // Content Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Screen Views Stack
            StackLayout {
                id: stackLayout
                anchors.fill: parent
                currentIndex: inDetailView ? 4 : activeTab

                // 0: Pantry
                PantryView {
                    id: pantryView
                }

                // 1: Shopping List
                ShoppingListView {
                    id: shoppingListView
                    onShowToast: function(msg) { appWindow.showNotification(msg); }
                }

                // 2: Recipes
                RecipesView {
                    id: recipesView
                    onRecipeSelected: function(recipeId) {
                        appWindow.activeRecipeId = recipeId;
                        appWindow.inDetailView = true;
                    }
                }

                // 3: Settings
                SettingsView {
                    id: settingsView
                    onShowToast: function(msg) { appWindow.showNotification(msg); }
                }

                // 4: Recipe Detail View
                RecipeDetailView {
                    id: recipeDetailView
                    recipeId: appWindow.activeRecipeId
                    onBackRequested: {
                        appWindow.inDetailView = false;
                    }
                    onShowToast: function(msg) { appWindow.showNotification(msg); }
                }
            }
        }

        // Bottom Navigation Bar (Hidden when in recipe detail view on small screens)
        Rectangle {
            Layout.fillWidth: true
            height: inDetailView ? 0 : 64
            visible: !inDetailView
            color: "#FFFFFF"
            border.color: "#E0E6E0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // 1. Pantry Tab
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: activeTab === 0 ? "#E8F5E9" : "transparent"

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            inDetailView = false;
                            activeTab = 0;
                        }
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "🥫"
                            font.pixelSize: 18
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Vorrat")
                            font.pixelSize: 11
                            font.bold: activeTab === 0
                            color: activeTab === 0 ? "#2E7D32" : "#666666"
                        }
                    }
                }

                // 2. Shopping List Tab
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: activeTab === 1 ? "#E8F5E9" : "transparent"

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            inDetailView = false;
                            activeTab = 1;
                        }
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "🛒"
                            font.pixelSize: 18
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Einkauf (%1)").arg(shoppingModel.itemCount)
                            font.pixelSize: 11
                            font.bold: activeTab === 1
                            color: activeTab === 1 ? "#2E7D32" : "#666666"
                        }
                    }
                }

                // 3. Recipes Tab
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: activeTab === 2 ? "#E8F5E9" : "transparent"

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            inDetailView = false;
                            activeTab = 2;
                        }
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "📖"
                            font.pixelSize: 18
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Rezepte")
                            font.pixelSize: 11
                            font.bold: activeTab === 2
                            color: activeTab === 2 ? "#2E7D32" : "#666666"
                        }
                    }
                }

                // 4. Settings Tab
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: activeTab === 3 ? "#E8F5E9" : "transparent"

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            inDetailView = false;
                            activeTab = 3;
                        }
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "⚙"
                            font.pixelSize: 18
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Einstellungen")
                            font.pixelSize: 11
                            font.bold: activeTab === 3
                            color: activeTab === 3 ? "#2E7D32" : "#666666"
                        }
                    }
                }
            }
        }
    }

    // Toast Notification Banner
    Rectangle {
        id: toastBox
        anchors.bottom: parent.bottom
        anchors.bottomMargin: inDetailView ? 24 : 80
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 48, toastText.implicitWidth + 32)
        height: 42
        radius: 21
        color: "#1B241B"
        opacity: 0.0

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "#FFFFFF"
            font.pixelSize: 13
            font.bold: true
        }

        SequentialAnimation {
            id: toastAnimation
            PauseAnimation { duration: 2500 }
            NumberAnimation { target: toastBox; property: "opacity"; to: 0.0; duration: 400 }
        }
    }
}
