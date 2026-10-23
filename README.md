# Mahlzeit 🥗

**Mahlzeit** ist eine moderne, datenschutzfreundliche Web- und Mobil-Applikation zur smarten Vorrats- und Rezeptplanung, entwickelt in **modernem C++20** und **Qt 6.8.3 Quick / QML**.

Die App läuft im Browser (WebAssembly single-threaded), auf dem Desktop zu Entwicklungs- und Testzwecken und ist so aufgebaut, dass sie nahtlos als Git-Submodul in ein **Android-Projekt** eingebunden werden kann.

---

## 🔒 Datenschutz-Philosophie (100% Offline & Lokal)

1. **Ausschließlich lokale Datenspeicherung**: Alle Vorratsdaten, Einkaufslisten, Favoriten und individuellen Einstellungen werden ausschließlich lokal auf deinem Gerät gespeichert (im Webbrowser via HTML5 IndexedDB über `QSettings`, auf dem Desktop in der lokalen Konfigurationsdatei).
2. **Kein Tracking & kein Phoning-Home**: Die App erhebt keinerlei personenbezogene Daten, verwendet keine Tracking-Cookies und sendet keine Telemetrie- oder Nutzungsdaten an externe Server.
3. **Freiwillige Haushalts-Synchronisation**: Falls du deine Vorratskammer und Einkaufsliste mit Partnern oder Familie teilen möchtest, geschieht dies über deine **eigene, private Cloudflare Durable Objects-Instanz**. Es gibt keinen zentralen Betreiber-Server.

---

## 📱 Bildschirme & Funktionen

### 1. Vorratskammer (The Pantry)
- **Kanonische Zutatenübersicht**: Zeigt alle einzigartigen Zutaten automatisch aggregiert und normalisiert über Alias-Regeln (z. B. werden `"Dotter"`, `"Eiklar"` und `"Harte Eier"` unter `"Ei"` zusammengefasst; `"Äpfel"` unter `"Apfel"`).
- **Verfügbarkeit umschalten**: Mit einem Klick/Tap Zutaten als *vorrätig* oder *fehlend* markieren.
- **Kategorien, Schnellfilter & Suche**: Filter nach Zutatenkategorie, Vorrätig / Fehlend sowie Textsuche (findet auch Alias-Begriffe wie `"Erdäpfel"` $\rightarrow$ `"Kartoffeln"`).
- **Sortierung**: Alphabetisch (`A - Z`) oder nach Supermarkt-Gang-Reihenfolge (`🛒 Gang`).
- **Massenaktionen**: *Alle auswählen* oder *Alle leeren*.
- **Fortschrittsanzeige**: Zeigt genau an, wie viele Zutaten vorhanden sind.

### 2. Einkaufsliste (The Shopping List)
- **Automatische Bedarfsermittlung**: Zeigt alle kanonischen Zutaten, die sich aktuell *nicht* in der Vorratskammer befinden.
- **Supermarkt-Gang-Sortierung**: Standardmäßig sortiert nach der Reihenfolge der Abteilungen im ausgewählten Supermarkt (`🛒 Gang`), inklusive übersichtlicher Abteilungs-Überschriften:
  1. *Obst, Gemüse & Salat*
  2. *Brot, Müsli, Nüsse & Aufstriche*
  3. *Kühlregal (Eier, Fleisch, Käse, Frischmilch)*
  4. *Fischkonserven, Oliven & Saures*
  5. *Nudeln, Konserven, Backen & Vorrat*
  6. *Tiefkühlkost*
  7. *Säfte, Getränke & Tee*
- **Supermarkt-Profile wechseln**: Direkt in der Einkaufsliste oder in den Einstellungen zwischen verschiedenen Supermarkt-Grundrissen wechseln (z. B. *Stamm-Supermarkt* vs. *Discounter*).
- **Rezept-Kontext & Favoriten-Filter**: Zu jeder Zutat wird angezeigt, für welche Rezepte sie benötigt wird; optional nur Zutaten für Favoriten-Rezepte (`★`) anzeigen.
- **Direkt als gekauft markieren**: Klick auf `"Gekauft ✓"` überträgt die Zutat sofort in die Vorratskammer.
- **Liste kopieren**: Formatiert die Einkaufsliste (inkl. Abteilungs-Gruppierung) als Text-Checkliste und kopiert sie in die Zwischenablage.

### 3. Rezepte & Vorschlagssystem (The Recipes & Suggestion System)
- **36 gesunde Rezepte** mit integriertem Vorschlagssystem:
  - **Filter-Schalter**:
    - *Kochbereit*: Alle benötigten Zutaten im Vorrat vorhanden.
    - *Passende Uhrzeit*: Passt zur aktuellen Uhrzeit (z. B. Frühstück 04:00–09:30, Hauptgerichte 11:30–21:30).
    - *Nur Favoriten (`★`)*.
  - **Kategorie-Pills**: Schnelle Filterung nach Rezeptkategorie.
  - **Rezept-Karten**: Farbige Badges zeigen Kochbereitschaft sowie passende Tageszeit an.

