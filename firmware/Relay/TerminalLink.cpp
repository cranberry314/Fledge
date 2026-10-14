#include "TerminalLink.h"

void TerminalLink::update() {
  while (Serial.available() > 0) {
    char letter = Serial.read();
    if (letter == '\n' || letter == '\r') {
      line[lineLength] = '\0';
      if (lineLength > 0) handleLine(line);
      lineLength = 0;
    } else if (lineLength < (int)sizeof(line) - 1) {
      line[lineLength++] = letter;
    }
  }
}

void TerminalLink::handleLine(const char *text) {
  if (strcmp(text, "p") == 0 || strcmp(text, "P") == 0 || strcmp(text, "pair") == 0) {
    pairAsked = true;
    return;
  }
  if (strcmp(text, "hello") == 0) {
    helloSaid = true;
    return;
  }

  int numbers[MESSAGE_SIZE];
  int found = sscanf(text, "controls %d %d %d %d %d %d %d %d",
                     &numbers[0], &numbers[1], &numbers[2], &numbers[3],
                     &numbers[4], &numbers[5], &numbers[6], &numbers[7]);
  if (found != MESSAGE_SIZE) {
    Serial.printf("Didn't understand: %s\n", text);
    return;
  }

  uint8_t message[MESSAGE_SIZE];
  for (int i = 0; i < MESSAGE_SIZE; i++) {
    message[i] = (uint8_t)numbers[i];  // -100 becomes 0x9C, as on Bluetooth
  }
  if (readMessage(message, newestControls)) {
    newestControlsAt = millis();
  } else {
    Serial.printf("Didn't understand: %s\n", text);
  }
}

unsigned long TerminalLink::latest(Controls &controls) const {
  if (newestControlsAt == 0) return 0;
  controls = newestControls;
  return newestControlsAt;
}

bool TerminalLink::pairWasAsked() {
  bool asked = pairAsked;
  pairAsked = false;
  return asked;
}

bool TerminalLink::helloWasSaid() {
  bool said = helloSaid;
  helloSaid = false;
  return said;
}
