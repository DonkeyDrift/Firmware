PUBLIC_COMMANDS = {"PING", "STATUS", "AUTH"}
PARK_LOCKED_COMMANDS = {"TEST", "TEST_TUI", "BENCH", "STRESS", "REGRESS", "FILTER_TEST"}
GENERAL_AUTHENTICATED_COMMANDS = {"ANSI", "NOANSI", "FILTER_DEBUG"}


def normalize_wireless_command(line):
    return line.strip().split(":", 1)[0].upper()


def is_control_command(line):
    parts = line.strip().split(":")
    if len(parts) < 2:
        return False
    try:
        int(parts[0])
        int(parts[1].split("*", 1)[0])
    except ValueError:
        return False
    return True


def is_wireless_command_allowed(line, authenticated, park_locked):
    command = normalize_wireless_command(line)
    if command in PUBLIC_COMMANDS:
        return True
    if not authenticated:
        return False
    if command in PARK_LOCKED_COMMANDS:
        return park_locked
    if command in GENERAL_AUTHENTICATED_COMMANDS:
        return True
    return is_control_command(line)


def authenticate_wireless_command(line, password):
    prefix = "AUTH:"
    return line.startswith(prefix) and line[len(prefix):] == password


class WirelessLineBuffer:
    def __init__(self, max_length=255):
        self.max_length = max_length
        self._buffer = []
        self.overflowed = False

    def feed(self, text):
        lines = []
        for char in text:
            if char == "\r":
                continue
            if char == "\n":
                lines.append("".join(self._buffer))
                self._buffer = []
                continue
            if len(self._buffer) < self.max_length:
                self._buffer.append(char)
            else:
                self.overflowed = True
        return lines
