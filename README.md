# Malzeit 🥗

**Malzeit** ist eine moderne, datenschutzfreundliche Web- und Mobil-Applikation zur smarten Vorrats- und Rezeptplanung, entwickelt in **modernem C++20** und **Qt 6.8.3 Quick / QML**.

Die App läuft im Browser (WebAssembly single-threaded), auf dem Desktop zu Entwicklungs- und Testzwecken und ist so aufgebaut, dass sie nahtlos als Git-Submodul in ein **Android-Projekt** eingebunden werden kann.

---

## 🔒 Datenschutz-Philosophie (100% Offline & Lokal)

1. **Ausschließlich lokale Datenspeicherung**: Alle Vorratsdaten, Einkaufslisten, Favoriten und individuellen Einstellungen werden ausschließlich lokal auf deinem Gerät gespeichert (im Webbrowser via HTML5 IndexedDB über `QSettings`, auf dem Desktop in der lokalen Konfigurationsdatei).
2. **Kein Tracking & kein Phoning-Home**: Die App erhebt keinerlei personenbezogene Daten, verwendet keine Tracking-Cookies und sendet keine Telemetrie- oder Nutzungsdaten an externe Server.
3. **Freiwillige Peer-to-Peer-Synchronisation**: Falls du deine Vorratskammer und Einkaufsliste mit Partnern oder Familie teilen möchtest, geschieht dies über deine **eigene, private Cloudflare Durable Objects-Instanz**. Es gibt keinen zentralen Betreiber-Server.

---

## 📱 Bildschirme & Funktionen

### 1. Vorratskammer (The Pantry)
- **Vollständige Zutatenübersicht**: Zeigt alle einzigartigen Zutaten (automatisch aggregiert aus allen 32 Rezepten).
- **Verfügbarkeit umschalten**: Mit einem Klick/Tap Zutaten als *vorrätig* oder *fehlend* markieren.
- **Schnellfilter & Suche**: Filter nach Vorrätig / Fehlend sowie Textsuche.
- **Massenaktionen**: *Alle auswählen* oder *Alle leeren*.
- **Fortschrittsanzeige**: Zeigt genau an, wie viele Zutaten vorhanden sind (z. B. "42 von 98 Zutaten vorrätig").

### 2. Einkaufsliste (The Shopping List)
- **Automatische Bedarfsermittlung**: Zeigt alle Zutaten, die sich aktuell *nicht* in der Vorratskammer befinden.
- **Rezept-Kontext**: Zu jeder Zutat wird angezeigt, für welche Rezepte sie benötigt wird.
- **Favoriten-Filter**: Mit einem Klick nur Zutaten anzeigen, die für mit einem Stern markierte Favoriten-Rezepte gebraucht werden.
- **Sortierung**: Alphabetisch (A-Z), nach Favoriten zuerst oder nach Häufigkeit (wichtigste Zutaten ganz oben).
- **Direkt als gekauft markieren**: Klick auf "Gekauft ✓" überträgt die Zutat sofort in die Vorratskammer.
- **Liste kopieren**: Formatiert die Einkaufsliste als Markdown-Checkliste und kopiert sie in die Zwischenablage zum Teilen via Chat oder Notizen.

### 3. Rezepte & Vorschlagssystem (The Recipes & Suggestion System)
- **32 gesunde Rezepte** in 6 Kategorien:
  1. *Frühstück & Breie*
  2. *Shakes*
  3. *Aufstriche & Salate*
  4. *Suppen & Nudelgerichte*
  5. *Hauptgerichte*
  6. *Süßspeisen*
