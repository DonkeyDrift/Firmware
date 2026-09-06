// Drifter Console 前端修复（libraries/mus4_web/src/WebConsoleAssets.h 内嵌 JS）行为级测试。
//
// 与 zcode_remote_url.test.mjs 同一范式：从 .h 提取真实函数实体源码（花括号配平，
// 跳过字符串/模板/正则/注释），放进 node:vm 沙箱执行——测的是固件里的真码，不是
// 测试里另抄一份实现；浏览器 API（WebSocket/DOM 元素/alert/fetch）全部打桩，
// waitWifiStaConnectionResult 的 800ms sleep 用立即执行的 setTimeout 打桩消解。
//
// 覆盖本批修复：
//  ① 设备重启 seq 回退重置：WS onmessage 的 hello 分支 + handleDataPayload 的 latest.seq 分支
//    （服务端遥测/日志 seq 是 RAM 计数器，重启归零；客户端只增不减会永久静默）；
//  ② explainCommandError/showCommandError 错误映射矩阵（含 403 {"error":"auth_required"}
//    新契约、JOYSTICK_INVALID_RANGE/JOYSTICK_SAVE_FAILED、NACK/JSON error 兜底原文）；
//  ③ staSsid input 监听清密码掩码占位（能提取，直接行为断言）+ renderStaPasswordState
//    的 SSID 失配跳过回填（防 5s 轮询把旧密码掩码复活成 keep_password=1）；
//  ④ handoff 成功 modal：标记改为显示成功后才写入，重复触发不重复弹；
//  ⑤ waitWifiStaConnectionResult：apply_pending=true 期间陈旧 connected/last_error 不评估；
//  ⑥ tub 录制批量帧逐点录入（points 末点与 latest 同 seq 由 tubLastSeq 去重，
//    TUB_MAX_SAMPLES 自动停不变）+ clearChart 停录回弹按钮并提示；
//  ⑦ joystickCalLive 在轮询刷新时同步写入（DONE 步骤"下方数值"不再空白）。
//
// 运行：node MUS4_FW/tests/web_console_fixes.test.mjs
// （pytest 包装见同目录 test_web_console_fixes_node.py）

import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const ASSETS = path.join(
  path.dirname(fileURLToPath(import.meta.url)),
  '..', 'libraries', 'mus4_web', 'src', 'WebConsoleAssets.h',
);
const src = fs.readFileSync(ASSETS, 'utf8');

// 花括号配平扫描：跳过字符串/模板/正则字面量与行/块注释——showWifiStaHandoffModal 里有
// 同时含 ' 和 / 的正则（/^http:\/\//），waitWifiStaConnectionResult 里嵌了块注释，
// 不识别这些状态会把括号/引号误判成结构符。
function scanBlockEnd(s, openIdx) {
  let depth = 0;
  let state = null; // null | "'" | '"' | '`' | 'regex' | 'line' | 'block'
  let prevSig = '('; // 上一个有效字符，用于区分 / 是除法还是正则起点
  for (let i = openIdx; i < s.length; i++) {
    const c = s[i];
    const n = s[i + 1];
    if (state === 'line') { if (c === '\n') state = null; continue; }
    if (state === 'block') { if (c === '*' && n === '/') { state = null; i++; } continue; }
    if (state) {
      if (c === '\\') { i++; continue; }
      if ((state === 'regex' && c === '/') || c === state) state = null;
      continue;
    }
    if (c === "'" || c === '"' || c === '`') { state = c; continue; }
    if (c === '/' && n === '/') { state = 'line'; i++; continue; }
    if (c === '/' && n === '*') { state = 'block'; i++; continue; }
    // 前一有效字符为 ( = , : ; ! & | ? { } 时 / 是正则起点
    if (c === '/' && '([=,:;!&|?{}'.includes(prevSig)) { state = 'regex'; continue; }
    if (c === '{') depth++;
    else if (c === '}') {
      depth--;
      if (depth === 0) return i;
    }
    if (!/\s/.test(c)) prevSig = c;
  }
  throw new Error('花括号未闭合（提取失败）');
}

