#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS   A3  // statt 10
#define TFT_DC   A2  // statt 8
#define TFT_RST  A1  // statt 9
#define SD_CS    4

#define BUTTON_LEFT   2
#define BUTTON_RIGHT  3
#define BUTTON_FIRE   5

#define MAX_HIGHSCORES 10
#define HUD_HEIGHT 20  // Bereich oben für Score & Lives reservieren

#define SOUND_PIN     8 //6

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

struct Highscore {
  char name[4];     // 3 Buchstaben + Nullterminator
  uint16_t score;
  uint8_t level;    // NEU: Level hinzufügen
};

Highscore highscores[MAX_HIGHSCORES];

// Spielvariablen
uint8_t playerX, playerY;
uint8_t bulletX, bulletY;
bool bulletActive;
uint8_t level = 1;

unsigned long lastEnemyMove = 0;
uint16_t enemyMoveDelay;

bool fireButtonPreviouslyPressed = false;

int16_t score;
int8_t lives;

// Gegner
#define ENEMY_WIDTH  8
#define ENEMY_HEIGHT 8
#define ENEMY_ROWS   2
#define ENEMY_COLS   5
#define ENEMY_SPACING_X 8
#define ENEMY_SPACING_Y 8
#define ENEMY_START_Y (HUD_HEIGHT + 5)
#define MAX_ENEMIES (ENEMY_ROWS * ENEMY_COLS)
#define MAX_ENEMY_BULLETS 3

struct EnemyBullet {
  int x, y;
  bool active;
};

EnemyBullet enemyBullets[MAX_ENEMY_BULLETS];
int enemyBulletSpeed = 2;
uint8_t enemyFireChance = 2; // Prozent

// Alte Positionen merken für flimmerfreies Zeichnen
int16_t lastEnemyX[MAX_ENEMIES];
int16_t lastEnemyY[MAX_ENEMIES];

struct Enemy {
  uint8_t x, y;
  int8_t dir;  // Bewegung horizontal: 1 oder -1
  bool active;
};

Enemy enemies[MAX_ENEMIES];

// Alte Positionen für Zeichnen merken
int16_t lastPlayerX = -1;
int16_t lastPlayerY = -1;
int16_t lastBulletX = -1;
int16_t lastBulletY = -1;
int16_t lastScore = -1;
int8_t lastLives = -1;

const uint8_t heartBitmap[] PROGMEM = {
  0b00000000,
  0b00010010,
  0b00110111,
  0b01111111,
  0b01111110,
  0b00111100,
  0b00011000,
  0b00000000
};

const uint8_t enemyBitmap[] PROGMEM = {
  0b00111100,
  0b01111110,
  0b11011011,
  0b11111111,
  0b11111111,
  0b00100100,
  0b01000010,
  0b10000001
};

const uint8_t playerBitmap[] PROGMEM = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b11111111,
  0b00111100,
  0b11111111,
  0b11111111
};

void playLoseLifeSound();

void setup() {
  //Serial.begin(9600); // Optional zum Debuggen
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_FIRE, INPUT_PULLUP);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  if (!SD.begin(SD_CS)) {
    while (1);
  }
  loadHighscores();
}

void loop() {
  showStartMenu();
  startGame();

  while (lives > 0) {
    updateGame();
    updateEnemies();
    drawGame();
  }
  showGameOverScreen();
}

void printPaddedNumber(uint32_t number, uint8_t width) {
  char buffer[10];  // max 5 Stellen + null
  uint8_t len = 0;

  // Umwandlung in String (rückwärts)
  do {
    buffer[len++] = '0' + (number % 10);
    number /= 10;
  } while (number > 0 && len < sizeof(buffer) - 1);

  // Auffüllen mit führenden Nullen
  while (len < width && len < sizeof(buffer) - 1) {
    buffer[len++] = '0';
  }

  // String rückwärts ausgeben
  for (int8_t i = len - 1; i >= 0; i--) {
    tft.print(buffer[i]);
  }
}

