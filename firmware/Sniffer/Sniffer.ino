// Sniffer: tries to read the remote's packets without knowing its address.
//
// It listens on each channel in turn, for LISTEN_MS each, and prints what
// arrives. With TRY_EVERYTHING it also tries every data rate and both ways
// of listening. Most of what arrives is random
// noise that happens to look like a packet. A real packet is sent over and
// over, so it arrives many times with exactly the same bytes: the summary
// after each listen lists anything that arrived 3 times or more.
//
// Open the Serial Monitor at 115200 baud.
//   PKT  one packet: time (ms), channel, data rate, way of listening, bytes
//   SUM  after each listen that heard something: how many packets, and
//        which ones repeated
//   ROUND  after every channel has been tried once

#include <SPI.h>
#include <RF24.h>

RF24 radio(4, 5);  // CE = GPIO 4, CSN = GPIO 5 (see firmware/SETUP.md)

// Channels to listen on: every channel from FIRST_CHANNEL to LAST_CHANNEL.
// The remote pairs on 16-17; once paired it uses 60, 63 and 65.
// 0 to 125: search everywhere. 65 to 65: a control session (channel 65
// also hears the remote's channel 63 packets).
const uint8_t FIRST_CHANNEL = 65;
const uint8_t LAST_CHANNEL = 65;

// false: listen only the way the remote talks, XN297 at 1 Mbps.
// true: also try the other data rates and the plain nRF24 way (slower).
const bool TRY_EVERYTHING = false;

// How long to listen on each channel, in milliseconds.
const unsigned long LISTEN_MS = 300;

// How many packets to print each time. The rest are only counted.
const int PRINT_LIMIT = 10;

// The three data rates the nRF24 can use.
const rf24_datarate_e RATES[] = {RF24_1MBPS, RF24_250KBPS, RF24_2MBPS};
const char* RATE_NAMES[] = {"1M", "250K", "2M"};

// Two ways of listening without knowing the remote's address:
//   XN297: listen for the fixed start pattern that XN297 radio chips put
//          before every packet (71 0F 55 on air). Many toy drones use them.
//   NRF24: a 2-byte address of noise then the nRF24 start pattern (00 AA
//          or 00 55 on air). Catches plain nRF24 and Beken packets.
const int XN297 = 0;
const int NRF24 = 1;
const char* WAY_NAMES[] = {"XN297", "NRF24"};

// Bytes as the radio's address register wants them: last byte on air first.
uint8_t xn297Start[] = {0x55, 0x0F, 0x71};
uint8_t nrf24StartAA[] = {0xAA, 0x00};
uint8_t nrf24Start55[] = {0x55, 0x00};

// Packets seen during one listen, to spot repeats. Only the first
// SAME_BYTES bytes are compared.
const int SAME_BYTES = 10;
const int MAX_KINDS = 64;
uint8_t kinds[MAX_KINDS][SAME_BYTES];
int kindCounts[MAX_KINDS];
int kindsSeen = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== Sniffer ===");

  if (!radio.begin() || !radio.isChipConnected()) {
    Serial.println("Radio not found: check the wiring with RadioCheck.");
    while (true) delay(1000);
  }
  radio.setAutoAck(false);     // don't answer anything we hear
  radio.disableCRC();          // keep packets even if their check fails
  radio.setPayloadSize(32);    // always read the longest packet
  radio.disableDynamicPayloads();
}

void loop() {
  for (int way = XN297; way <= NRF24; way++) {
    for (int r = 0; r < 3; r++) {
      bool remotesWay = (way == XN297 && r == 0);  // XN297 at 1 Mbps
      if (!TRY_EVERYTHING && !remotesWay) continue;
      for (int channel = FIRST_CHANNEL; channel <= LAST_CHANNEL; channel++) {
        listenOnce(channel, r, way);
      }
      if (FIRST_CHANNEL != LAST_CHANNEL) {  // one channel: nothing to report
        Serial.printf("ROUND %lu done: rate=%s way=%s channels %u-%u\n",
                      millis(), RATE_NAMES[r], WAY_NAMES[way], FIRST_CHANNEL,
                      LAST_CHANNEL);
      }
    }
  }
}

void listenOnce(uint8_t channel, int r, int way) {
  radio.stopListening();
  radio.setChannel(channel);
  radio.setDataRate(RATES[r]);
  if (way == XN297) {
    radio.setAddressWidth(3);
    radio.openReadingPipe(0, xn297Start);
    radio.closeReadingPipe(1);
  } else {
    radio.setAddressWidth(2);
    radio.openReadingPipe(0, nrf24StartAA);
    radio.openReadingPipe(1, nrf24Start55);
  }
  radio.startListening();

  // Every 30 seconds, print the radio's real settings, to check the
  // address, address width and CRC really are what we asked for.
  static unsigned long settingsShown = 0;
  if (settingsShown == 0 || millis() - settingsShown > 30000) {
    settingsShown = millis();
    Serial.printf("--- radio settings for way=%s ---\n", WAY_NAMES[way]);
    radio.printPrettyDetails();
  }

  kindsSeen = 0;
  int packets = 0;
  unsigned long start = millis();
  while (millis() - start < LISTEN_MS) {
    uint8_t pipe;
    if (radio.available(&pipe)) {
      uint8_t packet[32];
      radio.read(packet, 32);
      packets++;
      remember(packet);
      if (packets <= PRINT_LIMIT) {
        Serial.printf("PKT %lu ch=%u rate=%s way=%s pipe=%u :", millis(),
                      channel, RATE_NAMES[r], WAY_NAMES[way], pipe);
        printBytes(packet, 32);
        Serial.println();
      }
    }
  }

  if (packets == 0) return;  // nothing heard: keep the output short
  Serial.printf("SUM ch=%u rate=%s way=%s packets=%d repeated:", channel,
                RATE_NAMES[r], WAY_NAMES[way], packets);
  int repeated = 0;
  for (int k = 0; k < kindsSeen; k++) {
    if (kindCounts[k] >= 3) {
      Serial.printf(" [x%d", kindCounts[k]);
      printBytes(kinds[k], SAME_BYTES);
      Serial.print("]");
      repeated++;
    }
  }
  if (repeated == 0) Serial.print(" none");
  Serial.println();
}

// Count this packet: add it to the list, or add one to its count.
void remember(uint8_t* packet) {
  for (int k = 0; k < kindsSeen; k++) {
    if (memcmp(kinds[k], packet, SAME_BYTES) == 0) {
      kindCounts[k]++;
      return;
    }
  }
  if (kindsSeen < MAX_KINDS) {
    memcpy(kinds[kindsSeen], packet, SAME_BYTES);
    kindCounts[kindsSeen] = 1;
    kindsSeen++;
  }
}

void printBytes(uint8_t* bytes, int count) {
  for (int i = 0; i < count; i++) {
    Serial.printf(" %02X", bytes[i]);
  }
}
