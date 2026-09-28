# Sniffing the remote

What the remote sends for each control, found one control at a time.
Read this before a sniffing session and follow the steps exactly: the
point is that every packet can be matched to one known stick position or
button press, without guessing afterwards.

## What we already know

From the bench scans (`SETUP.md` step 5) and the drone's manual:

- **Pairing.** Drone on first: its light blinks slowly. Then remote on:
  its light blinks. Push the left stick full forward (the remote beeps),
  then full back (it beeps again). When both lights stay on, they are
  paired. While the remote is waiting to pair, it transmits on nRF24
  channels 16-17 (2416-2417 MHz) and keeps doing so. Once paired, the
  energy scan can no longer see it.
- **The left stick springs back to the middle** in both directions
  (checked on the real remote). Let go of it and the drone holds its
  height ("Hover" in the manual). So the left stick sets how fast to
  climb or sink, and holding it longer makes the drone go higher. That
  may explain why a short push gives "a little" and a long push "a lot".
  The hold test checks whether the remote's numbers change while a stick
  is held, or only the drone's response does.
- **The remote's radio is an XN297, or a copy of one, at 1 Mbps.** The
  nRF24 can read it. `Sniffer/` listened on channels 16-17 in every way
  it knows. With the remote off, nothing but noise. With the remote on
  (drone off), about 24 packets a second arrived on each channel, every
  one byte-for-byte the same, and only when listening for the XN297
  start pattern at 1 Mbps.
- **The pairing packet, decoded.** 24 bytes after the XN297 start
  pattern: a 5-byte address, a 16-byte payload, a 2-byte checksum, then
  one more byte that is always the same (not explained yet). Unscrambled
  with the XN297 scramble table, the address is `CC CC CC CC CC`, and the
  checksum matches (XN297 CRC-16, start value 0xB5D2, scrambled-mode
  xorout table), so the decoding is exact. The payload is `10`, then 5
  bytes that are probably this remote's own ID, then ten `00`s.
- **After pairing** (drone on, then remote on), the remote moves to
  channels **60, 63 and 65**, still at address `CC CC CC CC CC`, sending
  a 16-byte payload that passes the checksum. It left channels 16-17
  within about 8 seconds of being switched on, before the left stick
  forward-then-back. At rest the payload is:
  `ID 3F 70 0A 80 80 80 80 00 00 40 3F 00 C0 ID ID`, where the three
  `ID` bytes match part of the ID in the pairing packet.
  - `80 80 80 80`: probably the four stick directions, all centred
    (0x80 is half of 0xFF). The control session will tell.
  - Byte 1 is `3C`, `3F` or `41`: 60, 63 and 65 in decimal, the three
    channels. It is probably the next channel to hop to (60 -> 63 ->
    65 -> 60), but a packet can be heard on neighbouring channels too,
    so that is not proven yet.
  - Byte 13 changes with byte 1 (always byte 1 + 0x81 so far): probably
    a checksum, not yet worked out.
  - The energy scan could not see any of this because channels 56-68
    are busy with Wi-Fi here.
  - Weak packets 16 channels either side (44-49, 76-81) all fail the
    checksum: the nRF24 also hears strong signals 16 MHz away. They are
    not the drone talking back.
- **Not a protocol Multiprotocol already knows.** Its only protocol that
  pairs on `CC CC CC CC CC` is MT99xx, whose pairing payload is 9 bytes
  starting with `20`. So this one gets decoded here, step by step.
- The manual is shared by several models (it mentions "WIFI control",
  which this drone has no hardware for) and has no FCC ID. If no sticker
  turns up (remote battery cover, drone battery bay, box), open the remote
  and photograph the chip next to its antenna: its part number says which
  radio family it is. The sniffer's first run answers the same question:
  if it reads packets at all, the chip is nRF24-compatible.

## Names

Always use these names, never "throttle", "pitch" or "the flip thing".

**Sticks.** "Left stick" and "right stick". Directions as seen by the
person holding the remote: **forward** (away from you), **back** (towards
you), **left**, **right**.

| Stick | Forward | Back | Left | Right |
|-------|---------|------|------|-------|
| Left  | climb | sink | turn left | turn right |
| Right | fly forward | fly back | slide left | slide right |

**Positions.** Only ones that can be repeated by hand:

- **neutral**: hands off; the spring puts it there.
- **full**: pushed until it stops.
- **half**: to a pencil mark on the remote, drawn once and reused.

