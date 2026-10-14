# Drone actions: what each control sends

What the Sniffer found: every control on the FunPX SQN-053's remote and
the packet it sends. `DronePackets.h` is the same table, as code. The
controls by name, and how a sniffing session runs, are in
[SNIFFING.md](../SNIFFING.md).

⚠️ **Spoiler** if you want to find these yourself: try a control on the
remote, then find the row that changed.

Byte numbers are payload bytes 0-15, after XN297 unwrapping. "While
held" means the remote sends the value only as long as the control is
held, then goes back. Measured from the remote on 2026-09-28 (coach
sessions 142445, 150146, 152100, 153940) unless it says otherwise.

## Sticks

They spring back to the middle (`80`). In slow mode the remote stops
them at `41`-`C1`, in fast mode at `20`-`E0`; the left stick's up/down
always goes the whole way, `01`-`FF`. Values in between are
proportional.

| Control | Does | App / Mac sends | Byte | Values |
|---|---|---|---|---|
| left stick up/down | climb / sink; middle = hold height | `up` -100..100 | 4 | `01` full down, `80` middle, `FF` full up |
| left stick left/right | spin on the spot | `turn` | 5 | `41` full left, `C1` full right (slow) |
| right stick up/down | fly forward / back | `forward` | 6 | `C1` full forward, `41` full back (slow) |
| right stick left/right | fly sideways | `right` | 7 | `41` full left, `C1` full right (slow) |

## Buttons

| Control (by sight) | Does | App / Mac sends | Relay holds it | Bytes |
|---|---|---|---|---|
| top-left, press | speed: toggles fast / slow | switch `fast` | all the time, as a state | byte 12 `01` = fast; the stick ranges widen |
| top-right 🤸, press | arms a flip (remote beeps) | — | — | nothing: the arming stays in the remote |
| … then right stick, full | flips that way | command 4-7 | 250 ms | right stick pushed past its end: byte 6 `FF` forward, `7F` back; byte 7 `FF` right, `7F` left; ~0.2-0.3 s, then the normal value |
| oval, press | take off; press again: land | command 1 | 400 ms | byte 12 `0C` while held (~0.36 s for a press). Take off and land confirmed from the relay 2026-10-14 |
| oval, held 3 s / trims | trim, per the manual | — | — | nothing new: `0C` while held, the right stick as normal. Trims are not sent as numbers (bytes 8, 9 never change) |
| left small round, press | ? (manual: lights) | — | — | byte 12 `20` while held. Not used by the relay |
| left small round, held ~1 s | lights off? | switch `lights` | all the time, as a state | byte 10 `40` = lights on; it went off after ~1 s held |
| right small round, press | stop motors (manual) | command 2 | **can't**: throttle fully down for 3 s instead | **nothing seen**, in two sessions |
| right small round, held 3 s | level calibration | command 3 | 600 ms | byte 10 `80` for ~0.6 s |
| power switch | on / off | — | — | — |

## The other bytes

| Byte | Means |
|---|---|
| 0, 2 | the drone's ID (bytes 4-5 of its pairing answer) |
| 1 | the channel of the **next** packet |
| 3, 13 | the checksum: total = byte 1 + bytes 4-12 + 2; byte 13 = total & `FF`; byte 3 = (total >> 8) × 4 + 2. Fits all 1181 recorded packets |
| 8, 9 | `00` always (trims?) |
| 11 | `3F` always |
| 14, 15 | our remote's ID, bytes 1-2 |

## Pairing

| Packet | From | Channels | Bytes |
|---|---|---|---|
| pairing request | remote | 16, 17 in turn | `10`, then the remote's 5-byte ID, then zeros |
| answer | drone | 16 or 17 | `10 BB`, the remote's ID bytes 1-2, the drone's 2-byte ID, then `00 00 00 CC DD EE 00 00 00 00` |
| hop list | remote | 16, 17 in turn, ~160 ms | like a control packet, but byte 1 = `10`, bytes 4-7 = the hop channels (`3F 41 41 3C` = 63, 65, 65, 60), byte 12 = `A5` |
| controls | remote | 63, 65, 65, 60, one every 7.57 ms | the tables above |

