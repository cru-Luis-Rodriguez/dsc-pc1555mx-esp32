#!/usr/bin/env python3
"""
Capture N seconds of serial output to a file, non-interactively.

Why this exists:
  - `pio device monitor` requires a tty. Redirect its output and it dies with
    termios.error: (102, 'Operation not supported on socket'), so it cannot be
    used to capture output from a script or an agent session.
  - macOS has no `timeout` (that's GNU coreutils). `gtimeout` only exists if
    someone installed coreutils via brew.

Usage:
    python3 tools/capture_serial.py [port] [baud] [seconds] [outfile]

Defaults: /dev/cu.usbserial-0001 115200 30 serial-capture.log

Needs pyserial. PlatformIO already bundles it, so the reliable invocation is:

    ~/.platformio/penv/bin/python tools/capture_serial.py

Use that path rather than a Homebrew Cellar path — the Cellar path contains the
PlatformIO version number and breaks on every upgrade.

Output goes to both stdout and the file, so it works piped or redirected.
"""

import sys
import time

try:
    import serial  # pyserial
except ImportError:
    sys.exit(
        "pyserial not found.\n"
        "Run this with PlatformIO's bundled python instead:\n"
        "    ~/.platformio/penv/bin/python tools/capture_serial.py"
    )

DEFAULTS = ("/dev/cu.usbserial-0001", 115200, 30, "serial-capture.log")


def main() -> int:
    args = sys.argv[1:]
    port = args[0] if len(args) > 0 else DEFAULTS[0]
    baud = int(args[1]) if len(args) > 1 else DEFAULTS[1]
    seconds = float(args[2]) if len(args) > 2 else DEFAULTS[2]
    outfile = args[3] if len(args) > 3 else DEFAULTS[3]

    try:
        ser = serial.Serial(port, baud, timeout=0.2)
    except serial.SerialException as exc:
        sys.exit(f"could not open {port}: {exc}")

    print(
        f"# capturing {seconds:g}s from {port} @ {baud} -> {outfile}",
        file=sys.stderr,
        flush=True,
    )

    deadline = time.monotonic() + seconds
    total = 0

    with ser, open(outfile, "wb") as fh:
        while time.monotonic() < deadline:
            chunk = ser.read(4096)
            if not chunk:
                continue
            total += len(chunk)
            fh.write(chunk)
            fh.flush()
            sys.stdout.write(chunk.decode("utf-8", errors="replace"))
            sys.stdout.flush()

    print(f"\n# {total} bytes in {seconds:g}s", file=sys.stderr, flush=True)

    if total == 0:
        print(
            "# zero bytes. Board not running, wrong port, or another process "
            "holds the port (close any open serial monitor).",
            file=sys.stderr,
            flush=True,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
