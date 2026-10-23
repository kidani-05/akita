#include <Arduino.h>
#include "M5Unified.h"
#include "M5UnitENV.h"

// put function declarations here:
SHT4X sht4;
BMP280 bmp;

struct Bullet {
  bool active;
  int x;
  int y;
  int Z;
};

void drawPacman(int x, int y, int radius, uint16_t color, bool facingRight) {
  M5.Display.fillCircle(x, y, radius, color);
  // Black wedge makes the mouth; its point is at Pac-Man's center.
  const int mouthSize = min(radius, 12);
  const int mouthX = facingRight ? x + radius : x - radius;
  M5.Display.fillTriangle(x, y, mouthX, y - mouthSize, mouthX, y + mouthSize, BLACK);
}

void drawRotatedCharacter(lgfx::LGFX_Sprite& sprite, char character, int x, int y,
                          int textSize, int angle) {
  constexpr int spriteSize = 48;
  const int characterWidth = 6 * textSize;
  const int characterHeight = 8 * textSize;

  sprite.fillSprite(BLACK);
  sprite.setTextSize(textSize);
  sprite.setTextColor(WHITE, BLACK);
  sprite.setCursor((spriteSize - characterWidth) / 2,
                   (spriteSize - characterHeight) / 2);
  sprite.print(character);
  sprite.pushRotateZoom(&M5.Display,
                        x + characterWidth / 2, y + characterHeight / 2,
                        angle, 1.0f, 1.0f, BLACK);
}