## Every action as a whole packet

Built by the relay's own code (`DronePackets.h`), full stick, sent on
channel 63 (so byte 1 = `41`, the next channel). Compare each row with
`rest`: the bytes that change are the action. Three kinds of byte change
in every row whatever the action, so look past them:

- bytes 0, 2 and 14, 15: the drone's and the remote's IDs (never change)
- byte 1: the next channel (`41`, `3C` or `3F`, packet by packet)
- bytes 3 and 13: the checksum, which follows everything else

```
pair           = [10 AA E2 BD 5A 40 00 00 00 00 00 00 00 00 00 00]   # the remote asks, on 16/17
drone_answers  = [10 BB E2 BD BD 70 00 00 00 CC DD EE 00 00 00 00]   # the drone replies
hop_list       = [BD 10 70 0A 3F 41 41 3C 00 00 40 3F A5 33 E2 BD]   # then this, on 16/17
rest           = [BD 41 70 0A 80 80 80 80 00 00 40 3F 00 C2 E2 BD]   # hands off: compare every row below with this one
take_off       = [BD 41 70 0A 80 80 80 80 00 00 40 3F 0C CE E2 BD]   # while the oval is held
land           = [BD 41 70 0A 80 80 80 80 00 00 40 3F 0C CE E2 BD]   # the same packet: the oval is a toggle
up             = [BD 41 70 0E FF 80 80 80 00 00 40 3F 00 41 E2 BD]   # half: BF
down           = [BD 41 70 0A 01 80 80 80 00 00 40 3F 00 43 E2 BD]   # half: 41
turn_left      = [BD 41 70 0A 80 41 80 80 00 00 40 3F 00 83 E2 BD]   # half: 61
turn_right     = [BD 41 70 0E 80 C1 80 80 00 00 40 3F 00 03 E2 BD]   # half: A0
forward        = [BD 41 70 0E 80 80 C1 80 00 00 40 3F 00 03 E2 BD]   # half: A0
back           = [BD 41 70 0A 80 80 41 80 00 00 40 3F 00 83 E2 BD]   # half: 61
tilt_left      = [BD 41 70 0A 80 80 80 41 00 00 40 3F 00 83 E2 BD]   # half: 61
tilt_right     = [BD 41 70 0E 80 80 80 C1 00 00 40 3F 00 03 E2 BD]   # half: A0
fast_mode      = [BD 41 70 0A 80 80 80 80 00 00 40 3F 01 C3 E2 BD]   # and the sticks go further: 20..E0
lights_off     = [BD 41 70 0A 80 80 80 80 00 00 00 3F 00 82 E2 BD]
calibrate      = [BD 41 70 0E 80 80 80 80 00 00 C0 3F 00 42 E2 BD]   # for ~0.6 s
flip_forward   = [BD 41 70 0E 80 80 FF 80 00 00 40 3F 00 41 E2 BD]   # for ~0.25 s
flip_back      = [BD 41 70 0A 80 80 7F 80 00 00 40 3F 00 C1 E2 BD]   # for ~0.25 s
flip_left      = [BD 41 70 0A 80 80 80 7F 00 00 40 3F 00 C1 E2 BD]   # for ~0.25 s
flip_right     = [BD 41 70 0E 80 80 80 FF 00 00 40 3F 00 41 E2 BD]   # for ~0.25 s
```

## Still unknown

- The stop button's signal (nothing seen when pressed).
- What the left small round button's press (byte 12 `20`) does.
- Whether the drone lands by itself when the controls stop, or with the
  throttle held fully down (the relay's failsafe assumes it does).
- Whether another remote would use other hop channels.
