"""
relay_link.py: the USB cable between the Mac and the relay (the ESP32).

It sends lines of text to the relay and reads what the relay prints, in
the background. Every line both ways goes into a log file in
firmware/captures/, so each flight leaves a record.

Only drone.py uses this file.
"""

import datetime
import glob
import os
import select
import termios
import threading
import time
import tty

# The ESP32's USB serial port, and its speed.
PORT_PATTERN = "/dev/cu.usbserial-*"
SPEED = termios.B115200
BYTES_PER_SECOND = 115200 / 10  # 10 bits on the wire for each byte

# Right after the Mac first writes to the port, macOS hands us the relay's
# newest line again and again, thousands of times a second: far faster
# than the cable can carry (seen 2026-10-14, also in arduino-cli monitor).
# Those copies are dropped, and the log says how many.


def find_port():
    """The ESP32's serial port, or None if it isn't plugged in."""
    ports = sorted(glob.glob(PORT_PATTERN))
    return ports[0] if ports else None


class RelayLink:
    def __init__(self, port, log_path):
        self.fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        tty.setraw(self.fd)
        settings = termios.tcgetattr(self.fd)
        settings[4] = SPEED  # input speed
        settings[5] = SPEED  # output speed
        termios.tcsetattr(self.fd, termios.TCSANOW, settings)

        self.log = open(log_path, "w", encoding="utf-8")
        self.log_lock = threading.Lock()
        self.lines = []  # everything the relay printed: (time.monotonic(), text)
        self.dropped_copies = 0
        self.previous_text = None
        self.previous_at = 0.0
        self.running = True
        self.reader = threading.Thread(target=self._read_forever, daemon=True)
        self.reader.start()

    def send(self, text, log=True):
        """Send one line to the relay."""
        os.write(self.fd, (text + "\n").encode("ascii"))
        if log:
            self._log(">", text)

    def note(self, text):
        """Put a note of our own in the log."""
        self._log("#", text)

    def wait_for(self, words, seconds):
        """Wait until the relay prints a line containing `words` (or any
        of them, if `words` is a list). Returns that line, or None if
        `seconds` pass first."""
        if isinstance(words, str):
            words = [words]
        start = time.monotonic()
        while time.monotonic() - start < seconds:
            for at, text in list(self.lines):
                if at >= start and any(w in text for w in words):
                    return text
            time.sleep(0.05)
        return None

    def close(self):
        self.running = False
        self.reader.join(timeout=1)
        os.close(self.fd)
        self.log.close()

    def _read_forever(self):
        """Runs in the background: read the port one line at a time."""
        waiting = b""
        while self.running:
            ready, _, _ = select.select([self.fd], [], [], 0.1)
            if not ready:
                continue
            try:
                waiting += os.read(self.fd, 4096)
            except OSError:
                break  # unplugged
            while b"\n" in waiting:
                line, waiting = waiting.split(b"\n", 1)
                text = line.decode("ascii", "ignore")
                text = "".join(c for c in text if c.isprintable()).strip()
                if text and not self._impossible_copy(text):
                    if self.dropped_copies:
                        self.note(f"dropped {self.dropped_copies} copies of the line "
                                  "above: too fast to have come down the cable")
                        self.dropped_copies = 0
                    self.lines.append((time.monotonic(), text))
                    self._log("<", text)

    def _impossible_copy(self, text):
        """True if `text` repeats the line before it (kept or dropped)
        sooner than the cable could have carried it again."""
        now = time.monotonic()
        shortest_gap = (len(text) + 2) / BYTES_PER_SECOND  # + the line ending
        impossible = text == self.previous_text and now - self.previous_at < shortest_gap
        self.previous_text = text
        self.previous_at = now
        if impossible:
            self.dropped_copies += 1
        return impossible

    def _log(self, direction, text):
        now = datetime.datetime.now().isoformat(timespec="milliseconds")
        with self.log_lock:
            self.log.write(f"{now} {direction} {text}\n")
            self.log.flush()
