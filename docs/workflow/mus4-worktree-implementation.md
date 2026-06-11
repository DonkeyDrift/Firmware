# MUS4 Worktree 并行开发实施方案

依据 `docs/workflow/git-worktree-parallel-development.md` 规范，已完成两个 Agent 的 Worktree 隔离部署。

---

## 部署结果

```text
C:/Dev/DDC/MUS4_FW/
├── .git/                                    ← 主仓库对象库
├── .worktrees/
│   ├── ui-design/        → feature/ui-design        ← UI Agent
│   └── sta-connectivity/ → feature/sta-connectivity ← STA Agent
└── ...
```

| Agent | 工作目录 | 分支 |
|-------|----------|------|
| UI-Design | `MUS4_FW/.worktrees/ui-design/` | `feature/ui-design` |
| STA-Connect | `MUS4_FW/.worktrees/sta-connectivity/` | `feature/sta-connectivity` |

基线提交：`99ba5a1`（v1.7.5-Trees + `.gitignore` 更新）

---

## 模块分工

### UI-Design Agent

**专注范围**：
- `WebConsoleAssets.h` — Web Console HTML/CSS/JS
- `WebConsoleServer.cpp` — HTTP 路由、API handler
- `WebTelemetry.cpp` — WebSocket 遥测推送
- `TUI.cpp` — ANSI 终端仪表盘
- `LedStatus.cpp` — LED 颜色与闪烁模式

**避免触碰**：
- `WifiStaConfig.cpp` / `WifiStaConfig.h`
- `WifiManager.cpp` 中的 STA 连接状态机
- `WifiIdentity.cpp`

### STA-Connect Agent

**专注范围**：
- `WifiStaConfig.cpp` / `WifiStaConfig.h` — STA 配置持久化
- `WifiManager.cpp` — STA 连接状态机、重连、漫游
- `WifiIdentity.cpp` — 身份标识
- `WifiOta.cpp` — OTA 网络保护
- `provisioning_system/` — 配网系统

**避免触碰**：
- `WebConsoleAssets.h`
- `TUI.cpp`
- `LedStatus.cpp`
- `WebTelemetry.cpp`

### 公共协商文件

| 文件 | 协商策略 |
|------|----------|
| `FirmwareConfig.h` | 只添加自己的编译开关，不改已有常量 |
| `SharedTypes.h` | 新增字段时提前同步 |
| `Mus4Log.cpp` | 可新增日志点，不改日志接口 |

---

## 快速命令

```bash
# 查看所有 worktree
git worktree list

# UI Agent 进入工作目录
cd C:/Dev/DDC/MUS4_FW/.worktrees/ui-design

# STA Agent 进入工作目录
cd C:/Dev/DDC/MUS4_FW/.worktrees/sta-connectivity

# 同步主分支最新代码
git fetch origin
git rebase origin/master

# 编译验证（在各工作目录内）
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino

# 合并完成后清理
git worktree remove .worktrees/ui-design
git worktree remove .worktrees/sta-connectivity
```