void showSht4xErrorAnimation() {
  struct Food {
    char character;
    int x;
    int y;
  };

  constexpr int textX = 10;
  constexpr int textY[] = {25, 92, 159};
  constexpr int characterWidth = 18;  // Default font at text size 3.
  constexpr int pacmanRadius = 18;
  constexpr uint32_t initialTextDuration = 1500;
  constexpr uint32_t scatterDuration = 1000;
  constexpr uint32_t foodDuration = 700;
  constexpr uint32_t resultDuration = 3000;
  const char* const messages[] = {"Couldn't", "find", "SHT4X"};
  const Food foods[] = {
    {'C',  20,  24}, {'o', 142,  18}, {'u', 272,  32},
    {'l',  75,  66}, {'d', 206,  73}, {'n', 292,  92},
    {'\'', 22, 118}, {'t', 126, 108}, {'f', 238, 124},
    {'i',  52, 164}, {'n', 165, 153}, {'d', 245, 150},
    {'S',  92, 164}, {'H', 202, 164}, {'T', 286, 164},
    {'4',  18,  83}, {'X', 292, 137},
  };
  constexpr int foodCount = sizeof(foods) / sizeof(foods[0]);
  constexpr int pacmanCount = 2;
  constexpr int eatingSteps = (foodCount + pacmanCount - 1) / pacmanCount;
  int eatingOrder[foodCount];
  int foodSizes[foodCount];
  int foodAngles[foodCount];
  constexpr int bulletCount = 6;
  constexpr uint16_t hitPacmanColor = 0xF800;
  constexpr uint16_t userColor = 0xAFE5;
  Bullet bullets[bulletCount] = {};
  int userX = M5.Display.width() / 2;
  int userHitCount = 0;
  uint32_t pacmanFreezeUntil[pacmanCount] = {};
  int frozenPacmanX[pacmanCount] = {};
  int frozenPacmanY[pacmanCount] = {};
  bool frozenPacmanFacingRight[pacmanCount] = {};
  lgfx::LGFX_Sprite characterSprite(&M5.Display);
  characterSprite.setColorDepth(16);
  characterSprite.createSprite(48, 48);
  characterSprite.setPivot(24, 24);

  const uint32_t eatingDuration = foodDuration * eatingSteps;
  const uint32_t cycleDuration = initialTextDuration + scatterDuration
    + eatingDuration + resultDuration;
  constexpr uint16_t pacmanColors[] = {
    0xFFE0,  // Yellow
    0x07FF,  // Cyan
    0xF81F,  // Pink
  };
  uint32_t previousCycle = UINT32_MAX;
  randomSeed(micros());

  while (true) {
    M5.update();
    const int buttonX[] = {
      M5.Display.width() / 6,
      M5.Display.width() / 2,
      M5.Display.width() * 5 / 6,
    };
    const bool buttonPressed[] = {
      M5.BtnA.wasPressed(),
      M5.BtnB.wasPressed(),
      M5.BtnC.wasPressed(),
    };
    for (int button = 0; button < 3; ++button) {
      if (!buttonPressed[button]) {
        continue;
      }
      userX = buttonX[button];
      for (int i = 0; i < bulletCount; ++i) {
        if (!bullets[i].active) {
          bullets[i] = {true, userX, M5.Display.height() - 27};
          break;
        }
      }
    }
    for (int i = 0; i < bulletCount; ++i) {
      if (bullets[i].active) {
        bullets[i].y -= 6;
        if (bullets[i].y < 0) {
          bullets[i].active = false;
        }
      }
    }

    const uint32_t elapsed = millis();
    const uint32_t phase = elapsed % cycleDuration;
    const uint32_t cycle = elapsed / cycleDuration;
    const uint32_t resultStart = initialTextDuration + scatterDuration + eatingDuration;
    const uint16_t firstPacmanColor = pacmanColors[cycle % 3];
    const uint16_t secondPacmanColor = pacmanColors[(cycle + 1) % 3];

    if (phase >= resultStart) {
      for (int i = 0; i < bulletCount; ++i) {
        bullets[i].active = false;
      }
    }

    if (cycle != previousCycle) {
      previousCycle = cycle;
      userHitCount = 0;
      for (int i = 0; i < foodCount; ++i) {
        eatingOrder[i] = i;
      }
      for (int i = foodCount - 1; i > 0; --i) {
        const int swapIndex = random(i + 1);
        const int temporary = eatingOrder[i];
        eatingOrder[i] = eatingOrder[swapIndex];
        eatingOrder[swapIndex] = temporary;
      }
      // 17 letters: six small, six medium, and five large letters.
      for (int i = 0; i < foodCount; ++i) {
        foodSizes[i] = (i % 3) + 2;
      }
      for (int i = foodCount - 1; i > 0; --i) {
        const int swapIndex = random(i + 1);
        const int temporary = foodSizes[i];
        foodSizes[i] = foodSizes[swapIndex];
        foodSizes[swapIndex] = temporary;
      }
      for (int i = 0; i < foodCount; ++i) {
        foodAngles[i] = random(-180, 181);
      }
    }
    M5.Display.clear();
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(WHITE, BLACK);

    if (phase < initialTextDuration) {
      // Show the original error message before scattering its letters.
      for (int i = 0; i < 3; ++i) {
        M5.Display.setCursor(textX, textY[i]);
        M5.Display.print(messages[i]);
      }
    } else if (phase < initialTextDuration + scatterDuration) {
      // Move each character smoothly from its original line to its food position.
      const uint32_t scatterPhase = phase - initialTextDuration;
      for (int i = 0; i < foodCount; ++i) {
        int initialX;
        int initialY;
        if (i < 8) {  // "Couldn't"
          initialX = textX + i * characterWidth;
          initialY = textY[0];
        } else if (i < 12) {  // "find"
          initialX = textX + (i - 8) * characterWidth;
          initialY = textY[1];
        } else {  // "SHT4X"
          initialX = textX + (i - 12) * characterWidth;
          initialY = textY[2];
        }
        const int scatteredX = map(scatterPhase, 0, scatterDuration, initialX, foods[i].x);
        const int scatteredY = map(scatterPhase, 0, scatterDuration, initialY, foods[i].y);
        // Keep the original appearance at first, then apply the random size and angle.
        if (scatterPhase < scatterDuration / 2) {
          M5.Display.setTextSize(3);
          M5.Display.setCursor(scatteredX, scatteredY);
          M5.Display.print(foods[i].character);
        } else {
          drawRotatedCharacter(characterSprite, foods[i].character, scatteredX, scatteredY,
                               foodSizes[i], foodAngles[i]);
        }
      }
    } else if (phase < initialTextDuration + scatterDuration + eatingDuration) {
      const uint32_t eatingPhase = phase - initialTextDuration - scatterDuration;
      const int currentStep = min(static_cast<int>(eatingPhase / foodDuration), eatingSteps - 1);
      const uint32_t foodPhase = eatingPhase % foodDuration;
      const int firstUneatenFood = currentStep * pacmanCount;

      // Draw only the letters that have not yet been eaten.
      for (int i = firstUneatenFood; i < foodCount; ++i) {
        const int foodIndex = eatingOrder[i];
        const Food& food = foods[foodIndex];
        drawRotatedCharacter(characterSprite, food.character, food.x, food.y,
                             foodSizes[foodIndex], foodAngles[foodIndex]);
      }

      // The left and right Pac-Men eat alternating letters from the shuffled order.
      for (int pacman = 0; pacman < pacmanCount; ++pacman) {
        const int foodPosition = firstUneatenFood + pacman;
        if (foodPosition >= foodCount) {
          continue;
        }

        const int currentFoodIndex = eatingOrder[foodPosition];
        const int startX = foodPosition < pacmanCount
          ? (pacman == 0 ? -pacmanRadius : M5.Display.width() + pacmanRadius)
          : foods[eatingOrder[foodPosition - pacmanCount]].x + characterWidth / 2;
        const int startY = foodPosition < pacmanCount
          ? M5.Display.height() / 2
          : foods[eatingOrder[foodPosition - pacmanCount]].y + 12;
        const int targetX = foods[currentFoodIndex].x + characterWidth / 2;
        const int targetY = foods[currentFoodIndex].y + 12;
        const int pacmanX = map(foodPhase, 0, foodDuration, startX, targetX);
        const int pacmanY = map(foodPhase, 0, foodDuration, startY, targetY);
        bool facingRight = targetX >= startX;
        const uint16_t pacmanColor = pacman == 0 ? firstPacmanColor : secondPacmanColor;
        int drawX = pacmanX;
        int drawY = pacmanY;
        uint16_t drawColor = pacmanColor;

        if (elapsed < pacmanFreezeUntil[pacman]) {
          drawX = frozenPacmanX[pacman];
          drawY = frozenPacmanY[pacman];
          drawColor = hitPacmanColor;
          facingRight = frozenPacmanFacingRight[pacman];
        } else {
          for (int i = 0; i < bulletCount; ++i) {
            if (!bullets[i].active) {
              continue;
            }
            const int dx = bullets[i].x - pacmanX;
            const int dy = bullets[i].y - pacmanY;
            const int hitDistance = pacmanRadius + 3;
            if (dx * dx + dy * dy <= hitDistance * hitDistance) {
              bullets[i].active = false;
              ++userHitCount;
              pacmanFreezeUntil[pacman] = elapsed + 1000;
              frozenPacmanX[pacman] = pacmanX;
              frozenPacmanY[pacman] = pacmanY;
              frozenPacmanFacingRight[pacman] = facingRight;
              drawX = pacmanX;
              drawY = pacmanY;
              drawColor = hitPacmanColor;
              break;
            }
          }
        }
        drawPacman(drawX, drawY, pacmanRadius, drawColor, facingRight);
      }
    } else {
      constexpr int firstPacmanScore = (foodCount + 1) / 2;
      constexpr int secondPacmanScore = foodCount / 2;
      M5.Display.setTextSize(3);
      M5.Display.setCursor(58, 30);
      M5.Display.print("RESULT");
      M5.Display.setTextSize(2);
      M5.Display.setCursor(32, 82);
      M5.Display.setTextColor(firstPacmanColor, BLACK);
      M5.Display.printf("PAC-MAN 1: %d", firstPacmanScore);
      M5.Display.setCursor(32, 122);
      M5.Display.setTextColor(secondPacmanColor, BLACK);
      M5.Display.printf("PAC-MAN 2: %d", secondPacmanScore);
      M5.Display.setCursor(32, 162);
      M5.Display.setTextColor(userColor, BLACK);
      M5.Display.printf("USER HITS: %d", userHitCount);
    }

    for (int i = 0; i < bulletCount; ++i) {
      if (bullets[i].active) {
        M5.Display.fillCircle(bullets[i].x, bullets[i].y, 3, WHITE);
      }
    }
    const uint32_t eatingStart = initialTextDuration + scatterDuration;
    if (phase >= eatingStart && phase < eatingStart + eatingDuration) {
      const int instructionTop = M5.Display.height() - 32;
      M5.Display.fillRect(0, instructionTop, M5.Display.width(), 32, 0x2104);
      M5.Display.drawFastHLine(0, instructionTop, M5.Display.width(), userColor);
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(userColor, 0x2104);
      M5.Display.setCursor(58, instructionTop + 8);
      M5.Display.print("PRESS THE BUTTON!");
    }
    delay(30);
  }
}

void setup() {
  M5.begin();
  if(!sht4.begin(&Wire, SHT40_I2C_ADDR_44, 21, 22, 400000U)) {
    showSht4xErrorAnimation();
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(sht4.update()) {
    M5.Display.clear();
    M5.Display.printf("Temperature: %f [deg C]\n",sht4.cTemp);
    M5.Display.printf("Humidity: %f [%%]\n",sht4.humidity);
  }
  delay(1000);
}
