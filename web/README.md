# Fledge Code

A small programming language for flying the drone, made for young
programmers. It looks like Python:

    take_off()
    up(1)
    repeat 4:
        forward(1)
        rotate(90)
    land()

It runs in a web page, so it works on a Chromebook, a Mac or a PC. It
flies a drone on the screen, and the real one too, over Bluetooth to the
relay. Pick a program from **📚 Examples** to start, then change it.

## Opening it

To fly on the screen only, open `index.html` in Chrome.

To fly the real drone, Chrome needs the page from `localhost` (it won't
use Bluetooth on a page opened from a file). In a terminal, from the
`Fledge` folder (on a Chromebook, in Linux):

    python3 -m http.server 8000 --bind 127.0.0.1 --directory web

Then open http://localhost:8000, press **📡 Connect** and pick
**Fledge Relay**, then **🔗 Pair** and switch the drone on.

## Two ways to fly

- **▶️ Fly** checks the whole program first, then flies it. A mistake
  stops it before take-off, never in the air.
- **fly>** under the drone flies one command at a time, as you type.

## The rules

Every program is checked on paper before it flies:

- A program lasts at most 45 seconds, then the drone lands by itself.
- The drone stays within 3 seconds of flying from where it took off,
  in every direction, and climbs at most 3 seconds above its hover.

The numbers are in `settings.js`: change one, save, and reload.

## The files

- `settings.js`: the numbers to change.
- `examples.js`: the programs in the Examples menu.
- `language.js`: reads a program into commands, with friendly mistakes.
- `checker.js`: flies the commands on paper and enforces the rules.
- `preview.js`: draws the drone and plays the flight.
- `bluetooth.js`: sends the controls to the relay (see PROTOCOL.md).
- `page.js`: the editor, buttons and command line.
