"""v1.10.17 第三轮：开机早期建连窗口 + 连续块门限消除 OOM 悬崖。

第二轮实车（.9 车）结论：纪元常驻模型本身正确（稳态 keep-alive 复用成立），
但运行中期 5 次 `POST /report` 真实尝试全部 -1，每次尝试门限都过
（free8≈81KB、max_blk≈38.9KB），握手仍失败，且每次把 8BIT 池真实低水位打到
176–212 字节（OOM 悬崖）。交叉证据：v1.10.16 全程成功，靠的是开机早期堆
干净时建立常驻会话、之后不再重新握手。根因：运行期堆碎片化后，最大连续块
38.9KB 装不下 mbedTLS 握手峰值需求——旧门限只看 free8 总量与 17KB 单缓冲，
对「总量够、连续块不够」的碎片化形态完全失明。

第三轮语义（本文件锁定）：
  1. 开机早期建连窗口：尚未成功过（everSucceeded=false）且开机未满
     CLOUD_REPORT_EARLY_WINDOW_MS 时，重试节奏用更积极的
     CLOUD_REPORT_EARLY_RETRY_MS（15s 一试）抢住低碎片窗口；一旦成功立即回
     5 分钟稳态心跳；窗口结束后回落既有 60s→120s→300s 退避阶梯。
  2. 冷态建连门限为「早期窗口 / 运行期」双档（第四轮实车修正）：
     - 早期窗口（开机 <5 分钟）：free8 ≥ 40000 且 maxAlloc8 ≥ 34000——
       按实测开机画像（free8≈47–48.5KB、max_blk≈47KB）放行，先建立常驻
       会话；此前单档 64000/49000 在开机窗口就不满足，三次尝试全部
       `skip low heap`，干净窗口被关死；
     - 窗口结束后：free8 ≥ 64000 且 maxAlloc8 ≥ 49000（峰值 + 地板 +
       双缓冲与瞬态的连续块需求），不达标零分配跳过。
  3. 门限不满足零分配跳过，跳过原因可观测：free8 不足沿用「skip low heap」
     日志行，free8 足够但连续块不足新打「wait low-frag window」日志行
     （等待系统安静期自然去碎片后按既有节奏复查门限，不无限硬试）。
"""

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


# --- 1. 开机早期建连窗口：干净堆期积极重试，成功立即回稳态 ---


def test_early_window_constants_exist_with_aggressive_retry():
    """早期窗口常量：窗口时长量级为「几分钟」（碎片化由 Web/遥测客户端接入
    逐渐累积，干净窗口在开机后前几分钟内），窗口内重试间隔显著低于 60s
    退避基数（抢窗口），又不至于密到每次 5s 超时叠加阻塞 worker。"""
    source = _source()
    window = re.search(
        r"CLOUD_REPORT_EARLY_WINDOW_MS\s*=\s*(\d+)UL", source
    )
    early = re.search(r"CLOUD_REPORT_EARLY_RETRY_MS\s*=\s*(\d+)UL", source)
    assert window is not None, "缺少早期窗口时长常量"
    assert early is not None, "缺少早期窗口重试间隔常量"
    assert 120000 <= int(window.group(1)) <= 600000
    assert 8000 <= int(early.group(1)) <= 30000


def test_retry_interval_picks_early_cadence_then_ladder():
    """间隔选择为纯函数 retryIntervalMs：成功过 → 5 分钟；未成功且仍在开机
    早期窗口 → 积极重试间隔；否则 → 既有退避阶梯。判断顺序必须正确：
    先 everSucceeded（成功立即脱离窗口节奏），再窗口，再阶梯。"""
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")
    interval = _function_block(
        source, "static unsigned long retryIntervalMs("
    )

    assert "if (everSucceeded)" in interval
    assert "return CLOUD_REPORT_INTERVAL_MS;" in interval
    assert "return CLOUD_REPORT_EARLY_RETRY_MS;" in interval
    assert "return retryBackoffMs(consecutiveFailures);" in interval
    assert (
        interval.index("if (everSucceeded)")
        < interval.index("nowMs < CLOUD_REPORT_EARLY_WINDOW_MS")
        < interval.index("return CLOUD_REPORT_EARLY_RETRY_MS;")
        < interval.index("return retryBackoffMs(consecutiveFailures);")
    )

    # worker 的当次间隔与回传日志的 next= 均由同一函数推导（口径一致）
    assert worker.count("retryIntervalMs(now, everSucceeded, consecutiveFailures)") == 2