- **Smarte Vorschlagslogik**:
  - **Standardmäßig aktiv**: Zeigt nur Rezepte an, für die **alle Zutaten vorrätig** sind **UND** die zur **aktuellen Tageszeit** passen.
  - **Filter-Schalter**:
    - *Kochbereit*: Alle Zutaten im Vorrat vorhanden.
    - *Passende Uhrzeit*: Passt zur aktuellen Uhrzeit (z. B. Frühstück 04:00–09:30, Hauptgerichte/Dinner 11:30–21:30).
    - *Nur Favoriten (★)*.
  - **Kategorie-Pills**: Schnelle Filterung nach "Frühstück", "Shakes", etc.
  - **Rezept-Karten**: Farbige Badges zeigen Kochbereitschaft ("Alle Zutaten im Vorrat vorhanden!" vs. "Es fehlen 2 Zutaten") sowie passende Tageszeit an.

### 4. Rezeptdetails & Portionen-Rechner (The Selected Recipe)
- **Vollständige Ansicht**: Rezepttitel, Kategorie, Kochbereitschaftsstatus und Favoriten-Button.
- **Dynamische Portionsgrößen**: Mit `−` und `+` die gewünschte Portionsanzahl wählen (z. B. von 1 auf 4 Portionen). Alle Zutatenmengen werden in Echtzeit mathematisch exakt neu berechnet.
- **Zutatenliste mit Vorrats-Status**: Jede Zutat zeigt die berechnete Menge (g / ml) und einen Vorrats-Indikator. Zutaten können direkt aus der Rezeptansicht heraus als vorrätig markiert werden.
- **Formatierte Zubereitungsschritte (Markdown)**: Darstellung formatierter Zubereitungsschritte via Qt Quick Markdown-Renderer (`Text.MarkdownText`). Eigene Markdown-Anleitungen können direkt eingetragen und gespeichert werden.

### 5. Einstellungen (The Settings)
- **Kategorien & Zeitfenster**: Zeitfenster für Mahlzeiten (Frühstück, Snack, Mittag, Abendessen) flexibel bearbeiten oder auf Standard zurücksetzen.
- **Cloudflare Durable Object Sync**: Konfiguration von Worker-URL, Raum-ID und Authentifizierungs-Token zur privaten Synchronisation.
- **Rezept-Katalog & Aktualisierung**: Rezepte können auf Knopfdruck online von `https://mesw.github.io/Malzeit/resources/recipes.json` aktualisiert werden, ohne die WebAssembly-Applikation neu kompilieren zu müssen.
- **Werksreset**: Zurücksetzen aller Einstellungen, Vorräte oder Rezepte.

---

## 📁 Repository-Struktur

```text
Malzeit/
├── source/                      # Sämtlicher C++ Quellcode und QML-Dateien
│   ├── main.cpp                 # Programmeinstieg & Registrierung der Controller
│   ├── models/                  # Datenmodelle
│   │   ├── Recipe.h / .cpp      # Datenstruktur für Rezepte & Zutaten
│   │   ├── PantryModel.h / .cpp # Vorratskammer-Model (QAbstractListModel)
│   │   ├── ShoppingListModel.h  # Einkaufslisten-Model mit Favoriten-Filter
│   │   └── RecipeModel.h / .cpp # Rezept-Model mit Vorschlagssystem-Filterung
│   ├── services/                # Logik-Dienste
│   │   ├── SettingsManager.h    # Persistierung (QSettings), Zeitfenster & Favoriten
│   │   ├── RecipeRepository.h   # Laden aus Ressource, Cache & Web-Aktualisierung
│   │   ├── SuggestionEngine.h   # Vorschlagslogik (Zutaten + Tageszeiten)
│   │   └── SyncManager.h        # Cloudflare Durable Object HTTP/JSON Sync
│   └── qml/                     # Responsive Qt Quick Benutzeroberfläche
│       ├── Main.qml             # Mobile-First Hauptfenster mit Bottom Navigation
│       ├── PantryView.qml       # Vorratskammer-Ansicht
│       ├── ShoppingListView.qml # Einkaufslisten-Ansicht
│       ├── RecipesView.qml      # Rezeptübersicht mit Filterleiste
│       ├── RecipeDetailView.qml # Rezeptdetail mit Portionsrechner & Markdown
│       └── SettingsView.qml     # Zeitfenster, Cloudflare-Sync & Aktualisierung
├── resources/                   # Ressourcen (in App gebündelt)
│   ├── recipes.json             # 32 vordefinierte Rezepte
│   └── fonts/                   # NotoColorEmoji.ttf für WebAssembly
├── cloudflare/                  # Cloudflare Worker & Durable Object
│   ├── worker.js                # Serverloser Durable-Object-Sync-Code
│   ├── wrangler.toml            # Cloudflare Deployment-Konfiguration
│   └── README.md                # 3-Minuten Deployment-Anleitung
├── tests/                       # QTest Unit-Tests
│   ├── CMakeLists.txt
│   └── test_malzeit.cpp         # Testabdeckung für Rezepte, Skalierung & Logik
├── .github/workflows/           # GitHub Actions Workflows
│   └── deploy.yml               # Automatisches Deployment auf GitHub Pages
├── build/                       # Build-Ordner von Qt Creator
├── index.html                   # GitHub Pages Landing-Page mit Datenschutzhinweis
├── CMakeLists.txt               # CMake-Projektdatei (Qt 6.8.3, C++20)
└── README.md                    # Diese Dokumentation
```

