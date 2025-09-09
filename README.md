# Arduino-Space-Invader

Ein klassischer Space Invaders-Klon für den Arduino Nano (2KB RAM) mit SPI-TFT-Display, Sound und Highscore-Speicherung auf SD-Karte.
Der Spieler steuert ein Raumschiff am unteren Bildschirmrand, schießt auf angreifende Gegner und weicht deren Schüssen aus.

**Spieler**

Bewegung: Links/Rechts über Taster

Schießen: Mit Feuer-Taste (ein Schuss gleichzeitig)

Leben: 3 (Herzen oben rechts)

Punkte: +10 pro getroffenen Gegner

**Gegner**

In Reihen und Spalten angeordnet

## Darstellung

Grafiken (Spieler, Gegner, Herz) als 8x8 Bitmaps

Flimmerfreies Zeichnen durch Zwischenspeichern der alten Positionen

Alle Elemente werden über die Adafruit GFX Library auf dem ST7735 TFT gezeichnet

## Hardware

| Komponente               | Beschreibung                             |
|--------------------------|------------------------------------------|
| Arduino Nano             | Mikrocontroller                          |
| 1.8" SPI TFT Display     | ST7735 (z. B. 128x160 Pixel)             |
| SD-Kartenmodul           | Für Highscore-Speicherung                |
| 3 Taster                 | Links / Rechts / Feuer                   |
| Piezo-Buzzer             | Für Soundeffekte                         |
| Steckbrett + Kabel       | Für Verdrahtung                          |
| Externe Stromversorgung  | Optional (z. B. USB oder 5V Netzteil)    |

---

## Pinbelegung

### TFT-Display (ST7735)

| TFT-Pin | Arduino-Pin | Beschreibung         |
|---------|-------------|----------------------|
| VCC     | 5V          | Stromversorgung      |
| GND     | GND         | Masse                |
| SCL     | D13         | SPI Clock            |
| SDA     | D11         | SPI MOSI             |
| RES     | A1          | Reset                |
| DC      | A2          | Data/Command         |
| CS      | A3          | Chip Select Display  |

### SD-Kartenmodul

| SD-Pin  | Arduino-Pin | Beschreibung         |
|---------|-------------|----------------------|
| VCC     | 5V          | Stromversorgung      |
| GND     | GND         | Masse                |
| MISO    | D12         | SPI MISO             |
| MOSI    | D11         | SPI MOSI (geteilt)   |
| SCK     | D13         | SPI Clock (geteilt)  |
| CS      | D4          | Chip Select SD       |

### Buttons

| Funktion | Arduino-Pin |
|----------|-------------|
| Links    | D2          |
| Rechts   | D3          |
| Feuer    | D5          |

> **Hinweis:** Die Taster werden mit `INPUT_PULLUP` verwendet. Das bedeutet: ein Taster schließt auf GND, wenn gedrückt.

### 🔊 Piezo-Buzzer

| Buzzer-Pin | Arduino-Pin |
|------------|-------------|
| Signal     | D8          |
| GND        | GND         |

---

## Bibliotheken (Arduino IDE)

Folgende Bibliotheken müssen über den **Bibliotheksverwalter** installiert werden:

- `Adafruit_GFX`
- `Adafruit_ST7735`
- `SPI`
- `SD`

---

## Hochladen des Codes

1. **Arduino IDE öffnen**
2. **Board auswählen**: `Arduino Nano`
3. **Prozessor auswählen**: `ATmega328P (Old Bootloader)` (je nach Nano-Modell)
4. **Port auswählen**
5. **Bibliotheken installieren** (siehe oben)
6. **Projekt öffnen oder Sketch kopieren**
7. **Hochladen klicken**

---

## SD-Karte:

- Formatiere die SD-Karte als **FAT16 oder FAT32**
- Keine Dateien notwendig beim ersten Start – `score.txt` wird automatisch
- Format: ABC 1250 02 (Name, Score, Level)
  