def test_first_attempt_still_immediate_and_epoch_model_untouched():
    """首次尝试仍在拿到 IP 后立即发起（!reportedOnce 触发），纪元常驻模型
    不因早期窗口改变：会话对象任务级构造、失败整体释放、成功置
    epochHealthy 复用 keep-alive。"""
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")
    loop_start = worker.index("for (;;")

    assert "!reportedOnce ||" in worker
    assert "WiFiClientSecure client;" in worker[:loop_start]
    assert "HTTPClient http;" in worker[:loop_start]
    assert "epochHealthy = true;" in worker
    assert "client.stop();" in worker
    assert worker.index("client.stop();") > loop_start


# --- 2. 门限收紧：峰值 + 地板双条件，拒绝实测失败形态 ---


def test_rebuild_gate_rejects_observed_fragmented_failure():
    """门限数值必须同时满足结构推算与实测拒绝：
    - maxAlloc8 门限 > 38900（第二轮 5 次 -1 悬崖案例的 max_blk 实测值），
      且 ≥ 2×16640+8000（两块 16.4KB 连续缓冲 + 瞬态下限）；
    - free8 门限 ≥ 48800+12000（实测峰值 + ≥12KB 残余地板）且 ≤ 70000
      （开机期无会话 free8 实测 ~81KB，门限不得高到早期窗口也进不去）。"""
    source = _source()
    free_gate = re.search(
        r"CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD\s*=\s*(\d+)", source
    )
    block_gate = re.search(
        r"CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD\s*=\s*(\d+)", source
    )
    assert free_gate is not None
    assert block_gate is not None
    free_v = int(free_gate.group(1))
    block_v = int(block_gate.group(1))

    assert block_v > 38900, "门限必须拒绝实测失败的 max_blk=38.9KB 形态"
    assert block_v >= 2 * 16640 + 8000, "门限须容纳两块 16.4KB 连续缓冲 + 瞬态"
    assert block_v <= 54000, "门限不得高过干净堆常态（开机期 max_blk≈free8≈81KB 有余量即可）"
    assert free_v >= 48800 + 12000, "free8 门限 = 峰值 + 残余地板"
    assert free_v <= 70000, "free8 门限不得把早期窗口关死"


def test_gate_split_into_free_floor_and_contiguous_predicates():
    """门限拆成两个纯谓词（free 地板 / 连续块），组合判据 reportHeapGateOk
    复用两者——跳过原因可区分、判据可单测。"""
    source = _source()
    free_floor = _function_block(source, "static bool reportHeapFreeFloorOk(")
    contiguous = _function_block(source, "static bool reportHeapContiguousOk(")
    gate = _function_block(source, "static bool reportHeapGateOk(")

    assert "earlyWindow ? CLOUD_REPORT_MIN_FREE_HEAP8_FOR_BOOT" in free_floor
    assert "CLOUD_REPORT_MIN_FREE_HEAP8_FOR_REBUILD" in free_floor
    assert "earlyWindow ? CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_BOOT" in contiguous
    assert "CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_REBUILD" in contiguous
    assert "reportHeapFreeFloorOk(free8, earlyWindow)" in gate
    assert "reportHeapContiguousOk(maxAlloc8, earlyWindow)" in gate


def test_cold_rebuild_skip_is_zero_alloc_with_reason_split():
    """冷态门限判断先于任何网络动作；free8 不足 → skippedLowHeap，
    free8 足够但连续块不足 → skippedFrag，两者都不触发 reportNow，
    且都不计入成功（不置 everSucceeded/epochHealthy）。"""
    source = _source()
    worker = _function_block(source, "void cloudReportTask(")

    gate_call = worker.index("!epochHealthy && !reportHeapGateOk(")
    attempt_call = worker.index("reportNow(client, http, hostname)")
    assert gate_call < attempt_call
    assert "result.skippedLowHeap = true;" in worker
    assert "result.skippedFrag = true;" in worker
    assert worker.index("result.skippedLowHeap = true;") < attempt_call
    assert worker.index("result.skippedFrag = true;") < attempt_call
    # 尝试分支同时排除两种跳过
    assert "if (!result.skippedLowHeap && !result.skippedFrag) {" in worker
    # 成功分类同样排除两种跳过
    ok_guard = worker.index("everSucceeded = true;")
    assert "result.skippedFrag" in worker[worker.rindex("if (", 0, ok_guard):ok_guard]