---

## 🛠️ Bauen & Ausführen in Qt Creator

### Voraussetzungen
- **Qt Creator** (empfohlen ab Version 13+)
- **Qt 6.8.3** mit folgenden Modulen:
  - `Qt Quick` / `Qt QML`
  - `Qt Network`
  - `Qt Core` / `Qt Gui`
- **C++20-fähiger Compiler** (MinGW 64-bit 13+, GCC 12+, Clang 15+, MSVC 2022 oder Emscripten 3.1.56+ für WebAssembly).

### 1. Für Desktop bauen (Testen & Entwicklung)
1. Öffne das Projekt in Qt Creator über `CMakeLists.txt`.
2. Wähle das Kit **Desktop Qt 6.8.3 MinGW 64-bit** (oder MSVC/Clang).
3. Klicke auf **Ausführen** (`Strg + R`).

### 2. Für WebAssembly (Single-Thread) bauen
1. Installiere das Kit **Qt 6.8.3 WebAssembly (single-threaded)** über den Qt Maintenance Tool.
2. Wähle in Qt Creator das Kit **WebAssembly Qt 6.8.3 single-threaded**.
3. Baue im **Release**-Modus. Die generierten Web-Dateien (`appMalzeit.wasm`, `appMalzeit.js`, `index.html`, `qtloader.js`) werden standardmäßig im Ordner:
   ```text
   build/WebAssembly_Qt_6_8_3_single_threaded-Release/
   ```
   abgelegt.
4. Qt Creator startet bei Klick auf "Ausführen" automatisch einen lokalen Webserver (z. B. auf Port 30000) und öffnet die Anwendung in deinem Standard-Browser.

### 3. Verwendung als Android Submodul (Zukunft)
Dieses Repository ist so strukturiert, dass es in ein übergeordnetes Android-App-Projekt als Git-Submodul eingebunden werden kann:
```bash
git submodule add https://github.com/mesw/Malzeit.git app/src/main/cpp/Malzeit
```
Die C++20-Logik und die QML-Dateien sind vollständig plattformunabhängig und verwenden keine proprietären Desktop- oder Web-APIs.

---

## 🚀 GitHub Pages & Bereitstellung der WebAssembly-Artefakte

> **Antwort auf deine Frage bezüglich der Bereitstellung von Build-Artefakten im Git-Repository:**

Im Git-Repository selbst große binäre Dateien wie `.wasm` (die oft 10–25 MB groß sind) mit jedem Commit zu speichern, bläht die Git-Historie auf Dauer auf. Es gibt **drei saubere Wege**, um GitHub Pages zu betreiben:

