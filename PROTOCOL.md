# Fledge Bluetooth protocol

How the phone app talks to the relay (the ESP32). Both sides follow this
file; change it here first, then change both sides to match.

## The link

- The relay is a Bluetooth LE peripheral named `Fledge Relay`.
- It advertises one service:
  `6E1D0001-3C5A-4B8E-9F21-7A4D2C8B5E10`
- The service has one characteristic, **controls**, which the phone
  writes without response:
  `6E1D0002-3C5A-4B8E-9F21-7A4D2C8B5E10`
- The phone scans for the service, connects to the first relay it finds,
  and reconnects by itself if the link drops.

## The controls message

The phone sends one message 20 times a second (every 50 ms), even when
nothing changes. Each message is 8 bytes:

| Byte | Name     | Values                                            |
|------|----------|---------------------------------------------------|
| 0    | version  | always 2                                          |
| 1    | forward  | -100 (full back) to 100 (full forward)            |
| 2    | right    | -100 (full left) to 100 (full right)              |
| 3    | turn     | -100 (spin left) to 100 (spin right), 0 = no spin |
| 4    | up       | -100 (down) to 100 (up), 0 = hold height          |
| 5    | switches | bit 0 (1): fast speed; bit 1 (2): lights on       |
| 6    | command  | the last one-off command (see below)              |
| 7    | count    | how many commands so far, 0 to 255, then 0 again  |

Bytes 1 to 4 are signed (two's complement): -100 is `0x9C`, 100 is `0x64`.
Each one means "how far to push that stick", as a percentage. While the
app is on ✋ Stop, they are all 0.

**Switches** say how things should be now: the relay makes the drone
match them. The app starts with slow speed and lights on, as the remote
does.

**Commands** happen once. The phone keeps sending the last command in
every message, so a lost message doesn't lose it; the relay does the
command when `count` changes. The first message after connecting only
sets the count: nothing happens.

| Command | Meaning                                      |
|---------|----------------------------------------------|
| 0       | none yet                                     |
| 1       | take off or land (the oval button)           |
| 2       | motors off: emergency stop, the drone falls   |
| 3       | level calibration (only sent while stopped)  |
| 4       | flip forward                                 |
| 5       | flip back                                    |
| 6       | flip left                                    |
| 7       | flip right                                   |

## If the messages stop

If the phone disconnects, or the relay gets no message for 500 ms, it
acts as if bytes 1 to 4 were 0. A lost link must never leave the drone
flying on the last command.