// --- Startmenü ---
void showStartMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(30, 5);
  tft.println(F("SPACE INVADERS"));

  tft.setCursor(10, 20);
  tft.setTextColor(ST77XX_WHITE);
  tft.println(F("Top 10 Scores:"));

  for (int8_t i = 0; i < MAX_HIGHSCORES; i++) {
    int16_t y = 35 + i * 8;
    tft.setCursor(10, y);

    // Platznummer mit führender Null (z. B. "01")
    tft.setTextColor(ST77XX_YELLOW);
    printPaddedNumber(i + 1, 2);
    tft.print(F(" "));  // Punkt + Leerzeichen

    // Name (feste Breite: 3 Zeichen)
    tft.setTextColor(ST77XX_MAGENTA);
    tft.print(highscores[i].name);
    tft.print(F(" "));  // 2 Leerzeichen Abstand zum Score

    // Score mit führenden Nullen (8 Stellen)
    tft.setTextColor(ST77XX_CYAN);
    printPaddedNumber(highscores[i].score, 8);
    tft.print(F(" "));  // Abstand zum Level

    // Level mit 3 führenden Nullen
    tft.setTextColor(ST77XX_BLUE);
    tft.print(F("LVL "));
    printPaddedNumber(highscores[i].level, 3);
  }

  // Blinkeaufforderung
  bool showText = true;
  unsigned long lastBlink = 0;

  while (true) {
    if (millis() - lastBlink > 500) {
      lastBlink = millis();
      showText = !showText;
      if (showText) {
        tft.setCursor(10, 120);
        tft.setTextColor(ST77XX_GREEN);
        tft.print(F("Press FIRE to Start"));
      } else {
        tft.fillRect(10, 120, 140, 10, ST77XX_BLACK);
      }
    }

    if (digitalRead(BUTTON_FIRE) == LOW) break;
    delay(10);
  }

  delay(300);
}

// --- Highscore Laden ---
void loadHighscores() {
  File file = SD.open("score.txt");

  if (!file) {
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
      strcpy(highscores[i].name, "AAA");
      highscores[i].score = 0;
      highscores[i].level = 1;
    }
    saveHighscores();
    return;
  }

  for (int8_t i = 0; i < MAX_HIGHSCORES; i++) {
    if (file.available()) {
      char buf[24];
      int len = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
      buf[len] = 0;

      char name[4];
      uint16_t score;
      uint8_t level;
      if (sscanf(buf, "%3s %hu %hhu", name, &score, &level) == 3) {
        strcpy(highscores[i].name, name);
        highscores[i].score = score;
        highscores[i].level = level;
      } else {
        strcpy(highscores[i].name, "AAA");
        highscores[i].score = 0;
        highscores[i].level = 1;
      }
    } else {
      strcpy(highscores[i].name, "AAA");
      highscores[i].score = 0;
      highscores[i].level = 1;
    }
  }

  file.close();
}

void saveHighscores() {
  if (SD.exists("score.txt")) {
    SD.remove("score.txt");
  }

  File file = SD.open("score.txt", FILE_WRITE);
  if (!file) return;

  for (int8_t i = 0; i < MAX_HIGHSCORES; i++) {
    file.print(highscores[i].name);
    file.print(F(" "));
    file.print(highscores[i].score);
    file.print(F(" "));
    file.println(highscores[i].level); // NEU: Level speichern
  }

  file.close();
}

// --- Spiel starten ---
void startGame() {
  tft.fillScreen(ST77XX_BLACK);

  playerX = 60;
  playerY = tft.height() - 10;

  bulletActive = false;

  score = 0;
  lives = 3;

  level = 1;
  updateEnemySpeed();

  spawnEnemies();

  lastPlayerX = -1; lastPlayerY = -1;
  lastBulletX = -1; lastBulletY = -1;
  lastScore = -1; lastLives = -1;

  // Alle alten Gegner-Positionen zurücksetzen
  for (int i = 0; i < MAX_ENEMIES; i++) {
    lastEnemyX[i] = -1;
    lastEnemyY[i] = -1;
  }
}

void showLevelBanner(uint8_t levelNumber) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(30, tft.height() / 2 - 10);
  tft.print(F("LEVEL "));
  tft.print(levelNumber);
  delay(1500);  // 1.5 Sekunden anzeigen
  tft.fillScreen(ST77XX_BLACK);  // danach löschen
}

void spawnEnemies() {
  for (int row = 0; row < ENEMY_ROWS; row++) {
    for (int col = 0; col < ENEMY_COLS; col++) {
      int idx = row * ENEMY_COLS + col;
      enemies[idx].x = col * (ENEMY_WIDTH + ENEMY_SPACING_X);
      enemies[idx].y = ENEMY_START_Y + row * (ENEMY_HEIGHT + ENEMY_SPACING_Y);
      enemies[idx].dir = 1;
      enemies[idx].active = true;

      lastEnemyX[idx] = -1;
      lastEnemyY[idx] = -1;
    }
  }
}

