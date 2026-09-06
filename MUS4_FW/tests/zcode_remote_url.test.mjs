// ZCode 远控链接逻辑（libraries/mus4_web/src/WebConsoleAssets.h 内嵌 JS）行为级测试。
//
// 直接从 .h 提取真实函数实体源码（花括号配平），放进 node:vm 沙箱执行——
// 测的是固件里的真码，不是测试里另抄一份实现；浏览器 API 全部打桩，
// 260ms 单击去抖定时器与 30s 取活链超时定时器均可手动推进。
// 与 DD 侧 EnterButtons.tsx 的 normalizeRemoteUrl 语义逐条对齐（同一功能的两侧实现）。
//
// v1.8.71 新交互流：单击先同步开 about:blank 占位标签（opener 置空），再 POST
// http://<launcherIp>:8000/api/zcode-remote/link 实时取活链（fetch + AbortController
// 30s 超时）；拿到活链则原样存档 + 占位标签原地导航（占位被拦则 window.open 直开
// 兜底）；取不到才回落 localStorage 存档现拼 / prompt 录入（此路径才唤醒桌面端），
// 都无果时关闭占位标签。
//
// 运行：node MUS4_FW/tests/zcode_remote_url.test.mjs
// （pytest 包装见同目录 test_zcode_remote_node.py；安全红线：只用占位链接，
//  绝不出现真实远控凭证）

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

// 提取 zcode 代码块：从 `let zcodeClickTimer=0;` 到 editZCodeUrl 函数结尾。
// 配平扫描要跳过字符串与正则字面量——normalize 里的去引号正则同时含 ' 和 "，
// 不识别正则状态会把引号误判成字符串起点。
function extractZcodeBlock(s) {
  const start = s.indexOf('let zcodeClickTimer=0;');
  assert.ok(start >= 0, '未找到 zcodeClickTimer 起点（WebConsoleAssets.h 结构变了？）');
  const fnAnchor = s.indexOf('function editZCodeUrl(){', start);
  assert.ok(fnAnchor > start, '未找到 editZCodeUrl 函数');
  const openIdx = s.indexOf('{', fnAnchor);
  let depth = 0;
  let state = null; // null | "'" | '"' | '`' | 'regex'
  let prevSig = '('; // 上一个有效字符，用于区分 / 是除法还是正则起点
  for (let i = openIdx; i < s.length; i++) {
    const c = s[i];
    if (state) {
      if (c === '\\') { i++; continue; }
      if ((state === 'regex' && c === '/') || c === state) state = null;
      continue;
    }
    if (c === "'" || c === '"' || c === '`') { state = c; continue; }
    // 该块无除法运算；前一有效字符为 ( = , : ; ! & | ? { } 时 / 是正则起点
    if (c === '/' && '([=,:;!&|?{}'.includes(prevSig)) { state = 'regex'; continue; }
    if (c === '{') depth++;
    else if (c === '}') {
      depth--;
      if (depth === 0) return s.slice(start, i + 1);
    }
    if (!/\s/.test(c)) prevSig = c;
  }
  throw new Error('editZCodeUrl 花括号未闭合（提取失败）');
}

const block = extractZcodeBlock(src);
for (const fn of ['zcodeRemoteGet', 'zcodeRemoteNormalize', 'zcodeRemoteFreshUrl',
  'zcodeRemoteCopy', 'zcodeRemoteWake', 'zcodeRemoteFetchLive', 'zcodeRemotePrompt',
  'openZCode', 'editZCodeUrl']) {
  assert.ok(block.includes(`function ${fn}(`), `提取的代码块缺 ${fn}`);
}
// let/函数声明在 vm 脚本顶层不落到 globalThis，末尾显式挂出测试触达面
const driver = `${block}\n;globalThis.__z = { openZCode, editZCodeUrl, zcodeRemoteNormalize, zcodeRemoteFreshUrl, zcodeRemoteGet, zcodeRemotePrompt, zcodeRemoteCopy, zcodeRemoteWake, zcodeRemoteFetchLive, getClickTimer: () => zcodeClickTimer };`;

