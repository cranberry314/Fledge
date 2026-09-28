#!/usr/bin/env python3
"""
coach.py tells you what to do with the remote, one step at a time, with a
countdown. At the same time it records what the Sniffer ESP32 hears, and
after each step says whether the remote's signal changed.

    python3 coach.py controls.txt
    python3 coach.py controls.txt --quick       # wait for Enter, no countdown
    python3 coach.py controls.txt --say         # also read each step aloud
    python3 coach.py controls.txt --no-sniffer  # just the countdown

Keys while it runs:
    r   that step went wrong: it is marked bad and done again
    q   stop
    Enter   with --quick: start the step shown

The steps are in the .txt files: one step per line, the number of seconds
first, then what to do. Steps starting with "Rest" or "Switch" aren't
checked for a change. Two logs go in firmware/captures/: the steps
(.csv) and everything the sniffer printed (-sniffer.log).
"""

import collections
import csv
import datetime
import os
import select
import subprocess
import sys
import termios
import time
import tty

from sniffer_link import SnifferLink, find_port

# Countdown before each step, in seconds.
GET_READY_SECONDS = 5

# Countdown after letting go, before the next step, in seconds.
REST_SECONDS = 3

# With --quick: no countdown; show each step and wait for Enter.
WAIT_FOR_ENTER = False

# How long to listen for the remote before step 1, in seconds.
CHECK_SECONDS = 5

# Fewer packets than this in a step means we're not hearing the remote
# (a paired remote gives about 30 a second; noise gives one now and then).
HEARING_PACKETS = 5

# The sniffer, once connected (None with --no-sniffer).
LINK = None


def read_steps(path):
    """Read the steps file: '<seconds> <what to do>' on each line."""
    steps = []
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if line == "" or line.startswith("#"):
            continue
        seconds, text = line.split(maxsplit=1)
        steps.append((int(seconds), text))
    return steps


def timestamp():
    """The Mac's clock, to the millisecond, as the sniffer log writes it."""
    return datetime.datetime.now().isoformat(timespec="milliseconds")


def say(text):
    """Print a line, and speak it too if --say was given."""
    print(text, flush=True)
    if SPEAK:
        subprocess.Popen(["say", text])


def key_pressed():
    """The key pressed since last time, or None."""
    if not KEYS:
        return None
    ready, _, _ = select.select([sys.stdin], [], [], 0)
    if ready:
        return sys.stdin.read(1).lower()
    return None


def count_down(seconds):
    """Say seconds, seconds-1, ... 1, one per second.
    Returns 'r' or 'q' if one of those keys is pressed, otherwise None."""
    start = time.monotonic()
    for n in range(seconds, 0, -1):
        say(str(n))
        # Wait until this second is over, checking the keys as we go.
        second_ends = start + (seconds - n + 1)
        while time.monotonic() < second_ends:
            key = key_pressed()
            if key in ("r", "q"):
                return key
            time.sleep(0.02)
    return None


def control_bytes(packet):
    """The bytes of a packet that carry the sticks and buttons: bytes 7-17.
    The rest is the address, the next hop channel and checksums, which
    change as the remote hops even when nobody touches it."""
    return packet[7:18]


def wait_for_enter():
    """Wait until Enter is pressed. Returns 'q' if q is pressed instead."""
    if not KEYS:
        return None
    while True:
        key = key_pressed()
        if key in ("\n", "\r"):
            return None
        if key == "q":
            return "q"
        time.sleep(0.02)


def remote_changed(before, during):
    """Did the remote send something different during the step?
    Yes if what it mostly sent during the step is not what it mostly sent
    in the countdown before it, or if something new arrived at least 3
    times (a quick button press). Only packets with a good checksum get
    this far (see sniffer_link.py)."""
    before = [control_bytes(p) for p in before]
    during = [control_bytes(p) for p in during]
    if not before:
        return False
    usual_before = collections.Counter(before).most_common(1)[0][0]
    usual_during = collections.Counter(during).most_common(1)[0][0]
    if usual_during != usual_before:
        return True
    seen_before = set(before)
    new = collections.Counter(p for p in during if p not in seen_before)
    return any(count >= 3 for count in new.values())


def check_step(kind, ready_from, hold_from, hold_to):
    """Say what the sniffer heard during the step. Returns 'yes', 'no' or
    '' for the log's remote_changed column."""
    during = LINK.packets_between(hold_from, hold_to)
    hearing = len(during) >= HEARING_PACKETS
    if kind == "switch":
        # Switching things on: nothing to compare with, just report.
        if hearing:
            say("📡 Hearing the remote")
        return ""
    if not hearing:
        say("⚠️  Not hearing the remote")
        return ""
    if kind == "rest":
        say("📡 Hearing the remote")
        return ""
    # Compare with the 2 seconds before Go!, leaving out the last moment:
    # people start early.
    before = LINK.packets_between(max(ready_from, hold_from - 2),
                                  hold_from - 0.3)
    if remote_changed(before, during):
        say("✅ The remote's signal changed")
        return "yes"
    say("❌ No change seen. Press r to do it again")
    return "no"


