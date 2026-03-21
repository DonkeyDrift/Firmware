<#
.SYNOPSIS
    WSL 高速构建脚本 (支持库同步)
.DESCRIPTION
    将项目同步到 WSL 原生文件系统进行编译，并支持将 Windows 端 Arduino 库同步到 WSL。
#>
[CmdletBinding()]
param(
    # --- 库同步参数 ---
    [Parameter(HelpMessage="是否启用库同步 (默认不启用)")]
    [Alias("Sync")]
    [switch]$SyncLibs,

    [Parameter(HelpMessage="Windows端Arduino库路径")]
    [string]$WinLibPath = "$env:USERPROFILE\Documents\Arduino\libraries",

    [Parameter(HelpMessage="WSL端目标路径")]
    [string]$WslLibPath = "~/Arduino/libraries",

    [Parameter(HelpMessage="是否覆盖已有库")]
    [bool]$OverwriteLibs = $true,

    [Parameter(HelpMessage="是否保留旧版本备份")]
    [switch]$BackupLibs,

    [Parameter(HelpMessage="排除列表(正则表达式数组)")]
    [string[]]$ExcludeLibs = @("^\.", "^tmp$"),

    [Parameter(HelpMessage="同步模式: rsync 或 robocopy")]
    [ValidateSet("rsync", "robocopy")]
    [string]$SyncMode = "rsync",

    [Parameter(HelpMessage="自定义附加参数")]
    [string]$ExtraArgs = "",

    [Parameter(HelpMessage="启用串口监视器")]
    [Alias("s")]
    [switch]$Serial,

    [Parameter(HelpMessage="执行编译")]
    [Alias("c")]
    [switch]$Compile,

    [Parameter(HelpMessage="执行上传")]
    [Alias("u")]
    [switch]$Upload
)

$ErrorActionPreference = "Stop"

# 配置输出编码为UTF8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ProjectRoot = "C:\Dev\DDC\mus4"
$SketchPath = "mus4/mus4.ino"
$BuildDir = "build_wsl"
$WSLProjectRoot = "/mnt/c/Dev/DDC/mus4"

# 优化后的 WSL 原生工作路径
# 使用 $HOME 环境变量而不是 ~，因为在引号中 ~ 不会被 bash 展开
$WSLWorkDir = "`$HOME/arduino-build/mus4"
$WSLSketchPath = "$WSLWorkDir/mus4/mus4.ino"
$WSLBuildDir = "$WSLWorkDir/$BuildDir"

# 动画字符集
$frames = @('⣾', '⣽', '⣻', '⢿', '⡿', '⣟', '⣯', '⣷')

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
    
    try { [Console]::CursorVisible = $false } catch {}

    $outputBuffer = New-Object System.Text.StringBuilder
    $errorBuffer = New-Object System.Text.StringBuilder

    $outEvent = { if (-not [string]::IsNullOrEmpty($EventArgs.Data)) { $null = $Event.MessageData.outputBuffer.AppendLine($EventArgs.Data) } }
    $errEvent = { if (-not [string]::IsNullOrEmpty($EventArgs.Data)) { $null = $Event.MessageData.errorBuffer.AppendLine($EventArgs.Data) } }
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
        $status = "$frame $TaskName... ($timeStr)"
        Write-Host -NoNewline "`r$status   "
        Start-Sleep -Milliseconds 150
    }
    
    $p.WaitForExit()
    try { [Console]::CursorVisible = $true } catch {}
    Write-Host "" 

    Unregister-Event -SourceIdentifier $sub1.Name -ErrorAction SilentlyContinue
    Unregister-Event -SourceIdentifier $sub2.Name -ErrorAction SilentlyContinue
    $sub1 | Remove-Job -ErrorAction SilentlyContinue
    $sub2 | Remove-Job -ErrorAction SilentlyContinue

    if ($p.ExitCode -ne 0) {
        Write-Host "Failed!" -ForegroundColor Red
        Write-Host "Error Code: $($p.ExitCode)" -ForegroundColor Red
        Write-Host "---------------- Error Output ----------------" -ForegroundColor Yellow
        Write-Host $errorBuffer.ToString() -ForegroundColor Yellow
        Write-Host "----------------------------------------------" -ForegroundColor Yellow
        return $false
    } else {
        Write-Host "Done!" -ForegroundColor Green
        return $true
    }
}

