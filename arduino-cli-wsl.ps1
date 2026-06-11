<#
.SYNOPSIS
    Fast WSL build script with optional library sync (universal version).
.DESCRIPTION
    Sync project to WSL native filesystem for compile, and optionally sync Arduino libs.
    Auto-detects project root, WSL distro, arduino-cli path, sketch file and FQBN.
.PARAMETER SyncLibs
    Enable library sync before build (default: off)
.PARAMETER WinLibPath
    Windows Arduino libraries path (default: auto-detect Documents/Arduino/libraries)
.PARAMETER WslLibPath
    Target libraries path in WSL (default: ~/Arduino/libraries)
.PARAMETER OverwriteLibs
    Overwrite existing libs in target (default: $true)
.PARAMETER BackupLibs
    Backup old libs before sync
.PARAMETER ExcludeLibs
    Exclude list (regex array)
.PARAMETER SyncMode
    Sync mode: rsync or robocopy (default: rsync)
.PARAMETER ExtraArgs
    Extra args for sync command
.PARAMETER Serial
    Open serial monitor after build
.PARAMETER Compile
    Run compile (alias: -c)
.PARAMETER Upload
    Run upload (alias: -u)
.PARAMETER Sketch
    Arduino Sketch file path (default: auto-detect)
.PARAMETER Distro
    WSL distro name to use (default: auto-detect default distro)
.PARAMETER ProjectRoot
    Project root directory (default: directory containing this script)
.PARAMETER FQBN
    Arduino board FQBN (default: read from sketch.yaml / config.yaml, fallback to esp32:esp32:esp32)
.PARAMETER NoCheck
    Skip pre-flight dependency check
#>
[CmdletBinding()]
param(
    # --- Library sync options ---
    [Parameter(HelpMessage="Enable library sync before build (default: off)")]
    [Alias("Sync")]
    [switch]$SyncLibs,

    [Parameter(HelpMessage="Windows Arduino libraries path")]
    [string]$WinLibPath,

    [Parameter(HelpMessage="Target libraries path in WSL")]
    [string]$WslLibPath = "~/Arduino/libraries",

    [Parameter(HelpMessage="Overwrite existing libs in target")]
    [bool]$OverwriteLibs = $true,

    [Parameter(HelpMessage="Backup old libs before sync")]
    [switch]$BackupLibs,

    [Parameter(HelpMessage="Exclude list (regex array)")]
    [string[]]$ExcludeLibs = @("^\.", "^tmp$"),

    [Parameter(HelpMessage="Sync mode: rsync or robocopy")]
    [ValidateSet("rsync", "robocopy")]
    [string]$SyncMode = "rsync",

    [Parameter(HelpMessage="Extra args for sync command")]
    [string]$ExtraArgs = "",

    [Parameter(HelpMessage="Open serial monitor")]
    [Alias("s")]
    [switch]$Serial,

    [Parameter(HelpMessage="Run compile")]
    [Alias("c")]
    [switch]$Compile,

    [Parameter(HelpMessage="Run upload")]
    [Alias("u")]
    [switch]$Upload,

    [Parameter(HelpMessage="Use ArduinoOTA for upload")]
    [switch]$Ota,

    [Parameter(HelpMessage="ArduinoOTA target host or IP")]
    [string]$OtaHost,

    [Parameter(HelpMessage="ArduinoOTA target port")]
    [int]$OtaPort = 3232,

    [Parameter(HelpMessage="ArduinoOTA password")]
    [string]$OtaPassword,

    [Parameter(HelpMessage="Path to espota.py")]
    [string]$EspotaTool,

    [Parameter(HelpMessage="Use HTTP /update endpoint for OTA upload (replaces ArduinoOTA)")]
    [switch]$HttpOta,

    [Parameter(HelpMessage="HTTP OTA target host or IP (default: read from .mus4_ota_target)")]
    [string]$HttpOtaHost,

    [Parameter(HelpMessage="HTTP OTA Web Console password (default: mus4-debug)")]
    [string]$HttpOtaPassword = "mus4-debug",

    [Parameter(HelpMessage="Arduino Sketch file path (default: auto-detect)")]
    [Alias("i")]
    [string]$Sketch,

    # --- New universal options ---
    [Parameter(HelpMessage="WSL distro name (default: auto-detect default distro)")]
    [string]$Distro,

    [Parameter(HelpMessage="Project root directory (default: script directory)")]
    [string]$ProjectRoot,

    [Parameter(HelpMessage="Arduino board FQBN (default: auto-detect from config files)")]
    [string]$FQBN,

    [Parameter(HelpMessage="Skip pre-flight dependency check (default: true)")]
    [switch]$NoCheck = $true,

    [Parameter(HelpMessage="Force run pre-flight dependency check")]
    [switch]$Check,

    [Parameter(HelpMessage="I/O mode: 'native' = sync to WSL ext4 for build (fast, default); 'mnt' = build directly on /mnt/c mounted NTFS (no sync, for small projects)")]
    [ValidateSet("native", "mnt")]
    [string]$IoMode = "native",

    [Parameter(HelpMessage="Clean build: remove WSL build directory before compiling")]
    [switch]$Clean,

    [Parameter(HelpMessage="Path to wslbuild.yaml config file (default: project root/wslbuild.yaml)")]
    [string]$Config,

    [Parameter(HelpMessage="Check firmware size against ESP32 partition table and give recommendations")]
    [switch]$CheckPartition
)

$ErrorActionPreference = "Stop"

# ==============================================================================
# 公共工具函数
# ==============================================================================

<#
.SYNOPSIS
    将 Windows 路径转换为 WSL 挂载路径格式
    例如: C:\Dev\foo\bar -> /mnt/c/Dev/foo/bar