function extractFn(s, signature) {
  const start = s.indexOf(signature);
  assert.ok(start >= 0, `未找到函数锚点 ${signature}（WebConsoleAssets.h 结构变了？）`);
  const openIdx = s.indexOf('{', start + signature.length);
  return s.slice(start, scanBlockEnd(s, openIdx) + 1);
}

// 顶层语句提取（staSsid input 监听不是函数）：从锚点扫到深度归零处的分号。
function extractStmt(s, anchor) {
  const start = s.indexOf(anchor);
  assert.ok(start >= 0, `未找到语句锚点 ${anchor}（WebConsoleAssets.h 结构变了？）`);
  let depth = 0;
  let state = null;
  let prevSig = '(';
  for (let i = start; i < s.length; i++) {
    const c = s[i];
    const n = s[i + 1];
    if (state === 'line') { if (c === '\n') state = null; continue; }
    if (state === 'block') { if (c === '*' && n === '/') { state = null; i++; } continue; }
    if (state) {
      if (c === '\\') { i++; continue; }
      if ((state === 'regex' && c === '/') || c === state) state = null;
      continue;
    }
    if (c === "'" || c === '"' || c === '`') { state = c; continue; }
    if (c === '/' && n === '/') { state = 'line'; i++; continue; }
    if (c === '/' && n === '*') { state = 'block'; i++; continue; }
    if (c === '/' && '([=,:;!&|?{}'.includes(prevSig)) { state = 'regex'; continue; }
    if (c === '(' || c === '[' || c === '{') depth++;
    else if (c === ')' || c === ']' || c === '}') depth--;
    else if (c === ';' && depth === 0) return s.slice(start, i + 1);
    if (!/\s/.test(c)) prevSig = c;
  }
  throw new Error(`语句未终止（提取失败）: ${anchor}`);
}

const FN_SIGNATURES = [
  'function resetDataSeqOnRollback(',
  'function handleDataPayload(', // 主控制台页在前，/judge 页同名函数在后，indexOf 取到的是前者
  'function latestPoint(',
  'function ts(',
  'function te(',
  'function tp(',
  'function clearChart(',
  'function explainCommandError(',
  'function showCommandError(',
  'function dataWsUrl(', // 同理取主控制台页版本
  'function connectDataSocket(',
  'function handoffStaUrl(',
  'function showWifiStaHandoffModal(',
  'async function waitWifiStaConnectionResult(',
  'function renderStaPasswordState(',
  'function parseJoystickCalStatus(',
  'function formatCalAxis(',
  'function renderCalStep(',
  'async function refreshJoystickCalStatus(',
];
const fnBlocks = FN_SIGNATURES.map((sig) => extractFn(src, sig));
const listenerStmt = extractStmt(src, "staSsid.addEventListener('input',");
const tubMaxConst = src.match(/const TUB_MAX_SAMPLES=\d+;/);
assert.ok(tubMaxConst, '未找到 TUB_MAX_SAMPLES 常量');