### Option A: Automatisches Deployment via GitHub Actions (Empfohlen)
Die sauberste und modernste Methode:
- Dein Quellcode bleibt schlank im `main`-Branch ohne kompilierte Binärdateien.
- Die mitgelieferte Workflow-Datei [`.github/workflows/deploy.yml`](.github/workflows/deploy.yml) deployt automatisch die Landing Page (`index.html`) und alle benötigten Web-Dateien direkt über die offizielle GitHub Pages Actions Schnittstelle.
- Alternativ kann ein GitHub Actions Runner mit Qt WebAssembly Docker-Container die `.wasm`-Datei bei jedem Push frisch kompilieren und direkt an GitHub Pages übergeben.

### Option B: Dedizierter `gh-pages` Branch
- Der Quellcode (`main`-Branch) bleibt komplett frei von Build-Artefakten.
- Nur der `gh-pages`-Branch enthält die gebauten HTML/WASM-Dateien.
- In den Repository-Einstellungen unter **Settings -> Pages** wählst du einfach den `gh-pages`-Branch als Quelle aus.

### Option C: Direkte Bereitstellung aus `build/...` im Git-Tree
- Wenn du die WebAssembly-Dateien direkt aus Qt Creator im Pfad `build/WebAssembly_Qt_6_8_3_single_threaded-Release/` mit in das Repository committen möchtest, ist dies ebenfalls vorkonfiguriert:
  - Die `.gitignore` ist so eingerichtet, dass Debug- und Desktop-Builds ignoriert werden, aber `build/WebAssembly_Qt_6_8_3_single_threaded-Release/` committet werden kann.
  - Die `index.html` im Root-Verzeichnis leitet den Benutzer nach Klick auf "Zustimmen und Starten" direkt zu `build/WebAssembly_Qt_6_8_3_single_threaded-Release/index.html` weiter.

---

## ☁️ Cloudflare Durable Objects Sync Einrichtung

Möchtest du deinen Vorrat und deine Einkaufsliste mit Familienmitgliedern oder anderen Geräten teilen?

1. Gehe in den Ordner `cloudflare/`.
2. Führe folgende Befehle aus:
   ```bash
   cd cloudflare
   npx wrangler login
   npx wrangler deploy
   ```
3. Kopiere die von Cloudflare ausgegebene URL (z. B. `https://malzeit-pantry-sync.dein-name.workers.dev`).
4. Öffne in der Malzeit-App **Einstellungen -> Cloudflare Vorratskammer-Sync**.
5. Trage die **Worker URL**, eine gemeinsame **Raum-ID** (z. B. `unser-haushalt`) und optional einen geheimen **Token** ein.
6. Klicke auf **"Jetzt synchronisieren"**.

Ausführliche Details findest du in der [cloudflare/README.md](cloudflare/README.md).

---

## 🧪 Unit-Tests ausführen

Unit-Tests sind mit **Qt Test (QTest)** im Verzeichnis `tests/` implementiert. Sie überprüfen:
1. Das korrekte Parsen aller 32 Rezepte aus der JSON-Ressource.
2. Die dynamische Skalierung der Zutatenmengen bei veränderter Portionsgröße.
3. Das Vorratsmanagement (Verfügbarkeit setzen, umschalten, Massenaktionen).
4. Die Einkaufslisten-Logik (Erkennung fehlender Zutaten, Filterung nach Favoriten, Übertrag in Vorrat).
5. Das Vorschlagssystem (Kombination aus Vorratsverfügbarkeit und Tageszeit-Matching).

### Tests in Qt Creator ausführen
- Aktiviere beim Konfigurieren des CMake-Projekts das Häkchen `BUILD_TESTING`.
- Wähle in der Projektansicht das Target `testMalzeit` und führe es aus.
- Alternativ im Terminal:
  ```bash
  ctest --output-on-failure
  ```

---

## 📄 Lizenz & Datenschutz

Dieses Projekt steht unter der MIT-Lizenz.
Die Nutzung ist garantiert frei von Datenerfassung, Werbung und Cookies.
