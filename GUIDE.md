# Building Fledge with a kid

A guide for a grown-up and a young kid (roughly 6 to 9 years old) who
want to hack a toy drone together: find out what its remote
says, then fly it from a Mac or an iPhone through a homemade relay. The
drone itself is never opened or changed.

The grown-up does the wiring, the typing and anything fiddly. The kid
makes the real decisions: what to try next, which colour, which emoji,
what order the moves go in, and what the numbers should be. Nothing here
needs the kid to read yet.

The technical steps are in other files, linked from each stage below.
This guide is the order to do them in, and how to make each one the
kid's.

**Contents:** [Before you start](#before-you-start) ·
[Safety](#safety) · [How to work with a young kid](#how-to-work-with-a-young-kid) ·
[The stages](#the-stages) · [A different drone](#a-different-drone) ·
[Your own app](#your-own-app-android-or-anything-else) ·
[Windows or Linux](#windows-or-linux) · [What isn't finished](#what-isnt-finished)

## Before you start

**What it was built with:** a FunPX SQN-053 toy drone and its remote, a
Mac, an iPhone 13 on iOS 26, and a few cheap parts (ESP32 boards,
nRF24L01+ radios, a breadboard kit). The exact parts and where to buy
them: [README.md, Buying the parts](README.md#buying-the-parts).

- **Same drone, Mac and iPhone:** follow this guide as it is.
- **Mac, no iPhone:** skip stages 1, 2 and 9; fly from the Mac.
- **A different drone:** the stages still work, but the sniffing
  (stage 5) becomes the real thing; see [A different drone](#a-different-drone).

**Time:** plan on short sessions over several weeks, not a weekend.
Each stage below is one to three sessions of 15-30 minutes with the kid,
plus some grown-up time beforehand.

## Safety

- **Props off, said out loud, before every step with the drone switched
  on.** The motors can spin whenever the drone is paired, even when you
  didn't expect it. Saying it out loud makes it a habit for both of you.
  Put the props back only for real flights.
- The SQN-053 has a plastic prop cage, but it still catches **hair**: tie
  long hair back, and keep faces away while it's powered.
- Fly indoors in a clear room, low, and know where the drone's power
  switch is. Full climb for two seconds reaches a normal ceiling.
- **The relay's "lost controls" behaviour isn't tested in flight yet**
  (see [What isn't finished](#what-isnt-finished)). Until it is, the
  power switch is the stop button.
- Wiring: everything runs on USB power at 3.3 V. The only parts that can
  be damaged are the radio (if it gets 5 V) and the capacitor (if it goes
  in backwards); the wiring checklist in the
  [README](README.md#check-before-plugging-in-usb) covers both.
- Charge the drone's battery before each flying session.

## How to work with a young kid

Suggestions, checked against what's known about teaching programming
to young children (sources at the end). The evidence is thinner than you
might hope, so treat them as advice, not rules.

**Rehearse first, then do it with them.** Do each stage yourself first,
alone, so you find the problems before they do. Then put the code back
the way it was, so they make the changes fresh:

    git tag before-colours          # before you rehearse
    # ...rehearse, find the problems...
    git tag rehearsed-colours       # the working result: a fallback
    git checkout before-colours -- ios/Fledge/Tuning.swift   # reset for their turn

If their run goes badly, swap in your rehearsed result quietly, then put
their own choices back in by hand, so the result is still theirs. Keep
the tags local (don't push them).

**Keep your answers out of sight.** If you rehearsed the sniffing, don't
leave your notes on the table. The fun is finding it.

**Predict, run, look, change.** Before running anything, ask "what do
you think will happen?" Run it. Look at what actually happened, and
talk about the difference. Then change one thing and go round again.
Starting from working code and changing it is easier for beginners than
starting from nothing (the "PRIMM" approach used in schools).

**Short sessions.** Plan 10-15 minutes of one thing, then move or
switch to something else. Stop while it's still fun. A young kid's
attention is short, and one success is better than a long struggle.

**Code they can follow before they can read.** Every setting is one
line with an emoji comment above it, so they can find "the 🐣 line" and
change the number under it:

```swift
// 🐣 the bird in the compass: it points the way you're facing
let compassBird = "🐣"
```

The same idea runs through `Tuning.swift` (the app) and
`firmware/fly/tuning.py` (flying from the Mac). Read the comments aloud
the first time; after that they'll find their own way around. The
session coach has a `--say` option to read its steps aloud (built, but
not yet heard working).

**Let them be the hands, and you be the typist.** They press the
buttons, push the sticks on the countdown, switch the drone on at the
right moment, and decide what changes. You type what they decide.

**Getting stuck is the job.** When something doesn't work, that's
debugging, and it's what programmers spend most of their time doing.
Try "what did you expect? what did it do instead?" before giving the
answer. Praise what they did ("you checked every wire", "you tried a
different number"), not what they are ("you're so clever").

**Warm up without the computer.** Before writing a flight plan, play
"drone": they give you moves ("up! turn right! land!") and you act
them out, exactly as said, including the silly results of a missed
step. Then the flight plan is the same game, written down.

**Make it theirs.** Their name for it, their emoji, their colours. Let
them show someone else what they made, and explain it.

**Later: a planted bug.** Once they know how something should behave,
you can break one setting on purpose (a turn that goes the wrong way, a
climb that's far too weak) and let them find and fix it. Keep it fair:
one bug, in a file they know.

## The stages

Each stage: what the grown-up does beforehand, what the kid does, and
where the technical steps are.

### 1. The app on the phone (no hardware)

- **Grown-up:** build the iPhone app:
  [README.md, Building the iPhone app](README.md#building-the-iphone-app).
  A free Apple account works, but then the app stops opening after 7
  days and needs building again.
- **Kid:** change colours and emoji in `ios/Fledge/Tuning.swift`, then
  build it onto the phone and see the change. To see changes without
  the phone, open `ContentView.swift`, show Xcode's Canvas (Editor →
  Canvas), pin the preview with the pin button, then go back to
  `Tuning.swift`: the pinned preview should follow your edits. (The
  previews exist; whether they show the hardware-free screens properly
  hasn't been checked.)

### 2. The bubble level and compass (phone only)

- **Grown-up:** nothing new: they're part of the app.
- **Kid:** tip the phone and watch the bubble; turn around and watch the
  compass. The 🐣 above the compass doesn't point up until
  `compassBirdTurn` in `Tuning.swift` is right: try 0, 90, 180 and 270.
  A first real fix, with a visible result.

### 3. The ESP32 boards

- **Grown-up:** install the Arduino tools and check each board:
  [SETUP.md, steps 1-2](firmware/SETUP.md). Label the boards 1, 2, 3
  with tape. Find a USB cable that carries data (some only charge).
- **Kid:** the board's blue light; Arduino's Blink example is a good
  first "I made a light blink" if they're keen.

### 4. Wire the radio

- **Grown-up:** follow the
  [wiring diagram and checklist](README.md#wiring-the-radio), then check
  it with RadioCheck: [SETUP.md, step 4](firmware/SETUP.md#4-check-the-radio-with-radiocheck).
- **Kid:** hand you the wires by colour ("the yellow one!"), and help
  check each one against the diagram. Keep to the diagram's colours:
  they make every wire easy to trace.

### 5. Sniffing: what does the remote say?

The heart of the project: finding out, by listening, what the remote
sends for each stick and button. Do it even with the same drone: the
answers are published, but working them out is the point.

- **Grown-up:** read [SNIFFING.md](firmware/SNIFFING.md) and rehearse a
  session first. Find the remote's channels with
  [SETUP.md, step 5](firmware/SETUP.md#5-find-the-remotes-channels).
- **Kid:** with props off, they push the stick or press the button when
  `coach.py` counts down, and watch whether the screen says something
  changed. `--pseudo` is a gentle
  practice run that cheers every step and checks nothing; use it while
  they learn the routine.
- **Together:** compare a "hands off" packet with a "stick pushed"
  packet and find the number that changed. Draw the answers as a table
  on paper: which control, which byte, what it becomes. Only then open
  [DroneActions.md](firmware/Relay/DroneActions.md), the answer key, and
  check.

Every session's log goes in `firmware/captures/`, with a line in its
`INDEX.md` saying what happened. Keep them all, even the failed ones:
they're how you find out later what went wrong.

### 6. The relay, and pairing

- **Grown-up:** put the relay on the board:
  [firmware/README.md, The relay](firmware/README.md#the-relay-relay).
- **Kid:** props off, out loud. Type `p` (or run a flight plan), then
  they switch the drone on when told. The drone's lights stop flashing
  when it has paired with the relay instead of its remote.

### 7. A flight plan

- **Grown-up:** read
  [firmware/README.md, Flying from the Mac](firmware/README.md#flying-from-the-mac-fly).
  Rehearse with props off first, then with props on in a clear room.
- **Kid:** `firmware/fly/flight_plan.py` is a list of moves, one per line,
  in real Python:

  ```python
  drone.take_off()       # 🛫
  drone.turn_right(180)  # ↩️ half a turn
  drone.land()           # 🛬
  ```

  Play "drone" first (see above). Then they choose the moves and the
  numbers, predict where the drone will end up, and run it.
- **A real tuning job:** does `turn_right(180)` really make half a turn?
  If not, `turn_speed` in `firmware/fly/tuning.py` is wrong. They change
  it and try again until it's right.

### 8. Flying with the keyboard

- `python3 fly_with_keys.py` in `firmware/fly/`. Each key press pushes
  for a moment; space lands. Which key does what is in `tuning.py`,
  so they can make their own layout.

### 9. Flying with the phone

- Tilt the phone to fly, turn the ring to turn, and the slider to climb.
  How it feels (sensitivity, dead zones) is all in `Tuning.swift`.
- **Not tested yet with the current relay:** see below.

## A different drone

Most cheap 2.4 GHz toy drones use one of a handful of radio chips. The
nRF24L01+ can listen to and imitate the nRF24 family, XN297 (like the
SQN-053) and Beken BK24xx; it can't do A7105 or CC2500 drones, which
need a different radio. The open-source
[DIY-Multiprotocol-TX-Module](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module)
supports a long list of toy drones: check it for your model first, and
note it is GPL-licensed if you copy code from it.

For a drone it doesn't know, the steps are the same as stage 5, for
real this time:

1. Find the channels with the energy scan
   ([SETUP.md, step 5](firmware/SETUP.md#5-find-the-remotes-channels)).
2. Find the packets with `Sniffer/` and decode them
   ([SNIFFING.md](firmware/SNIFFING.md) shows how this one was done).
3. Record each control with `coach/` and step lists like
   `firmware/coach/sticks.txt`, and work out which byte is which.
4. Rewrite `firmware/Relay/DronePackets.h` (the byte table) and, if the
   timing or channels differ, the constants at the top of
   `firmware/Relay/DroneLink.cpp`. The rest of the relay, the app and
   `fly/` don't change.

## Your own app: Android, or anything else

The relay doesn't care what talks to it. Everything an app needs is in
[PROTOCOL.md](PROTOCOL.md): a Bluetooth LE service with one
characteristic, and an 8-byte message sent 20 times a second (sticks as
-100 to 100, switches, and one-off commands). An Android app, a web page
using Web Bluetooth, or a game controller adapter can all fly the drone
by sending that message. The same message also works as a line of text
over the USB cable.

## Windows or Linux

The relay firmware and the Arduino bench tools work anywhere the Arduino
tools run. The Python programs in `firmware/fly/` and `firmware/coach/`
were written and tested on macOS only: they look for the board at
`/dev/cu.usbserial-*` and use Unix terminal settings. On Linux, changing
the port pattern (to `/dev/ttyUSB*`) may be all that's needed; on
Windows they need reworking (for example with the `pyserial` library).
Neither has been tried. The iPhone app needs a Mac.

## What isn't finished

Tested on the bench: pairing, take off, climbing, turning and landing,
from a Mac program. Not yet tested:

- **When the controls stop** (program ends, cable out, phone gone), the
  relay centres the sticks and holds the throttle fully down, expecting
  the drone to come down. That hasn't been tried in flight.
- **The remote's stop button** sends nothing that the sniffer could see,
  so "motors off" in the app holds the throttle down instead.
- **Flying from the phone through the current relay.** The phone link
  worked with an earlier version of the relay; the current one hasn't
  been tried with the phone.
- **Another SQN-053.** The relay pretends to be the remote it was
  decoded from. Whether a second drone of the same model pairs with it
  the same way is likely but untested.

## Sources

On teaching programming to young children:

- Marina Bers, *Coding as a Playground* (Routledge, 2nd ed. 2021), and
  [Teaching Computational Thinking and Coding to Young Children](https://sites.bc.edu/devtech/wp-content/uploads/sites/181/2023/11/Teaching_Computational_Thinking_and_Coding_to_Youn.pdf):
  coding as play, storytelling and making; kids as makers, not only users.
- [ScratchJr design in practice (2023)](https://sites.bc.edu/devtech/wp-content/uploads/sites/181/2023/07/ScratchJr-design-in-practice-Low-fl_2023_International-Journal-of-Child-Com.pdf):
  picture-based code so pre-readers can program; "low floor, wide walls".
- [Introducing computational thinking to second and third graders (2025)](https://www.sciencedirect.com/science/article/pii/S2666557325000199):
  planning a whole path beat giving step-by-step commands.
- [Coding and Computational Thinking Across the Curriculum, a review (2025)](https://journals.sagepub.com/doi/10.3102/00346543241241327):
  short instruction, debugging challenges, predicting outcomes.
- [PRIMM (Raspberry Pi Foundation)](https://static.raspberrypi.org/files/curriculum/quickreads/11-Pedagogy_Summary_PRIMM_V4_2023.pdf):
  predict, run, investigate, modify, make.
- [CSTA K-2 standard on debugging (California's version)](https://www2.cde.ca.gov/cacs/cs?c2=6):
  fix programs by changing the order, stepping through, or trial and error.
- [Unplugged sequencing with K-2 children (NSF)](https://par.nsf.gov/servlets/purl/10516546).
- On praise: [process vs person praise, replication with 8-year-olds](https://par.nsf.gov/biblio/10572660)
  (a smaller effect than the original 1998 study, but process praise
  still helped persistence).
- On attention: [pace teaching with attention (University of Waikato)](https://www.waikato.ac.nz/teaching-and-learning/student-learning/english-language/english-for-papers/education/teaching-tip-1);
  the figures for young kids vary and are rough.
