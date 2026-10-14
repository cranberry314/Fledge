// XN297: how the drone's radio chip wraps every packet.
//
// The drone and its remote use XN297 radio chips. The nRF24 can pretend to
// be one if we scramble the bytes and add the checksum ourselves, the way
// the XN297 would. This file does only that wrapping; what the 16 bytes
// inside mean is DronePackets.h's job.

#pragma once

#include <Arduino.h>

namespace XN297 {

const int PAYLOAD_SIZE = 16;  // the bytes that mean something
const int PACKET_SIZE = 23;   // 5 address + 16 payload + 2 checksum

// Give these to the nRF24 to sound like an XN297. Every XN297 packet
// starts with the same pattern on air; these addresses make the nRF24 send
// it (SEND_ADDRESS) or listen for it (LISTEN_ADDRESS).
extern const uint8_t SEND_ADDRESS[5];
extern const uint8_t LISTEN_ADDRESS[3];

// Wraps 16 payload bytes into a 23-byte packet for the drone's address.
void encode(const uint8_t payload[PAYLOAD_SIZE], uint8_t packet[PACKET_SIZE]);

// Unwraps a packet. Returns false for noise: wrong address or a failed
// checksum.
bool decode(const uint8_t packet[PACKET_SIZE], uint8_t payload[PAYLOAD_SIZE]);

}  // namespace XN297