void resetScreenAfterLifeLost() {
  tft.fillScreen(ST77XX_BLACK);

  lastScore = -1; 
  lastLives = -1;

  for (int8_t i = 0; i < MAX_ENEMIES; i++) {
    lastEnemyX[i] = -1;
    lastEnemyY[i] = -1;
  }
  lastPlayerX = -1;
  lastPlayerY = -1;
  lastBulletX = -1;
  lastBulletY = -1;

  drawGame();
}

void updateEnemies() {

  if (millis() - lastEnemyMove < enemyMoveDelay) return;  // Noch warten
  lastEnemyMove = millis();

  // Finde linken und rechten Rand der aktiven Gegner
  int leftMost = 255;
  int rightMost = 0;
  bool anyActive = false;

  // Bestimme aktuelle Bewegungsrichtung eines aktiven Gegners
  int8_t currentDir = 1;  // Standard
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      currentDir = enemies[i].dir;
      break;
    }
  }

  for (int8_t i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    anyActive = true;

    if (enemies[i].x < leftMost) leftMost = enemies[i].x;
    if (enemies[i].x > rightMost) rightMost = enemies[i].x;
  }

  if (!anyActive) return; // Keine Gegner aktiv

  // Richtung wechseln, wenn Rand erreicht
  bool changeDir = false;
  if (rightMost + ENEMY_WIDTH >= tft.width() && currentDir > 0) changeDir = true;
  if (leftMost <= 0 && currentDir < 0) changeDir = true;

  if (changeDir) {
    for (int8_t i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].active) continue;
      enemies[i].dir = -currentDir;  // Richtungswechsel für aktive Gegner
      enemies[i].y += 6;             // Gegner rücken nach unten
    }
  } else {
    for (int8_t i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].active) continue;
      //int speed = 1 + level / 2;  // alle 2 Level +1 Speed
      //enemies[i].x += enemies[i].dir * speed;
      enemies[i].x += enemies[i].dir * 1;
    }
  }

  // Prüfe Spieler-Kollision (Gegner zu tief)
  for (int8_t i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
    if (enemies[i].y + ENEMY_HEIGHT >= playerY) {
      lives--;
      playLoseLifeSound();
      spawnEnemies();
      resetScreenAfterLifeLost();
      break;
    }
  }

}

// --- Spiel-Update ---
void updateGame() {
  // Spieler bewegen
  if (digitalRead(BUTTON_LEFT) == LOW && playerX > 0) playerX -= 2;
  if (digitalRead(BUTTON_RIGHT) == LOW && playerX < tft.width() - 8) playerX += 2;

  // Schuss abfeuern
  bool firePressed = (digitalRead(BUTTON_FIRE) == LOW);
  if (!bulletActive && firePressed && !fireButtonPreviouslyPressed) {
    bulletActive = true;
    bulletX = playerX + 4;
    bulletY = playerY - 4;
    playShootSound();
  }
  fireButtonPreviouslyPressed = firePressed;

  // Schuss bewegen und Treffer prüfen
  if (bulletActive) {
    bulletY -= 4;
    if (bulletY < HUD_HEIGHT) bulletActive = false;

    for (int8_t i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].active) continue;

      if (bulletX >= enemies[i].x && bulletX <= enemies[i].x + ENEMY_WIDTH &&
          bulletY >= enemies[i].y && bulletY <= enemies[i].y + ENEMY_HEIGHT) {
        score += 10;
        bulletActive = false;
        playHitSound();

        // Visuell vom Bildschirm löschen
        tft.fillRect(enemies[i].x, enemies[i].y, ENEMY_WIDTH, ENEMY_HEIGHT, ST77XX_BLACK);

        enemies[i].active = false;  // Gegner deaktivieren, nicht neu spawnen!

        lastEnemyX[i] = -1;
        lastEnemyY[i] = -1;
        break;  // Nur einen Gegner pro Schuss treffen
      }
    }
  }

  checkLevelUp();

  //gegner schuss
  handleEnemyBullets();
}