function Sync-ArduinoLibraries {
    Write-Host "`n>>> Starting Library Sync..." -ForegroundColor Cyan
    Write-Host "Source (Win): $WinLibPath" -ForegroundColor Gray
    Write-Host "Target (WSL): $WslLibPath" -ForegroundColor Gray

    # 1. 预检查
    if (-not (Test-Path $WinLibPath)) {
        Write-Error "Windows source library path not found: $WinLibPath"
        exit 1
    }
    
    # Check WSL status
    $wslStatus = wsl --list --running
    if ($wslStatus -notmatch "DKC") {
        Write-Warning "WSL distro 'DKC' is not running. Starting it..."
        wsl -d DKC echo "Starting..." | Out-Null
    }

    # Expand tilde in WslLibPath for proper bash execution
    if ($WslLibPath.StartsWith("~")) {
        $WslLibPath = "`$HOME" + $WslLibPath.Substring(1)
    }

    # Check/Create WSL path
    wsl -d DKC bash -c "mkdir -p `"$WslLibPath`""
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to create/access WSL path: $WslLibPath"
        exit 1
    }

    # 2. 备份逻辑
    if ($BackupLibs) {
        $backupPath = "${WslLibPath}_backup_$(Get-Date -Format 'yyyyMMddHHmmss')"
        Write-Host "Backing up existing libraries to $backupPath ..." -ForegroundColor Gray
        wsl -d DKC cp -r "$WslLibPath" "$backupPath"
    }

    # 3. 执行同步
    $syncSuccess = $false
    
    if ($SyncMode -eq "rsync") {
        # Convert Win path to WSL mount path
        # e.g. C:\Users\xxx -> /mnt/c/Users/xxx
        $drive = $WinLibPath.Substring(0,1).ToLower()
        $pathWithoutDrive = $WinLibPath.Substring(3).Replace('\', '/')
        $wslSourcePath = "/mnt/$drive/$pathWithoutDrive/"
        
        # Build rsync command
        # -a: archive mode, -v: verbose
        # --delete: remove files in dest not in src (if overwrite/mirror)
        $rsyncBase = "rsync -av"
        if ($OverwriteLibs) { $rsyncBase += " --delete" }
        
        # Add exclusions
        foreach ($ex in $ExcludeLibs) {
            $rsyncBase += " --exclude='$ex'"
        }
        
        if (-not [string]::IsNullOrEmpty($ExtraArgs)) {
            $rsyncBase += " $ExtraArgs"
        }
        
        $finalCmd = "$rsyncBase `"$wslSourcePath`" `"$WslLibPath/`""
        $syncSuccess = Run-WithAnimation -Command "wsl" -Arguments "-d DKC bash -c '$finalCmd'" -TaskName "Syncing Libraries (rsync)"
        
    } elseif ($SyncMode -eq "robocopy") {
        # Robocopy needs \\wsl.localhost path
        $wslNetPath = "\\wsl.localhost\DKC\home\$(wsl -d DKC whoami)\Arduino\libraries"
        # Note: mapping user home path from WslLibPath is tricky if it contains ~, 
        # so we assume standard layout or user provides full path. 
        # For robustness, we'll try to resolve the path inside WSL first to get absolute path
        $absWslPath = wsl -d DKC readlink -f "$WslLibPath"
        $wslNetPath = "\\wsl.localhost\DKC$absWslPath".Replace('/', '\')
        
        $roboArgs = "`"$WinLibPath`" `"$wslNetPath`" /E"
        if ($OverwriteLibs) { $roboArgs += " /MIR" } # Mirror implies delete destination extras
        
        # Exclusions for robocopy (/XD dirs)
        # Note: Robocopy regex support is limited, assumes simple names
        if ($ExcludeLibs.Count -gt 0) {
            $roboArgs += " /XD " + ($ExcludeLibs -join " ")
        }
        
        if (-not [string]::IsNullOrEmpty($ExtraArgs)) {
            $roboArgs += " $ExtraArgs"
        }
        
        # Robocopy returns exit codes 0-7 for success
        $p = Start-Process "robocopy" -ArgumentList $roboArgs -NoNewWindow -PassThru -Wait
        if ($p.ExitCode -le 8) { $syncSuccess = $true } else { $syncSuccess = $false }
    }

    if (-not $syncSuccess) {
        Write-Error "Library sync failed."
        exit 1
    }

    # 4. 验证校验
    Write-Host "Verifying sync integrity..." -ForegroundColor Gray
    
    # Get Win Stats
    $winCount = (Get-ChildItem -Recurse $WinLibPath -File | Measure-Object).Count
    $winSize = (Get-ChildItem -Recurse $WinLibPath -File | Measure-Object -Property Length -Sum).Sum
    
    # Get WSL Stats
    # Convert WinLibPath to WSL format again for exclusion logic consistency if needed, 
    # but here we just verify destination.
    # We use 'find' to sum only file sizes (bytes) to match Windows 'Get-ChildItem -File | Measure-Object -Sum' logic.
    # du -sb includes directory entry sizes (4KB per dir), which causes mismatches.
    # Method: find prints sizes, python3 calculates sum.
    # Strategy: Use double quotes for paths (to allow $HOME expansion by Bash) and single quotes for Python command.
    # Use single quotes for -printf format to avoid backslash escaping hell.
    $debugInfo = wsl -d DKC bash -c "id -un && ls -ld `"$WslLibPath`""
    Write-Host "WSL Debug: User=$($debugInfo[0]), Path=$($debugInfo[1])" -ForegroundColor DarkGray
    
    $wslStats = wsl -d DKC bash -c "find `"$WslLibPath`" -type f | wc -l && find `"$WslLibPath`" -type f -printf '%s\n' | python3 -c 'import sys; print(sum(int(l) for l in sys.stdin))'"
    $wslCount = [int]$wslStats[0]
    if ($wslStats.Count -ge 2 -and [string]::IsNullOrWhiteSpace($wslStats[1]) -eq $false) {
        $wslSize = [int64]$wslStats[1]
    } else {
        $wslSize = 0
    }

    Write-Host "Windows: $winCount files, $([math]::Round($winSize/1MB, 2)) MB"
    Write-Host "WSL:     $wslCount files, $([math]::Round($wslSize/1MB, 2)) MB"
    
    # Allow 1% tolerance (metadata diffs, line endings etc)
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

# 如果启用了同步，则先执行库同步
if ($SyncLibs) {
    Sync-ArduinoLibraries
}

# 默认行为逻辑：如果未指定 -c, -u, -s，则默认执行编译和上传
if (-not $Compile.IsPresent -and -not $Upload.IsPresent -and -not $Serial.IsPresent) {
    $Compile = $true
    $Upload = $true
}

if ($Compile) {
    Write-Host ">>> Starting Optimized WSL Build (Fast I/O)..." -ForegroundColor Cyan

    # 1. Sync Source to WSL Native FS
    $syncCmd = "wsl"
    # 使用 rsync 进行增量同步，排除构建目录和不必要的文件
    $syncArgs = "-d DKC bash -c 'mkdir -p $WSLWorkDir && rsync -av --delete --exclude=build_wsl --exclude=.git --exclude=.venv ""$WSLProjectRoot/"" ""$WSLWorkDir/""'"
    $syncStart = Get-Date
    if (-not (Run-WithAnimation -Command $syncCmd -Arguments $syncArgs -TaskName "Syncing Source to WSL")) { exit 1 }
    $syncTime = ((Get-Date) - $syncStart).TotalSeconds

    # 2. Compile in WSL Native FS
    $compileCmd = "wsl"
    $compileArgs = "-d DKC bash -c '~/bin/arduino-cli compile --fqbn esp32:esp32:esp32 --build-path ""$WSLBuildDir"" --output-dir ""$WSLBuildDir"" ""$WSLSketchPath""'"
    $compileStart = Get-Date
    if (-not (Run-WithAnimation -Command $compileCmd -Arguments $compileArgs -TaskName "Compiling in WSL (Native FS)")) { exit 1 }
    $compileTime = ((Get-Date) - $compileStart).TotalSeconds

    # 3. Sync Artifacts Back to Windows
    $syncBackCmd = "wsl"
    # 只同步生成的 .bin 和 .elf 文件回 Windows
    $syncBackArgs = "-d DKC bash -c 'mkdir -p ""$WSLProjectRoot/$BuildDir"" && cp ""$WSLBuildDir""/*.bin ""$WSLProjectRoot/$BuildDir/"" && cp ""$WSLBuildDir""/*.elf ""$WSLProjectRoot/$BuildDir/""'"
    $syncBackStart = Get-Date
    if (-not (Run-WithAnimation -Command $syncBackCmd -Arguments $syncBackArgs -TaskName "Syncing Artifacts to Windows")) { exit 1 }
    $syncBackTime = ((Get-Date) - $syncBackStart).TotalSeconds

    # 4. Get actual bin filename from build output
    $actualBinFile = wsl -d DKC bash -c "ls ""$WSLBuildDir""/*.bin 2>/dev/null | head -1 | xargs -r basename"
    if ([string]::IsNullOrWhiteSpace($actualBinFile)) {
        Write-Error "No .bin file found in build output: $WSLBuildDir"
        exit 1
    }
    $BinPath = "$ProjectRoot\$BuildDir\$actualBinFile"

    # Output Performance Report
    Write-Host "`n=== Performance Report ===" -ForegroundColor Yellow
    Write-Host "Sync to WSL:   $("{0:N2}" -f $syncTime)s"
    Write-Host "Compilation:   $("{0:N2}" -f $compileTime)s"
    Write-Host "Sync back:     $("{0:N2}" -f $syncBackTime)s"
    $totalTime = $syncTime + $compileTime + $syncBackTime
    Write-Host "Total Build:   $("{0:N2}" -f $totalTime)s"
    Write-Host "========================`n" -ForegroundColor Yellow
}

# If $BinPath not set (e.g., only -u flag), try to detect from WSL build output
if (-not $BinPath -or -not (Test-Path $BinPath)) {
    $actualBinFile = wsl -d DKC bash -c "ls ""$WSLBuildDir""/*.bin 2>/dev/null | head -1 | xargs -r basename"
    if ([string]::IsNullOrWhiteSpace($actualBinFile)) {
        Write-Error "No .bin file found in build output: $WSLBuildDir"
        exit 1
    }
    $BinPath = "$ProjectRoot\$BuildDir\$actualBinFile"
}

if ($Upload -or $Serial) {
    if ($Upload) {
        Write-Host ">>> Build successful. Starting Upload..." -ForegroundColor Cyan
        if (-not (Test-Path $BinPath)) {
            Write-Error "Binary file not found: $BinPath"
            exit 1
        }
    }

    # Call upload / serial
    $pyArgs = @()
    if ($Upload) {
        $pyArgs += "-u"
        $pyArgs += "-i"
        $pyArgs += "`"$BinPath`""
    }
    if ($Serial) {
        $pyArgs += "-s"
    }
    
    python "$ProjectRoot\arduino-cli.py" $pyArgs
}

Write-Host ">>> All Done!" -ForegroundColor Green