// 构造一套全新沙箱（每个测试独立）：打桩 localStorage/window（含 about:blank
// 占位标签对象）/navigator/document/fetch（区分 :8000 取活链与 :8090 唤醒）/
// AbortController/showToast/line/t 与可手动推进的定时器队列。
// opts.live 控制取活链响应形态：'ok'（默认）| 'fail'（json null）| 'httpfail'
// （!ok）| 'badstatus'（status 非 ok）| 'nourl'（缺 url）| 'reject'（fetch 拒绝）
// | 'timeout'（挂起直到 abort）；opts.blockPopup 模拟占位标签被拦截。
function makeEnv(opts = {}) {
  const store = new Map();
  const calls = { open: [], prompt: [], alert: [], toast: [], line: [], fetch: [], copy: [] };
  const placeholders = [];
  const timers = new Map();
  let nextId = 1;
  const localStorage = {
    getItem: (k) => (store.has(k) ? store.get(k) : null),
    setItem: (k, v) => { store.set(k, String(v)); },
  };
  if (opts.getThrows) localStorage.getItem = () => { throw new Error('SecurityError'); };
  if (opts.setThrows) localStorage.setItem = () => { throw new Error('QuotaExceededError'); };
  const LIVE_URL = opts.liveUrl ?? LIVE;
  const sandbox = {
    URL, URLSearchParams,
    localStorage,
    _launcherIp: opts.launcherIp ?? '192.0.2.10', // RFC 5737 占位 IP
    AbortController: class {
      constructor() {
        this.signal = {
          aborted: false,
          _listeners: [],
          addEventListener(type, fn) { if (type === 'abort') this._listeners.push(fn); },
        };
      }
      abort() {
        this.signal.aborted = true;
        for (const fn of this.signal._listeners) fn();
      }
    },
    window: {
      open: (...a) => {
        calls.open.push(a);
        if (a[0] === 'about:blank') {
          if (opts.blockPopup) return null; // 占位标签被浏览器拦截
          const ph = {
            opener: 'SHOULD_BE_NULLED',
            location: { href: '' },
            closed: false,
            close() { this.closed = true; },
          };
          placeholders.push(ph);
          return ph;
        }
        return null; // 直开兜底：只记录调用，不返回句柄
      },
      prompt: (...a) => { calls.prompt.push(a); return opts.promptAnswer ?? null; },
    },
    alert: (...a) => { calls.alert.push(a); },
    navigator: opts.clipboard === 'ok'
      ? { clipboard: { writeText: (v) => { calls.copy.push(v); return Promise.resolve(); } } }
      : {},
    document: {
      createElement: () => ({ style: {}, value: '', select() {}, remove() {} }),
      body: { appendChild() {} },
      execCommand: opts.execThrows
        ? () => { throw new Error('execCommand unavailable'); }
        : () => true,
    },
    fetch: (url, init) => {
      calls.fetch.push([url, init]);
      if (String(url).includes(':8000/api/zcode-remote/link')) {
        const mode = opts.live ?? 'ok';
        if (mode === 'reject') return Promise.reject(new Error('network down'));
        if (mode === 'timeout') {
          return new Promise((_, rej) => {
            init.signal.addEventListener('abort', () => rej(new Error('AbortError')));
          });
        }
        if (mode === 'httpfail') return Promise.resolve({ ok: false, status: 502 });
        if (mode === 'badstatus') {
          return Promise.resolve({ ok: true, json: () => Promise.resolve({ status: 'error' }) });
        }
        if (mode === 'nourl') {
          return Promise.resolve({ ok: true, json: () => Promise.resolve({ status: 'ok' }) });
        }
        if (mode === 'fail') return Promise.resolve({ ok: true, json: () => Promise.resolve(null) });
        return Promise.resolve({ ok: true, json: () => Promise.resolve({ status: 'ok', url: LIVE_URL }) });
      }
      return Promise.resolve({ ok: true }); // :8090 唤醒等其它请求
    },
    showToast: (...a) => { calls.toast.push(a); },
    line: (...a) => { calls.line.push(a); },
    t: (k) => k,
    setTimeout: (fn) => { const id = nextId++; timers.set(id, fn); return id; },
    clearTimeout: (id) => { timers.delete(id); },
  };
  vm.createContext(sandbox);
  vm.runInContext(driver, sandbox);
  const flush = () => {
    const fns = [...timers.values()];
    timers.clear();
    for (const f of fns) f();
  };
  const liveFetches = () => calls.fetch.filter((c) => String(c[0]).includes(':8000/api/zcode-remote/link'));
  const wakeFetches = () => calls.fetch.filter((c) => String(c[0]).includes(':8090/api/launch/zcode-remote'));
  return { z: sandbox.__z, calls, store, flush, timers, placeholders, liveFetches, wakeFetches };
}