// 被测函数引用的顶层 let 状态（照 line 341 的声明子集），与提取的函数同一脚本作用域。
const driver = `
let lastLogSeq=0,lastDataSeq=0,tubRecording=false,tubSamples=[],tubStartedMs=0,tubStoppedMs=0,tubLastSeq=0,pointHead=0,pointCount=0,points=new Array(256),scrollOffset=0,smoothedDt=16,gridReady=false,chartPaused=false,screenSaverActive=false,dataTransport='poll',dataWs=null,dataWsConnected=false,dataWsReconnectDelay=500,dataPolling=false,staSelectedChannel=0,staPasswordPlaceholder=false,staPasswordDirty=false,staPasswordVisible=false,staSavedPassword='',staSavedPasswordKnown=false;
${tubMaxConst[0]}
${fnBlocks.join('\n')}
${listenerStmt}
;globalThis.__x = {
  resetDataSeqOnRollback, handleDataPayload, latestPoint, ts, te, tp, clearChart,
  explainCommandError, showCommandError, dataWsUrl, connectDataSocket,
  handoffStaUrl, showWifiStaHandoffModal, waitWifiStaConnectionResult,
  renderStaPasswordState, parseJoystickCalStatus, formatCalAxis, renderCalStep,
  refreshJoystickCalStatus,
  get state() {
    return { lastDataSeq, lastLogSeq, tubRecording, tubSamples, tubLastSeq, tubStartedMs,
      tubStoppedMs, staSelectedChannel, staPasswordPlaceholder, staPasswordDirty,
      staPasswordVisible, staSavedPassword, staSavedPasswordKnown, dataWsConnected, pointCount };
  },
  setSeq: (d, l) => { lastDataSeq = d; lastLogSeq = l; },
  setSta: (o) => {
    staSelectedChannel = o.ch || 0;
    staPasswordPlaceholder = !!o.ph;
    staPasswordDirty = !!o.dirty;
    staPasswordVisible = !!o.vis;
    staSavedPassword = o.saved || '';
    staSavedPasswordKnown = !!o.known;
  },
};`;

const CAL_TEXT = 'JOYSTICK_CAL steer_en=1 steer={100,1500,2000} throt_en=1 throt={200,1500,2100} state=3';

function makeEl() {
  const added = [];
  return {
    textContent: '', value: '', type: '', style: {}, added,
    classList: {
      add: (c) => added.push(c),
      remove() {},
      toggle() {},
      contains: () => false,
    },
  };
}

// 每个测试一套全新沙箱。opts.staScript：refreshWifiSta 依序返回的轮询脚本，
// 用完后兜底返回成功终态，保证等待循环必然退出（立即化 setTimeout 下没有真超时兜底）。
function makeEnv(opts = {}) {
  const calls = {
    line: [], alert: [], toast: [], rdl: [], tubMeta: [], addPoint: [], updateState: [],
    scheduleDraw: [], appendLog: [], eye: [], failure: [], refreshStatus: [], draw: [],
  };
  let staPolls = 0;
  const staScript = [...(opts.staScript || [])];
  const els = {
    thrMeta: makeEl(), strMeta: makeEl(), gzMeta: makeEl(), staNotice: makeEl(),
    cmd: makeEl(), staPassword: makeEl(), wifiStaHandoffText: makeEl(),
    wifiStaHandoffModal: makeEl(), joystickCalStatus: makeEl(), joystickCalLive: makeEl(),
    joystickCalStepText: makeEl(), joystickCalActionBtn: makeEl(),
    joystickCalRetryBtn: makeEl(), joystickCalSaveBtn: makeEl(),
  };
  const staSsid = Object.assign(makeEl(), {
    _handlers: {},
    addEventListener(type, fn) { this._handlers[type] = fn; },
  });
  els.staSsid = staSsid;
  const wsInstances = [];
  class FakeWebSocket {
    constructor(url) { this.url = url; this.sent = []; this.readyState = FakeWebSocket.OPEN; wsInstances.push(this); }
    send(m) { this.sent.push(m); }
    close() { this.readyState = FakeWebSocket.CLOSED; }
  }
  FakeWebSocket.OPEN = 1;
  FakeWebSocket.CLOSED = 3;
  const win = {};
  const sandbox = {
    location: { protocol: 'http:', hostname: '192.0.2.1' }, // RFC 5737 占位 IP
    window: win,
    document: { activeElement: null },
    WebSocket: FakeWebSocket,
    Blob: class {},
    fetch: async () => ({ text: async () => opts.calText ?? CAL_TEXT }),
    alert: (...a) => { calls.alert.push(a); },
    showToast: (...a) => { calls.toast.push(a); },
    line: (...a) => { calls.line.push(a); },
    appendLogLine: (...a) => { calls.appendLog.push(a); },
    t: (k) => k,
    // 立即执行的 setTimeout：消解 800ms 等待（onclose 的 setTimeout(pollData) 同理无害）
    setTimeout: (fn) => { fn(); return 1; },
    clearTimeout: () => {},
    addPoint: (p) => { calls.addPoint.push(p); },
    updateState: (p) => { calls.updateState.push(p); },
    scheduleDraw: () => { calls.scheduleDraw.push(1); },
    draw: () => { calls.draw.push(1); },
    updateTubMeta: () => { calls.tubMeta.push(1); },
    refreshDynamicLabels: () => { calls.rdl.push(1); },
    updateStaPasswordEye: () => { calls.eye.push(1); },
    refreshStatus: async () => { calls.refreshStatus.push(1); },
    showWifiStaFailureModal: (j) => { calls.failure.push(j); },
    refreshWifiSta: async () => {
      staPolls++;
      if (staScript.length) return staScript.shift();
      return { apply_pending: false, connected: true, sta_ip: '192.0.2.55', ssid: 'HomeNet' };
    },
    scheduleDataWsReconnect: () => {},
    pollData: () => {},
  };
  Object.assign(sandbox, els);
  vm.createContext(sandbox);
  vm.runInContext(driver, sandbox);
  return { x: sandbox.__x, calls, els, wsInstances, window: win, staPolls: () => staPolls };
}

