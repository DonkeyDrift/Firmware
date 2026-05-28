PUBLIC_COMMANDS = {"PING", "STATUS", "AUTH", "WIFI_STA_STATUS"}
PARK_LOCKED_COMMANDS = {"TEST", "TEST_TUI", "BENCH", "STRESS", "REGRESS", "FILTER_TEST"}
GENERAL_AUTHENTICATED_COMMANDS = {"ANSI", "NOANSI", "FILTER_DEBUG", "LOG_WEB", "LOG_SERIAL"}
WIFI_STA_CONFIG_COMMANDS = {"WIFI_STA_SSID", "WIFI_STA_PASSWORD", "WIFI_STA_APPLY", "WIFI_STA_CLEAR"}
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


def is_wireless_command_allowed(line, authenticated, park_locked, *, dev_mode=False, origin="tcp"):
    command = normalize_wireless_command(line)
    web_dev_mode = dev_mode and origin == "web"
    if command in PUBLIC_COMMANDS:
        return True
    if command in OTA_OPEN_COMMANDS:
        return web_dev_mode or (authenticated and park_locked)
    if command in OTA_STATUS_COMMANDS or command in OTA_CLOSE_COMMANDS:
        return web_dev_mode or authenticated
    if not authenticated:
        return False
    if command in PARK_LOCKED_COMMANDS:
        return park_locked
    if command in GENERAL_AUTHENTICATED_COMMANDS or command in WIFI_STA_CONFIG_COMMANDS:
        return True
    return is_control_command(line)


def redact_wireless_console_line(line):
    stripped = line.strip()
    command = normalize_wireless_command(stripped)
    if command == "AUTH":
        return "AUTH:<redacted>"
    if command == "WIFI_STA_PASSWORD":
        return "WIFI_STA_PASSWORD:<redacted>"
    return line


def is_web_command_allowed(line, authenticated, park_locked, dev_mode=False):
    return is_wireless_command_allowed(line, authenticated, park_locked, dev_mode=dev_mode, origin="web")


def is_local_ota_open_command_allowed(line, password, park_locked):
    prefix = "ENABLE_OTA:"
    return line.startswith(prefix) and line[len(prefix):] == password


def is_local_ota_status_command(line):
    return line.strip().upper() == "OTA_STATUS"


def is_local_ota_close_command(line):
    return line.strip().upper() == "DISABLE_OTA"


def select_log_target(configured, wifi_console_enabled):
    if configured == "serial":
        return "serial"
    if configured == "web" and wifi_console_enabled:
        return "web"
    if configured is None and wifi_console_enabled:
        return "web"
    return "serial"


def describe_wifi_mode(ap_enabled, sta_configured, sta_connected):
    if not ap_enabled:
        return "off"
    if sta_configured and sta_connected:
        return "ap_sta_connected"
    if sta_configured:
        return "ap_sta_pending"
    return "ap"


def format_network_status(
    ap_ip,
    web_port,
    sta_configured,
    sta_connected,
    sta_ip,
    free_heap=0,
    min_free_heap=0,
    ws_queue_full_skip=0,
    ws_max_backlog=0,
    ws_connects=0,
    ws_disconnects=0,
    web_update_dt_max=0,
    web_sample_dt_max=0,
    web_http_dt_max=0,
    web_ws_dt_max=0,
    http_status_count=0,
    http_log_count=0,
    http_data_count=0,
    http_cmd_count=0,
):
    normalized_sta_ip = sta_ip if sta_connected and sta_ip else "0.0.0.0"
    return (
        f"web_port={web_port} free_heap={free_heap} min_free_heap={min_free_heap} "
        f"ws_queue_full_skip={ws_queue_full_skip} ws_max_backlog={ws_max_backlog} "
        f"ws_connects={ws_connects} ws_disconnects={ws_disconnects} "
        f"web_update_dt_max={web_update_dt_max} web_sample_dt_max={web_sample_dt_max} "
        f"web_http_dt_max={web_http_dt_max} web_ws_dt_max={web_ws_dt_max} "
        f"http_status_count={http_status_count} http_log_count={http_log_count} "
        f"http_data_count={http_data_count} http_cmd_count={http_cmd_count} ap_ip={ap_ip} "
        f"sta_configured={1 if sta_configured else 0} "
        f"sta_connected={1 if sta_connected else 0} "
        f"sta_ip={normalized_sta_ip}"
    )


def is_ota_window_active(now_ms, deadline_ms, *, dev_mode=False):
    return dev_mode or (deadline_ms > 0 and now_ms < deadline_ms)


def should_emit_serial1_telemetry(ota_window_open, ota_in_progress):
    return not (ota_window_open or ota_in_progress)


def should_force_park_for_ota(park_guard_active, ota_in_progress, *, dev_mode=False):
    return park_guard_active or ota_in_progress


def dev_mode_ota_state(dev_mode, ota_window_open):
    return {
        "ota_window_open": bool(dev_mode or ota_window_open),
        "park_guard_active": False,
    }


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


class WebLogBuffer:
    def __init__(self, capacity=64):
        self.capacity = capacity
        self._entries = []
        self._next_seq = 1
        self.dropped = 0

    def append(self, now_ms, source, line):
        entry = {"seq": self._next_seq, "t": now_ms, "src": source, "line": line}
        self._next_seq += 1
        if len(self._entries) >= self.capacity:
            self._entries.pop(0)
            self.dropped += 1
        self._entries.append(entry)
        return entry["seq"]

    def since(self, seq):
        return [entry for entry in self._entries if entry["seq"] > seq]


def format_web_data_point(seq, now_ms, control, rc, pilot, sensor, drift=None):
    drift = drift or {}
    return {
        "seq": seq,
        "t": now_ms,
        "thr": control["throttle"],
        "str": control["steering"],
        "mode": control["mode"],
        "park": 1 if control["park"] else 0,
        "rct": rc["throttle"],
        "rcs": rc["steering"],
        "ch1": rc.get("ch1", rc["steering"]),
        "ch2": rc.get("ch2", rc["throttle"]),
        "ch3": rc.get("ch3", 1500),
        "ch4": rc.get("ch4", 1500),
        "ch5": rc.get("ch5", 1000),
        "ch6": rc.get("ch6", 1500),
        "pt": pilot["throttle"],
        "ps": pilot["steering"],
        "cur": sensor["current_mA"],
        "vol": sensor["voltage"],
        "gz": sensor["gyroZ"],
        "de": 1 if drift.get("enabled", False) else 0,
        "da": 1 if drift.get("active", False) else 0,
        "dc": drift.get("compensation", 0.0),
        "gzf": drift.get("gyroZFiltered", 0.0),
    }