const VALID = 'https://zcode.z.ai/remote/v4?sid=placeholder-sid&hash=placeholder-hash&t=1';
const LIVE = 'https://zcode.z.ai/remote/v4?sid=live-sid&hash=live-hash&t=777';
const microtasks = () => new Promise((r) => setImmediate(r));

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

// ---------- zcodeRemoteNormalize 边界矩阵 ----------

await test('query 形式有效链接：sid/hash 保留、t 刷成当前毫秒戳', () => {
  const { z } = makeEnv();
  const u = new URL(z.zcodeRemoteNormalize(VALID));
  assert.equal(u.searchParams.get('sid'), 'placeholder-sid');
  assert.equal(u.searchParams.get('hash'), 'placeholder-hash');
  const t = Number(u.searchParams.get('t'));
  assert.ok(t > 1 && Math.abs(Date.now() - t) < 5000, `t 应为当前时间戳，实际 ${t}`);
});

await test('fragment 形式链接：参数归并进 query、hash 清空、t 刷新', () => {
  const { z } = makeEnv();
  const u = new URL(z.zcodeRemoteNormalize(
    'https://zcode.z.ai/remote/v4#sid=frag-sid&hash=frag-hash&t=9'));
  assert.equal(u.searchParams.get('sid'), 'frag-sid');
  assert.equal(u.searchParams.get('hash'), 'frag-hash');
  assert.equal(u.hash, '');
  assert.ok(Number(u.searchParams.get('t')) > 9);
});

await test('fragment 带路径形式（#/remote/v4?sid=…）：取 ? 后半归并', () => {
  const { z } = makeEnv();
  const u = new URL(z.zcodeRemoteNormalize(
    'https://zcode.z.ai/remote/v4#/remote/v4?sid=path-sid&hash=path-hash'));
  assert.equal(u.searchParams.get('sid'), 'path-sid');
  assert.equal(u.searchParams.get('hash'), 'path-hash');
});

await test('query 已有同名参数时 fragment 不覆盖', () => {
  const { z } = makeEnv();
  const u = new URL(z.zcodeRemoteNormalize(
    'https://zcode.z.ai/remote/v4?sid=query-sid&hash=H#sid=frag-sid&t=9'));
  assert.equal(u.searchParams.get('sid'), 'query-sid');
});

await test('fragment 不含 = 且 query 无凭证：拒绝', () => {
  const { z } = makeEnv();
  assert.equal(z.zcodeRemoteNormalize('https://zcode.z.ai/remote/v4#section'), '');
});

await test('remoteControlToken 链接原样通过（不刷 t）', () => {
  const { z } = makeEnv();
  const tokenUrl = 'https://zcode.z.ai/remote/v4?remoteControlToken=placeholder-token&t=1';
  assert.equal(z.zcodeRemoteNormalize(tokenUrl), tokenUrl);
});

await test('裸 /remote/v4 链接（缺 sid/hash）：拒绝', () => {
  const { z } = makeEnv();
  assert.equal(z.zcodeRemoteNormalize('https://zcode.z.ai/remote/v4'), '');
});

await test('缺 sid（只有 hash）：拒绝；缺 hash（只有 sid）：拒绝', () => {
  const { z } = makeEnv();
  assert.equal(z.zcodeRemoteNormalize('https://zcode.z.ai/remote/v4?hash=H'), '');
  assert.equal(z.zcodeRemoteNormalize('https://zcode.z.ai/remote/v4?sid=S'), '');
});

