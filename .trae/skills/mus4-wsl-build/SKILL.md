---
name: mus4-wsl-build
description: 指导 MUS4 的 WSL 高速构建脚本用法，重点解释 .\arduino-cli-wsl.ps1 的 -c/-u/-s。用户要编译/烧录/串口监控或问这些参数时调用。
---

# MUS4 WSL 高速构建（arduino-cli-wsl.ps1）

用于本仓库的 `.\\arduino-cli-wsl.ps1`：把源码同步到 WSL 原生文件系统编译，并可在 Windows 侧触发上传与串口监视器。

## 入口脚本

- PowerShell 脚本：`c:\Dev\DDC\mus4\arduino-cli-wsl.ps1`
- 上传/串口由 Python 脚本执行：`c:\Dev\DDC\mus4\arduino-cli.py`
- 默认端口/板型/波特率从 `c:\Dev\DDC\mus4\config.yaml` 读取（除非你直接运行 `arduino-cli.py` 并覆盖参数）

## -c / -u / -s 的含义（在 arduino-cli-wsl.ps1 中）

### `-c`（Compile）

- 执行“WSL 内编译”流程：Windows → WSL 同步源码（rsync）→ WSL 编译（Linux arduino-cli）→ 把产物拷回 Windows。
- 产物默认写回：`c:\Dev\DDC\mus4\build_wsl\mus4.ino.bin`（以及 `.elf`）。
- 只加 `-c` 不会上传、不打开串口监控。

### `-u`（Upload）

- 执行“上传”流程，但上传动作本身在 Windows 侧调用 `arduino-cli.py` 完成。
- 该脚本会把 `build_wsl\mus4.ino.bin` 作为预编译固件传给 `arduino-cli.py`（等价于 `python arduino-cli.py -u -i build_wsl\mus4.ino.bin`）。
- 只加 `-u` 时不会自动先编译：要求 `build_wsl\mus4.ino.bin` 已存在（通常是你刚跑过 `-c` 或默认流程后）。

### `-s`（Serial）

- 只负责打开串口监视器（同样由 `arduino-cli.py -s` 实现）。
- 可以单独用：不会编译、不会上传；会直接打开串口监控。
- 和 `-u` 同时用时：会先上传，再延时 1 秒后打开监控。

### 默认行为（不带 -c/-u/-s）

- 如果你不带任何 `-c/-u/-s`，脚本会默认执行：编译 + 上传（不自动打开串口监控）。

## 常用命令（PowerShell）

### 1) 一键：编译 + 上传（默认）

```powershell
.\arduino-cli-wsl.ps1
```

### 2) 只编译（WSL 交叉编译，回传产物到 build_wsl）

```powershell
.\arduino-cli-wsl.ps1 -c
```

### 3) 编译 + 上传

```powershell
.\arduino-cli-wsl.ps1 -c -u
```

### 4) 编译 + 上传 + 串口监控

```powershell
.\arduino-cli-wsl.ps1 -c -u -s
```

### 5) 只打开串口监控

```powershell
.\arduino-cli-wsl.ps1 -s
```

### 6) 只上传（使用上一次编译出来的 build_wsl 固件）

```powershell
.\arduino-cli-wsl.ps1 -u
```

## 常见问题定位要点

- `-u` 报 “Binary file not found”：先跑一次 `.\arduino-cli-wsl.ps1 -c` 或直接跑默认 `.\arduino-cli-wsl.ps1`。
- 上传端口不对：改 `config.yaml` 的 `default.port`（例如 `COM9`），或直接运行 `python arduino-cli.py -u ... -p COMx` 做一次覆盖验证。
- WSL 编译失败：确认 WSL 发行版名为 `DKC`，并且 WSL 内存在 `~/bin/arduino-cli`。