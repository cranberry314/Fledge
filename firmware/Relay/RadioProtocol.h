#pragma once

#include <Arduino.h>

namespace DroneRadio {

const uint8_t PACKET_SIZE = 23;
const uint8_t ADDRESS_SIZE = 5;
const uint8_t DATA_SIZE = 16;

const uint8_t TX_ADDRESS[5] = {0x55, 0x0F, 0x71, 0x0C, 0x00};
const uint8_t CONTROL_CHANNELS[3] = {60, 63, 65};

const uint8_t SCRAMBLE[21] = {
  0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5,
  0x66, 0x0D, 0xAE, 0x8C, 0x88, 0x12, 0x69,
  0xEE, 0x1F, 0xC7, 0x62, 0x97, 0xD5, 0x0B
};

uint8_t reverseBits(uint8_t value) {
  value = (value >> 4) | (value << 4);
  value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
  return ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
}

uint16_t updateCrc(uint16_t crc, uint8_t value) {
  crc ^= (uint16_t)value << 8;
  for (uint8_t bit = 0; bit < 8; bit++) {
    crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}

void encodePacket(const uint8_t data[DATA_SIZE], uint8_t packet[PACKET_SIZE]) {
  for (uint8_t i = 0; i < ADDRESS_SIZE; i++) {
    packet[i] = 0xCC ^ SCRAMBLE[i];
  }
  for (uint8_t i = 0; i < DATA_SIZE; i++) {
    packet[ADDRESS_SIZE + i] =
        reverseBits(data[i]) ^ SCRAMBLE[ADDRESS_SIZE + i];
  }

  uint16_t crc = 0xB5D2;
  for (uint8_t i = 0; i < PACKET_SIZE - 2; i++) {
    crc = updateCrc(crc, packet[i]);
  }
  crc ^= 0x8B17;
  packet[PACKET_SIZE - 2] = crc >> 8;
  packet[PACKET_SIZE - 1] = crc & 0xFF;
}

// The opposite of encodePacket. Returns false for noise: wrong address
// or a failed check.
bool decodePacket(const uint8_t packet[PACKET_SIZE], uint8_t data[DATA_SIZE]) {
  for (uint8_t i = 0; i < ADDRESS_SIZE; i++) {
    if (packet[i] != (0xCC ^ SCRAMBLE[i])) return false;
  }

  uint16_t crc = 0xB5D2;
  for (uint8_t i = 0; i < PACKET_SIZE - 2; i++) {
    crc = updateCrc(crc, packet[i]);
  }
  crc ^= 0x8B17;
  if (packet[PACKET_SIZE - 2] != (crc >> 8)) return false;
  if (packet[PACKET_SIZE - 1] != (crc & 0xFF)) return false;

  for (uint8_t i = 0; i < DATA_SIZE; i++) {
    data[i] = reverseBits(packet[ADDRESS_SIZE + i] ^ SCRAMBLE[ADDRESS_SIZE + i]);
  }
  return true;
}

void makeBindingPacket(const uint8_t radioId[ADDRESS_SIZE],
                       uint8_t packet[PACKET_SIZE]) {
  uint8_t data[DATA_SIZE] = {};
  data[0] = 0x10;
  memcpy(&data[1], radioId, ADDRESS_SIZE);
  encodePacket(data, packet);
}

uint8_t stickValue(int percent, uint8_t negativeRange, uint8_t positiveRange) {
  percent = constrain(percent, -100, 100);
  if (percent < 0) {
    return 0x80 - (uint8_t)((-percent * negativeRange) / 100);
  }
  return 0x80 + (uint8_t)((percent * positiveRange) / 100);
}

// The two check bytes. Add up byte 1 and bytes 4-12, plus 2: byte 13 is
// the low 8 bits of the total, byte 3 holds the bits above that (as 06,
// 0A or 0E). Fits every control packet recorded from the remote.
void addCheckBytes(uint8_t data[DATA_SIZE]) {
  uint16_t total = data[1] + 2;
  for (uint8_t i = 4; i <= 12; i++) {
    total += data[i];
  }
  data[13] = total & 0xFF;
  data[3] = ((total >> 8) << 2) | 0x02;
}

// After the drone answers, the remote sends this on 16/17: the four hop
// channels in bytes 4-7, byte 1 = 0x10, byte 12 = 0xA5. The drone's ID
// (bytes 4-5 of its answer) goes in bytes 0 and 2.
void makeHopListPacket(const uint8_t radioId[ADDRESS_SIZE],
                       const uint8_t droneId[2], const uint8_t hops[4],
                       uint8_t packet[PACKET_SIZE]) {
  uint8_t data[DATA_SIZE] = {};
  data[0] = droneId[0];
  data[1] = 0x10;
  data[2] = droneId[1];
  memcpy(&data[4], hops, 4);
  data[10] = 0x40;
  data[11] = 0x3F;
  data[12] = 0xA5;
  data[14] = radioId[1];
  data[15] = radioId[2];
  addCheckBytes(data);
  encodePacket(data, packet);
}

// channel: the NEXT hop channel, not the one this packet is sent on.
void makeControlPacket(const uint8_t radioId[ADDRESS_SIZE],
                       const uint8_t droneId[2], uint8_t channel,
                       int forward, int right, int turn, int up,
                       uint8_t switches, uint8_t command,
                       uint8_t packet[PACKET_SIZE]) {
  uint8_t data[DATA_SIZE] = {};
  data[0] = droneId[0];
  data[1] = channel;
  data[2] = droneId[1];
  data[4] = stickValue(up, 127, 127);
  uint8_t negativeRange = (switches & 0x01) ? 96 : 63;
  uint8_t positiveRange = (switches & 0x01) ? 96 : 65;
  data[5] = stickValue(turn, negativeRange, positiveRange);
  data[6] = stickValue(forward, negativeRange, positiveRange);
  data[7] = stickValue(right, negativeRange, positiveRange);

  data[10] = (switches & 0x02) ? 0x40 : 0x00;
  data[11] = 0x3F;
  data[12] = switches & 0x01;

  if (command == 1) {
    data[12] |= 0x0C;
  } else if (command == 3) {
    data[10] |= 0x80;
  } else if (command == 4) {
    data[6] = 0xFF;
  } else if (command == 5) {
    data[6] = 0x7F;
  } else if (command == 6) {
    data[7] = 0x7F;
  } else if (command == 7) {
    data[7] = 0xFF;
  }
  data[14] = radioId[1];
  data[15] = radioId[2];
  addCheckBytes(data);

  encodePacket(data, packet);
}

}  // namespace DroneRadio