### 4. Rezeptdetails & Portionen-Rechner (The Selected Recipe)
- **Vollständige Ansicht**: Rezepttitel, Kategorie, Kochbereitschaftsstatus und Favoriten-Button.
- **Dynamische Portionsgrößen**: Mit `−` und `+` die gewünschte Portionsanzahl wählen. Alle Zutatenmengen werden in Echtzeit neu berechnet.
- **Zutatenliste mit Vorrats-Status**: Jede Zutat behält ihren exakten Rezeptnamen (z. B. `"4 Stk. Dotter"`), prüft und schaltet im Hintergrund aber automatisch die kanonische Vorratszutat (`"Ei"`) um.
- **Formatierte Zubereitungsschritte (Markdown)**: Darstellung und direkte Bearbeitung von Markdown-Zubereitungsanleitungen.

### 5. Einstellungen (The Settings)
- **Kategorien & Zeitfenster**: Zeitfenster für Mahlzeiten flexibel bearbeiten oder auf Standard zurücksetzen.
- **Cloudflare Durable Object Sync**: Konfiguration von Worker-URL, Raum-ID und Token zur privaten Echtzeit-/Hintergrund-Synchronisation.
- **Rezepte-Katalog, Supermarkt-Gänge & Eigenes Fork-Hosting**: Die Datei `resources/recipes.json` bündelt `"aliases"`, `"categories"`, `"stores"` und `"recipes"` in einer einzigen Datei. Du kannst das Repository forken, deine Supermarkt-Gänge in `recipes.json` anpassen und die URL deines Forks in den Einstellungen hinterlegen.
- **Werksreset**: Zurücksetzen aller Einstellungen, Vorräte oder Rezepte.

---

## ☁️ Echtzeit Multi-User-Synchronisation (Cloudflare Durable Objects + WebSockets)

### Wie funktioniert der Sync?
Sobald **Worker URL** und **Raum-ID** in den Einstellungen eingetragen sind (und der Schalter *"Automatisch im Hintergrund synchronisieren"* aktiv ist), verbindet sich `SyncManager` per **`QWebSocket` (`wss://`)** mit deinem Cloudflare Durable Object (**WebSocket Hibernation API**):

1. **Sofortige Live-Verbindung & Hibernation (`wss://.../api/room/:roomId`)**:
   - Beim Starten der App baut `SyncManager` automatisch eine WebSocket-Verbindung zum Raum auf und erhält sofort den aktuellen Raum-Status (`pantry`, `starred`, `activeStoreId`).
   - Dank der **Cloudflare WebSocket Hibernation API** (`ctx.acceptWebSocket`) schläft das Durable Object zwischen Nachrichten kostenlos im Hintergrund, hält aber alle verbundenen Browser (GitHub Pages / WebAssembly) und Geräte offen.
   - Bricht die Verbindung kurzzeitig ab, verbindet sich die App automatisch alle **3 Sekunden** neu.
   - Da WebSockets (`wss://`) keine HTTP-CORS-Preflight-Requests (`OPTIONS`) benötigen, funktioniert die Synchronisation auf GitHub Pages (WebAssembly) ohne jegliche Browser-CORS-Blockaden.
2. **Echtzeit Delta-Push, Offline-Warteschlange & Last-Write-Wins (CRDT)**:
   - Sobald du eine Zutat in der Vorratskammer umschaltest, auf der Einkaufsliste als *"Gekauft ✓"* markierst, ein Rezept als Favorit (`★`) markierst/entfernst oder den aktiven Supermarkt wechselst, wird jede Änderung mit einem **exakten Millisekunden-Zeitstempel (`ts`)** versehen und lokal in der **persistenten Offline-Warteschlange** (`QSettings` + `localStorage`) gesichert.
   - Nach einer kurzen Entprellzeit (200 ms) wird **nur die tatsächliche Änderung (`pantryDelta` / `starredDelta`)** über den WebSocket gesendet. Erst wenn der Server den Zeitstempel bestätigt, wird der Eintrag aus der lokalen Warteschlange entfernt (selbst wenn der Browser-Tab offline geschlossen und später wieder geöffnet wird, gehen keine Offline-Einkäufe verloren).
   - Das Durable Object führt einen **Last-Write-Wins (LWW)-Vergleich pro Zutat** durch und sendet (**Broadcast**) den aktualisierten Zustand in Echtzeit an alle verbundenen Haushaltsmitglieder:
     - Bearbeiten zwei Personen **unterschiedliche Zutaten**, bleiben beide Änderungen vollständig erhalten.
     - Bearbeiten zwei Personen (z. B. eine davon offline im Supermarkt) **dieselbe Zutat**, gewinnt deterministisch die zeitlich neueste Änderung (`itemTs >= storedItemTs`).