await test('非 https 链接：拒绝', () => {
  const { z } = makeEnv();
  assert.equal(z.zcodeRemoteNormalize('http://zcode.z.ai/remote/v4?sid=S&hash=H'), '');
});

await test('非 URL 垃圾输入：拒绝不炸', () => {
  const { z } = makeEnv();
  assert.equal(z.zcodeRemoteNormalize('not a url at all'), '');
});

await test('空串 / null / undefined / 0：拒绝不炸', () => {
  const { z } = makeEnv();
  assert.equal(z.zcodeRemoteNormalize(''), '');
  assert.equal(z.zcodeRemoteNormalize(null), '');
  assert.equal(z.zcodeRemoteNormalize(undefined), '');
  assert.equal(z.zcodeRemoteNormalize(0), '');
});

await test('首尾空白与引号包裹（半角/全角/书名号）被剥掉', () => {
  const { z } = makeEnv();
  for (const wrap of [(u) => `  ${u}  `, (u) => `"${u}"`, (u) => `'${u}'`,
    (u) => `「${u}」`, (u) => `“${u}”`, (u) => `‘${u}’`]) {
    const u = new URL(z.zcodeRemoteNormalize(wrap(VALID)));
    assert.equal(u.searchParams.get('sid'), 'placeholder-sid');
  }
});

// ---------- 单击 / 双击交互流（打桩定时器推进，v1.8.71 取活链新流程） ----------

await test('单击去抖：260ms 内不开标签，到点先同步开 about:blank 占位并置空 opener', () => {
  const env = makeEnv();
  env.z.openZCode();
  assert.equal(env.calls.open.length, 0, '260ms 去抖内不应立即打开');
  env.flush();
  assert.deepEqual(env.calls.open[0], ['about:blank', '_blank'], '先同步开占位标签');
  assert.equal(env.placeholders.length, 1);
  assert.equal(env.placeholders[0].opener, null, '占位标签 opener 置空');
});

await test('活链获取成功：原样存档 + 占位标签原地导航 + 复制，不 prompt 不唤醒不关占位', async () => {
  const env = makeEnv({ clipboard: 'ok' });
  env.z.openZCode();
  env.flush();
  assert.equal(env.liveFetches().length, 1, 'POST 取活链一次');
  assert.equal(env.liveFetches()[0][0], 'http://192.0.2.10:8000/api/zcode-remote/link');
  assert.equal(env.liveFetches()[0][1].method, 'POST');
  assert.ok(env.liveFetches()[0][1].signal, '带 AbortController signal（30s 超时）');
  await microtasks();
  assert.equal(env.store.get('zcodeRemoteUrl'), LIVE, '活链原样写入存档');
  assert.equal(env.placeholders[0].location.href, LIVE, '占位标签原地导航到活链');
  assert.equal(env.placeholders[0].closed, false, '成功路径不关占位');
  assert.equal(env.calls.prompt.length, 0);
  assert.equal(env.calls.alert.length, 0);
  assert.equal(env.wakeFetches().length, 0, '活链路径不唤醒（桌面端由 DD 后端代拉起）');
  assert.equal(env.calls.open.length, 1, '只有占位这一次 window.open');
  await microtasks();
  assert.equal(env.calls.copy.length, 1, 'clipboard API 复制一次');
  assert.equal(env.calls.toast.length, 1, '复制成功 toast');
});

await test('占位标签被拦截 + 活链成功：window.open 直开兜底，活链仍存档', async () => {
  const env = makeEnv({ blockPopup: true, clipboard: 'ok' });
  env.z.openZCode();
  env.flush();
  assert.deepEqual(env.calls.open[0], ['about:blank', '_blank']);
  await microtasks();
  assert.equal(env.calls.open.length, 2, '占位被拦后直开兜底');
  assert.deepEqual(env.calls.open[1], [LIVE, '_blank', 'noopener']);
  assert.equal(env.store.get('zcodeRemoteUrl'), LIVE, '活链仍写入存档');
});

