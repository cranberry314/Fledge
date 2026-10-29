"""
fly_with_keys.py: fly the drone with the keyboard.

    python3 fly_with_keys.py

Which key does what is in tuning.py. Each press pushes for a moment
(key_press_time); hold a key down to keep pushing.
    space   land now
    q       land and quit
"""

import os
import select
import sys
import termios
import tty

import drone
import tuning

# What the arrow keys send to the terminal.
ARROW_KEYS = {
    "\x1b[A": "up arrow",
    "\x1b[B": "down arrow",
    "\x1b[C": "right arrow",
    "\x1b[D": "left arrow",
}

# Actions that are a whole move (they wait until done).
MOVES = {
    "take off": drone.take_off,
    "land": drone.land,
    "pair": drone.pair,
    "flip forward": drone.flip_forward,
    "flip back": drone.flip_back,
    "flip left": drone.flip_left,
    "flip right": drone.flip_right,
}


def push_for(action):
    """The sticks one key press pushes, or None if `action` isn't a push."""
    pushes = {
        "up": {"up": tuning.up_power},
        "down": {"up": -tuning.down_power},
        "forward": {"forward": tuning.move_power},
        "back": {"forward": -tuning.move_power},
        "left": {"right": -tuning.move_power},
        "right": {"right": tuning.move_power},
        "turn left": {"turn": -tuning.turn_power},
        "turn right": {"turn": tuning.turn_power},
    }
    return pushes.get(action)


def read_keys():
    """The keys pressed in the last 0.1 s, by name (often none)."""
    ready, _, _ = select.select([sys.stdin], [], [], 0.1)
    if not ready:
        return []
    typed = os.read(sys.stdin.fileno(), 64).decode("utf-8", "ignore")
    keys = []
    while typed:
        if typed[:3] in ARROW_KEYS:  # an arrow key sends three characters
            keys.append(ARROW_KEYS[typed[:3]])
            typed = typed[3:]
        else:
            keys.append(typed[0])
            typed = typed[1:]
    return keys


def do(action):
    if action in MOVES:
        MOVES[action]()
        return
    sticks = push_for(action)
    if sticks is None:
        print(f"🤔 tuning.py says a key does \"{action}\", but I don't know that move.")
        return
    if not drone.the_drone.flying:
        print("   🛬 Not flying yet: press the take off key first.")
        return
    drone.the_drone.push(tuning.key_press_time, **sticks)


def show_keys():
    print()
    for key, action in tuning.keys.items():
        print(f"   {key:12} {action}")
    print(f"   {'space':12} land now")
    print(f"   {'q':12} land and quit")
    print()


def main():
    drone.pair()
    show_keys()

    saved = termios.tcgetattr(sys.stdin)
    tty.setcbreak(sys.stdin)  # keys arrive at once, without Enter
    try:
        quitting = False
        while not quitting:
            for key in read_keys():
                if key == "q":
                    quitting = True
                    break
                if key == " ":
                    drone.land()
                elif key in tuning.keys:
                    do(tuning.keys[key])
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, saved)
    # drone.py lands the drone, if it is still flying, as the program ends.


if __name__ == "__main__":
    main()
