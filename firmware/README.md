# Fledge firmware

The ESP32 code, in Arduino C++. First time? Install the Arduino tools
and check your board with [SETUP.md](SETUP.md) steps 1-2.

## The relay: `Relay/`

Receives the phone's controls over Bluetooth, in the format described in
[PROTOCOL.md](../PROTOCOL.md). So far it only prints them: nothing is sent to the
drone yet. **It needs only the ESP32 on USB, no radio.**

To put it on the board, in the Arduino IDE: open `Relay/Relay.ino`, pick
**ESP32 Dev Module** and the `usbserial` port, and press Upload. Or from
the command line, in `firmware/`:

    arduino-cli compile --fqbn esp32:esp32:esp32 Relay
    arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-0001 Relay

To test it, open the Serial Monitor at **115200** and open Fledge on the
iPhone. You should see:

- The board's blue LED lights and `Phone connected` is printed. The app's
  top bar says `🔗 relay connected`.
- A line like `forward 20 right 0 turn 0 up 0` whenever the controls
  change (tap ✈️ Fly first: while stopped the app sends only zeros).
- A line for each button, e.g. `Command: take off or land`.
- Close the app: `Phone disconnected`, then `No phone: everything back to
  0`.

## Testing without an ESP32: `StandIn/`

Runs on the Mac and pretends to be the relay, so the app's Bluetooth can
be tried with no board at all:

    swift firmware/StandIn/StandIn.swift

Stop it (Ctrl-C) before using a real relay: the app connects to the
first relay it finds.

## Bench tools, for decoding the drone's remote

How the remote's radio was worked out. You need the radio wired up
([wiring](../README.md#wiring-the-radio)) for these.

- [SETUP.md](SETUP.md): Arduino tools, wiring, and checking the radio,
  step by step.
- `RadioCheck/`: checks the radio's wiring and scans for radio activity.
  Run it at the start of every bench session.
- `Sniffer/`: listens for the remote's packets without knowing its
  address, and reports the ones that repeat.
- [SNIFFING.md](SNIFFING.md): names for every control, and how a
  sniffing session runs.
- `coach/`: a terminal program that counts you through each step of a
  sniffing session, and the step lists it reads.
- `captures/` (not in git): the raw logs of every bench run, with an
  `INDEX.md` saying what each one was.