await test('活链响应异常形态（!ok / status 非 ok / 缺 url / fetch 拒绝）一律回落存档流程', async () => {
  for (const mode of ['httpfail', 'badstatus', 'nourl', 'reject']) {
    const env = makeEnv({ live: mode, clipboard: 'ok' });
    env.store.set('zcodeRemoteUrl', VALID);
    env.z.openZCode();
    env.flush();
    await microtasks();
    assert.equal(env.liveFetches().length, 1, `${mode}: 尝试过取活链`);
    const opened = new URL(env.placeholders[0].location.href);
    assert.equal(opened.searchParams.get('sid'), 'placeholder-sid', `${mode}: 回落打开存档链接`);
    assert.ok(Number(opened.searchParams.get('t')) > 1, `${mode}: t 已刷新`);
    assert.equal(env.wakeFetches().length, 1, `${mode}: 回落路径唤醒桌面端`);
    assert.equal(env.wakeFetches()[0][1].method, 'POST');
    assert.equal(env.calls.prompt.length, 0, `${mode}: 有存档不 prompt`);
  }
});

await test('取活链 30s 超时（AbortController abort）：回落存档流程并唤醒', async () => {
  const env = makeEnv({ live: 'timeout' });
  env.store.set('zcodeRemoteUrl', VALID);
  env.z.openZCode();
  env.flush(); // 去抖到点：开占位 + 发起取活链（挂起等 abort）
  assert.equal(env.timers.size, 1, '30s 超时定时器在队列里');
  env.flush(); // 推进 30s：abort → fetch 拒绝 → catch 回调 null
  await microtasks();
  const opened = new URL(env.placeholders[0].location.href);
  assert.equal(opened.searchParams.get('sid'), 'placeholder-sid', '超时后回落打开存档链接');
  assert.equal(env.wakeFetches().length, 1);
});

await test('活链失败 + 无存档 + prompt 取消：关闭占位标签，不导航不唤醒不 alert', async () => {
  const env = makeEnv({ live: 'fail' }); // promptAnswer 缺省 null = 用户取消
  env.z.openZCode();
  env.flush();
  await microtasks();
  assert.equal(env.calls.prompt.length, 1);
  assert.equal(env.placeholders[0].location.href, '', '占位标签未导航');
  assert.equal(env.placeholders[0].closed, true, '无果时关闭占位标签');
  assert.equal(env.calls.alert.length, 0);
  assert.equal(env.wakeFetches().length, 0);
  assert.equal(env.calls.open.length, 1, '只有占位这一次 window.open');
});

await test('活链失败 + 无存档 + prompt 录入有效链接：保存归一化值并导航+唤醒', async () => {
  const env = makeEnv({ live: 'fail', clipboard: 'ok', promptAnswer: VALID });
  env.z.openZCode();
  env.flush();
  await microtasks();
  const stored = new URL(env.store.get('zcodeRemoteUrl'));
  assert.equal(stored.searchParams.get('sid'), 'placeholder-sid');
  assert.ok(Number(stored.searchParams.get('t')) > 1);
  const opened = new URL(env.placeholders[0].location.href);
  assert.equal(opened.searchParams.get('sid'), 'placeholder-sid');
  assert.equal(env.wakeFetches().length, 1);
  assert.equal(env.placeholders[0].closed, false);
  await microtasks();
  assert.equal(env.calls.toast.length, 1);
});

await test('活链失败 + prompt 录入裸链接：alert 提示、不保存、关闭占位不导航不唤醒', async () => {
  const env = makeEnv({ live: 'fail', promptAnswer: 'https://zcode.z.ai/remote/v4' });
  env.z.openZCode();
  env.flush();
  await microtasks();
  assert.deepEqual(env.calls.alert, [['zcode.remoteInvalid']]);
  assert.equal(env.store.has('zcodeRemoteUrl'), false);
  assert.equal(env.placeholders[0].location.href, '');
  assert.equal(env.placeholders[0].closed, true, '录入无效无果时关闭占位');
  assert.equal(env.wakeFetches().length, 0);
});