void drawEnemyBullets() {
  static int16_t lastEnemyBulletX[MAX_ENEMY_BULLETS];
  static int16_t lastEnemyBulletY[MAX_ENEMY_BULLETS];

  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
    // Alte Position löschen
    if (lastEnemyBulletX[i] != -1) {
      tft.fillRect(lastEnemyBulletX[i], lastEnemyBulletY[i], 2, 5, ST77XX_BLACK);
    }

    // Neue Position zeichnen
    if (enemyBullets[i].active) {
      tft.fillRect(enemyBullets[i].x, enemyBullets[i].y, 2, 5, ST77XX_WHITE);
      lastEnemyBulletX[i] = enemyBullets[i].x;
      lastEnemyBulletY[i] = enemyBullets[i].y;
    } else {
      lastEnemyBulletX[i] = -1;
      lastEnemyBulletY[i] = -1;
    }
  }
}

void checkLevelUp() {
  bool allDead = true;
  for (int8_t i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      allDead = false;
    }
  }
  if (allDead) {
    level++;
    updateEnemySpeed();
    showLevelBanner(level);
    spawnEnemies();
  }
}

void updateEnemyBulletBehavior() {
  // Geschwindigkeit: 2 + (Level / 2), maximal 6
  enemyBulletSpeed = min(6, 2 + level / 2);

  // Feuerrate: 1 + Level, maximal 10 %
  enemyFireChance = min(10, 1 + level);
}

void updateEnemySpeed() {
  enemyMoveDelay = max(5, 300 / level);
  updateEnemyBulletBehavior();
}

// --- Zeichnen ---
void drawGame() {
  // Spieler zeichnen
  if (lastPlayerX != -1) tft.fillRect(lastPlayerX, lastPlayerY, 8, 8, ST77XX_BLACK);
  drawBitmap8x8(playerBitmap, playerX, playerY, ST77XX_BLUE);

  lastPlayerX = playerX;
  lastPlayerY = playerY;

  // Schuss zeichnen
  if (lastBulletX != -1) tft.fillRect(lastBulletX, lastBulletY, 3, 5, ST77XX_BLACK);
  if (bulletActive) tft.fillRect(bulletX, bulletY, 3, 5, ST77XX_YELLOW);
  lastBulletX = bulletActive ? bulletX : -1;
  lastBulletY = bulletActive ? bulletY : -1;

  // Gegner zeichnen
  for (int8_t i = 0; i < MAX_ENEMIES; i++) {
    if (lastEnemyX[i] != -1) tft.fillRect(lastEnemyX[i], lastEnemyY[i], ENEMY_WIDTH, ENEMY_HEIGHT, ST77XX_BLACK);
    if (enemies[i].active) {
      drawBitmap8x8(enemyBitmap, enemies[i].x, enemies[i].y, ST77XX_RED);
      lastEnemyX[i] = enemies[i].x;
      lastEnemyY[i] = enemies[i].y;
    } else {
      lastEnemyX[i] = -1;
      lastEnemyY[i] = -1;
    }
  }

  // Score & Leben zeichnen, nur bei Änderung
  if (score != lastScore || lives != lastLives) {
    tft.fillRect(0, 0, tft.width(), HUD_HEIGHT, ST77XX_BLACK);

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    // Zeile 1: Score
    tft.setCursor(2, 2);
    tft.print(F("Score: "));
    tft.print(score);

    // Zeile 1: Level (rechtsbündig)
    tft.setCursor(2, 12); 
    tft.print(F("Lvl: "));
    tft.print(level);

    // Herzen für Leben
    for (int8_t i = 0; i < lives; i++) {
      int x = tft.width() - 8 * (i + 1) - 2;  // Abstand von rechts
      int8_t y = 2;                          // Abstand von oben
      drawBitmap8x8(heartBitmap, x, y, ST77XX_RED);
    }

    lastScore = score;
    lastLives = lives;
  }

  drawEnemyBullets();

}

// --- Game Over ---
void showGameOverScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(20, tft.height() / 2 - 20);
  tft.println(F("GAME OVER"));

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, tft.height() / 2 + 10);
  tft.println(F("Final Score: "));
  tft.setCursor(20, tft.height() / 2 + 25);
  tft.println(score);

  delay(2000);
  updateHighscores();
}

