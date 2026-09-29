# Setting up the sniffer bench

Steps to get from a new Mac to an ESP32 & nRF24L01+ that can listen to the
drone's remote. Tick each one off as it works.

## 1. Install the Arduino software ✅

    brew install --cask arduino-ide     # the app, with Serial Monitor and Plotter
    brew install arduino-cli            # the same thing from the command line

Then add ESP32 support and the nRF24 library (the app shares these):

    arduino-cli config add board_manager.additional_urls \
      https://espressif.github.io/arduino-esp32/package_esp32_index.json
    arduino-cli core update-index
    arduino-cli core install esp32:esp32
    arduino-cli lib install RF24

Board setting in the app: **ESP32 Dev Module**.

**Apple-chip Macs:** Arduino's `ctags` helper is Intel-only, so compiling
fails with "bad CPU type in executable" until Rosetta is installed:

    softwareupdate --install-rosetta --agree-to-license

## 2. Check each ESP32 talks to the Mac ✅

Plug it in with a USB-C cable that carries data (charge-only cables light
the board but the Mac never sees it). A new port such as
`/dev/cu.usbserial-0001` should appear:

    ls /dev/cu.*

## 3. Wire the nRF24L01+ to the ESP32 ✅

Use female-to-male jumper wires; the nRF24's 2x4 pins don't fit a breadboard.

| nRF24 | Wire   | ESP32 |
|-------|--------|-------|
| GND   | black  | GND (− rail) |
| VCC   | red    | 3V3 (+ rail; never 5V or VIN) |
| CE    | yellow | GPIO 4 (j26) |
| CSN   | orange | GPIO 5 (j23) |
| SCK   | green  | GPIO 18 (j22) |
| MOSI  | blue   | GPIO 23 (j16) |
| MISO  | purple | GPIO 19 (j21) |
| IRQ   | none   | not connected |

The ESP32's own two jumpers to the rails: **red** from j30 (3V3) to +,
**black** from j29 (GND) to −.

Put a 10 µF capacitor across VCC and GND, close to the nRF24. If it has a
stripe, that leg goes to GND.

### Exact layout: 30-pin ESP32 on a half-size (30-column) breadboard

This board is 1 inch wide, so it leaves one free hole per pin on one side
only (row j). Every pin the radio needs is on the `3V3 GND D15 … D23` row.

- ESP32 label side up, USB to the right, its `D23 … 3V3` row in row i,
  columns 16-30 (D23 in i16, 3V3 in i30). The other row lands in row a.
- j30 (3V3) to the top + rail (the rail row next to row j).
- j29 (GND) to the top − rail (the outer row, blue line).
- 10 µF capacitor across the top rails near column 5, stripe on −.
- Radio: VCC to + and GND to − near the capacitor, CE to j26, CSN to j23,
  SCK to j22, MISO to j21, MOSI to j16.

![Wiring diagram: the nRF24L01+ radio on seven jumper wires to the ESP32 on a half-size breadboard](wiring.svg)

Seen from above, the radio chip side up with its antenna on the left. Its
pins stick out of the back, where left and right are swapped, so keep it
chip side up and push each wire on from behind. **GND is the pin with its
own printed box.** Use the colours in the table and the drawing: this
exact layout was built and worked first time, and matching colours make
each wire easy to trace from pin to hole.

Leave the kit's breadboard power supply module off: the ESP32 already
powers the rails, and the module can put 5 V on them.

**Check before plugging in USB.** The red and black wires are the ones
that can destroy the radio:

- Radio VCC goes to the 3V3 rail: not 5V, not VIN.
- Black on the radio's boxed pin (GND), red on the pin beside it (VCC).
- The ESP32's 3V3 pin feeds the + rail, not its 5V or VIN pin, and
  nothing is in the VIN column.
- Capacitor stripe on the − rail: a backwards electrolytic capacitor can
  pop.
- Each signal wire is in row j of the right column: one column off puts a
  signal on the wrong pin.
- The radio isn't lying on anything metal, and no bare wire ends touch.

## 4. Check the radio with RadioCheck ✅

`firmware/RadioCheck/RadioCheck.ino` checks the wiring, then scans every
channel for radio activity. Run it at the start of each bench session: it
answers "is the radio alive?" before anything harder goes wrong.

In the Arduino IDE: open `RadioCheck.ino`, pick **ESP32 Dev Module** and the
`usbserial` port, press Upload, then open the Serial Monitor at **115200**.
Or from the command line, in `firmware/`:

    arduino-cli compile --fqbn esp32:esp32:esp32 RadioCheck
    arduino-cli upload --fqbn esp32:esp32:esp32 -p /dev/cu.usbserial-0001 RadioCheck
    arduino-cli monitor -p /dev/cu.usbserial-0001 --config baudrate=115200

It repeats forever: a wiring report, then six scan lines about 8 seconds
apart, so a full round takes about a minute. If you open the monitor late,
wait for the next `=== RadioCheck ===`.

**Wiring is good when** it says `isChipConnected(): yes`, every read-back
line says `ok`, and `SPI read-back: ALL OK`. `NO`, `MISMATCH` or read-backs
of 0 or 255 mean a loose or swapped wire (check MISO, SCK, MOSI, CSN) or no
3.3 V power.

**The receiver is good when** the scan shows known signals. Each scan line
has one character per channel, 0 on the left to 125 on the right (channel N
is 2400 + N MHz). `-` means nothing was heard; `1` to `f` mean it was heard
a little to a lot. Bluetooth always advertises on channels 2, 26 and 80,
and a Wi-Fi network shows as a block about 20 channels wide. A scan of all
`-` means the receiver is not working, even if the wiring check passed.

A few bytes of junk when the monitor first connects are normal.

The scan only hears that a channel is busy. It does not read packets.

## 5. Find the remote's channels

1. Remote **off**: save a minute of scan lines. This is the baseline. On
   the bench so far: Bluetooth at 2, 26 and 80, and a Wi-Fi block at about
   56-68.
2. Remote **on**: save a minute of scan lines, including the first few
   seconds after switching it on. Many toy remotes send "bind" packets on
   one channel at power-on, then hop across several.
3. Channels busy only in the second scan are the remote's. Write them down.

Busy channels do not prove the remote's chip is nRF24-compatible: any
2.4 GHz radio shows up here. Reading its packets is the next step.

**Result (drone off, remote on, sticks untouched):** channels **16 and 17**
(2416-2417 MHz), with a little spill onto 14-15. They were quiet in 68
scan lines before, busy in 11-13 of the 14 lines while the remote was on,
and quiet again once it was switched off. Channels 41, 46, 47 and 71 got
a few faint hits while it was on: possibly hopping, possibly noise.
With the drone off, the remote is probably still trying to pair, so the
channels may change once the drone is on.

**Props off** for every bench test with the drone switched on: a paired
drone spins up as soon as the throttle moves.

**Result (drone on, then remote on):** the drone alone showed nothing
above the baseline. About 10 seconds after the remote came on, 16-17 went
quiet and stayed quiet, apart from one burst across about 11 scattered
channels. After that the remote looked no different from the remote being
off. It had paired: the motors ran when the throttle went up. So the
remote was transmitting the whole time, and this scan could not see it.
The sniffer later showed why: once paired it uses channels 60-66, right
under the Wi-Fi block. Reading the packets is in `SNIFFING.md`.