let passed = 0;
const failures = [];
async function test(name, fn) {
  try {
    await fn();
    passed++;
    console.log(`ok ${passed + failures.length} - ${name}`);
  } catch (e) {
    failures.push([name, e]);
    console.error(`not ok ${passed + failures.length} - ${name}`);
    console.error(e);
  }
}

// ---------- ① 设备重启 seq 回退重置 ----------

await test('HTTP 轮询 latest.seq 回退（设备重启）：双 seq 重置并提示，本帧数据照常处理', () => {
  const env = makeEnv();
  env.x.setSeq(100, 50);
  env.x.handleDataPayload(
    { points: [{ seq: 5, t: 80, dt: 16, thr: 1, str: 2, gz: 0.1 }], latest: { seq: 6, t: 96, dt: 16, thr: 3, str: 4, gz: 0.2, ch6: 1500 } },
    'poll', 30);
  assert.equal(env.x.state.lastDataSeq, 6, '先重置为 0，再 Math.max 推进到本帧 6');
  assert.equal(env.x.state.lastLogSeq, 0);
  assert.ok(env.calls.line.some((m) => String(m[0]).includes('data.seqReset')), 'line 提示一行');
  assert.equal(env.calls.addPoint.length, 1, '本帧历史点照常上图');
  assert.equal(env.calls.updateState.length, 1, 'latest 照常更新状态卡片');
});

await test('latest.seq 不回退：不重置不提示，lastDataSeq 照常推进', () => {
  const env = makeEnv();
  env.x.setSeq(3, 2);
  env.x.handleDataPayload({ points: [], latest: { seq: 5, t: 1, ch6: 1500 } }, 'poll', 10);
  assert.equal(env.x.state.lastDataSeq, 5);
  assert.equal(env.x.state.lastLogSeq, 2);
  assert.equal(env.calls.line.length, 0);
});

await test('WS hello.seq 回退（设备重启后重连）：双 seq 重置 + 一行提示', () => {
  const env = makeEnv();
  env.x.setSeq(100, 50);
  env.x.connectDataSocket();
  const ws = env.wsInstances[0];
  assert.equal(ws.url, 'ws://192.0.2.1:81/');
  ws.onopen();
  assert.deepEqual(ws.sent, ['since:100'], '开打时仍带旧 seq');
  ws.onmessage({ data: '{"type":"hello","seq":3}' });
  assert.equal(env.x.state.lastDataSeq, 0);
  assert.equal(env.x.state.lastLogSeq, 0);
  assert.ok(env.calls.line.some((m) => String(m[0]).includes('data.seqReset')));
  assert.ok(!env.calls.line.some((m) => String(m[0]).includes('ws error')), '不应走 ws error 兜底');
});

