# Fledge firmware

The ESP32 code, in Arduino C++. First time? Install the Arduino tools
and check your board with [SETUP.md](SETUP.md) steps 1-2.

## The relay: `Relay/`

Takes controls from the phone (Bluetooth) or the Mac (the USB cable), in
the format described in [PROTOCOL.md](../PROTOCOL.md), and flies the
drone with them through the nRF24, pretending to be the drone's own
remote. The radio must be wired as shown in the
[main README](../README.md#wiring-the-radio).

To put it on the board, in the Arduino IDE: open `Relay/Relay.ino`, pick
**ESP32 Dev Module** and the `usbserial` port, and press Upload. Or from
the command line, in `firmware/`:

    arduino-cli compile --fqbn esp32:esp32:esp32 Relay
    arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-0001 Relay

Each file does one job; the list is at the top of `Relay.ino`.
`DronePackets.h` holds what each byte of a drone packet means, for the
FunPX SQN-053; [DroneActions.md](Relay/DroneActions.md) is the same, as a
table. For a different drone, those are the files to redo (see
[GUIDE.md](../GUIDE.md#a-different-drone)).

**Pairing:** type `p` in the Serial Monitor (115200 baud), tap **Pair**
in the app, or run a program in `fly/`; then switch the drone on within
30 seconds. The relay prints `The drone answered`, then `Linked`, and
stays linked until it is reset.

**If the controls stop** for half a second (phone gone, Mac program
ended, cable out), the relay centres the sticks and holds the throttle
fully down. It never presses take-off/land by itself: that button is a
toggle, so pressing it on the ground would take off. *Not yet tested in
flight.* The app's 🛑 **Motors off** does the same for 3 seconds,
because the drone's own motors-off signal hasn't been found.

## Flying from the Mac: `fly/`

Python programs that fly the drone through the relay on the USB cable
(close any Serial Monitor first: only one program can use the port).
Every run's log goes in `captures/`.

    cd firmware/fly
    python3 flight_plan.py      # does the moves in flight_plan.py
    python3 fly_with_keys.py    # fly with the keyboard

- `flight_plan.py`: a list of moves, one per line, e.g.
  `drone.turn_right(180)`. Rearrange them and run it again.
- `fly_with_keys.py`: each key press pushes for a moment. Space lands,
  `q` lands and quits.
- `tuning.py`: how hard each move pushes, how fast the drone turns, and
  which key does what.
- `drone.py`: the moves themselves. If a program ends while the drone is
  flying, it lands first.

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
  address, and reports the ones that repeat. Its optional protocol capture
  mode records packets from the binding and known control channels; see
  [SNIFFING.md](SNIFFING.md#recording-the-binding-and-hop-channels).
- [SNIFFING.md](SNIFFING.md): names for every control, and how a
  sniffing session runs.
- `coach/`: a terminal program that counts you through each step of a
  sniffing session, and the step lists it reads.
- `captures/` (not in git): the raw logs of every bench run, with an
  `INDEX.md` saying what each one was.
