// RadioCheck: is the nRF24L01+ wired right, and does its receiver hear anything?
//
// Open the Serial Monitor at 115200 baud. Every few seconds this prints:
//   1. A wiring check: it writes settings into the radio and reads them back.
//      "SPI read-back: ALL OK" means the six signal wires and power are good.
//   2. Six scan lines, one character per channel (0 on the left, 125 on the
//      right; channel N is 2400 + N MHz). "-" means nothing heard there.
//      A digit means something was heard: 1 is a little, f is a lot.
//
// This only hears that a channel is busy. It does not read any packets.

#include <SPI.h>
#include <RF24.h>

RF24 radio(4, 5);  // CE = GPIO 4, CSN = GPIO 5 (see firmware/SETUP.md)

int scanLines = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);
}

void loop() {
  if (scanLines == 0) {
    checkWiring();
  }
  scanOnce();
  scanLines++;
  if (scanLines == 6) {
    Serial.println("=== done ===");
    scanLines = 0;
  }
}

void checkWiring() {
  Serial.println("\n=== RadioCheck ===");

  bool began = radio.begin();
  Serial.printf("begin(): %s\n", began ? "ok" : "FAILED");
  Serial.printf("isChipConnected(): %s\n", radio.isChipConnected() ? "yes" : "NO");
  Serial.printf("isPVariant() (nRF24L01+): %s\n", radio.isPVariant() ? "yes" : "no");

  // Write settings and read them back. A loose wire reads back as 0 or 255.
  int bad = 0;
  uint8_t channels[] = {1, 42, 85, 125};
  for (uint8_t ch : channels) {
    radio.setChannel(ch);
    uint8_t got = radio.getChannel();
    Serial.printf("setChannel(%u) -> read back %u %s\n", ch, got, got == ch ? "ok" : "MISMATCH");
    if (got != ch) bad++;
  }
  rf24_datarate_e rates[] = {RF24_250KBPS, RF24_1MBPS, RF24_2MBPS};
  for (rf24_datarate_e r : rates) {
    radio.setDataRate(r);
    rf24_datarate_e got = radio.getDataRate();
    Serial.printf("setDataRate(%d) -> read back %d %s\n", r, got, got == r ? "ok" : "MISMATCH");
    if (got != r) bad++;
  }
  Serial.printf("SPI read-back: %s\n", bad == 0 ? "ALL OK" : "FAILURES");

  radio.printPrettyDetails();

  radio.setAutoAck(false);
  radio.setDataRate(RF24_1MBPS);
  Serial.println("\nScan: each column is a channel 0-125, digit = hits in 100 tries (hex, capped at f)");
}

// Listen on every channel 100 times, then print one line.
void scanOnce() {
  uint8_t hits[126] = {0};
  for (int rep = 0; rep < 100; rep++) {
    for (int ch = 0; ch < 126; ch++) {
      radio.setChannel(ch);
      radio.startListening();
      delayMicroseconds(130);
      if (radio.testCarrier()) hits[ch]++;
      radio.stopListening();
    }
  }
  for (int ch = 0; ch < 126; ch++) {
    if (hits[ch] == 0) {
      Serial.print("-");
    } else {
      Serial.print(min((int)hits[ch], 15), HEX);
    }
  }
  Serial.println();
}