**Buttons.** By what you see, holding the remote normally: two on the
top edge, then below the sticks a row of an oval button, two small round
buttons and the power switch. (The manual numbers them in a diagram; the
numbers aren't on the remote, so they are only here for looking things
up in the manual.)

| Name | Manual # | Manual's name | Press | Hold ~3 s |
|------|----------|---------------|-------|-----------|
| **top-left button** | 1 | High/low speed | change speed | ? |
| **top-right button** 🤸 | 2 | One-key flips & rolls 360° | arm a flip; then push the right stick full in a direction | ? |
| **oval button** | 5 | One-key ascend/landing | take off / land | trim (see below) |
| **left small round button** | 6 | Light switch | lights (manual unclear) | lights off? |
| **right small round button** 🛑 | 7 | Emergency stop / correction | **stop motors** | level calibration (remote beeps, drone lights flicker ~3 s) |
| **power switch** | 9 | Power switch | on / off | |

**Trims** ("fine-tuning"): the manual says to long-press the oval button
"and then" push the right stick in the direction to correct. It doesn't
say whether the oval button stays held while pushing, or whether a long
press enters a trim mode. The trim steps try both, and the packets say
which works.
Count each push as one click.

**Rest** means: both sticks neutral, hands off the buttons, trims
untouched, speed as the remote starts up. Every test starts from rest and
returns to it.

## Before each session

1. **Props off**, said out loud. The oval button and the left stick both
   spin the motors once paired.
2. Drone battery charged, 3 fresh AAAs in the remote.
3. RadioCheck passes (`SETUP.md` step 4).
4. The ESP32 running `Sniffer` plugged in, with its channels set to 65
   to 65 for a control session. The coach records it by itself.

The motors will probably spin during some steps (left stick forward,
the oval button). With props off that is expected: carry on. The right
small round button, pressed once, stops them at any time.

## Running a session

`coach/coach.py` reads out each step with a countdown, like this. At
the same time it records everything the sniffer hears, and after each
step it says whether the remote's signal changed:

    Right stick ⬆️ full forward, for 5 seconds, in
    5 4 3 2 1 (one per line)
    Go!
    5 4 3 2 1
    ✅ The remote's signal changed      (or ❌ No change seen)
    Let go ✋
    3 2 1

Run it in a terminal, in `firmware/coach/`:

    python3 coach.py pairing.txt      # first, about 2.5 minutes
    python3 coach.py controls.txt     # then, about 7 (5 with --quick)

Before step 1 it listens for 5 seconds and says whether it can hear the
remote; if the sniffer isn't plugged in it doesn't start. Add `--say` to
hear each line read aloud too, `--quick` for short countdowns (2 seconds
to get ready, 1 second between steps), or `--no-sniffer` to run the
countdown alone. Press **r** if a step went wrong, or showed ❌: it is
marked bad in the log and done again. Press **q** to stop.

"Changed" means a packet arrived at least 3 times during the step that
never arrived in the countdown just before it. The coach doesn't know
which byte means what; that's for the paper table afterwards.

Two logs go in `firmware/captures/`, which git ignores: the steps (.csv,
with a remote_changed column) and everything the sniffer printed
(-sniffer.log), both timed by the Mac's clock.

The steps are in `pairing.txt` and `controls.txt`: one per line, the
number of seconds, then what to do. Change them there, not here.

**Pairing first**, on its own, starting with the remote and drone off:
the pairing packets probably carry the remote's ID and maybe its list of
channels. `controls.txt` starts where it ends, with the two paired.

**Then the controls**, in this order:

- **Rest, 8 s**: the baseline. A number that changes when nobody
  touches anything is probably a counter or checksum, not a control.
- **Each stick direction, full**: which number moves, and its two ends.
- **Half** (to a pencil mark): if full, half and neutral fall on a
  straight line, every position in between follows from them.
- **Diagonal**, right stick into the corner: if the packet shows the
  full-forward number and the full-right number together, forward and
  right are two separate numbers, and any angle is just a mix of the two.
- **Hold 8 s, and a quick tap**: does the remote's number grow while a
  stick is held (the remote ramps), or stay the same (the drone ramps)?
- **Buttons**, one at a time, then **trims** (both ways the manual might
  mean, each undone afterwards).
- **Last, the oval button then the right small round button**, because
  they start and stop the motors.

## Afterwards

For each step, write the result in a table on paper: which byte changed,
its value at rest, and its value in the step.