# --- 3. 可观测性：等待低碎片窗口的独立日志行 ---


def test_skip_reason_observability_in_emit_log():
    """主 loop 日志保留既有 skip low heap 行与全部追溯字段，新增
    「wait low-frag window」行（free8 足够、max_blk 不足的碎片化等待），
    两类跳过都带 free8/max_blk/next/fails 供部署期对表。"""
    source = _source()
    emit = _function_block(source, "static void emitPendingReportLog(")

    assert "skip low heap" in emit
    assert "wait low-frag window" in emit
    frag_line_start = emit.index("wait low-frag window")
    frag_line = emit[frag_line_start : emit.index("\n", frag_line_start)]
    assert "free8=%u" in frag_line
    assert "max_blk=%u" in frag_line
    assert "next=%lus" in frag_line
    assert "fails=%u" in frag_line
    # POD 回传通道补 skippedFrag（不跨任务共享 String）
    assert "bool skippedFrag;" in source
    assert "reportResultSkippedFrag = result.skippedFrag;" in source
    assert "skippedFrag = reportResultSkippedFrag;" in source


def test_boot_gate_matches_measured_boot_heap_profile():
    """第四轮实车修正：开机无会话画像 free8≈47–48.5KB、max_blk≈47KB，
    早期窗口门限必须放行（否则干净建连窗口被关死、常驻会话永远建不起来），
    连续块下限仍须容纳两块 16.4KB TLS 缓冲；窗口后严格门限不变。"""
    source = _source()
    free_boot = re.search(
        r"CLOUD_REPORT_MIN_FREE_HEAP8_FOR_BOOT\s*=\s*(\d+)", source
    )
    block_boot = re.search(
        r"CLOUD_REPORT_MIN_MAX_ALLOC8_FOR_BOOT\s*=\s*(\d+)", source
    )
    assert free_boot is not None and block_boot is not None
    free_v = int(free_boot.group(1))
    block_v = int(block_boot.group(1))
    assert free_v <= 46000, "早期门限必须低于实测开机 free8=47100/47300/48548"
    assert free_v >= 32768, "早期门限仍需保留总量地板"
    assert 2 * 16640 <= block_v <= 40000, "容纳双 16.4KB 缓冲且低于实测开机 max_blk≈47KB"

    worker = _function_block(source, "void cloudReportTask(")
    assert "now < CLOUD_REPORT_EARLY_WINDOW_MS" in worker
    assert "reportHeapGateOk(result.freeHeap8, result.maxAlloc8, earlyWindow)" in worker
    assert "reportHeapFreeFloorOk(result.freeHeap8, earlyWindow)" in worker


def test_helpers_and_worker_still_do_not_log_directly():
    """新增纯函数与 worker/attempt 一样不得直接写日志——跨任务约束不变。"""
    source = _source()
    for signature in (
        "void cloudReportTask(",
        "static ReportAttemptResult reportNow(",
        "static bool reportHeapGateOk(",
        "static bool reportHeapFreeFloorOk(",
        "static bool reportHeapContiguousOk(",
        "static unsigned long retryIntervalMs(",
    ):
        body = _function_block(source, signature)
        for log_call in ("mus4LogLine", "mus4Logf", "appendWebLog"):
            assert not re.search(rf"\b{log_call}\s*\(", body)


def test_main_loop_semantics_unchanged():
    """建连仍在 cloud worker 任务内：update() 不做同步网络操作，worker 仍
    钉在 core0（与 Wi-Fi/LWIP 同核），主 loop 语义不变。"""
    source = _source()
    update = _function_block(source, "void update()")

    assert "reportNow(" not in update
    assert ".POST(" not in update
    create = source[source.index("xTaskCreatePinnedToCore") :]
    args = create[create.index("(") + 1 : create.index(")")].split(",")
    assert args[0].strip() == "cloudReportTask"
    assert args[-1].strip() == "0"