await test('WS hello.seq 不回退（正常连接/网络闪断重连）：不重置不提示', () => {
  const env = makeEnv();
  env.x.setSeq(10, 4);
  env.x.connectDataSocket();
  const ws = env.wsInstances[0];
  ws.onopen();
  ws.onmessage({ data: '{"type":"hello","seq":42}' });
  assert.equal(env.x.state.lastDataSeq, 10);
  assert.equal(env.x.state.lastLogSeq, 4);
  assert.equal(env.calls.line.length, 0);
});

await test('WS hello 缺 seq 字段：不炸不重置', () => {
  const env = makeEnv();
  env.x.setSeq(10, 4);
  env.x.connectDataSocket();
  const ws = env.wsInstances[0];
  ws.onopen();
  ws.onmessage({ data: '{"type":"hello"}' });
  assert.equal(env.x.state.lastDataSeq, 10);
  assert.equal(env.x.state.lastLogSeq, 4);
});

await test('WS log 分支回归：日志行照常 append 且 lastLogSeq 推进', () => {
  const env = makeEnv();
  env.x.setSeq(0, 7);
  env.x.connectDataSocket();
  const ws = env.wsInstances[0];
  ws.onopen();
  ws.onmessage({ data: JSON.stringify({ type: 'log', seq: 9, t: 1, src: 'web', line: 'hi' }) });
  assert.equal(env.x.state.lastLogSeq, 9);
  assert.equal(env.calls.appendLog.length, 1);
  assert.ok(String(env.calls.appendLog[0][0]).includes('hi'));
});

// ---------- ② 命令错误映射矩阵 ----------

await test('explainCommandError 映射矩阵（含 403 auth_required 新契约与兜底原文）', () => {
  const env = makeEnv();
  const cases = [
    ['NACK:PARK_REQUIRED', 'error.parkRequired'],
    ['NACK:AUTH_REQUIRED', 'error.authRequired'],
    ['NACK:UNAUTHORIZED', 'error.authRequired'],
    ['{"error":"auth_required"}', 'error.authRequired'], // 新 403 契约，与大写 NACK 同路
    ['NACK:JOYSTICK_INVALID_RANGE steer_ok=0 thr_ok=1', 'error.joystickInvalidRange'],
    ['NACK:JOYSTICK_SAVE_FAILED', 'error.joystickSaveFailed'],
    ['NACK:EMPTY', 'NACK:EMPTY'], // 未映射 NACK 兜底原文展示
    ['NACK:UPDATE_FAILED', 'NACK:UPDATE_FAILED'],
    ['{"error":"boom"}', '{"error":"boom"}'], // 其它 JSON error 兜底原文展示
    ['OK', ''], // 正常命令输出不提示
    ['ACK:UPDATE_OK', ''],
    ['', ''],
    [null, ''],
    [undefined, ''],
  ];
  for (const [input, expected] of cases) {
    assert.equal(env.x.explainCommandError(input), expected, `input=${input}`);
  }
});

await test('showCommandError：有映射才弹窗（正常输出/空串/null 不弹）', () => {
  const env = makeEnv();
  env.x.showCommandError('NACK:PARK_REQUIRED');
  env.x.showCommandError('OK');
  env.x.showCommandError('');
  env.x.showCommandError(null);
  env.x.showCommandError('NACK:JOYSTICK_SAVE_FAILED');
  assert.deepEqual(env.calls.alert, [['error.parkRequired'], ['error.joystickSaveFailed']]);
});

// ---------- ③ STA 配网：改 SSID 清旧密码占位 ----------

