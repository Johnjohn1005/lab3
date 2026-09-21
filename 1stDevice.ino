#include <Arduino.h>

const int LED_PINS[4] = {2, 21, 19, 18};
const int BUZZER_PIN = 4;
const int BUTTON_ARM_PIN = 15;
const int BUTTON_MODE_PIN = 23;

const int DOT_TIME = 200;
const int DASH_TIME = DOT_TIME * 3;
const int SYMBOL_SPACE = DOT_TIME;
const int LETTER_SPACE = DOT_TIME * 3;
const int WORD_SPACE = DOT_TIME * 7;

const char* morseTable[26] = {
  ".-", "-...", ".-.-", "-..", ".", "..-.", "--.", "....", "..",
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
};

bool systemArmed = false;
enum TransmissionMode { MODE_SOS, MODE_FAITH };
TransmissionMode currentMode = MODE_SOS;

int lastArmReading = HIGH;
int armButtonState = HIGH;
unsigned long lastArmDebounceTime = 0;

int lastModeReading = HIGH;
int modeButtonState = HIGH;
unsigned long lastModeDebounceTime = 0;

const unsigned long DEBOUNCE_DELAY = 50;

void checkButtons();
void signalOn();
void signalOff();
void sendDot();
void sendDash();
void transmitSymbol(char symbol);
void transmitCharacter(char c);
void transmitString(String text);
void turnOnAllLEDs();
void turnOffAllLEDs();
void turnEverythingOff();
bool customDelay(unsigned long duration);

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_ARM_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);

  turnEverythingOff();

  Serial.begin(115200);
  delay(1000);

  Serial.println("==========================================");
  Serial.println("     DUAL-MODE MORSE CODE TRANSMITTER     ");
  Serial.println("==========================================");
}

void loop() {
  checkButtons();

  if (systemArmed) {
    String textToTransmit = (currentMode == MODE_SOS) ? "SOS" : "FAITH";
    
    Serial.println("\n>>> Transmitting Mode [" + textToTransmit + "] <<<");
    transmitString(textToTransmit);
    
    unsigned long waitStart = millis();
    while (systemArmed && (millis() - waitStart < WORD_SPACE)) {
      checkButtons();
    }
  }
}

void checkButtons() {
  unsigned long now = millis();

  int readingArm = digitalRead(BUTTON_ARM_PIN);
  if (readingArm != lastArmReading) {
    lastArmDebounceTime = now;
  }
  if ((now - lastArmDebounceTime) > DEBOUNCE_DELAY) {
    if (readingArm != armButtonState) {
      armButtonState = readingArm;
      if (armButtonState == LOW) {
        systemArmed = !systemArmed;
        if (systemArmed) {
          String activeModeStr = (currentMode == MODE_SOS) ? "SOS" : "FAITH";
          Serial.println("\n>>> SYSTEM ARMED: Playing " + activeModeStr + " <<<");
        } else {
          turnEverythingOff();
          Serial.println("\n>>> SYSTEM DISARMED <<<");
        }
      }
    }
  }
  lastArmReading = readingArm;

  int readingMode = digitalRead(BUTTON_MODE_PIN);
  if (readingMode != lastModeReading) {
    lastModeDebounceTime = now;
  }
  if ((now - lastModeDebounceTime) > DEBOUNCE_DELAY) {
    if (readingMode != modeButtonState) {
      modeButtonState = readingMode;
      if (modeButtonState == LOW) {
        if (currentMode == MODE_SOS) {
          currentMode = MODE_FAITH;
          Serial.println("\n>>> MODE CHANGED TO: FAITH <<<");
        } else {
          currentMode = MODE_SOS;
          Serial.println("\n>>> MODE CHANGED TO: SOS <<<");
        }

        if (systemArmed) {
          turnEverythingOff();
        }
      }
    }
  }
  lastModeReading = readingMode;
}

void turnOnAllLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
}

void turnOffAllLEDs() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

void turnEverythingOff() {
  turnOffAllLEDs();
  digitalWrite(BUZZER_PIN, LOW);
}

void signalOn() {
  turnOnAllLEDs();
  digitalWrite(BUZZER_PIN, HIGH);
}

void signalOff() {
  turnEverythingOff();
}

bool customDelay(unsigned long duration) {
  unsigned long start = millis();
  while (millis() - start < duration) {
    checkButtons();
    if (!systemArmed) {
      turnEverythingOff();
      return false;
    }
  }
  return true;
}

void sendDot() {
  if (!systemArmed) return;
  signalOn();
  if (!customDelay(DOT_TIME)) return;
  signalOff();
  customDelay(SYMBOL_SPACE);
}

void sendDash() {
  if (!systemArmed) return;
  signalOn();
  if (!customDelay(DASH_TIME)) return;
  signalOff();
  customDelay(SYMBOL_SPACE);
}

void transmitSymbol(char symbol) {
  if (symbol == '.') {
    sendDot();
  } else if (symbol == '-') {
    sendDash();
  }
}

void transmitCharacter(char c) {
  if (!systemArmed) return;

  c = toupper(c);

  if (c >= 'A' && c <= 'Z') {
    int index = c - 'A';
    const char* morseCode = morseTable[index];

    Serial.print("Letter '");
    Serial.print(c);
    Serial.print("' -> Morse: ");
    Serial.println(morseCode);

    for (int i = 0; morseCode[i] != '\0' && systemArmed; i++) {
      transmitSymbol(morseCode[i]);
    }

    customDelay(LETTER_SPACE - SYMBOL_SPACE);
  } else if (c == ' ') {
    customDelay(WORD_SPACE);
  }
}

void transmitString(String text) {
  TransmissionMode startedMode = currentMode;

  for (unsigned int i = 0; i < text.length() && systemArmed; i++) {
    if (currentMode != startedMode) {
      break;
    }
    transmitCharacter(text[i]);
  }
}