await test('prompt 预填：有效存档预填归一化链接，无效存档预填空串', async () => {
  const envValid = makeEnv({ promptAnswer: null });
  envValid.store.set('zcodeRemoteUrl', VALID);
  envValid.z.openZCode(); // 去抖定时器未触发，双击路径直接清掉，不会发取活链
  envValid.z.editZCodeUrl();
  const prefill = envValid.calls.prompt[0][1];
  assert.equal(new URL(prefill).searchParams.get('sid'), 'placeholder-sid');
  assert.equal(envValid.calls.fetch.length, 0, '双击重录不发任何 fetch');
  const envBad = makeEnv({ live: 'fail', promptAnswer: null });
  envBad.store.set('zcodeRemoteUrl', 'https://zcode.z.ai/remote/v4'); // 裸链接无效
  envBad.z.openZCode();
  envBad.flush();
  await microtasks();
  assert.equal(envBad.calls.prompt[0][1], '');
});

await test('localStorage.getItem 抛错（存储禁用）：活链失败后按无存档走 prompt，点击不失效', async () => {
  const env = makeEnv({ live: 'fail', getThrows: true, promptAnswer: VALID });
  env.z.openZCode();
  env.flush();
  await microtasks();
  assert.equal(env.calls.prompt.length, 1);
  const opened = new URL(env.placeholders[0].location.href);
  assert.equal(opened.searchParams.get('sid'), 'placeholder-sid', '录入链接正常导航');
});

await test('localStorage.setItem 抛错（隐私模式）：活链存档失败但本次仍正常导航+复制', async () => {
  const env = makeEnv({ clipboard: 'ok', setThrows: true });
  env.z.openZCode();
  env.flush();
  await microtasks();
  assert.equal(env.placeholders[0].location.href, LIVE, '活链照常导航');
  await microtasks();
  assert.equal(env.calls.toast.length, 1);
});

await test('快速连点两次：去抖守卫只留一个定时器，只开一个占位只取一次活链', async () => {
  const env = makeEnv();
  env.z.openZCode();
  env.z.openZCode(); // zcodeClickTimer 未清零时直接 return
  assert.equal(env.timers.size, 1);
  env.flush();
  await microtasks();
  assert.equal(env.calls.open.length, 1, '占位标签只开一个');
  assert.equal(env.liveFetches().length, 1, '活链只取一次');
  assert.equal(env.placeholders[0].location.href, LIVE);
});

await test('双击序列（click→click→dblclick）：定时器取消，只 prompt 不开占位不发 fetch', () => {
  const env = makeEnv({ promptAnswer: VALID });
  env.store.set('zcodeRemoteUrl', VALID);
  env.z.openZCode();
  env.z.openZCode();
  env.z.editZCodeUrl();
  env.flush();
  assert.equal(env.calls.prompt.length, 1);
  assert.equal(env.calls.open.length, 0, '现状：DC 双击只重录不打开（DD 侧会打开，见工程日志记录的不一致项）');
  assert.equal(env.calls.fetch.length, 0, '双击路径不取活链不唤醒');
  assert.ok(env.store.get('zcodeRemoteUrl').includes('placeholder-sid'));
});

await test('clipboard API 不可用且 execCommand 也失败：仅 log 一行，导航不受影响', async () => {
  const env = makeEnv({ execThrows: true }); // navigator 无 clipboard → 走降级
  env.z.openZCode();
  env.flush();
  await microtasks();
  assert.equal(env.placeholders[0].location.href, LIVE, '复制失败不阻塞导航');
  await microtasks();
  assert.equal(env.calls.toast.length, 0, '复制失败不应显示"已复制"toast');
  assert.ok(env.calls.line.some((m) => String(m[0]).includes('zcode remote copy failed')));
});

await test('_launcherIp 为空：取活链同步回落（不发 fetch），有存档正常导航、唤醒同样跳过', async () => {
  const env = makeEnv({ launcherIp: '' });
  env.store.set('zcodeRemoteUrl', VALID);
  env.z.openZCode();
  env.flush();
  await microtasks();
  assert.equal(env.calls.fetch.length, 0, '取活链与唤醒都不发 fetch');
  const opened = new URL(env.placeholders[0].location.href);
  assert.equal(opened.searchParams.get('sid'), 'placeholder-sid', '回落存档链接正常导航');
});

console.log(`\n${passed} passed, ${failures.length} failed`);
if (failures.length > 0) process.exit(1);
