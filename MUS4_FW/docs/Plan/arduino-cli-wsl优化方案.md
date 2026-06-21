# arduino-cli-wsl.ps1 通用性与兼容性优化方案

## 1. 背景与现状问题

### 1.1 现状
当前 `arduino-cli-wsl.ps1` 是针对 MUS4 项目定制的 WSL 跨环境编译脚本，解决了直接在 `/mnt/c` NTFS 挂载分区下编译 Arduino 项目速度过慢的问题，核心思路是「源码同步到 WSL 原生 ext4 分区 → 编译 → 固件回传 Windows」。

### 1.2 现存通用性问题
| # | 问题点 | 影响范围 | 严重程度 |
|---|--------|----------|----------|
| 1 | 项目根路径硬编码为 `C:\Dev\DDC\mus4`、`/mnt/c/Dev/DDC/mus4` | 任何路径不同的环境均无法运行 | 🔴 高 |
| 2 | WSL 发行版名称硬编码为 `DKC` | 其他 WSL 发行版（如 Ubuntu、Debian 等）无法使用 | 🔴 高 |
| 3 | WSL 端 `arduino-cli` 路径硬编码为 `~/bin/arduino-cli` | 安装到其他路径（如全局 `/usr/local/bin`）的用户无法运行 | 🟡 中 |
| 4 | 默认 sketch 路径为 `mus4/mus4.ino`（旧版目录结构） | v1.1 版本源码已迁移到根目录，默认值失效 | 🟡 中 |
| 5 | 编译目标 FQBN 硬编码为 `esp32:esp32:esp32` | 其他开发板（如 FireBeetle 2 ESP32-E）无法直接切换 | 🟡 中 |
| 6 | 未读取项目已有配置（`config.yaml`、`sketch.yaml`） | 与 `arduino-cli.py` 的配置体系割裂，需要重复配置 | 🟡 中 |
| 7 | 无前置依赖检查 | WSL 未安装、rsync 不存在、arduino-cli 未安装等场景直接报无意义错误 | 🟡 中 |
| 8 | 库同步功能默认路径仅适配英文 Windows | `~/Documents/Arduino/libraries` 在中文系统中为 `~/文档/Arduino/libraries` | 🟠 中 |
| 9 | 无配置文件支持，常用参数每次都要手动传入 | 多项目、多环境切换效率低 | 🟢 低 |

---

## 2. 优化目标

1. **零配置开箱即用**：脚本放置到任意 Arduino 项目根目录即可运行，无需修改硬编码路径。
2. **配置即代码**：优先读取项目已有配置文件（`sketch.yaml`、`config.yaml`），避免重复维护。
3. **环境兼容性**：支持任意 WSL 发行版、任意 arduino-cli 安装路径、中英文 Windows 系统。
4. **向后兼容**：现有用法（直接运行、`-SyncLibs`、`-c/-u/-s` 等参数）保持不变，不破坏现有工作流。
5. **鲁棒性提升**：前置依赖检查不通过时给出明确的修复建议，而非抛出晦涩的底层错误。

---

## 3. 具体优化项

### 3.1 P0（必须完成，解决核心硬编码问题）

#### 3.1.1 自动探测项目根目录
- **方案**：脚本启动时以 `$PSScriptRoot`（脚本自身所在目录）作为默认项目根目录，同时支持 `--project-root` 参数覆盖。
- **对应 WSL 路径自动转换**：将 Windows 盘符路径（如 `C:\Dev\foo\bar`）自动转换为 `/mnt/c/Dev/foo/bar` 格式，避免硬编码。

#### 3.1.2 WSL 发行版可配置
- **新增参数**：`-Distro <string>`（默认值：自动探测默认 WSL 发行版，即 `wsl --list` 第一个带 `(Default)` 标记的发行版）。
- **环境变量覆盖**：支持 `WSL_DISTRO_NAME` 环境变量，适配 CI/CD 场景。
- **参数优先级**：命令行参数 > 环境变量 > 自动探测默认发行版。

#### 3.1.3 自动探测 sketch 路径
- **逻辑**：项目根目录下递归查找 `*.ino` 文件，若只有一个则自动使用；多个则提示用户通过 `--sketch` 指定。
- **默认值修正**：匹配 v1.1 目录结构，不再使用旧的 `mus4/mus4.ino`。

#### 3.1.4 WSL 端 arduino-cli 路径自动探测
- **方案**：在 WSL 内执行 `which arduino-cli` 获取实际路径，回退到 `~/bin/arduino-cli`。
- **失败时给出明确指引**：如「未在 WSL 中找到 arduino-cli，请参考 https://arduino.github.io/arduino-cli/latest/installation 安装」。

#### 3.1.5 FQBN 自动读取
- **优先级**：`--fqbn` 命令行参数 > `sketch.yaml` 中 `default_fqbn` 字段 > `config.yaml` 中 `build.fqbn` 字段 > 默认 `esp32:esp32:esp32`。

### 3.2 P1（提升兼容性与鲁棒性）

#### 3.2.1 前置依赖自检
脚本启动时自动检查以下依赖，不通过则直接退出并给出修复指引：
1. **Windows 端**：WSL 是否已启用、指定的发行版是否存在、Python 3 是否可用（用于后续上传）。
2. **WSL 端**：`rsync`、`arduino-cli`、`python3`、基础 Unix 工具（`find`、`wc`、`cp` 等）是否存在。

