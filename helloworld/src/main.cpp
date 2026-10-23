#include <Arduino.h>
#include "M5Unified.h"

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Display.println("Hello, M5Stack!");
}

void loop() {
  // put your main code here, to run repeatedly:
  M5.update();
  if(M5.BtnA.wasPressed()) {
    M5.Display.println("Button A pressed!");
  }
}

// put function definitions here:
