# Fledge firmware

The ESP32 code goes here, in Arduino C++. It receives commands from the
iPhone over Bluetooth and sends them to the drone through the nRF24L01+
radio.

- `Relay/`: the relay. So far only the Bluetooth side: it receives the
  phone's controls (see `PROTOCOL.md` at the top of the repo) and prints
  them on the Serial Monitor at 115200 baud. Nothing goes to the drone.

Bench tools:

- `SETUP.md`: setting up the bench, from a new Mac to a working radio.
- `RadioCheck/`: checks the nRF24 wiring and scans for radio activity.
- `Sniffer/`: listens for the remote's packets without knowing its
  address, and reports the ones that repeat.
- `SNIFFING.md`: names for every control, and how a sniffing session
  runs.
- `coach/`: a terminal program that counts you through each step of a
  session, and the step lists it reads.
- `StandIn/`: runs on the Mac and pretends to be the relay, so the
  phone app's Bluetooth can be tested without an ESP32:
  `swift firmware/StandIn/StandIn.swift`. The messages are described
  in `PROTOCOL.md` at the top of the repo.
- `captures/` (not in git): the raw logs of every bench run, with an
  `INDEX.md` saying what each one was.
