"""
drone.py: fly the drone from Python, through the relay on the USB cable.

    import drone

    drone.pair()           # then switch the drone on
    drone.take_off()
    drone.turn_right(180)
    drone.land()

How it works: the Mac sends the relay a controls message 20 times a
second (the same 8 numbers the phone sends, see PROTOCOL.md), and the
relay flies the drone with them. If the messages stop (the program ends,
crashes or the cable comes out), the relay holds the throttle down.

The settings (how hard, how fast, which keys) are in tuning.py.
"""

import atexit
import datetime
import os
import threading
import time

import tuning
from relay_link import RelayLink, find_port

# The controls message, from PROTOCOL.md. The relay's Controls.h and the
# app's Buttons.swift use the same numbers.
MESSAGE_VERSION = 2
TAKE_OFF_OR_LAND = 1
MOTORS_OFF = 2
CALIBRATE = 3
FLIP_FORWARD = 4
FLIP_BACK = 5
FLIP_LEFT = 6
FLIP_RIGHT = 7
PAIR = 8

SEND_EVERY = 0.05  # seconds: 20 messages a second, like the phone
PAIRING_SECONDS = 35  # the relay tries for 30 s

CAPTURES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "captures")


class Drone:
    """The drone, as seen from the Mac. There is only ever one: `the_drone`
    at the bottom of this file."""

    def __init__(self):
        self.link = None
        self.flying = False
        self.lock = threading.Lock()  # the sender thread reads what we set
        self.sticks = {"forward": 0, "right": 0, "turn": 0, "up": 0}
        self.let_go_at = None  # time.monotonic() when the sticks go back
        self.command = 0
        self.command_count = 0

    # ---- Connecting ----

    def connect(self):
        """Open the cable to the relay and start sending controls."""
        if self.link:
            return
        port = find_port()
        if not port:
            raise SystemExit("🔌 The relay isn't plugged in (no /dev/cu.usbserial-*).")
        stamp = datetime.datetime.now().strftime("%Y-%m-%d-%H%M%S")
        log_path = os.path.join(CAPTURES, f"{stamp}-fly.log")
        self.link = RelayLink(port, log_path)

        # Opening the port can restart the relay: wait until it says hello.
        if not self.link.wait_for("Relay ready", 3):
            self.link.send("hello")
            if not self.link.wait_for("Relay ready", 3):
                raise SystemExit("🤷 The relay doesn't answer. Is the Relay firmware on it?")

        threading.Thread(target=self._send_forever, daemon=True).start()
        atexit.register(self.finish)
        time.sleep(0.2)  # the relay's first message only sets the command count

    def _send_forever(self):
        """Runs in the background: the controls message, 20 times a second."""
        while self.link:
            with self.lock:
                if self.let_go_at and time.monotonic() >= self.let_go_at:
                    self.sticks = dict.fromkeys(self.sticks, 0)
                    self.let_go_at = None
                switches = (1 if tuning.fast else 0) + (2 if tuning.lights else 0)
                numbers = [MESSAGE_VERSION, self.sticks["forward"], self.sticks["right"],
                           self.sticks["turn"], self.sticks["up"], switches,
                           self.command, self.command_count]
            self.link.send("controls " + " ".join(str(n) for n in numbers), log=False)
            time.sleep(SEND_EVERY)

    def give(self, command):
        """A one-off command: the relay does it once."""
        with self.lock:
            self.command = command
            self.command_count = (self.command_count + 1) % 256
        self.link.note(f"command {command}")

    def push(self, seconds, forward=0, right=0, turn=0, up=0):
        """Push the sticks for `seconds`, then let go. Doesn't wait."""
        with self.lock:
            self.sticks = {"forward": forward, "right": right, "turn": turn, "up": up}
            self.let_go_at = time.monotonic() + seconds

    def move(self, seconds, **sticks):
        """Push the sticks for `seconds`, and wait until done."""
        if not self.flying:
            print("   🛬 Not flying yet: take_off() first.")
            return
        self.push(seconds, **sticks)
        time.sleep(seconds)

    def flip(self, command):
        if not self.flying:
            print("   🛬 Not flying yet: take_off() first.")
            return
        self.give(command)
        time.sleep(1)

    # ---- Pairing, taking off, landing ----

    def pair(self):
        self.connect()
        self.link.send("hello")
        if self.link.wait_for("Relay ready: linked", 1):
            print("🔗 Already paired.")
            return
        self.give(PAIR)
        print("🔗 Pairing: switch the drone on now!")
        if self.link.wait_for("Linked:", PAIRING_SECONDS):
            print("✅ Paired! The drone is listening.")
        else:
            raise SystemExit("😕 The drone didn't answer. Switch it off, run me, then switch it on.")

    def take_off(self):
        if self.flying:
            print("   🛫 Already flying.")
            return
        self.give(TAKE_OFF_OR_LAND)
        self.flying = True
        time.sleep(tuning.take_off_time)

    def land(self):
        if not self.flying:
            print("   🛬 Already on the ground.")
            return
        self.give(TAKE_OFF_OR_LAND)
        self.flying = False
        time.sleep(tuning.land_time)

    def finish(self):
        """At the end of the program, however it ends: land if still
        flying, then stop sending."""
        if not self.link:
            return
        if self.flying:
            print("🛬 Landing before I stop.")
            self.land()
        link, self.link = self.link, None  # stops the sender thread
        time.sleep(SEND_EVERY * 2)
        link.close()