#>
function ConvertTo-WslPath {
    param([Parameter(Mandatory=$true)][string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }

    # 处理已经是 WSL 路径的情况
    if ($Path.StartsWith("/") -or $Path.StartsWith("~")) {
        return $Path
    }

    # 规范化路径分隔符
    $normalized = $Path.Replace('\', '/')

    # 处理带盘符的路径
    if ($normalized -match '^([A-Za-z]):/(.*)$') {
        $drive = $matches[1].ToLower()
        $rest = $matches[2]
        return "/mnt/$drive/$rest"
    }

    return $normalized
}

<#
.SYNOPSIS
    将 WSL 路径转换为 Windows 路径格式
    例如: /mnt/c/Dev/foo/bar -> C:\Dev\foo\bar
#>
function ConvertTo-WinPath {
    param([Parameter(Mandatory=$true)][string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }

    # 处理已经是 Windows 路径的情况
    if ($Path -match '^[A-Za-z]:\\' -or $Path -match '^[A-Za-z]:/') {
        return $Path.Replace('/', '\')
    }

    # 处理 /mnt/x/... 格式
    if ($Path -match '^/mnt/([a-z])/(.*)$') {
        $drive = $matches[1].ToUpper()
        $rest = $matches[2].Replace('/', '\')
        return "${drive}:\$rest"
    }

    return $Path.Replace('/', '\')
}

<#
.SYNOPSIS
    在指定 WSL 发行版中执行命令，返回输出结果
    使用 bash -lc (login shell) 加载 .profile，确保用户 PATH（如 ~/bin）完整
#>
function Invoke-WslCommand {
    param(
        [Parameter(Mandatory=$true)][string]$Command,
        [string]$Distro = $script:WslDistro,
        [switch]$IgnoreExitCode
    )

    $wslArgs = @()
    if (-not [string]::IsNullOrWhiteSpace($Distro)) {
        $wslArgs += @("-d", $Distro)
    }
    # 使用 -lc 而非 -c，加载登录配置文件，确保 PATH 包含 ~/bin 等用户自定义路径
    $wslArgs += @("bash", "-lc", $Command)

    # WSL 会将非致命提示（如代理配置警告）输出到 stderr，
    # PowerShell 的 2>&1 会将其包装为 ErrorRecord，
    # 临时切换为 Continue 避免 Stop 模式将其误判为终止错误
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $rawOutput = & wsl @wslArgs 2>&1
    } finally {
        $ErrorActionPreference = $prevEAP
    }

    # 将所有输出统一为字符串，并过滤 WSL 非致命提示行
    # WSL 代理警告特征：行首以 "wsl:" 或 "wsl.exe" 开头，或含 localhost/代理/NAT/回退 等关键字
    $wslInfoPattern = '(?i)^wsl[:.]\s|localhost|proxy|代理|回退.*NAT|NAT.*回退|请检查|检测到'
    $result = @($rawOutput | ForEach-Object {
        if ($_ -is [System.Management.Automation.ErrorRecord]) { $_.Exception.Message } else { "$_" }
    } | Where-Object {
        [string]::IsNullOrWhiteSpace($_) -or $_ -notmatch $wslInfoPattern
    })

    if (-not $IgnoreExitCode -and $LASTEXITCODE -ne 0) {
        Write-Error "WSL command failed: $Command`n$result"
    }
    return $result
}

<#
.SYNOPSIS
    执行命令并显示带动画的进度条
#>
function Run-WithAnimation {
    param(
        [string]$Command,
        [string]$Arguments,
        [string]$TaskName
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $Command
    $psi.Arguments = $Arguments
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.StandardOutputEncoding = [System.Text.Encoding]::UTF8
    $psi.StandardErrorEncoding = [System.Text.Encoding]::UTF8

    try {
        $p = [System.Diagnostics.Process]::Start($psi)
    } catch {
        Write-Host "无法启动命令: $Command $Arguments" -ForegroundColor Red
        return $false
    }

    $startTime = Get-Date
    $frameIdx = 0
    $frames = @('|', '/', '-', '\')
    # ANSI 转义序列：\r 回行首 + \e[K 清行尾
    $esc = [char]0x1b

    try { [Console]::CursorVisible = $false } catch {}

    $outputBuffer = New-Object System.Text.StringBuilder
    $errorBuffer = New-Object System.Text.StringBuilder

    $outEvent = { if (-not [string]::IsNullOrEmpty($EventArgs.Data)) { $null = $Event.MessageData.outputBuffer.AppendLine($EventArgs.Data) } }
    # 过滤 WSL 非致命提示（代理/NAT/网络警告），仅保留真正的命令错误
    $wslWarnPattern = '(?i)^wsl[:.]\s|localhost|proxy|代理|回退.*NAT|NAT.*回退|请检查|检测到'
    $errEvent = {
        if (-not [string]::IsNullOrEmpty($EventArgs.Data)) {
            if ($EventArgs.Data -match $wslWarnPattern) { return }
            $null = $Event.MessageData.errorBuffer.AppendLine($EventArgs.Data)
        }
    }
    $eventData = @{ outputBuffer = $outputBuffer; errorBuffer = $errorBuffer }

    $sub1 = Register-ObjectEvent -InputObject $p -EventName OutputDataReceived -Action $outEvent -MessageData $eventData
    $sub2 = Register-ObjectEvent -InputObject $p -EventName ErrorDataReceived -Action $errEvent -MessageData $eventData

    $p.BeginOutputReadLine()
    $p.BeginErrorReadLine()

    while (-not $p.HasExited) {
        $elapsed = (Get-Date) - $startTime
        $timeStr = "{0:mm}:{0:ss}" -f $elapsed
        $frame = $frames[$frameIdx % $frames.Length]
        $frameIdx++
        # 用 [Console]::Write 直接写控制台，\r 回行首 + ESC[K 清行尾，彻底避免残留字符
        [Console]::Write("`r$frame $TaskName... ($timeStr)$esc[K")
        Start-Sleep -Milliseconds 150
    }

    $p.WaitForExit()
    try { [Console]::CursorVisible = $true } catch {}

    Unregister-Event -SourceIdentifier $sub1.Name -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier $sub2.Name -ErrorAction SilentlyContinue
    $sub1 | Remove-Job -ErrorAction SilentlyContinue
    $sub2 | Remove-Job -ErrorAction SilentlyContinue

    # 用 \r + ESC[K 清除动画行，在同一行打印最终结果
    # 统一使用 [Console]::Write 避免与 Write-Host 的光标位置不同步导致后续输出错位
    if ($p.ExitCode -ne 0) {
        [Console]::Write("`r$esc[K$esc[31mX $TaskName - Failed!$esc[0m`n")
        Write-Host "Error Code: $($p.ExitCode)" -ForegroundColor Red
        Write-Host "---------------- Error Output ----------------" -ForegroundColor Yellow
        Write-Host $errorBuffer.ToString() -ForegroundColor Yellow
        Write-Host "----------------------------------------------" -ForegroundColor Yellow
        return $false
    } else {
        [Console]::Write("`r$esc[K$esc[32m  $TaskName - Done!$esc[0m`n")
        return $true
    }
}

# ==============================================================================
# 分区大小检查模块
# ==============================================================================

<#
.SYNOPSIS
    从 FQBN 字符串中提取 PartitionScheme
#>
function Get-PartitionSchemeFromFQBN {
    param([string]$FQBN)

    if ($FQBN -match 'PartitionScheme=([^:,]+)') {
        return $matches[1]
    }
    return "default"
}

<#
.SYNOPSIS
    在 WSL 中查找 ESP32 平台分区表目录
#>
function Find-Esp32PartitionTableDir {
    $searchPatterns = @(
        "`$HOME/.arduino15/packages/esp32/hardware/esp32/*/tools/partitions"
        "`$HOME/Arduino/packages/esp32/hardware/esp32/*/tools/partitions"
        "/usr/share/arduino/packages/esp32/hardware/esp32/*/tools/partitions"
    )

    foreach ($pattern in $searchPatterns) {
        $result = Invoke-WslCommand -Command "ls -d $pattern 2>/dev/null | head -1" -IgnoreExitCode
        $cleanResult = ($result | Out-String).Trim()
        if (-not [string]::IsNullOrWhiteSpace($cleanResult) -and $cleanResult -match 'partitions$') {
            return $cleanResult
        }
    }
    return $null
}

<#
.SYNOPSIS
    读取分区表 CSV，返回 app0 大小（字节）
#>
function Get-AppPartitionSize {
    param([string]$PartitionDir, [string]$Scheme)

    $csvPath = "$PartitionDir/$Scheme.csv"
    $content = Invoke-WslCommand -Command "cat `"$csvPath`" 2>/dev/null || echo ''" -IgnoreExitCode
    $cleanContent = ($content | Out-String).Trim()

    if ([string]::IsNullOrWhiteSpace($cleanContent)) {
        return 0
    }

    foreach ($line in $cleanContent -split "`n") {
        $trimmed = $line.Trim()
        if ($trimmed.StartsWith('#') -or [string]::IsNullOrWhiteSpace($trimmed)) { continue }

        $parts = $trimmed -split ','
        if ($parts.Count -ge 5) {
            $name = $parts[0].Trim()
            $type = $parts[1].Trim()

            if ($name -eq 'app0' -and $type -eq 'app') {
                $sizeStr = $parts[4].Trim()
                # 支持 0x 前缀的十六进制
                if ($sizeStr -match '^0x([0-9a-fA-F]+)$') {
                    return [Convert]::ToInt32($matches[1], 16)
                }
                # 支持十进制
                if ($sizeStr -match '^\d+$') {
                    return [int]$sizeStr
                }
            }
        }
    }
    return 0
}

<#
.SYNOPSIS
    获取固件大小估算值（字节）。优先编译产物，其次历史 build 目录
#>
function Get-FirmwareSizeEstimate {
    param([string]$ProjectRoot, [string]$BuildDir = "build_wsl")

    $searchDirs = @(
        (Join-Path $ProjectRoot $BuildDir)
        (Join-Path $ProjectRoot "build")
    )

    foreach ($dir in $searchDirs) {
        if (Test-Path $dir) {
            $bin = Get-ChildItem -Path $dir -Filter "*.bin" -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -notmatch '\.(bootloader|partitions|merged)\.bin$' } |
                Sort-Object LastWriteTime -Descending |
                Select-Object -First 1

            if ($bin) {
                return @{
                    Size = $bin.Length
                    Path = $bin.FullName
                    LastWrite = $bin.LastWriteTime
                }
            }
        }
    }
    return $null
}

<#
.SYNOPSIS
    执行固件大小与分区匹配度检查，并输出评估报告
#>
function Invoke-PartitionFitCheck {
    param(
        [string]$FQBN,
        [string]$ProjectRoot,
        [string]$BuildDir = "build_wsl",
        [switch]$PreBuild
    )

    $partitionScheme = Get-PartitionSchemeFromFQBN -FQBN $FQBN
    $partitionDir = Find-Esp32PartitionTableDir

    if (-not $partitionDir) {
        Write-Warning "无法定位 ESP32 分区表目录，跳过分区检查"
        return
    }

    $appSize = Get-AppPartitionSize -PartitionDir $partitionDir -Scheme $partitionScheme
    if ($appSize -eq 0) {
        Write-Warning "未找到分区方案 '$partitionScheme' 的分区表，跳过检查"
        return
    }

    $fwInfo = Get-FirmwareSizeEstimate -ProjectRoot $ProjectRoot -BuildDir $BuildDir

    Write-Host "`n=== Partition Fit Report ===" -ForegroundColor Cyan
    Write-Host "FQBN:        $FQBN" -ForegroundColor Gray
    Write-Host "Partition:   $partitionScheme" -ForegroundColor Gray
    Write-Host "App0 limit:  $($appSize) bytes ($([math]::Round($appSize/1KB,1)) KB / $([math]::Round($appSize/1MB,2)) MB)" -ForegroundColor Gray

    if (-not $fwInfo) {
        Write-Host "Firmware:    (无历史构建产物，编译后将再次检查)" -ForegroundColor Yellow
        Write-Host "==============================`n" -ForegroundColor Cyan
        return
    }

    $fwSize = $fwInfo.Size
    $pct = [math]::Round(($fwSize / $appSize) * 100, 1)

    $phaseLabel = if ($PreBuild) { "预估（基于历史产物）" } else { "实际" }
    Write-Host "Firmware:    $($fwSize) bytes ($([math]::Round($fwSize/1KB,1)) KB) [$phaseLabel]" -ForegroundColor Gray
    Write-Host "Utilization: $pct%" -ForegroundColor $(if ($pct -gt 95) { "Red" } elseif ($pct -gt 80) { "Yellow" } else { "Green" })

    # 评估与建议
    $recommendations = @()

    if ($pct -gt 100) {
        Write-Host "Status:      CRITICAL - 固件已超过 app0 分区大小，编译或上传必然失败!" -ForegroundColor Red
        $recommendations += "当前固件 ($([math]::Round($fwSize/1KB,1)) KB) 超过 app0 分区 ($([math]::Round($appSize/1KB,1)) KB)"
    } elseif ($pct -gt 95) {
        Write-Host "Status:      WARNING - 固件非常接近分区上限，OTA 可能失败（OTA 需要约 2x 空闲空间）" -ForegroundColor Red
        $recommendations += "空间极度紧张，建议更换分区方案"
    } elseif ($pct -gt 80) {
        Write-Host "Status:      CAUTION - 固件较大，OTA 升级时需谨慎" -ForegroundColor Yellow
    } else {
        Write-Host "Status:      OK - 空间充足" -ForegroundColor Green
    }

    # 给出分区调整建议
    if ($pct -gt 80 -or $recommendations.Count -gt 0) {
        Write-Host "`nRecommendations:" -ForegroundColor Cyan

        if ($partitionScheme -ne 'huge_app' -and $partitionScheme -ne 'max_app_8MB') {
            if ($pct -gt 95) {
                Write-Host "  - 若不需要 OTA，改用 huge_app 分区（单 app 约 3MB，无 OTA 备份区）:" -ForegroundColor Yellow
                Write-Host "    --build-property build.partitions=huge_app" -ForegroundColor Gray
            } elseif ($pct -gt 80) {
                Write-Host "  - 若 SPIFFS 空间需求不大，改用 min_spiffs（app 约 1.875MB，SPIFFS 仅 128KB）:" -ForegroundColor Yellow
                Write-Host "    --build-property build.partitions=min_spiffs" -ForegroundColor Gray
            }
        }

        if ($partitionScheme -eq 'huge_app' -and $pct -gt 95) {
            Write-Host "  - 当前已是最大单 app 分区，考虑以下方案:" -ForegroundColor Yellow
            Write-Host "    1) 精简固件：检查是否启用了未使用的库或功能" -ForegroundColor Gray
            Write-Host "    2) 使用 8MB Flash 开发板 + default_8MB 分区（app 约 3.25MB）" -ForegroundColor Gray
            Write-Host "    3) 修改 FQBN 为 esp32:esp32:esp32:PartitionScheme=default_8MB" -ForegroundColor Gray
        }

        if ($partitionScheme -eq 'default' -and $pct -gt 70) {
            Write-Host "  - 从 default (1.25MB) 升级到 min_spiffs (1.875MB) 可提升 50% 空间:" -ForegroundColor Yellow
            Write-Host "    FQBN 追加 :PartitionScheme=min_spiffs" -ForegroundColor Gray
        }

        if ($recommendations.Count -gt 0) {
            foreach ($rec in $recommendations) {
                Write-Host "  - $rec" -ForegroundColor Yellow
            }
        }
    }

    Write-Host "==============================`n" -ForegroundColor Cyan
}

# ==============================================================================
# HTTP OTA 上传模块
# ==============================================================================

<#
.SYNOPSIS
    读取项目根目录下的 .mus4_ota_target 文件，获取默认 HTTP OTA 目标主机
#>
function Get-HttpOtaTarget {
    param([string]$ProjectRoot)
    $targetFile = Join-Path $ProjectRoot ".mus4_ota_target"
    if (Test-Path $targetFile) {
        $content = Get-Content $targetFile -Raw
        $firstLine = ($content -split "`r?`n")[0].Trim()
        if (-not [string]::IsNullOrWhiteSpace($firstLine)) {
            return $firstLine
        }
    }
    return $null
}

<#
.SYNOPSIS
    使用 curl.exe 通过 HTTP /update 端点上传固件
    curl 自带进度条输出到 stderr，直接显示在控制台
#>
function Invoke-HttpOtaUpload {
    param(
        [Parameter(Mandatory=$true)][string]$BinPath,
        [Parameter(Mandatory=$true)][string]$TargetHost,
        [Parameter(Mandatory=$false)][string]$Password = "mus4-debug"
    )
    if (-not (Test-Path $BinPath)) {
        Write-Error "Binary file not found: $BinPath"
        return $false
    }

    $baseUri = "http://$TargetHost"
    $updateUri = "$baseUri/update?auth=$Password"
    Write-Host "`n>>> HTTP OTA Upload to $updateUri" -ForegroundColor Cyan
    Write-Host "    Firmware: $BinPath" -ForegroundColor Gray

    # 预检：认证 + 打开 OTA 窗口（强制 Park 锁定）
    Write-Host "    Pre-flight: authenticating and opening OTA window..." -ForegroundColor DarkGray
    $authBody = "AUTH:$Password"
    $authResponse = curl.exe -s -X POST "$baseUri/api/cmd" -H "Content-Type: text/plain" --data "$authBody" --connect-timeout 10 --max-time 30
    if ($authResponse -notmatch "AUTH_OK") {
        Write-Warning "Auth pre-flight did not return AUTH_OK (response: $authResponse). Continuing anyway (device may already be authenticated or in dev mode)."
    }

    $enableOtaResponse = curl.exe -s -X POST "$baseUri/api/cmd" -H "Content-Type: text/plain" --data "ENABLE_OTA" --connect-timeout 10 --max-time 30
    if ($enableOtaResponse -notmatch "OTA_READY") {
        Write-Warning "ENABLE_OTA pre-flight did not return OTA_READY (response: $enableOtaResponse). Continuing anyway (OTA window may already be open)."
    }

    # 确保使用绝对路径，避免 curl 解析相对路径问题
    $absBinPath = Resolve-Path $BinPath

    $curlArgs = @(
        "-X", "POST",
        "-H", "Expect:",
        "-F", "firmware=@$absBinPath;type=application/octet-stream",
        "--progress-bar",
        "--fail",
        "--connect-timeout", "10",
        "--max-time", "180",
        "--retry", "2",
        "--retry-delay", "3",
        "--retry-connrefused",
        $updateUri
    )

    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        # 直接调用 curl.exe，让进度条实时输出到控制台 stderr
        & curl.exe @curlArgs
        $exitCode = $LASTEXITCODE
    } catch {
        Write-Error "curl.exe execution failed: $_"
        return $false
    } finally {
        $ErrorActionPreference = $prevEAP
    }

    if ($exitCode -eq 0) {
        Write-Host "`n>>> HTTP OTA Upload Successful!" -ForegroundColor Green
        return $true
    } else {
        Write-Host "`n>>> HTTP OTA Upload Failed (curl exit code: $exitCode)" -ForegroundColor Red
        Write-Host "    Tips: check device Wi-Fi connection, ensure Park is locked, and verify the password." -ForegroundColor Yellow
        Write-Host "    Common causes: auth expired, Park not locked, low free heap, or partition too small." -ForegroundColor Yellow
        return $false
    }
}

# ==============================================================================
# 自动探测模块
# ==============================================================================

<#
.SYNOPSIS
    探测默认 WSL 发行版
#>
function Get-DefaultWslDistro {
    try {
        # 先检查 wsl 命令是否可用
        $null = wsl --version 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "WSL 命令执行失败，请确认 WSL 是否已正确安装"
            return $null
        }

        # 方法1: 用 wsl --status 获取默认发行版（输出更稳定）
        $statusOutput = wsl --status 2>&1
        foreach ($line in $statusOutput) {
            # 匹配中英文输出: "默认分发版:" / "Default Distribution:"
            if ($line -match '(默认分发版|Default Distribution)[:：]\s*(\S+)') {
                Write-Verbose "通过 wsl --status 探测到默认发行版: $($matches[2])"
                return $matches[2]
            }
        }

        # 方法2: wsl -l --verbose 匹配带 * 号的默认发行版
        $output = wsl -l --verbose 2>&1
        foreach ($line in $output) {
            # 处理编码问题，移除不可见字符
            $cleanLine = $line -replace '[^\x20-\x7E]', ''
            if ($cleanLine -match '^\*\s+(\S+)\s+') {
                Write-Verbose "通过 wsl -l 探测到默认发行版: $($matches[1])"
                return $matches[1]
            }
        }

        # 方法3: 取列表中第一个发行版
        foreach ($line in $output) {
            $cleanLine = $line -replace '[^\x20-\x7E]', ''
            if ($cleanLine -match '^\s*(\S+)\s+(Running|Stopped)') {
                Write-Verbose "使用第一个可用发行版: $($matches[1])"
                return $matches[1]
            }
        }

        # 所有方法都失败，但 wsl 命令可用，就让 wsl 自己用默认发行版
        Write-Verbose "未明确探测到发行版，将使用 WSL 默认发行版"
        return ""
    } catch {
        Write-Warning "无法探测 WSL 发行版列表: $_"
    }
    Write-Warning "WSL 可能未正确安装，请检查后重试，或通过 -Distro 参数显式指定。"
    return $null
}

<#
.SYNOPSIS
    将 sketch 文件转为相对项目根目录的路径
#>
function Get-RelativeSketchPath {
    param(
        [Parameter(Mandatory=$true)][System.IO.FileInfo]$File,
        [Parameter(Mandatory=$true)][string]$Root
    )

    return $File.FullName.Substring($Root.Length).TrimStart('\', '/').Replace('\', '/')
}

<#
.SYNOPSIS
    从候选 sketch 中选择一个；交互式终端可主动询问，非交互环境要求显式指定
#>
function Select-SketchCandidate {
    param(
        [Parameter(Mandatory=$true)][System.IO.FileInfo[]]$Candidates,
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][string]$Message
    )

    if ($Candidates.Count -eq 1) {
        return Get-RelativeSketchPath -File $Candidates[0] -Root $Root
    }

    Write-Host $Message -ForegroundColor Yellow
    for ($i = 0; $i -lt $Candidates.Count; $i++) {
        $relative = Get-RelativeSketchPath -File $Candidates[$i] -Root $Root
        Write-Host "  [$($i + 1)] $relative"
    }

    if (-not [Environment]::UserInteractive -or [Console]::IsInputRedirected) {
        Write-Error "非交互环境无法主动选择 sketch，请重新运行并通过 -Sketch 显式指定。"
        exit 1
    }

    while ($true) {
        $choice = Read-Host "请选择 sketch 编号"
        $index = 0
        if ([int]::TryParse($choice, [ref]$index) -and $index -ge 1 -and $index -le $Candidates.Count) {
            return Get-RelativeSketchPath -File $Candidates[$index - 1] -Root $Root
        }
        Write-Host "无效选择，请输入 1-$($Candidates.Count) 之间的数字。" -ForegroundColor Yellow
    }
}

<#
.SYNOPSIS
    探测项目中的 sketch 文件路径
    优先选择项目根目录下的 .ino 文件，再搜索常见源码目录并排除依赖与构建产物
#>
function Find-SketchFile {
    param([string]$Root)

    $rootInos = @(Get-ChildItem -Path $Root -Filter "*.ino" -File)
    if ($rootInos.Count -gt 0) {
        return Select-SketchCandidate -Candidates $rootInos -Root $Root -Message "根目录下存在多个 .ino 文件，请选择主 sketch："
    }

    $excludedDirs = @('libraries', 'build', 'build_wsl', '.git', '.claude', '.qoder', '.trae', '.pytest_cache')
    $inoFiles = @(Get-ChildItem -Path $Root -Filter "*.ino" -Recurse -File | Where-Object {
        $relative = $_.FullName.Substring($Root.Length).TrimStart('\', '/')
        $firstSegment = ($relative -split '[\\/]')[0]
        $excludedDirs -notcontains $firstSegment
    })

    if ($inoFiles.Count -eq 0) {
        Write-Error "在项目目录 $Root 下未找到任何 .ino 文件，请通过 -Sketch 参数显式指定。"
        exit 1
    }

    return Select-SketchCandidate -Candidates $inoFiles -Root $Root -Message "找到多个 .ino 文件，请选择主 sketch："
}

<#
.SYNOPSIS
    解析 sketch 路径；配置指向失效文件时自动回退到探测逻辑
#>
function Resolve-SketchFile {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [string]$Sketch
    )

    if (-not [string]::IsNullOrWhiteSpace($Sketch)) {
        $candidate = if ([System.IO.Path]::IsPathRooted($Sketch)) { $Sketch } else { Join-Path $Root $Sketch }
        if (Test-Path $candidate -PathType Leaf) {
            return $Sketch.Replace('\', '/')
        }
        Write-Warning "配置中的 sketch 不存在: $Sketch，尝试自动搜索 .ino 文件。"
    }

    $resolved = Find-SketchFile -Root $Root
    Write-Warning "已自动选择 sketch: $resolved"
    return $resolved
}

<#
.SYNOPSIS
    读取 FQBN 配置：优先 sketch.yaml，其次 config.yaml
#>
function Get-ProjectFQBN {
    param([string]$Root)

    # 1. 读取 sketch.yaml
    $sketchYaml = Join-Path $Root "sketch.yaml"
    if (Test-Path $sketchYaml) {
        $content = Get-Content $sketchYaml -Raw
        if ($content -match 'default_fqbn\s*[:=]\s*[""]?([^\s""'']+)[""]?') {
            return $matches[1]
        }
    }

    # 2. 读取 config.yaml
    $configYaml = Join-Path $Root "config.yaml"
    if (Test-Path $configYaml) {
        $content = Get-Content $configYaml -Raw
        if ($content -match 'fqbn\s*[:=]\s*[""]?([^\s""'']+)[""]?') {
            return $matches[1]
        }
        if ($content -match 'build\s*:[\s\S]*?fqbn\s*[:=]\s*[""]?([^\s""'']+)[""]?') {
            return $matches[1]
        }
    }

    Write-Warning "未在配置文件中找到 FQBN，使用默认值 esp32:esp32:esp32"
    return "esp32:esp32:esp32"
}

<#
.SYNOPSIS
    探测 WSL 端 arduino-cli 的实际路径
#>
function Get-WslArduinoCliPath {
    # 1. 用 command -v 探测（最可靠）
    $output = Invoke-WslCommand -Command "command -v arduino-cli 2>/dev/null || echo ''" -IgnoreExitCode
    # 只提取路径行（以 / 或 ~ 开头），防御 WSL 警告文本混入
    $cleanOutput = ($output | Out-String).Trim() -replace '[^\x20-\x7E/]', ''
    $pathLine = ($cleanOutput -split "`n" | Where-Object { $_ -match '^[~/]' }) | Select-Object -First 1
    if (-not [string]::IsNullOrWhiteSpace($pathLine)) {
        return $pathLine.Trim()
    }

    # 2. 回退到常见路径
    $commonPaths = @("~/bin/arduino-cli", "/usr/local/bin/arduino-cli", "/usr/bin/arduino-cli", "\$HOME/bin/arduino-cli")
    foreach ($p in $commonPaths) {
        $exists = Invoke-WslCommand -Command "test -f $p && echo OK || echo ''" -IgnoreExitCode
        $cleanExists = ($exists | Out-String).Trim()
        if ($cleanExists -eq "OK") {
            return $p
        }
    }

    Write-Warning "未在 WSL 中找到 arduino-cli，请先安装：https://arduino.github.io/arduino-cli/latest/installation"
    return $null
}

<#
.SYNOPSIS
    获取 Windows 文档目录路径，兼容中英文系统
#>
function Get-WindowsDocumentsPath {
    return [Environment]::GetFolderPath("MyDocuments")
}

<#
.SYNOPSIS
    读取 wslbuild.yaml 配置文件，返回配置对象
    为避免引入外部依赖，仅解析简单的 key: value 格式和列表格式，支持注释
#>
function Get-ProjectConfig {
    param([string]$ConfigPath)

    if (-not (Test-Path $ConfigPath)) {
        return @{}
    }

    $config = @{}
    $lastKey = $null
    $lines = Get-Content $ConfigPath

    foreach ($line in $lines) {
        # 跳过空行和注释
        if ($line -match '^\s*#' -or [string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        # 匹配 key: value 格式
        if ($line -match '^\s*([a-zA-Z0-9_]+)\s*:\s*[""]?([^""'']*?)[""]?\s*$') {
            $lastKey = $matches[1]
            $value = $matches[2]
            $config[$lastKey] = $value
            continue
        }

        # 匹配数组（简单支持：- item 格式）
        if ($line -match '^\s*-\s*[""]?([^""'']*?)[""]?\s*$' -and $lastKey) {
            $item = $matches[1]
            if (-not $config[$lastKey] -or $config[$lastKey] -isnot [array]) {
                $config[$lastKey] = @()
            }
            $config[$lastKey] += $item
            continue
        }
    }
    return $config
}

# ==============================================================================
# 依赖自检模块
# ==============================================================================

function Invoke-PreFlightCheck {
    Write-Host ">>> Running pre-flight checks..." -ForegroundColor Cyan
    $allPass = $true

    # 1. 检查 WSL 是否可用
    try {
        $null = wsl --version 2>&1
        Write-Host "  [OK] WSL is available" -ForegroundColor Green
    } catch {
        Write-Host "  [FAIL] WSL is not installed or not enabled" -ForegroundColor Red
        Write-Host "         请先启用 WSL 功能并安装 Linux 发行版" -ForegroundColor Yellow
        $allPass = $false
    }

    # 2. 检查指定发行版是否存在
    if (-not [string]::IsNullOrWhiteSpace($script:WslDistro)) {
        # wsl --list 输出是 UTF-16 编码，会有乱码和空字符，先清理
        $distrosRaw = wsl --list 2>&1
        $distrosClean = $distrosRaw | ForEach-Object { $_ -replace '[^\x20-\x7Ea-zA-Z0-9_\-\(\) ]', '' } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

        $found = $false
        foreach ($d in $distrosClean) {
            if ($d -match [regex]::Escape($script:WslDistro)) {
                $found = $true
                break
            }
        }

        if ($found) {
            Write-Host "  [OK] WSL distro '$script:WslDistro' exists" -ForegroundColor Green
        } else {
            Write-Host "  [FAIL] WSL distro '$script:WslDistro' not found" -ForegroundColor Red
            Write-Host "         可用的发行版: $($distrosClean -join ', ')" -ForegroundColor Yellow
            $allPass = $false
        }
    } else {
        Write-Host "  [OK] Using WSL default distro" -ForegroundColor Green
    }

    # 3. 检查 WSL 端依赖（只要 WSL 可用就检查）
    $requiredTools = @("rsync", "python3", "find", "wc", "cp")
        if ($script:ArduinoCliPath) { $requiredTools += "arduino-cli" }

        foreach ($tool in $requiredTools) {
            # 使用 command -v 比 which 更可靠，同时清理输出中的乱码和空字符
            $output = Invoke-WslCommand -Command "command -v $tool 2>/dev/null || echo ''" -IgnoreExitCode
            $cleanOutput = ($output | Out-String).Trim() -replace '[^\x20-\x7E/]', ''

            if (-not [string]::IsNullOrWhiteSpace($cleanOutput)) {
                Write-Host "  [OK] WSL: $tool is available" -ForegroundColor Green
                Write-Verbose "        path: $cleanOutput"
            } else {
                Write-Host "  [FAIL] WSL: $tool is not installed" -ForegroundColor Red
                Write-Host "         请在 WSL 中执行: sudo apt install $tool" -ForegroundColor Yellow
                $allPass = $false
            }
        }

    # 4. 检查 Windows 端 Python
    try {
        $pyVer = python --version 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  [OK] Python is available" -ForegroundColor Green
            Write-Verbose "        version: $pyVer"
        } else {
            throw ""
        }
    } catch {
        Write-Host "  [FAIL] Python is not installed or not in PATH" -ForegroundColor Red
        Write-Host "         上传和串口监视功能需要 Python 3" -ForegroundColor Yellow
        $allPass = $false
    }

    if (-not $allPass) {
        Write-Host "`nPre-flight checks failed. Fix issues above or use -NoCheck to skip." -ForegroundColor Red
        exit 1
    }
    Write-Host ">>> All checks passed.`n" -ForegroundColor Green
}

# ==============================================================================
# 库同步模块
# ==============================================================================

function Sync-ArduinoLibraries {
    Write-Host "`n>>> Starting Library Sync..." -ForegroundColor Cyan
    Write-Host "Source (Win): $script:WinLibPath" -ForegroundColor Gray
    Write-Host "Target (WSL): $WslLibPath" -ForegroundColor Gray

    # 1. 预检查
    if (-not (Test-Path $script:WinLibPath)) {
        Write-Error "Windows source library path not found: $script:WinLibPath"
        exit 1
    }

    # 检查 WSL 状态（有指定发行版才检查）
    if (-not [string]::IsNullOrWhiteSpace($script:WslDistro)) {
        $wslStatus = wsl -d $script:WslDistro echo "ok" 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "WSL distro '$script:WslDistro' is not running. Starting it..."
            wsl -d $script:WslDistro echo "Starting..." | Out-Null
        }
    }

    # Expand tilde in WslLibPath for proper bash execution
    if ($WslLibPath.StartsWith("~")) {
        $WslLibPath = "`$HOME" + $WslLibPath.Substring(1)
    }

    # Check/Create WSL path
    Invoke-WslCommand -Command "mkdir -p `"$WslLibPath`""
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to create/access WSL path: $WslLibPath"
        exit 1
    }

    # 2. 备份逻辑
    if ($BackupLibs) {
        $backupPath = "${WslLibPath}_backup_$(Get-Date -Format 'yyyyMMddHHmmss')"
        Write-Host "Backing up existing libraries to $backupPath ..." -ForegroundColor Gray
        Invoke-WslCommand -Command "cp -r `"$WslLibPath`" `"$backupPath`""
    }

    # 3. 执行同步
    $syncSuccess = $false

    if ($SyncMode -eq "rsync") {
        $wslSourcePath = ConvertTo-WslPath -Path $script:WinLibPath
        if (-not $wslSourcePath.EndsWith("/")) { $wslSourcePath += "/" }

        $rsyncBase = "rsync -av"
        if ($OverwriteLibs) { $rsyncBase += " --delete" }

        foreach ($ex in $ExcludeLibs) {
            $rsyncBase += " --exclude='$ex'"
        }

        if (-not [string]::IsNullOrEmpty($ExtraArgs)) {
            $rsyncBase += " $ExtraArgs"
        }

        $distroArg = if (-not [string]::IsNullOrWhiteSpace($script:WslDistro)) { "-d $script:WslDistro " } else { "" }
        $finalCmd = "$rsyncBase `"$wslSourcePath`" `"$WslLibPath/`""
        $syncSuccess = Run-WithAnimation -Command "wsl" -Arguments "${distroArg}bash -lc '$finalCmd'" -TaskName "Syncing Libraries (rsync)"

    } elseif ($SyncMode -eq "robocopy") {
        if ([string]::IsNullOrWhiteSpace($script:WslDistro)) {
            Write-Error "robocopy 模式需要显式指定 WSL 发行版名称，请通过 -Distro 参数指定"
            exit 1
        }
        $absWslPath = Invoke-WslCommand -Command "readlink -f `"$WslLibPath`""
        $wslNetPath = "\\wsl.localhost\$script:WslDistro$absWslPath".Replace('/', '\')

        $roboArgs = "`"$script:WinLibPath`" `"$wslNetPath`" /E"
        if ($OverwriteLibs) { $roboArgs += " /MIR" }

        if ($ExcludeLibs.Count -gt 0) {
            $roboArgs += " /XD " + ($ExcludeLibs -join " ")
        }

        if (-not [string]::IsNullOrEmpty($ExtraArgs)) {
            $roboArgs += " $ExtraArgs"
        }

        $p = Start-Process "robocopy" -ArgumentList $roboArgs -NoNewWindow -PassThru -Wait
        if ($p.ExitCode -le 8) { $syncSuccess = $true } else { $syncSuccess = $false }
    }

    if (-not $syncSuccess) {
        Write-Error "Library sync failed."
        exit 1
    }

    # 4. 验证校验
    Write-Host "Verifying sync integrity..." -ForegroundColor Gray

    $winCount = (Get-ChildItem -Recurse $script:WinLibPath -File | Measure-Object).Count
    $winSize = (Get-ChildItem -Recurse $script:WinLibPath -File | Measure-Object -Property Length -Sum).Sum

    $debugInfo = Invoke-WslCommand -Command "id -un && ls -ld `"$WslLibPath`""
    Write-Verbose "WSL Debug: User=$($debugInfo[0]), Path=$($debugInfo[1])"

    $wslStats = Invoke-WslCommand -Command "find `"$WslLibPath`" -type f | wc -l && find `"$WslLibPath`" -type f -printf '%s\n' | python3 -c 'import sys; print(sum(int(l) for l in sys.stdin))'"
    $wslCount = [int]$wslStats[0]
    if ($wslStats.Count -ge 2 -and [string]::IsNullOrWhiteSpace($wslStats[1]) -eq $false) {
        $wslSize = [int64]$wslStats[1]
    } else {
        $wslSize = 0
    }

    Write-Host "Windows: $winCount files, $([math]::Round($winSize/1MB, 2)) MB"
    Write-Host "WSL:     $wslCount files, $([math]::Round($wslSize/1MB, 2)) MB"

    if ($winSize -eq 0) { $diffPercent = 0 } else {
        $diffPercent = [math]::Abs(($winSize - $wslSize) / $winSize) * 100
    }

    if ($diffPercent -gt 1) {
        Write-Error "Sync verification failed! Size difference is ${diffPercent}% (>1%)"
        exit 2
    }

    Write-Host "Library Sync Completed Successfully." -ForegroundColor Green
    Write-Host "----------------------------------------`n"
}

# ==============================================================================
# 主流程
# ==============================================================================

# 配置输出编码为UTF8，启用 WSL UTF-8 模式避免中文乱码
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:WSL_UTF8 = "1"

# --------------------------
# 1. 加载配置文件
# --------------------------
# 项目根目录：优先参数，其次脚本所在目录
if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = $PSScriptRoot
}
$ProjectRoot = Resolve-Path $ProjectRoot

# 配置文件路径：优先参数，其次项目根目录
if ([string]::IsNullOrWhiteSpace($Config)) {
    $Config = Join-Path $ProjectRoot "wslbuild.yaml"
}
$projectConfig = Get-ProjectConfig -ConfigPath $Config
if ($projectConfig.Count -gt 0) {
    Write-Verbose "Loaded config from: $Config"
}

# --------------------------
# 2. 初始化基础配置（优先级：命令行参数 > 环境变量 > 配置文件 > 默认值）
# --------------------------

Write-Verbose "Project root: $ProjectRoot"

# WSL 发行版：优先参数，其次环境变量，其次配置文件，其次自动探测
if ([string]::IsNullOrWhiteSpace($Distro)) {
    $Distro = $env:WSL_DISTRO_NAME
}
if ([string]::IsNullOrWhiteSpace($Distro) -and $projectConfig["distro"]) {
    $Distro = $projectConfig["distro"]
}
if ([string]::IsNullOrWhiteSpace($Distro)) {
    $Distro = Get-DefaultWslDistro
}
$script:WslDistro = $Distro
Write-Verbose "Using WSL distro: $Distro"

# Sketch 路径：优先参数，其次配置文件，其次自动探测；配置失效时自动回退
if ([string]::IsNullOrWhiteSpace($Sketch) -and $projectConfig["sketch"]) {
    $Sketch = $projectConfig["sketch"]
}
$Sketch = Resolve-SketchFile -Root $ProjectRoot -Sketch $Sketch
$SketchPath = $Sketch.Replace('\', '/')
Write-Verbose "Using sketch: $SketchPath"

# FQBN：优先参数，其次配置文件，其次 sketch.yaml / config.yaml
if ([string]::IsNullOrWhiteSpace($FQBN) -and $projectConfig["fqbn"]) {
    $FQBN = $projectConfig["fqbn"]
}
if ([string]::IsNullOrWhiteSpace($FQBN)) {
    $FQBN = Get-ProjectFQBN -Root $ProjectRoot
}
Write-Verbose "Using FQBN: $FQBN"

# 工作目录：优先配置文件，其次默认
if ($projectConfig["work_dir"]) {
    $WSLWorkDir = $projectConfig["work_dir"]
} else {
    $WSLWorkDir = "`$HOME/arduino-build/$(Split-Path $ProjectRoot -Leaf)"
}

# IO 模式：优先参数，其次配置文件
if ($IoMode -eq "native" -and $projectConfig["io_mode"]) {
    $IoMode = $projectConfig["io_mode"]
}
Write-Verbose "Using I/O mode: $IoMode"

# 库同步相关配置
$pythonExtraArgs = if ($PSBoundParameters.ContainsKey("ExtraArgs")) { $ExtraArgs } else { "" }
$script:LibrariesPath = if ($projectConfig["libraries_path"]) { [string]$projectConfig["libraries_path"] } else { "libraries" }
if ($projectConfig["sync_libs"] -and $projectConfig["sync_libs"] -match "^(true|1|yes)$") {
    $SyncLibs = $true
}
if ($projectConfig["lib_exclude"] -and $ExcludeLibs.Count -eq 2 -and $ExcludeLibs[0] -eq "^\." -and $ExcludeLibs[1] -eq "^tmp$") {
    # 用户没有显式传入 ExcludeLibs 参数，使用配置文件中的
    $ExcludeLibs = $projectConfig["lib_exclude"]
}
if ($projectConfig["extra_sync_args"] -and -not $PSBoundParameters.ContainsKey("ExtraArgs")) {
    $ExtraArgs = $projectConfig["extra_sync_args"]
}

# Windows 库路径：优先参数，其次自动探测文档目录
if ([string]::IsNullOrWhiteSpace($WinLibPath)) {
    $docPath = Get-WindowsDocumentsPath
    $script:WinLibPath = Join-Path $docPath "Arduino\libraries"
} else {
    $script:WinLibPath = $WinLibPath
}

# WSL 项目路径与工作目录
$WSLProjectRoot = ConvertTo-WslPath -Path $ProjectRoot
$BuildDir = "build_wsl"

# 根据 IO 模式决定编译路径
if ($IoMode -eq "mnt") {
    # 直接使用挂载分区下的项目目录，不需要同步
    $WSLWorkDir = $WSLProjectRoot
    $WSLBuildDir = "$WSLWorkDir/$BuildDir"
    Write-Verbose "Using direct mount mode: building on $WSLProjectRoot"
} else {
    $WSLBuildDir = "$WSLWorkDir/$BuildDir"
    Write-Verbose "WSL work dir: $WSLWorkDir"
}
$WSLSketchPath = "$WSLWorkDir/$SketchPath"
$localLibrariesWinPath = Join-Path $ProjectRoot $script:LibrariesPath
$localLibrariesArg = ""
if (Test-Path $localLibrariesWinPath) {
    $wslLibrariesPath = if ($IoMode -eq "native") { "$WSLWorkDir/$script:LibrariesPath" } else { "$WSLProjectRoot/$script:LibrariesPath" }
    $localLibrariesArg = " --libraries `"`"$wslLibrariesPath`"`""
    Write-Verbose "Using local Arduino libraries: $wslLibrariesPath"
}

# WSL 端 arduino-cli 路径
$script:ArduinoCliPath = Get-WslArduinoCliPath

# --------------------------
# 2. 前置检查
# 优先级：-Check 强制开启 > -NoCheck 强制关闭 > 默认不检查
# --------------------------
$runCheck = $false
if ($Check) {
    $runCheck = $true
} elseif (-not $NoCheck) {
    $runCheck = $true
}

if ($runCheck) {
    Invoke-PreFlightCheck
}

# --------------------------
# 3. 库同步
# --------------------------
if ($SyncLibs) {
    Sync-ArduinoLibraries
}

# --------------------------
# 4. 默认行为逻辑
# --------------------------
if (-not $Compile.IsPresent -and -not $Upload.IsPresent -and -not $Serial.IsPresent -and -not $CheckPartition.IsPresent) {
    $Compile = $true
    $Upload = $true
}

# --------------------------
# 5. 编译流程
# --------------------------
$BinPath = $null
if ($Compile) {
    Write-Host ">>> Starting WSL Build (mode: $IoMode)..." -ForegroundColor Cyan

    $syncTime = 0
    $syncBackTime = 0

    # Clean 模式：清理构建目录
    if ($Clean) {
        Write-Host "Cleaning previous build directory..." -ForegroundColor Gray
        Invoke-WslCommand -Command "rm -rf ""$WSLBuildDir""" -IgnoreExitCode
        if ($IoMode -eq "native") {
            Invoke-WslCommand -Command "rm -rf ""$WSLWorkDir""" -IgnoreExitCode
        }
    }

    # 构造 WSL 命令前缀（仅当有指定发行版时才加 -d 参数）
    $distroPrefix = if (-not [string]::IsNullOrWhiteSpace($script:WslDistro)) { "-d $script:WslDistro " } else { "" }

    # native 模式需要同步源码到 WSL 原生分区
    if ($IoMode -eq "native") {
        # 1. Sync Source to WSL Native FS
        $syncCmd = "wsl"
        $syncArgs = "${distroPrefix}bash -lc 'mkdir -p $WSLWorkDir && rsync -av --delete --exclude=build_wsl --exclude=.git --exclude=.venv ""$WSLProjectRoot/"" ""$WSLWorkDir/""'"
        $syncStart = Get-Date
        if (-not (Run-WithAnimation -Command $syncCmd -Arguments $syncArgs -TaskName "Syncing Source to WSL")) { exit 1 }
        $syncTime = ((Get-Date) - $syncStart).TotalSeconds
    }

    # 2. Compile
    $compileCmd = "wsl"
    $compileTask = if ($IoMode -eq "native") { "Compiling in WSL (Native FS)" } else { "Compiling in WSL (Mounted FS)" }
    $compileArgs = "${distroPrefix}bash -lc '$script:ArduinoCliPath compile --fqbn $FQBN --build-path ""$WSLBuildDir"" --output-dir ""$WSLBuildDir""$localLibrariesArg ""$WSLSketchPath""'"
    $compileStart = Get-Date
    if (-not (Run-WithAnimation -Command $compileCmd -Arguments $compileArgs -TaskName $compileTask)) { exit 1 }
    $compileTime = ((Get-Date) - $compileStart).TotalSeconds

    # 3. Sync Artifacts Back to Windows (仅 native 模式需要，mnt 模式已经在同目录下)
    if ($IoMode -eq "native") {
        $syncBackCmd = "wsl"
        $syncBackArgs = "${distroPrefix}bash -lc 'mkdir -p ""$WSLProjectRoot/$BuildDir"" && cp ""$WSLBuildDir""/*.bin ""$WSLProjectRoot/$BuildDir/"" && cp ""$WSLBuildDir""/*.elf ""$WSLProjectRoot/$BuildDir/""'"
        $syncBackStart = Get-Date
        if (-not (Run-WithAnimation -Command $syncBackCmd -Arguments $syncBackArgs -TaskName "Syncing Artifacts to Windows")) { exit 1 }
        $syncBackTime = ((Get-Date) - $syncBackStart).TotalSeconds
    }

    # 4. Get actual bin filename from build output
    $actualBinFile = Invoke-WslCommand -Command "find ""$WSLBuildDir"" -maxdepth 1 -type f -name '*.bin' ! -name '*.bootloader.bin' ! -name '*.partitions.bin' ! -name '*.merged.bin' | sort | head -1 | xargs -r basename"
    if ([string]::IsNullOrWhiteSpace($actualBinFile)) {
        Write-Error "No .bin file found in build output: $WSLBuildDir"
        exit 1
    }
    $BinPath = Join-Path (Join-Path $ProjectRoot $BuildDir) $actualBinFile

    # Output Performance Report
    Write-Host "`n=== Performance Report ===" -ForegroundColor Yellow
    if ($IoMode -eq "native") {
        Write-Host "Sync to WSL:   $("{0:N2}" -f $syncTime)s"
    }
    Write-Host "Compilation:   $("{0:N2}" -f $compileTime)s"
    if ($IoMode -eq "native") {
        Write-Host "Sync back:     $("{0:N2}" -f $syncBackTime)s"
    }
    $totalTime = $syncTime + $compileTime + $syncBackTime
    Write-Host "Total Build:   $("{0:N2}" -f $totalTime)s"
    Write-Host "========================`n" -ForegroundColor Yellow

    # 编译后分区大小检查
    Invoke-PartitionFitCheck -FQBN $FQBN -ProjectRoot $ProjectRoot -BuildDir $BuildDir
}

# --------------------------
# 5b. 纯分区检查模式（不编译时）
# --------------------------
if ($CheckPartition -and -not $Compile) {
    Invoke-PartitionFitCheck -FQBN $FQBN -ProjectRoot $ProjectRoot -BuildDir $BuildDir -PreBuild
}

# --------------------------
# 6. 上传 / 串口监视
# --------------------------
# 仅上传场景需要固件文件；串口监视(-s)不应依赖 .bin
if ($Upload -and (-not $BinPath -or -not (Test-Path $BinPath))) {
    $candidateBuildDirs = @(
        (Join-Path $ProjectRoot $BuildDir),
        (Join-Path $ProjectRoot "build")
    )
    $localBin = $candidateBuildDirs |
        Where-Object { Test-Path $_ } |
        ForEach-Object {
            Get-ChildItem -Path $_ -Filter "*.bin" -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -notmatch '\.(bootloader|partitions|merged)\.bin$' }
        } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($localBin) {
        $BinPath = $localBin.FullName
    } else {
        $actualBinFile = Invoke-WslCommand -Command "find ""$WSLBuildDir"" -maxdepth 1 -type f -name '*.bin' ! -name '*.bootloader.bin' ! -name '*.partitions.bin' ! -name '*.merged.bin' | sort | head -1 | xargs -r basename"
        if ([string]::IsNullOrWhiteSpace($actualBinFile)) {
            Write-Error "No app .bin file found in build output: $($candidateBuildDirs -join ', ') or $WSLBuildDir"
            exit 1
        }
        $BinPath = Join-Path (Join-Path $ProjectRoot $BuildDir) $actualBinFile
    }
}

if ($Upload -or $Serial) {
    if ($Upload) {
        Write-Host ">>> Build successful. Starting Upload..." -ForegroundColor Cyan
        if (-not (Test-Path $BinPath)) {
            Write-Error "Binary file not found: $BinPath"
            exit 1
        }
    }

    if ($HttpOta) {
        # HTTP OTA 上传路径（新方式，通过 Web Console /update 端点）
        if ([string]::IsNullOrWhiteSpace($HttpOtaHost)) {
            $HttpOtaHost = Get-HttpOtaTarget -ProjectRoot $ProjectRoot
        }
        if ([string]::IsNullOrWhiteSpace($HttpOtaHost)) {
            Write-Error "HTTP OTA 需要指定目标主机。请使用 -HttpOtaHost 参数，或在项目根目录创建 .mus4_ota_target 文件（每行一个 IP/主机名）。"
            exit 1
        }
        $success = Invoke-HttpOtaUpload -BinPath $BinPath -TargetHost $HttpOtaHost -Password $HttpOtaPassword
        if (-not $success) { exit 1 }
    } else {
        # 原有上传路径：串口 / ArduinoOTA
        $pyArgs = @()
        if ($Upload) {
            $pyArgs += "-u"
            $pyArgs += "-i"
            $pyArgs += "`"$BinPath`""
            if ($Ota) {
                $pyArgs += "--ota"
                if (-not [string]::IsNullOrWhiteSpace($OtaHost)) {
                    $pyArgs += "--ota-host"
                    $pyArgs += $OtaHost
                }
                if ($OtaPort -gt 0) {
                    $pyArgs += "--ota-port"
                    $pyArgs += $OtaPort.ToString()
                }
                if (-not [string]::IsNullOrWhiteSpace($OtaPassword)) {
                    $pyArgs += "--ota-password"
                    $pyArgs += $OtaPassword
                }
                if (-not [string]::IsNullOrWhiteSpace($EspotaTool)) {
                    $pyArgs += "--espota-tool"
                    $pyArgs += "`"$EspotaTool`""
                }
            }
        }
        if ($Serial) {
            $pyArgs += "-s"
        }

        if ($Sketch) {
            $pyArgs += "--sketch"
            $pyArgs += "`"$Sketch`""
        }
        if (-not [string]::IsNullOrWhiteSpace($pythonExtraArgs)) {
            $pyArgs += $pythonExtraArgs.Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries)
        }

        python (Join-Path $ProjectRoot "arduino-cli.py") $pyArgs
    }
}

Write-Host ">>> All Done!" -ForegroundColor Green
