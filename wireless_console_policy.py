PUBLIC_COMMANDS = {"PING", "STATUS", "AUTH"}
PARK_LOCKED_COMMANDS = {"TEST", "TEST_TUI", "BENCH", "STRESS", "REGRESS", "FILTER_TEST"}
GENERAL_AUTHENTICATED_COMMANDS = {"ANSI", "NOANSI", "FILTER_DEBUG"}
OTA_OPEN_COMMANDS = {"ENABLE_OTA"}
OTA_STATUS_COMMANDS = {"OTA_STATUS"}
OTA_CLOSE_COMMANDS = {"DISABLE_OTA"}


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
    if command in OTA_OPEN_COMMANDS:
        return authenticated and park_locked
    if command in OTA_STATUS_COMMANDS or command in OTA_CLOSE_COMMANDS:
        return authenticated
    if not authenticated:
        return False
    if command in PARK_LOCKED_COMMANDS:
        return park_locked
    if command in GENERAL_AUTHENTICATED_COMMANDS:
        return True
    return is_control_command(line)


def is_web_command_allowed(line, authenticated, park_locked):
    return is_wireless_command_allowed(line, authenticated, park_locked)


def describe_wifi_mode(ap_enabled, sta_configured, sta_connected):
    if not ap_enabled:
        return "off"
    if sta_configured and sta_connected:
        return "ap_sta_connected"
    if sta_configured:
        return "ap_sta_pending"
    return "ap"


def format_network_status(ap_ip, web_port, sta_configured, sta_connected, sta_ip):
    normalized_sta_ip = sta_ip if sta_connected and sta_ip else "0.0.0.0"
    return (
        f"web_port={web_port} ap_ip={ap_ip} "
        f"sta_configured={1 if sta_configured else 0} "
        f"sta_connected={1 if sta_connected else 0} "
        f"sta_ip={normalized_sta_ip}"
    )


def is_ota_window_active(now_ms, deadline_ms):
    return deadline_ms > 0 and now_ms < deadline_ms


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