await test('staSsid input：清掩码占位/dirty/已取密码，saveWifiSta 不再走 keep_password=1', () => {
  const env = makeEnv();
  env.els.staSsid.value = 'NewNet';
  env.els.staPassword.value = '********'; // 旧配置的掩码占位
  env.els.staPassword.type = 'text';
  env.x.setSta({ ch: 6, ph: true, dirty: false, vis: true, saved: 'oldpw', known: true });
  env.els.staSsid._handlers.input();
  const s = env.x.state;
  assert.equal(s.staSelectedChannel, 0);
  assert.equal(env.els.staPassword.value, '', '掩码占位清空');
  assert.equal(env.els.staPassword.type, 'password');
  assert.equal(s.staPasswordPlaceholder, false);
  assert.equal(s.staPasswordDirty, false);
  assert.equal(s.staPasswordVisible, false);
  assert.equal(s.staSavedPassword, '');
  assert.equal(s.staSavedPasswordKnown, false);
  assert.equal(env.calls.eye.length, 1);
  // saveWifiSta 的 keep_password 判定式：!dirty && (placeholder || known) —— 现在恒 false
  assert.equal(!s.staPasswordDirty && (s.staPasswordPlaceholder || s.staSavedPasswordKnown), false);
});

await test('renderStaPasswordState：SSID 已改动时跳过旧配置掩码回填（防 5s 轮询复活占位）', () => {
  const env = makeEnv();
  env.els.staSsid.value = 'NewNet';
  env.x.renderStaPasswordState({ ssid: 'OldNet', password_set: 1, password_len: 8 });
  assert.equal(env.els.staPassword.value, '', '不回填旧配置掩码');
  assert.equal(env.x.state.staPasswordPlaceholder, false);
  assert.equal(env.calls.eye.length, 0, '提前返回，不动眼睛按钮');
});

await test('renderStaPasswordState：SSID 未改动 / force 时照常回填', () => {
  const env = makeEnv();
  env.els.staSsid.value = 'OldNet';
  env.x.renderStaPasswordState({ ssid: 'OldNet', password_set: 1, password_len: 8 });
  assert.equal(env.els.staPassword.value, '********');
  assert.equal(env.x.state.staPasswordPlaceholder, true);
  env.els.staSsid.value = 'Other';
  env.x.renderStaPasswordState({ ssid: 'OldNet', password_set: 1, password_len: 6 }, true);
  assert.equal(env.els.staPassword.value, '******', 'force（开弹窗初始化）绕过守卫');
});

// ---------- ④ handoff 成功 modal 标记语义 ----------

await test('handoff modal：首次显示并把标记写入函数内部，重复触发不重复弹', () => {
  const env = makeEnv();
  env.els.staSsid.value = 'HomeNet';
  const j = { ssid: 'HomeNet', sta_ip: '192.0.2.55' };
  env.x.showWifiStaHandoffModal(j);
  assert.equal(env.els.wifiStaHandoffModal.added.filter((c) => c === 'show').length, 1);
  assert.equal(env.window.handoffShownForStaIp, '192.0.2.55', '显示成功后才写标记');
  assert.ok(env.els.wifiStaHandoffText.textContent.includes('192.0.2.55'));
  assert.ok(env.els.wifiStaHandoffText.textContent.includes('http://192.0.2.55/'));
  env.x.showWifiStaHandoffModal(j);
  assert.equal(env.els.wifiStaHandoffModal.added.filter((c) => c === 'show').length, 1, '同 IP 去重不重弹');
});

await test('handoff modal connecting 态：照常显示但不参与去重也不写标记', () => {
  const env = makeEnv();
  env.els.staSsid.value = 'HomeNet';
  env.x.showWifiStaHandoffModal({ ssid: 'HomeNet', connecting: true });
  assert.equal(env.els.wifiStaHandoffModal.added.length, 1);
  assert.equal(env.window.handoffShownForStaIp, undefined);
});

// ---------- ⑤ waitWifiStaConnectionResult 的 apply_pending 等待 ----------