the_drone = Drone()


# ---- The moves: these are the ones to use in a flight plan ----

def pair():
    """🔗 Pair with the drone: run this, then switch the drone on."""
    the_drone.pair()


def take_off():
    """🛫 Take off and rise to a hover."""
    print("🛫 Taking off!")
    the_drone.take_off()


def land():
    """🛬 Come down and stop."""
    print("🛬 Landing.")
    the_drone.land()


def up(seconds=1):
    """⬆️ Climb for `seconds`."""
    print(f"⬆️  Up for {seconds} s")
    the_drone.move(seconds, up=tuning.up_power)


def down(seconds=1):
    """⬇️ Sink for `seconds`."""
    print(f"⬇️  Down for {seconds} s")
    the_drone.move(seconds, up=-tuning.up_power)


def forward(seconds=1):
    """⏩ Fly forward for `seconds`."""
    print(f"⏩ Forward for {seconds} s")
    the_drone.move(seconds, forward=tuning.move_power)


def back(seconds=1):
    """⏪ Fly backward for `seconds`."""
    print(f"⏪ Back for {seconds} s")
    the_drone.move(seconds, forward=-tuning.move_power)


def left(seconds=1):
    """⬅️ Fly sideways to the left for `seconds`."""
    print(f"⬅️  Left for {seconds} s")
    the_drone.move(seconds, right=-tuning.move_power)


def right(seconds=1):
    """➡️ Fly sideways to the right for `seconds`."""
    print(f"➡️  Right for {seconds} s")
    the_drone.move(seconds, right=tuning.move_power)


def turn_left(degrees=90):
    """↪️ Spin left on the spot."""
    print(f"↪️  Turn left {degrees}°")
    the_drone.move(degrees / tuning.turn_speed, turn=-tuning.turn_power)


def turn_right(degrees=90):
    """↩️ Spin right on the spot."""
    print(f"↩️  Turn right {degrees}°")
    the_drone.move(degrees / tuning.turn_speed, turn=tuning.turn_power)


def flip_forward():
    """🤸 Flip forward (needs height: the drone's manual says 3 m)."""
    print("🤸 Flip forward!")
    the_drone.flip(FLIP_FORWARD)


def flip_back():
    """🤸 Flip backward."""
    print("🤸 Flip back!")
    the_drone.flip(FLIP_BACK)


def flip_left():
    """🤸 Flip to the left."""
    print("🤸 Flip left!")
    the_drone.flip(FLIP_LEFT)


def flip_right():
    """🤸 Flip to the right."""
    print("🤸 Flip right!")
    the_drone.flip(FLIP_RIGHT)


def wait(seconds=1):
    """⏸️ Hover where you are for `seconds`."""
    print(f"⏸️  Wait {seconds} s")
    time.sleep(seconds)
