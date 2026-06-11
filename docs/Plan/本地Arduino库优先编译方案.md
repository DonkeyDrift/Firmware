# 本地 Arduino 库优先编译方案

## 背景

项目已将 `mus4.ino` 依赖的第三方 Arduino 库复制到工作区 `libraries/`，用于代码同步和分享。当前 `arduino-cli.py` 与 `arduino-cli-wsl.ps1` 的编译命令未显式指定本地库路径，Arduino CLI 仍可能优先解析用户全局 Arduino 库，导致不同机器或不同库版本下编译结果不一致。

## 目标

让两条构建入口都优先使用工作区内的本地库：

```text
C:\Dev\FFE\Baoshan\mus4\libraries\
```

WSL native 编译时对应同步后的：

```text
$WSLWorkDir/libraries
```

## 推荐方案

使用 Arduino CLI 官方 `--libraries <路径>` 参数，不修改 `mus4.ino` 中的 `#include <...>` 写法。

原因：

- `--libraries` 会影响 Arduino CLI 的库解析路径，能覆盖第三方库内部 include 的依赖解析。
- 保持 Arduino 项目常规 include 风格，不把本地目录结构硬编码进源码。
- 对 WSL 和 Python 原生构建都适用。

## 配置

### `config.yaml`

在 `default` 下新增：

```yaml
libraries_path: "libraries"
```

供 `arduino-cli.py` 读取。

### `wslbuild.yaml`

新增：

```yaml
libraries_path: libraries
```

供 `arduino-cli-wsl.ps1` 读取。

## Python 原生构建行为

`arduino-cli.py` 在构造 compile 命令时：

1. 从配置读取 `default.libraries_path`，默认值为 `libraries`。
2. 将相对路径解析为项目根目录下的绝对路径。
3. 若目录存在，追加：

```bash
--libraries <项目根目录>/libraries
```

4. 若目录不存在，不追加该参数，保持旧行为。

## WSL 构建行为

`arduino-cli-wsl.ps1` 在构造 compile 命令时：

1. 从 `wslbuild.yaml` 读取 `libraries_path`，默认值为 `libraries`。
2. 检查 Windows 项目根目录下的库目录是否存在。
3. 若存在，按 I/O 模式选择 WSL 侧库路径：
   - `native`：`$WSLWorkDir/libraries`
   - `mnt`：`$WSLProjectRoot/libraries`
4. 追加：

```bash
--libraries "<WSL 侧 libraries 路径>"
```

5. 若目录不存在，不追加该参数，保持旧行为。

## 测试计划

先补红灯测试：

- `tests/test_arduino_cli.py`
  - 本地 `libraries/` 存在时，compile 命令包含 `--libraries` 和本地库路径。
  - 本地库目录不存在时，compile 命令不包含 `--libraries`。
- `tests/test_firmware_feature_flags.py`
  - 断言 `arduino-cli-wsl.ps1` 包含 `--libraries` 拼接逻辑。
  - 断言 `config.yaml` 与 `wslbuild.yaml` 包含 `libraries_path`。

验证命令：

```powershell
python -m pytest tests/test_arduino_cli.py
python -m pytest tests/test_firmware_feature_flags.py
.\arduino-cli-wsl.ps1 -Compile
```

## 非目标

- 不修改 `mus4.ino` 中的 `#include <FastLED.h>` 等 include 写法。
- 不复制或 vendor ESP32 Arduino core。
- 不改变上传、OTA、串口监视流程。