### Einrichtung in 3 Minuten
1. Gehe in den Ordner `cloudflare/`:
   ```bash
   cd cloudflare
   npx wrangler login
   npx wrangler deploy
   ```
2. Kopiere die ausgegebene Worker-URL (z. B. `https://Mahlzeit-pantry-sync.<dein-name>.workers.dev`).
3. Trage in der App unter **Einstellungen -> Cloudflare Vorratskammer-Sync** die **Worker URL**, eine gemeinsame **Raum-ID** (z. B. `unser-haushalt`) und optional einen geheimen **Token** ein.

Ausführliche Details findest du in [cloudflare/README.md](cloudflare/README.md).

---

## 📁 Repository-Struktur

```text
Mahlzeit/
├── source/                      # Sämtlicher C++ Quellcode und QML-Dateien
│   ├── main.cpp                 # Programmeinstieg & Registrierung der Controller
│   ├── models/                  # Datenmodelle
│   │   ├── Recipe.h / .cpp      # Datenstruktur für Rezepte & Zutaten
│   │   ├── PantryModel.h / .cpp # Vorratskammer-Model mit Kategorien & Alias-Auflösung
│   │   ├── ShoppingListModel.h  # Einkaufslisten-Model mit Supermarkt-Gang-Sortierung
│   │   └── RecipeModel.h / .cpp # Rezept-Model mit Vorschlagssystem-Filterung
│   ├── services/                # Logik-Dienste
│   │   ├── SettingsManager.h    # Persistierung (QSettings), Zeitfenster, Markt & Favoriten
│   │   ├── RecipeRepository.h   # Laden von Rezepten, Kategorien, Aliasen & Supermärkten
│   │   ├── SuggestionEngine.h   # Vorschlagslogik (Zutaten + Tageszeiten)
│   │   └── SyncManager.h        # Automatischer Cloudflare Pull & Delta-Push Sync
│   └── qml/                     # Responsive Qt Quick Benutzeroberfläche (ab 320px Breite)
│       ├── Main.qml             # Mobile-First Hauptfenster mit Bottom Navigation
│       ├── PantryView.qml       # Vorratskammer-Ansicht
│       ├── ShoppingListView.qml # Einkaufslisten-Ansicht mit Gang-Gruppierung
│       ├── RecipesView.qml      # Rezeptübersicht mit Filterleiste
│       ├── RecipeDetailView.qml # Rezeptdetail mit Portionsrechner & Markdown
│       └── SettingsView.qml     # Zeitfenster, Supermarkt, Cloudflare-Sync & Fork-URL
├── resources/                   # Ressourcen (in App gebündelt)
│   ├── recipes.json             # Einzeldatei mit aliases, categories, stores & recipes
│   └── fonts/                   # NotoColorEmoji.ttf für WebAssembly
├── cloudflare/                  # Cloudflare Worker & Durable Object
│   ├── worker.js                # Serverloser Durable-Object-Sync-Code (GET Pull + POST Delta)
│   ├── wrangler.toml            # Cloudflare Deployment-Konfiguration (new_sqlite_classes)
│   └── README.md                # 3-Minuten Deployment-Anleitung & Sync-Architektur
├── tests/                       # QTest Unit-Tests
│   ├── CMakeLists.txt
│   └── test_Mahlzeit.cpp         # Testabdeckung für Rezepte, Skalierung, Aliase & Logik
├── index.html                   # GitHub Pages Landing-Page mit Datenschutzhinweis
├── CMakeLists.txt               # CMake-Projektdatei (Qt 6.8+, C++20)
└── README.md                    # Diese Dokumentation
```

---

## 🛠️ Bauen & Ausführen in Qt Creator

### Voraussetzungen
- **Qt Creator** (empfohlen ab Version 13+)
- **Qt 6.8+** mit den Modulen `Qt Quick`, `Qt Network`, `Qt WebSockets`, `Qt Core`, `Qt Gui`
- **C++20-fähiger Compiler** (MinGW 64-bit, GCC, Clang, MSVC oder Emscripten für WebAssembly)

### Tests ausführen
- Aktiviere beim Konfigurieren des CMake-Projekts `BUILD_TESTING`.
- Führe das Target `testMahlzeit` aus oder starte im Terminal:
  ```bash
  ctest --output-on-failure
  ```

---

## 📄 Lizenz & Datenschutz

Dieses Projekt steht unter der MIT-Lizenz.
Die Nutzung ist garantiert frei von Datenerfassung, Werbung und Cookies.
