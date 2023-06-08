#include <Arduino.h>
#include <LiquidCrystal.h>
#include <OneButton.h>

byte player[] = {  // custom character of a player
  B11111,
  B10101,
  B11111,
  B11111,
  B01110,
  B01010,
  B11011,
  B00000
};

byte enemy[] = {  // custom character of an enemy
  B00000,
  B01110,
  B10001,
  B11111,
  B11111,
  B10001,
  B01110,
  B00000
};

byte topBottom[] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111
};

byte topLeftBottom[] = {
  B11111,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B11111
};

byte topRightBottom[] = {
  B11111,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B00001,
  B11111
};

byte full[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

const int PLAYER_CHAR = 1;
const int ENEMY_CHAR = 2;
const int TOP_BOTTOM = 3;
const int TOP_LEFT_BOTTOM = 4;
const int TOP_RIGHT_BOTTOM = 5;
const int FULL = 6;

const int RS = 12, EN = 11, D4 = 9, D5 = 8, D6 = 7, D7 = 6;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
OneButton DecisionButton(A0, true);

const int UP_BUTTON_PIN = 3, DOWN_BUTTON_PIN = 2;

void setup() {
  lcd.begin(16, 2);  // Initialize the LCD with 16 columns and 2 rows

  // create custom player, enemy, and level appearance
  lcd.createChar(PLAYER_CHAR, player);
  lcd.createChar(ENEMY_CHAR, enemy);
  lcd.createChar(TOP_BOTTOM, topBottom);
  lcd.createChar(TOP_LEFT_BOTTOM, topLeftBottom);
  lcd.createChar(TOP_RIGHT_BOTTOM, topRightBottom);
  lcd.createChar(FULL, full);

  // set up buttons
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(UP_BUTTON_PIN), handleUp, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DOWN_BUTTON_PIN), handleDown, CHANGE);
  DecisionButton.attachClick(handleDecisionOneClick);
  DecisionButton.attachDoubleClick(handleDecisionDoubleClick);
  DecisionButton.attachLongPressStart(handleDecisionLongPress);

  setUpEnemies();

  lcd.setCursor(0, 0);
}

bool gameStarted = false;

void loop() {
  DecisionButton.tick();
  if (gameStarted) {
    moveUser();

    detectCollisions();

    displayScore();

    spawnEnemy();

    moveEnemies();
  } else {
    displayLevelChoice();
  }
}

int level = 1;
const int STARTING_CURSOR_X = 7;
const int LEVEL_SPACE = 3;
const int MAXIMUM_X = 15;
int i = STARTING_CURSOR_X;

void displayLevelChoice() {
  lcd.setCursor(0, 0);
  lcd.print("LEVEL");

  lcd.setCursor(STARTING_CURSOR_X, 0);
  lcd.print(" 1  2  3 ");
  displayLevelBar(i);
  i++;
  if (i > MAXIMUM_X) {
    i = STARTING_CURSOR_X;
  }
}

void displayLevelBar(int i) {
  lcd.setCursor(i, 1);
  if (i < (level * LEVEL_SPACE) + STARTING_CURSOR_X) {
    lcd.write(FULL);
  } else if (i < MAXIMUM_X) {
    lcd.write(TOP_BOTTOM);
  } else {
    lcd.write(TOP_RIGHT_BOTTOM);
  }
}

int score = 0;
void displayScore() {
  lcd.setCursor(0, 0);
  lcd.print("SCR");
  lcd.setCursor(0, 1);
  lcd.print(score);
}

int playerX = 5;
int playerY = 0;
long timeFromLastMove = 0;
long clearRefreshRate = 100;

void moveUser() {
  // clear previous position
  long currentTime = millis();
  if (currentTime - timeFromLastMove < clearRefreshRate) {
    lcd.setCursor(playerX, !playerY);
    lcd.print(' ');
    timeFromLastMove = 0;
  }

  // print current position of player
  lcd.setCursor(playerX, playerY);
  lcd.write(PLAYER_CHAR);
}


const int MAX_ENEMIES = 6;
int enemies[MAX_ENEMIES][2];
int startingEnemies[MAX_ENEMIES][2];

void setUpEnemies() {

  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i][0] = MAXIMUM_X + 1;
    enemies[i][1] = random(2);
    startingEnemies[1][0] = MAXIMUM_X + 1;
    startingEnemies[1][1] = random(2);
  }
}

int startingEnemyRefreshRate = 500;
long previousMillisEnemy = 0;
long moveEnemyRefreshRate = startingEnemyRefreshRate;
const int SPAWN_GAP = 3;
bool addedScore = false;
int enemiesAlive = 0;
int enemiesMoving = 0;

