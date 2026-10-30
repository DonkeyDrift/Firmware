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


def test_update_never_executes_blocking_report_on_main_loop():
    source = _source()
    update = _function_block(source, "void update()")

    assert "reportNow(" not in update
    assert ".POST(" not in update
    assert "xTaskCreatePinnedToCore" in update


def test_lazy_worker_uses_16384_byte_stack():
    source = _source()
    stack = re.search(
        r"CLOUD_REPORT_TASK_STACK_SIZE\s*=\s*(\d+)", source
    )

    assert "void cloudReportTask(" in source
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
