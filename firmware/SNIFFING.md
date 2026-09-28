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
  Test 13 checks whether the remote's numbers change while a stick is
  held, or only the drone's response does.
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

**Buttons.** By the number in the manual's "Parts of remote control"
picture. Put a small numbered sticker on each one before the first
session. Positions below are as the manual draws them; check each one on
the real remote and fix this table if it differs.

| # | Where (remote held normally) | Manual's name | Press | Hold ~3 s |
|---|------------------------------|---------------|-------|-----------|
| 1 | top edge, left (shoulder) | High/low speed | change speed | ? |
| 2 | top edge, right (shoulder) | One-key flips & rolls 360° | arm a flip; then push the right stick full in a direction | ? |
| 5 | front row, left oval | One-key ascend/landing | take off / land | trim mode (see below) |
| 6 | front row, first round | Light switch | lights (manual unclear) | lights off? |
| 7 | front row, second round | Emergency stop / correction | **stop motors** | level calibration (remote beeps, drone lights flicker ~3 s) |
| 8 | front row, small dot | Indicator light | (not a button) | |
| 9 | front row, right oval | Power switch | on / off | |

**Trims** ("fine-tuning"): the manual says to long-press button 5 "and
then" push the right stick in the direction to correct. It doesn't say
whether button 5 stays held while pushing, or whether a long press enters
a trim mode. Test 19 tries both, and the packets say which one works.
Count each push as one click.

**Rest** means: both sticks neutral, hands off the buttons, trims
untouched, speed as the remote starts up. Every test starts from rest and
returns to it.

## Before each session

1. **Props off**, said out loud. Button 5 and the left stick both spin
   the motors once paired.
2. Drone battery charged, 3 fresh AAAs in the remote.
3. RadioCheck passes (`SETUP.md` step 4).
4. Remote and drone paired as above, then **5 seconds at rest**.

The motors will probably spin during some steps (left stick forward,
button 5). With props off that is expected: carry on. Button 7, pressed
once, stops them at any time.

## First: the pairing packets

Before the control tests, record pairing on its own, because the pairing
packets probably carry the remote's ID and maybe its channel list.

| # | Step |
|---|------|
| P1 | Drone off. Remote on, sticks untouched, 30 s. (The remote stays on channels 16-17, waiting to pair.) Then remote off. |
| P2 | The manual's order: drone on, wait until its light blinks slowly. Remote on. Left stick full forward (beep), left stick full back (beep). Wait until both lights stay on, then 10 s at rest. |

## The tests

Hold each step for **5 seconds**, then **5 seconds at rest**, unless the
step says otherwise. Change only the one thing the step names. If a step
goes wrong (wrong button, stick slipped), say so, go back to rest, and
repeat that step; don't carry on as if it worked.

**Sticks, full** (which number moves, and its two ends)

| # | Step |
|---|------|
| 1 | Rest, 10 s: the baseline. Note which numbers change by themselves. |
| 2 | Left stick full forward |
| 3 | Left stick full back |
| 4 | Left stick full left |
| 5 | Left stick full right |
| 6 | Right stick full forward |
| 7 | Right stick full back |
| 8 | Right stick full left |
| 9 | Right stick full right |

**Sticks, half** (is the scale a straight line?)

| # | Step |
|---|------|
| 10 | Left stick half forward |
| 11 | Right stick half forward |
| 12 | Right stick half right |

**Timing** (does the remote's number ramp while held?)

| # | Step |
|---|------|
| 13 | Right stick full forward, **held 10 s** |
| 14 | Left stick full forward, **held 10 s** |
| 15 | Right stick full forward, **quick tap** (under half a second) |

**Buttons** (one press each, then rest; drone lights and remote beeps
noted as they happen)

| # | Step |
|---|------|
| 16 | Button 1, press once. Then press again until it is back where it started, counting presses and beeps: that is the number of speeds. |
| 17 | Button 6, press once. Then button 6, hold 3 s. Then put the lights back as they were. |
| 18 | Button 2, press once, then right stick full forward (the flip command; on the bench the drone will not flip). |
| 19a | Trim, try 1: hold button 5 down, and while holding it, right stick full right 3 times. Let go of button 5. Note any beeps. |
| 19b | Trim, try 2: hold button 5 for 3 s and let go. Then right stick full right 3 times. Note any beeps. |
| 19c | Undo whichever try changed a byte: the same method, right stick full left, until that byte is back to its rest value. |
| 20 | Button 7, hold 3 s (level calibration). Drone must sit level. |
| 21 | Button 5, press once (take off: motors spin, props off). |
| 22 | Button 7, press once (emergency stop: motors stop). |

Button 7's press is last because it stops the motors, and button 5
before it because it starts them.

## Afterwards

For each test, write the result in a table on paper: which byte changed,
its value at rest, and its value in the step. A byte that changed in
test 1 (by itself) is probably a counter or checksum, not a control.