void updateHighscores() {
  for (int i = 0; i < MAX_HIGHSCORES; i++) {
    if (score > highscores[i].score) {
      for (int j = MAX_HIGHSCORES - 1; j > i; j--) {
        highscores[j] = highscores[j - 1];
      }

      char playerName[4];
      enterName(playerName);
      strcpy(highscores[i].name, playerName);
      highscores[i].score = score;
      highscores[i].level = level;

      saveHighscores();
      break;
    }
  }
}

void drawBitmap8x8(const uint8_t* bitmap, int x, int y, uint16_t color) {
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t row = pgm_read_byte(&bitmap[i]);
    for (uint8_t j = 0; j < 8; j++) {
      if (row & (1 << (7 - j))) {
        tft.drawPixel(x + j, y + i, color);
      }
    }
  }
}

void handleEnemyBullets() {
  // Gegner zufällig schießen lassen
  if (random(0, 100) < enemyFireChance) {
    int shooters[MAX_ENEMIES];
    int count = 0;

    // Nur unterste aktive Gegner in jeder Spalte finden
    for (int col = 0; col < ENEMY_COLS; col++) {
      int lowestEnemyIndex = -1;
      int lowestEnemyY = -1;

      for (int row = 0; row < ENEMY_ROWS; row++) {
        int idx = row * ENEMY_COLS + col;
        if (enemies[idx].active && enemies[idx].y > lowestEnemyY) {
          lowestEnemyY = enemies[idx].y;
          lowestEnemyIndex = idx;
        }
      }

      if (lowestEnemyIndex != -1) {
        shooters[count++] = lowestEnemyIndex;
      }
    }

    if (count > 0) {
      int shooterIndex = shooters[random(count)];
      int bx = enemies[shooterIndex].x + ENEMY_WIDTH / 2;
      int by = enemies[shooterIndex].y + ENEMY_HEIGHT;

      for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) {
          enemyBullets[i].x = bx;
          enemyBullets[i].y = by;
          enemyBullets[i].active = true;
          break;
        }
      }
    }
  }

  // Bewegung + Kollision bleibt unverändert
  for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (!enemyBullets[i].active) continue;

    enemyBullets[i].y += enemyBulletSpeed;

    if (enemyBullets[i].y > tft.height()) {
      enemyBullets[i].active = false;
      continue;
    }

    // Spieler getroffen?
    if (enemyBullets[i].y >= playerY &&
        enemyBullets[i].x >= playerX &&
        enemyBullets[i].x <= playerX + 8) {
      enemyBullets[i].active = false;
      lives--;
      playLoseLifeSound();

      if (lives > 0) {
        spawnEnemies();
        resetScreenAfterLifeLost();
      }

      break;
    }
  }
}

void enterName(char* nameBuffer) {
  const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int currentLetter[3] = {0, 0, 0};
  int currentPos = 0;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.println(F("Enter your name:"));

  while (currentPos < 3) {
    // Anzeige aktualisieren
    for (uint8_t i = 0; i < 3; i++) {
      uint8_t x = 30 + i * 20;
      uint8_t y = 40;

      // Zeichenbereich zuerst löschen
      tft.fillRect(x, y, 16, 16, ST77XX_BLACK);

      // Neuen Buchstaben zeichnen
      tft.setTextColor(i == currentPos ? ST77XX_GREEN : ST77XX_WHITE);
      tft.setTextSize(2);
      tft.setCursor(x, y);

      char c = alphabet[currentLetter[i]];
      tft.print(c);

    }

    // Button-Eingabe
    if (digitalRead(BUTTON_LEFT) == LOW) {
      currentLetter[currentPos] = (currentLetter[currentPos] + 25) % 26;  // zurück
      delay(200);
    }
    if (digitalRead(BUTTON_RIGHT) == LOW) {
      currentLetter[currentPos] = (currentLetter[currentPos] + 1) % 26;   // vor
      delay(200);
    }
    if (digitalRead(BUTTON_FIRE) == LOW) {
      currentPos++;  // Nächster Buchstabe
      delay(300);
    }
  }

  // In Buffer kopieren
  for (uint8_t i = 0; i < 3; i++) {
    nameBuffer[i] = alphabet[currentLetter[i]];
  }
  nameBuffer[3] = '\0';
}

// --- Sounds ---
void playShootSound() {
  tone(SOUND_PIN, 1000, 50);
}

void playHitSound() {
  tone(SOUND_PIN, 500, 100);
}

void playLoseLifeSound() {
  tone(SOUND_PIN, 200, 200);
}