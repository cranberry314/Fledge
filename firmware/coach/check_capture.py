"""Check a coach.py session: did the sniffer record every step properly?

For each step in the step log (.csv) it looks at the sniffer log
(-sniffer.log) and prints:

  good      packets with a correct checksum during the step
  damaged   packets with a wrong checksum (only counted)
  per s     good packets a second (about 24 is normal on one channel)
  gap       the longest time with no good packet (big gaps = missed data)
  changed   payload byte numbers that differ from the 2 seconds before
            the step started, or "-" if nothing changed

It doesn't know which byte means what. The bytes are still scrambled
(XN297), so only the byte numbers are shown, not their values.

Run it in firmware/coach/, with the step log of the session:

    python3 check_capture.py ../captures/2026-09-28-152100-sticks.csv
"""

import collections
import csv
import datetime
import sys

from sniffer_link import PACKET_BYTES, packet_ok

# The 16-byte payload comes after the 5-byte address.
PAYLOAD_START = 5
PAYLOAD_END = 21

# Same rules as coach.py: compare with the 2 seconds before the step,
# and something new counts if it arrived at least 3 times.
BASELINE_SECONDS = 2
NEW_PACKET_TIMES = 3


def read_packets(log_path):
    """All packets in a sniffer log: lists of (time, bytes) good and damaged."""
    good = []
    damaged = []
    with open(log_path, encoding="utf-8", errors="replace") as log:
        for line in log:
            # "2026-09-28T15:21:00.123 PKT <ms> ch=65 ... : 2F 7D 87 ..."
            parts = line.split(" ", 2)
            if len(parts) < 3 or parts[1] != "PKT" or ":" not in parts[2]:
                continue
            try:
                when = datetime.datetime.fromisoformat(parts[0])
                data = bytes.fromhex(parts[2].split(":", 1)[1])
            except ValueError:
                continue  # a line cut short when the port opened
            if len(data) >= PACKET_BYTES and packet_ok(data):
                good.append((when, data[:PACKET_BYTES]))
            else:
                damaged.append((when, data))
    return good, damaged


def between(packets, start, end):
    return [p for t, p in packets if start <= t < end]


def times_between(packets, start, end):
    return [t for t, p in packets if start <= t < end]


def changed_bytes(before, during):
    """Payload byte numbers that differ between the two, as text."""
    if not before or not during:
        return "?"
    usual_before = collections.Counter(before).most_common(1)[0][0]
    counts = collections.Counter(during)
    # The usual packet during the step, plus anything new seen 3+ times.
    seen = [counts.most_common(1)[0][0]]
    seen += [p for p, n in counts.items() if n >= NEW_PACKET_TIMES and p not in before]
    numbers = set()
    for packet in seen:
        for i in range(PAYLOAD_START, PAYLOAD_END):
            if packet[i] != usual_before[i]:
                numbers.add(i - PAYLOAD_START)
    if not numbers:
        return "-"
    return " ".join(str(n) for n in sorted(numbers))


def longest_gap(times, start, end):
    """Longest stretch in seconds with no good packet, from start to end."""
    edges = [start] + times + [end]
    return max((b - a).total_seconds() for a, b in zip(edges, edges[1:]))


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    csv_path = sys.argv[1]
    log_path = csv_path[: -len(".csv")] + "-sniffer.log"
    try:
        good, damaged = read_packets(log_path)
    except FileNotFoundError:
        print(f"No sniffer log next to it: {log_path}")
        print("The sniffer wasn't recording this session.")
        sys.exit(1)

    print(f"{log_path}")
    print(f"{len(good)} good packets, {len(damaged)} damaged\n")
    print(f"{'step':>4} {'result':6} {'good':>5} {'damaged':>7} {'per s':>5} {'gap':>6}  changed  what")

    problems = 0
    with open(csv_path, encoding="utf-8") as steps:
        for row in csv.DictReader(steps):
            start = datetime.datetime.fromisoformat(row["start"])
            end = datetime.datetime.fromisoformat(row["end"])
            seconds = (end - start).total_seconds()
            during = between(good, start, end)
            before = between(good, start - datetime.timedelta(seconds=BASELINE_SECONDS), start)
            n_damaged = len(between(damaged, start, end))
            gap = longest_gap(times_between(good, start, end), start, end)
            rate = len(during) / seconds if seconds else 0

            # A step is suspect if almost nothing arrived, or a long gap.
            warning = ""
            if rate < 5 or gap > 2:
                warning = "  <-- check"
                problems += 1

            print(f"{row['step']:>4} {row['result']:6} {len(during):>5} {n_damaged:>7} "
                  f"{rate:>5.1f} {gap:>5.1f}s  {changed_bytes(before, during):7}  "
                  f"{row['what'][:50]}{warning}")

    print()
    if problems:
        print(f"{problems} step(s) marked '<-- check': few packets or a gap over 2 s.")
        print("Steps before pairing (drone/remote off) are expected to be quiet.")
    else:
        print("Every step recorded packets with no gap over 2 s.")


main()