void moveEnemies() {
  long currentMillis = millis();

  if (currentMillis - previousMillisEnemy > moveEnemyRefreshRate) {
    for (int i = 0; i < enemiesMoving; i++) {
      printCurrentEnemyPosition(i);
      lcd.print(' ');

      enemies[i][0]--;
      if (enemies[i][0] < playerX - 2) {
        clearPreviousEnemyPosition(i);  // this is to clear the enemy right after it passes the player
        addScore();
      }
      
    }

    previousMillisEnemy = currentMillis;
  }
}

void printCurrentEnemyPosition(int i) {
  lcd.setCursor(enemies[i][0], enemies[i][1]);
  lcd.write(ENEMY_CHAR);
}

void clearPreviousEnemyPosition(int i) {
  if (enemies[i][0] < MAXIMUM_X) {
    lcd.setCursor((enemies[i][0] + 1), enemies[i][1]);
    lcd.print(' ');
  }
}

int scoreMultiplier = 1;
int scoreBreakpoint = 10;
const int SPEED_UP_RATE = 50;

void addScore() {
  if (!addedScore) {
    score = score + 1 * scoreMultiplier;  // this is to prevent adding score multiple times,
    addedScore = true;                    // because of different refresh rate for spawning enemy and moving enemy

    if (score != 0 && score % scoreBreakpoint == 0) {
      moveEnemyRefreshRate -= SPEED_UP_RATE;
      scoreMultiplier++;
    }
  }
}

long previousMillisSpawnEnemy = 0;

void spawnEnemy() {
  long currentMillis = millis();
  long spawnRefreshRate = moveEnemyRefreshRate * SPAWN_GAP;

  if (currentMillis - previousMillisSpawnEnemy > spawnRefreshRate) {
    if (enemiesAlive >= MAX_ENEMIES && enemies[0][0] < playerX) {
      enemiesAlive = 0;
    }
    int randomY = random(2);
    enemies[enemiesAlive][0] = MAXIMUM_X;
    enemies[enemiesAlive][1] = randomY;

    if (enemiesMoving < MAX_ENEMIES) {
      enemiesMoving++;
    }

    addedScore = false;
    enemiesAlive++;
    previousMillisSpawnEnemy = currentMillis;
  }
}

void detectCollisions() {
  for (int i = 0; i < enemiesMoving; i++) {
    if (enemies[i][0] == playerX && enemies[i][1] == playerY) {
      delay(500);

      displayLose();
      clearEnemies();
      clearGameVariables();

      delay(1000);
      lcd.clear();
      break;
    }
  }
}

void clearGameVariables() {
  score = 0;
  moveEnemyRefreshRate = startingEnemyRefreshRate;
  scoreMultiplier = 1;
}

void displayLose() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("YOU LOST!");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SCORE: ");
  lcd.print(score);
  delay(1000);
}

void clearEnemies() {
  enemiesAlive = 0;
  enemiesMoving = 0;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i][0] = startingEnemies[i][0];
    enemies[i][1] = startingEnemies[i][1];
  }
}


// buttons handling
void handleUp() {
  if (playerY - 1 >= 0) {
    playerY--;
  }
  timeFromLastMove = millis();
}

void handleDown() {
  if (playerY + 1 <= 1) {
    playerY++;
  }
  timeFromLastMove = millis();
}

// increase the level
void handleDecisionOneClick() {
  if (!gameStarted) {
    if (level + 1 <= 3) {
      level++;
    }
  } else {  // in game, this button returns to level choice
    lcd.clear();
    gameStarted = false;
    clearEnemies();
    clearGameVariables();
  }
}

// decrease the level
void handleDecisionDoubleClick() {
  if (level - 1 >= 1) {
    level--;
  }
}

const int LEVEL_1_REFRESH_RATE = 500;
const int LEVEL_2_REFRESH_RATE = 400;
const int LEVEL_3_REFRESH_RATE = 300;
const int LEVEL_1_SCORE_MULTIPLIER = 1;
const int LEVEL_2_SCORE_MULTIPLIER = 3;
const int LEVEL_3_SCORE_MULTIPLIER = 5;

// start game
void handleDecisionLongPress() {
  if (!gameStarted) {
    lcd.clear();
    gameStarted = true;
    switch (level) {
      case 1:
        startingEnemyRefreshRate = LEVEL_1_REFRESH_RATE;
        scoreMultiplier = LEVEL_1_SCORE_MULTIPLIER;
        break;
      case 2:
        startingEnemyRefreshRate = LEVEL_2_REFRESH_RATE;
        scoreMultiplier = LEVEL_2_SCORE_MULTIPLIER;
        break;
      case 3:
        startingEnemyRefreshRate = LEVEL_3_REFRESH_RATE;
        scoreMultiplier = LEVEL_3_SCORE_MULTIPLIER;
        break;
    }
  } else {  // in game, this button returns to level choice
    lcd.clear();
    gameStarted = false;
    clearEnemies();
    clearGameVariables();
  }
}