await test('apply_pending 期间陈旧 last_error 不判失败、陈旧 connected 不判成功，false 后才评估', async () => {
  const env = makeEnv({
    staScript: [
      { apply_pending: true, connected: false, last_error: 'stale_err', last_error_message: 'stale' },
      { apply_pending: true, connected: true, sta_ip: '0.0.0.0' },
      { apply_pending: false, connected: true, sta_ip: '192.0.2.55', ssid: 'HomeNet' },
    ],
  });
  env.els.staSsid.value = 'HomeNet';
  const ok = await env.x.waitWifiStaConnectionResult();
  assert.equal(ok, true);
  assert.equal(env.staPolls(), 3, '两轮 apply_pending 后继续等');
  assert.equal(env.calls.failure.length, 0, '陈旧 last_error 不触发假失败');
  assert.equal(env.window.handoffShownForStaIp, '192.0.2.55', '成功路径 modal 真正弹出并写标记');
  assert.ok(env.els.wifiStaHandoffModal.added.includes('show'));
  assert.ok(env.els.wifiStaHandoffText.textContent.includes('http://192.0.2.55/'));
  assert.ok(env.calls.toast.length >= 1);
});

await test('apply_pending 结束后的真实失败：正常弹失败窗并清标记', async () => {
  const env = makeEnv({
    staScript: [
      { apply_pending: true, connected: true, sta_ip: '192.0.2.55', ssid: 'HomeNet' }, // 陈旧假成功
      { apply_pending: false, connected: false, last_error: 'auth', last_error_message: 'bad password' },
    ],
  });
  env.els.staSsid.value = 'HomeNet';
  const ok = await env.x.waitWifiStaConnectionResult();
  assert.equal(ok, false, '陈旧 connected 不算成功');
  assert.equal(env.staPolls(), 2);
  assert.equal(env.calls.failure.length, 1);
  assert.equal(env.window.handoffShownForStaIp, '');
});

// ---------- ⑥ tub 录制批量帧与 clearChart ----------

await test('tub 录制：一帧多个历史点逐点录入，points 末点与 latest 同 seq 去重', () => {
  const env = makeEnv();
  env.x.ts();
  env.x.handleDataPayload(
    { points: [
        { seq: 1, t: 1000, dt: 16, thr: 10, str: 20, gz: 0.1 },
        { seq: 2, t: 1016, dt: 16, thr: 11, str: 21, gz: 0.2 },
      ], latest: { seq: 2, t: 1016, dt: 16, thr: 11, str: 21, gz: 0.2, ch6: 1500 } },
    'ws', 0);
  assert.equal(env.x.state.tubSamples.length, 2, 'latest 与末点重复由 tubLastSeq 去重');
  // vm 跨 realm：tubSamples 是沙箱 realm 的数组，map 结果需转宿主数组再过 deepEqual
  assert.deepEqual(Array.from(env.x.state.tubSamples.map((p) => p.seq)), [1, 2]);
  assert.equal(env.x.state.tubStartedMs, 1000);
  assert.equal(env.x.state.tubLastSeq, 2);
  env.x.handleDataPayload(
    { points: [{ seq: 3, t: 1032, dt: 16, thr: 12, str: 22, gz: 0.3 }],
      latest: { seq: 4, t: 1048, dt: 16, thr: 13, str: 23, gz: 0.4, ch6: 1500 } },
    'ws', 0);
  assert.deepEqual(Array.from(env.x.state.tubSamples.map((p) => p.seq)), [1, 2, 3, 4], '第二帧历史点+新 latest 都进');
});

await test('tub 未录制时帧数据不进 tub', () => {
  const env = makeEnv();
  env.x.handleDataPayload({ points: [{ seq: 1, t: 1, dt: 16 }], latest: { seq: 2, t: 2, ch6: 1500 } }, 'poll', 10);
  assert.equal(env.x.state.tubSamples.length, 0);
});

