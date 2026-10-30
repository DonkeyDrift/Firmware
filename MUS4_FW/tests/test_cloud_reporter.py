import pathlib
import re


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
REPORTER_PATH = (
    PROJECT_ROOT / "libraries" / "mus4_cloud" / "src" / "CloudReporter.cpp"
)


def _source():
    return REPORTER_PATH.read_text(encoding="utf-8")


def _function_block(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


def _call_block(source, signature):
    start = source.index(signature)
    opening = source.index("(", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "(":
            depth += 1
        elif source[index] == ")":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated call: {signature}")


def test_update_never_executes_blocking_report_on_main_loop():
    source = _source()
    update = _function_block(source, "void update()")

    assert "reportNow(" not in update
    assert ".POST(" not in update
    assert "xTaskCreatePinnedToCore" in update


def test_lazy_worker_uses_16384_byte_stack():
    source = _source()
    update = _function_block(source, "void update()")
    create = _call_block(update, "xTaskCreatePinnedToCore")
    arguments = create[create.index("(") + 1 : -1].split(",")
    stack = re.search(
        r"CLOUD_REPORT_TASK_STACK_SIZE\s*=\s*(\d+)", source
    )

    assert "void cloudReportTask(" in source
    assert arguments[2].strip() == "CLOUD_REPORT_TASK_STACK_SIZE"
    assert stack is not None
    assert int(stack.group(1)) == 16384
    assert int(stack.group(1)) >= 12288


def test_worker_and_blocking_attempt_do_not_call_logging_code():
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")
    attempt = _function_block(source, "reportNow(")

    for body in (worker, attempt):
        for log_call in ("mus4LogLine", "mus4Logf", "appendWebLog"):
            assert not re.search(rf"\b{log_call}\s*\(", body)


def test_worker_owns_network_objects_and_retry_state():
    worker = _function_block(_source(), "void cloudReportTask(")

    assert "WiFiClientSecure client;" in worker
    assert "HTTPClient http;" in worker
    assert "bool reportedOnce = false;" in worker
    assert "bool everSucceeded = false;" in worker
    assert "unsigned long lastReportMs = 0;" in worker


def test_worker_uses_main_loop_hostname_snapshot():
    source = _source()
    update = _function_block(source, "void update()")
    worker = _function_block(source, "void cloudReportTask(")
    body = _function_block(source, "buildReportBody(")
    attempt = _function_block(source, "reportNow(")
    create = _call_block(update, "xTaskCreatePinnedToCore")
    arguments = create[create.index("(") + 1 : -1].split(",")

    assert "WiFi.getHostname()" not in body
    assert "WiFi.getHostname()" in update
    assert update.index("WiFi.getHostname()") < update.index(
        "xTaskCreatePinnedToCore"
    )
    assert "char reportTaskHostname[" in source
    assert ".toCharArray(reportTaskHostname" in update
    assert update.index(".toCharArray(reportTaskHostname") < update.index(
        "xTaskCreatePinnedToCore"
    )
    assert arguments[3].strip() == "reportTaskHostname"
    assert re.search(
        r"String\s+hostname\s*\(\s*static_cast<const char\*>"
        r"\(parameter\)\s*\)",
        worker,
    )
    assert "WiFi.getHostname()" not in worker
    assert "buildReportBody(hostname)" in attempt
