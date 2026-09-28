"""
sniffer_link.py: records what the Sniffer ESP32 prints, for coach.py.

It opens the ESP32's USB serial port, writes every line the sniffer prints
to a log file with the Mac's time, and keeps the packets so coach.py can
tell whether the remote's signal changed during a step.
"""

import datetime
import glob
import os
import select
import termios
import threading
import time
import tty

# The ESP32's USB serial port. The sniffer prints at 115200 baud.
PORT_PATTERN = "/dev/cu.usbserial-*"
BAUD = termios.B115200

# Compare this many bytes of each packet: address (5), payload (16) and
# checksum (2). Anything after that is noise.
PACKET_BYTES = 23


def find_port():
    """The ESP32's serial port, or None if it isn't plugged in."""
    ports = sorted(glob.glob(PORT_PATTERN))
    if ports:
        return ports[0]
    return None


class SnifferLink:
    def __init__(self, port, log_path):
        # Open the port and set it to 115200 baud, bytes passed through as
        # they are.
        self.fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        tty.setraw(self.fd)
        settings = termios.tcgetattr(self.fd)
        settings[4] = BAUD  # input speed
        settings[5] = BAUD  # output speed
        termios.tcsetattr(self.fd, termios.TCSANOW, settings)

        self.log = open(log_path, "w", encoding="utf-8")
        self.packets = []  # (time.monotonic() when it arrived, bytes)
        self.radio_missing = False
        self.running = True
        self.thread = threading.Thread(target=self.read_lines, daemon=True)
        self.thread.start()

    def read_lines(self):
        """Runs in the background: read the port, one line at a time."""
        waiting = b""
        while self.running:
            ready, _, _ = select.select([self.fd], [], [], 0.1)
            if not ready:
                continue
            try:
                chunk = os.read(self.fd, 4096)
            except OSError:
                break  # unplugged
            waiting += chunk
            while b"\n" in waiting:
                line, waiting = waiting.split(b"\n", 1)
                text = line.decode("ascii", "ignore")
                text = "".join(c for c in text if c.isprintable()).strip()
                if text:
                    self.handle_line(text)

    def handle_line(self, line):
        """Log one line, and keep it if it is a packet."""
        now = datetime.datetime.now().isoformat(timespec="milliseconds")
        self.log.write(f"{now} {line}\n")
        self.log.flush()

        if line.startswith("Radio not found"):
            self.radio_missing = True
        # A packet line: "PKT <ms> ch=65 ... : 2F 7D 87 ..."
        if line.startswith("PKT ") and ":" in line:
            try:
                data = bytes.fromhex(line.split(":", 1)[1])
            except ValueError:
                return  # a line cut short when the port opened
            if len(data) >= PACKET_BYTES:
                self.packets.append((time.monotonic(), data[:PACKET_BYTES]))

    def packets_between(self, start, end):
        """Packets that arrived between two time.monotonic() times."""
        return [p for t, p in list(self.packets) if start <= t < end]

    def close(self):
        self.running = False
        self.thread.join(timeout=1)
        os.close(self.fd)
        self.log.close()