await test('tub 紧凑历史点（无 ch6）也能录入——ch6 门槛已移除', () => {
  const env = makeEnv();
  env.x.ts();
  env.x.tp({ seq: 9, t: 900, dt: 16, thr: 1, str: 2, gz: 0.1 });
  assert.equal(env.x.state.tubSamples.length, 1);
});

await test('tub TUB_MAX_SAMPLES 自动停语义不变', () => {
  const env = makeEnv();
  env.x.ts();
  for (let i = 1; i <= 12050; i++) env.x.tp({ seq: i, t: i * 16, dt: 16 });
  assert.equal(env.x.state.tubSamples.length, 12000);
  assert.equal(env.x.state.tubRecording, false, '到量自动 te() 停录');
});

await test('clearChart：录制中被清空——停录 + refreshDynamicLabels 回弹按钮 + 一行提示', () => {
  const env = makeEnv();
  env.x.ts();
  env.x.tp({ seq: 1, t: 1, dt: 16 });
  env.calls.rdl.length = 0;
  env.x.clearChart();
  assert.equal(env.x.state.tubRecording, false);
  assert.equal(env.x.state.tubSamples.length, 0);
  assert.equal(env.calls.rdl.length, 1, '录制按钮从"停止录制"回弹');
  assert.ok(env.calls.line.some((m) => String(m[0]).includes('tub.clearedWhileRecording')));
});

await test('clearChart：未录制时清空——按钮照常刷新，无录制提示', () => {
  const env = makeEnv();
  env.x.clearChart();
  assert.equal(env.calls.rdl.length, 1);
  assert.ok(!env.calls.line.some((m) => String(m[0]).includes('tub.clearedWhileRecording')));
});

// ---------- ⑦ 手柄校准弹窗 live 行 ----------

await test('joystickCalLive：轮询解析成功后同步写入弹窗 live 行（DONE 态可见数值）', async () => {
  const env = makeEnv();
  await env.x.refreshJoystickCalStatus();
  const expected = 'cal.label.steering: 100 / 1500 / 2000 | cal.label.throttle: 200 / 1500 / 2100';
  assert.equal(env.els.joystickCalStatus.textContent, expected);
  assert.equal(env.els.joystickCalLive.textContent, expected, '弹窗内 live 行与弹窗外状态行一致');
  assert.equal(env.els.joystickCalSaveBtn.style.display, 'inline-block', 'state=3 DONE 显示保存按钮');
});

await test('joystickCalLive：解析失败时 live 行清空', async () => {
  const env = makeEnv({ calText: 'garbage' });
  await env.x.refreshJoystickCalStatus();
  assert.equal(env.els.joystickCalLive.textContent, '');
});

// ---------- 静态断言：i18n 成对与整页 <script> 语法 ----------

await test('静态断言：新增 i18n 键 zh/en 成对存在', () => {
  for (const k of ['data.seqReset', 'error.joystickInvalidRange', 'error.joystickSaveFailed', 'tub.clearedWhileRecording']) {
    assert.ok(src.includes(`I18N.zh['${k}']`), `缺 zh 键 ${k}`);
    assert.ok(src.includes(`I18N.en['${k}']`), `缺 en 键 ${k}`);
  }
});

await test('静态断言：整个 .h 内全部 <script> 块语法可编译（R"rawliteral 定界完好）', () => {
  const blocks = src.match(/<script>[\s\S]*?<\/script>/g) || [];
  assert.ok(blocks.length >= 4, `应至少有主控制台/judge/drift/OTA 四页脚本，实际 ${blocks.length}`);
  for (const b of blocks) {
    const code = b.slice('<script>'.length, -'</script>'.length);
    new vm.Script(code); // 仅编译不执行，语法错误在此抛出
  }
  assert.ok(src.includes(')rawliteral";'), 'R"rawliteral 收尾定界完好');
});

console.log(`\n${passed} passed, ${failures.length} failed`);
if (failures.length > 0) process.exit(1);
