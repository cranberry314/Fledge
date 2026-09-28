#!/usr/bin/env python3
"""
coach.py tells you what to do with the remote, one step at a time, with a
countdown, and writes down exactly when each step happened. The sniffer's
recording can then be matched to the steps by time.

    python3 coach.py pairing.txt
    python3 coach.py controls.txt
    python3 coach.py controls.txt --say     # also read each step aloud
    python3 coach.py controls.txt --quick   # shorter countdowns

Keys while it runs:
    r   that step went wrong: it is marked bad and done again
    q   stop

The steps are in the .txt files: one step per line, the number of seconds
first, then what to do. The log goes in firmware/captures/.
"""

import csv
import datetime
import os
import select
import subprocess
import sys
import termios
import time
import tty

# Countdown before each step, in seconds.
GET_READY_SECONDS = 5

# Countdown after letting go, before the next step, in seconds.
REST_SECONDS = 3


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


def run_step(seconds, text):
    """Do one step. Returns (start time, end time, key pressed or None)."""
    is_rest = text.lower().startswith("rest")
    unit = "second" if seconds == 1 else "seconds"

    if is_rest:
        say(f"{text}, for {seconds} {unit}")
    else:
        say(f"{text}, for {seconds} {unit}, in")
        key = count_down(GET_READY_SECONDS)
        if key:
            return None, None, key
        say("Go!")

    start = timestamp()
    key = count_down(seconds)
    end = timestamp()
    if key:
        return start, end, key

    if not is_rest:
        say("Let go ✋")
        key = count_down(REST_SECONDS)
    return start, end, key


def main():
    steps_file = sys.argv[1]
    steps = read_steps(steps_file)

    # The log file: firmware/captures/<date-time>-<steps file name>.csv
    here = os.path.dirname(os.path.abspath(__file__))
    captures = os.path.join(here, "..", "captures")
    os.makedirs(captures, exist_ok=True)
    name = os.path.splitext(os.path.basename(steps_file))[0]
    started = datetime.datetime.now().strftime("%Y-%m-%d-%H%M%S")
    log_path = os.path.normpath(os.path.join(captures, f"{started}-{name}.csv"))
    log_file = open(log_path, "w", newline="", encoding="utf-8")
    log = csv.writer(log_file)
    log.writerow(["step", "what", "start", "end", "result"])

    print(f"{len(steps)} steps from {steps_file}")
    print(f"Log: {log_path}")
    print("Keys: r = that step went wrong, do it again.  q = stop.")
    if KEYS:
        input("Press Enter to start. ")

    number = 0
    while number < len(steps):
        seconds, text = steps[number]
        print()
        print(f"--- Step {number + 1} of {len(steps)} ---")
        start, end, key = run_step(seconds, text)

        if key == "q":
            log.writerow([number + 1, text, start, end, "stopped"])
            say("Stopped.")
            break
        if key == "r":
            log.writerow([number + 1, text, start, end, "redo"])
            log_file.flush()
            say("Oops. Again!")
            continue  # the same step again

        log.writerow([number + 1, text, start, end, "ok"])
        log_file.flush()
        number += 1

    if number == len(steps):
        say("All done! 🎉")
    log_file.close()
    print(f"Log saved: {log_path}")


SPEAK = "--say" in sys.argv
if "--quick" in sys.argv:
    GET_READY_SECONDS = 2
    REST_SECONDS = 1
sys.argv = [a for a in sys.argv if a not in ("--say", "--quick")]
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
    # Ctrl-C: steps finished so far are already in the log.
    print("\nStopped.")
finally:
    if KEYS:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, saved_terminal)