#### 3.2.2 Windows 库路径自动适配
- **方案**：通过 Windows API 或注册表获取「文档」文件夹实际路径（支持中英文系统），不再硬编码 `Documents`。
- **实现方式**：`[Environment]::GetFolderPath("MyDocuments")`，自动处理 `Documents` / `文档` 差异。

#### 3.2.3 路径处理统一化
- 封装 `ConvertTo-WslPath` / `ConvertTo-WinPath` 两个工具函数，处理盘符转换、斜杠归一化，避免散落各处的字符串替换逻辑出错。
- 所有涉及跨环境路径的地方均通过这两个函数处理。

### 3.3 P2（易用性提升）

#### 3.3.1 支持项目级配置文件
- 新增 `wslbuild.yaml` 项目配置文件，支持配置：
  ```yaml
  distro: DKC
  fqbn: esp32:esp32:dfrobot_firebeetle2_esp32e
  sketch: mus4.ino
  work_dir: ~/arduino-build/mus4
  sync_libs: true
  lib_exclude:
    - ^\.
    - ^tmp$
  extra_sync_args: --no-perms --no-owner
  ```
- 配置文件存在时自动加载，命令行参数优先级高于配置文件。

#### 3.3.2 性能模式可选
- 新增参数 `--io-mode`，可选值：
  - `native`（默认）：同步到 WSL 原生 ext4 分区编译，性能最优，即当前行为。
  - `mnt`：直接使用 `/mnt/c` 挂载分区编译，省去同步步骤，适合小项目快速编译。

#### 3.3.3 增量编译缓存
- 默认保留 WSL 端的 `build_wsl` 目录，利用 Arduino CLI 的增量编译能力，避免每次全量编译。
- 新增 `--clean` 参数，清理 WSL 端构建目录后重新编译。

#### 3.3.4 帮助信息完善
- 所有参数添加完整的 `Get-Help` 说明，支持 `-?` / `--help` 查看完整用法。

---

## 4. 配置与参数优先级约定

为避免多来源配置冲突，明确优先级顺序（高优先级覆盖低优先级）：

```
命令行参数 > 环境变量 > 项目 wslbuild.yaml > 项目 sketch.yaml / config.yaml > 脚本内置默认值
```

---

## 5. 实施步骤

### 阶段一：重构基础框架（P0）
1. 抽离路径转换、WSL 命令执行等公共逻辑为独立工具函数。
2. 实现项目根目录自动探测、WSL 发行版自动探测、arduino-cli 路径自动探测。
3. 实现 sketch 路径自动查找，替换原有硬编码。
4. 实现 FQBN 从配置文件读取逻辑。

### 阶段二：鲁棒性与兼容性（P1）
1. 实现前置依赖自检模块，每个失败项对应明确的修复指引。
2. 适配中文系统文档路径。
3. 统一所有跨环境路径处理逻辑，消除散落的字符串替换。

### 阶段三：易用性增强（P2）
1. 实现 `wslbuild.yaml` 配置文件加载逻辑。
2. 新增 `--io-mode`、`--clean` 参数。
3. 完善帮助文档与注释。
4. 处理所有边界条件（空项目、无 sketch、多 sketch、WSL 未启动等）。

---

## 6. 测试验证方案

### 兼容性测试用例
| 测试场景 | 预期结果 |
|----------|----------|
| 脚本移动到其他目录/其他项目运行 | 自动探测项目根，无需修改参数 |
| WSL 默认发行版为 Ubuntu（非 DKC） | 自动使用默认发行版编译成功 |
| arduino-cli 安装在 `/usr/local/bin` | 自动探测到路径，编译成功 |
| 中文 Windows 系统，库目录为 `文档/Arduino/libraries` | `-SyncLibs` 自动找到正确源路径 |
| 项目根目录只有一个 `.ino` 文件 | 自动选中，无需传入 `--sketch` |

### 错误场景测试用例
| 测试场景 | 预期结果 |
|----------|----------|
| 未启用 WSL | 明确提示「请先启用 WSL 功能」，退出码 1 |
| 指定的 WSL 发行版不存在 | 列出可用发行版列表，提示正确参数 |
| WSL 中未安装 rsync | 提示「请在 WSL 内执行 sudo apt install rsync」 |
| WSL 中未安装 arduino-cli | 给出官方安装链接指引 |
| 项目根目录找不到 `.ino` 文件 | 明确报错，提示指定 sketch 路径 |

### 回归测试用例
| 测试场景 | 预期结果 |
|----------|----------|
| 无参数直接运行脚本 | 行为与旧版一致：编译 + 上传 |
| `-SyncLibs` 参数 | 库同步逻辑与旧版完全一致 |
| `-c` / `-u` / `-s` 单独/组合使用 | 行为与旧版完全一致 |
| 原有自定义参数（`-WinLibPath`、`-WslLibPath` 等） | 保持可用，不破坏现有工作流 |

---

## 7. 向后兼容策略

1. **所有原有参数保持不变**：`-SyncLibs`、`-WinLibPath`、`-WslLibPath`、`-OverwriteLibs`、`-BackupLibs`、`-ExcludeLibs`、`-SyncMode`、`-ExtraArgs`、`-Serial`、`-Compile`、`-Upload`、`-Sketch` 等参数签名完全保留，默认值仅在用户未传入时走新的自动探测逻辑。
2. **硬编码路径仅作为兜底**：自动探测失败时才回退到旧的硬编码路径，并打印警告提示用户显式配置。
3. **配置文件可选**：没有 `wslbuild.yaml` 时完全走参数 + 默认逻辑，不强制新增配置。
4. **升级方式**：直接替换脚本文件即可，无需修改现有调用方式。
