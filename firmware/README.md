# Fledge firmware

The ESP32 code goes here, in Arduino C++. It receives commands from the
iPhone over Bluetooth and sends them to the drone through the nRF24L01+
radio.

The relay is not written yet. For now:

- `SETUP.md`: setting up the bench, from a new Mac to a working radio.
- `RadioCheck/`: checks the nRF24 wiring and scans for radio activity.
- `Sniffer/`: listens for the remote's packets without knowing its
  address, and reports the ones that repeat.
- `SNIFFING.md`: names for every control, and how a sniffing session
  runs.
- `coach/`: a terminal program that counts you through each step of a
  session, and the step lists it reads.
- `captures/` (not in git): the raw logs of every bench run, with an
  `INDEX.md` saying what each one was.
