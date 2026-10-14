#include "XN297.h"

namespace XN297 {

// The nRF24 wants addresses last byte first. On air these are the XN297's
// start pattern: 0C 71 0F 55 before every packet.
const uint8_t SEND_ADDRESS[5] = {0x55, 0x0F, 0x71, 0x0C, 0x00};
const uint8_t LISTEN_ADDRESS[3] = {0x55, 0x0F, 0x71};

// The drone and remote talk on address CC CC CC CC CC (found with the
// Sniffer, see SNIFFING.md).
const uint8_t DRONE_ADDRESS_BYTE = 0xCC;
const int ADDRESS_SIZE = 5;

// The XN297 scrambles each byte with this list: byte 0 of the packet is
// XORed with SCRAMBLE[0], and so on.
const uint8_t SCRAMBLE[ADDRESS_SIZE + PAYLOAD_SIZE] = {
    0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5, 0x66, 0x0D, 0xAE, 0x8C,
    0x88, 0x12, 0x69, 0xEE, 0x1F, 0xC7, 0x62, 0x97, 0xD5, 0x0B,
};

// The checksum: a CRC-16 (CCITT) over the scrambled bytes, started at
// CRC_START and XORed with CRC_FINISH for a packet of this length.
const uint16_t CRC_START = 0xB5D2;
const uint16_t CRC_FINISH = 0x8B17;

// The XN297 sends each payload byte's bits in the opposite order.
static uint8_t reverseBits(uint8_t value) {
  uint8_t reversed = 0;
  for (int bit = 0; bit < 8; bit++) {
    reversed = (reversed << 1) | (value & 1);
    value >>= 1;
  }
  return reversed;
}

static uint16_t checksum(const uint8_t packet[PACKET_SIZE]) {
  uint16_t crc = CRC_START;
  for (int i = 0; i < ADDRESS_SIZE + PAYLOAD_SIZE; i++) {
    crc ^= (uint16_t)packet[i] << 8;
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
  }
  return crc ^ CRC_FINISH;
}

void encode(const uint8_t payload[PAYLOAD_SIZE], uint8_t packet[PACKET_SIZE]) {
  for (int i = 0; i < ADDRESS_SIZE; i++) {
    packet[i] = DRONE_ADDRESS_BYTE ^ SCRAMBLE[i];
  }
  for (int i = 0; i < PAYLOAD_SIZE; i++) {
    packet[ADDRESS_SIZE + i] = reverseBits(payload[i]) ^ SCRAMBLE[ADDRESS_SIZE + i];
  }
  uint16_t crc = checksum(packet);
  packet[PACKET_SIZE - 2] = crc >> 8;
  packet[PACKET_SIZE - 1] = crc & 0xFF;
}

bool decode(const uint8_t packet[PACKET_SIZE], uint8_t payload[PAYLOAD_SIZE]) {
  for (int i = 0; i < ADDRESS_SIZE; i++) {
    if (packet[i] != (DRONE_ADDRESS_BYTE ^ SCRAMBLE[i])) return false;
  }
  uint16_t crc = checksum(packet);
  if (packet[PACKET_SIZE - 2] != (crc >> 8)) return false;
  if (packet[PACKET_SIZE - 1] != (crc & 0xFF)) return false;

  for (int i = 0; i < PAYLOAD_SIZE; i++) {
    payload[i] = reverseBits(packet[ADDRESS_SIZE + i] ^ SCRAMBLE[ADDRESS_SIZE + i]);
  }
  return true;
}

}  // namespace XN297
