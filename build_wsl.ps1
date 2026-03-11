$ErrorActionPreference = "Stop"

# 配置输出编码为UTF8，确保Unicode字符正常显示
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ProjectRoot = "C:\Dev\DDC\mus4"
$SketchPath = "mus4/mus4.ino"
$BuildDir = "build_wsl"
$WSLProjectRoot = "/mnt/c/Dev/DDC/mus4"
$WSLSketchPath = "$WSLProjectRoot/$SketchPath"
$WSLBuildDir = "$WSLProjectRoot/$BuildDir"

# 动画字符集 (Braille Spinner)
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
    # 设置编码
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
    
    # 尝试隐藏光标
    try { [Console]::CursorVisible = $false } catch {}

    $outputBuffer = New-Object System.Text.StringBuilder
    $errorBuffer = New-Object System.Text.StringBuilder

    # 异步读取输出流
    $outEvent = { 
        if (-not [string]::IsNullOrEmpty($EventArgs.Data)) { 
            $null = $Event.MessageData.outputBuffer.AppendLine($EventArgs.Data) 
        } 
    }
    $errEvent = { 
        if (-not [string]::IsNullOrEmpty($EventArgs.Data)) { 
            $null = $Event.MessageData.errorBuffer.AppendLine($EventArgs.Data) 
        } 
    }

    $eventData = @{ outputBuffer = $outputBuffer; errorBuffer = $errorBuffer }
    
    $sub1 = Register-ObjectEvent -InputObject $p -EventName OutputDataReceived -Action $outEvent -MessageData $eventData
    $sub2 = Register-ObjectEvent -InputObject $p -EventName ErrorDataReceived -Action $errEvent -MessageData $eventData
    
    $p.BeginOutputReadLine()
    $p.BeginErrorReadLine()

    # 模拟进度 (由于无法精确获取编译百分比，这里使用时间推算模拟一个非线性的进度展示)
    # 假设初次编译约30秒，增量编译约5秒。我们显示一个动态的 "Running..." 状态
    
    while (-not $p.HasExited) {
        $elapsed = (Get-Date) - $startTime
        $timeStr = "{0:mm}:{0:ss}" -f $elapsed
        
        # 动画帧
        $frame = $frames[$frameIdx % $frames.Length]
        $frameIdx++

        # 构造状态行 (使用回车符覆盖当前行)
        # 格式: [⣾] Compiling in WSL... (00:05)
        # 注意：PowerShell host 中 Write-Host -NoNewline 配合 `r 可以实现覆盖
        $status = "$frame $TaskName... ($timeStr)"
        Write-Host -NoNewline "`r$status   "
        
        Start-Sleep -Milliseconds 150
    }
    
    # 等待进程完全退出并确保所有输出被捕获
    $p.WaitForExit()
    
    # 恢复光标
    try { [Console]::CursorVisible = $true } catch {}
    Write-Host "" # 换行

    # 清理事件订阅
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
        # 有时错误信息也在标准输出中
        if ($outputBuffer.Length -gt 0) {
            Write-Host "Standard Output (Tail):" -ForegroundColor Gray
            $lines = $outputBuffer.ToString().Split("`n")
            $lines | Select-Object -Last 10 | ForEach-Object { Write-Host $_ -ForegroundColor Gray }
        }
        return $false
    } else {
        Write-Host "Done!" -ForegroundColor Green
        # 显示编译统计信息 (通常在标准输出的最后几行)
        $outStr = $outputBuffer.ToString()
        if ($outStr -match "Sketch uses") {
            $lines = $outStr.Split("`n")
            $lines | Select-Object -Last 5 | ForEach-Object { 
                if($_ -match "Sketch uses|Global variables") { Write-Host $_ -ForegroundColor Gray } 
            }
        }
        return $true
    }
}

Write-Host ">>> Starting WSL Build..." -ForegroundColor Cyan

# Ensure build directory exists
if (-not (Test-Path "$ProjectRoot\$BuildDir")) {
    New-Item -ItemType Directory -Path "$ProjectRoot\$BuildDir" | Out-Null
}

# Run compilation in WSL
# Note: Using full path to arduino-cli in WSL user's bin
# 为了避免路径转义问题，这里仔细构造参数
$wslCmd = "wsl"
# 注意：PowerShell中传递带引号的参数给外部命令比较棘手，这里使用单引号包裹整个bash命令
$wslArgs = "-d DKC bash -c '~/bin/arduino-cli compile --fqbn esp32:esp32:esp32 --build-path ""$WSLBuildDir"" --output-dir ""$WSLBuildDir"" ""$WSLSketchPath""'"

$success = Run-WithAnimation -Command $wslCmd -Arguments $wslArgs -TaskName "Compiling Sketch"

if (-not $success) {
    exit 1
}

Write-Host ">>> Compilation successful. Starting Upload..." -ForegroundColor Cyan

# Run upload using Python script
$BinPath = "$ProjectRoot\$BuildDir\mus4.ino.bin"
if (-not (Test-Path $BinPath)) {
    Write-Error "Binary file not found: $BinPath"
}

# 调用 arduino-cli.py (它自带进度动画)
# 直接运行，让它接管控制台输出
python "$ProjectRoot\arduino-cli.py" -u -i "$BinPath"

Write-Host ">>> All Done!" -ForegroundColor Green