def run_step(seconds, text):
    """Do one step. Returns (start time, end time, key pressed or None,
    whether the remote's signal changed)."""
    kind = "step"
    if text.lower().startswith("rest"):
        kind = "rest"
    if text.lower().startswith("switch"):
        kind = "switch"
    unit = "second" if seconds == 1 else "seconds"

    ready_from = time.monotonic()
    if WAIT_FOR_ENTER:
        say(f"{text}, for {seconds} {unit}.")
        print("Press Enter to start.", flush=True)
        key = wait_for_enter()
        if key:
            return None, None, key, ""
        say("Go!")
    elif kind == "rest":
        say(f"{text}, for {seconds} {unit}")
    else:
        say(f"{text}, for {seconds} {unit}, in")
        key = count_down(GET_READY_SECONDS)
        if key:
            return None, None, key, ""
        say("Go!")

    hold_from = time.monotonic()
    start = timestamp()
    key = count_down(seconds)
    end = timestamp()
    hold_to = time.monotonic()
    if key:
        return start, end, key, ""

    changed = ""
    if LINK:
        changed = check_step(kind, ready_from, hold_from, hold_to)
    if kind == "step":
        say("Let go ✋")
        key = count_down(REST_SECONDS)
    return start, end, key, changed


def connect_sniffer(sniffer_log_path):
    """Start recording the sniffer, and check it hears the remote.
    Returns False if the session should not start."""
    global LINK
    port = find_port()
    if port is None:
        print("❌ Sniffer not found: is the ESP32 plugged in?")
        print("   (To run without it: add --no-sniffer)")
        return False
    try:
        LINK = SnifferLink(port, sniffer_log_path)
    except OSError as error:
        print(f"❌ Can't open {port}: {error}")
        print("   Is something else using it, like the Serial Monitor?")
        return False

    print(f"Listening for the remote for {CHECK_SECONDS} seconds...")
    time.sleep(CHECK_SECONDS)
    if LINK.radio_missing:
        print("❌ The sniffer says its radio isn't connected: run RadioCheck.")
        return False
    heard = len(LINK.packets)
    if heard >= HEARING_PACKETS:
        print(f"📡 Hearing the remote ({heard} packets).")
    else:
        print(f"⚠️  Not hearing the remote ({heard} packets). Fine if the")
        print("   steps start by switching it on. If not: is it on and")
        print("   paired, and is the sniffer on the right channel?")
    return True


def main():
    steps_file = sys.argv[1]
    steps = read_steps(steps_file)

    # The logs: firmware/captures/<date-time>-<steps file name>...
    here = os.path.dirname(os.path.abspath(__file__))
    captures = os.path.normpath(os.path.join(here, "..", "captures"))
    os.makedirs(captures, exist_ok=True)
    name = os.path.splitext(os.path.basename(steps_file))[0]
    started = datetime.datetime.now().strftime("%Y-%m-%d-%H%M%S")
    log_path = os.path.join(captures, f"{started}-{name}.csv")
    sniffer_log_path = os.path.join(captures, f"{started}-{name}-sniffer.log")

    print(f"{len(steps)} steps from {steps_file}")
    if USE_SNIFFER and not connect_sniffer(sniffer_log_path):
        return
    log_file = open(log_path, "w", newline="", encoding="utf-8")
    log = csv.writer(log_file)
    log.writerow(["step", "what", "start", "end", "result", "remote_changed"])
    print(f"Logs: {log_path}")
    if LINK:
        print(f"      {sniffer_log_path}")
    print("Keys: r = that step went wrong, do it again.  q = stop.")
    if KEYS:
        answer = input("Press Enter to start (q then Enter to stop). ")
        if answer.strip().lower() == "q":
            log_file.close()
            return

    number = 0
    while number < len(steps):
        seconds, text = steps[number]
        print()
        print(f"--- Step {number + 1} of {len(steps)} ---")
        start, end, key, changed = run_step(seconds, text)

        if key == "q":
            log.writerow([number + 1, text, start, end, "stopped", changed])
            say("Stopped.")
            break
        if key == "r":
            log.writerow([number + 1, text, start, end, "redo", changed])
            log_file.flush()
            say("Oops. Again!")
            continue  # the same step again

        log.writerow([number + 1, text, start, end, "ok", changed])
        log_file.flush()
        number += 1

    if number == len(steps):
        say("All done! 🎉")
    log_file.close()
    print(f"Log saved: {log_path}")


SPEAK = "--say" in sys.argv
USE_SNIFFER = "--no-sniffer" not in sys.argv
if "--quick" in sys.argv:
    WAIT_FOR_ENTER = True
    REST_SECONDS = 1
sys.argv = [a for a in sys.argv if not a.startswith("--")]
if len(sys.argv) != 2:
    print(__doc__)
    sys.exit(1)

# Read single key presses (r, q) without waiting for Enter. This only
# works in a real terminal; elsewhere the keys are simply ignored.
KEYS = sys.stdin.isatty()
if KEYS:
    saved_terminal = termios.tcgetattr(sys.stdin)
try:
    if KEYS:
        tty.setcbreak(sys.stdin)
    main()
except KeyboardInterrupt:
    # Ctrl-C: steps finished so far are already in the logs.
    print("\nStopped.")
finally:
    if LINK:
        LINK.close()
    if KEYS:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, saved_terminal)
