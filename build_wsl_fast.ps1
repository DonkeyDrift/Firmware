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

# Output Performance Report
Write-Host "`n=== Performance Report ===" -ForegroundColor Yellow
Write-Host "Sync to WSL:   $("{0:N2}" -f $syncTime)s"
Write-Host "Compilation:   $("{0:N2}" -f $compileTime)s"
Write-Host "Sync back:     $("{0:N2}" -f $syncBackTime)s"
$totalTime = $syncTime + $compileTime + $syncBackTime
Write-Host "Total Build:   $("{0:N2}" -f $totalTime)s"
Write-Host "========================`n" -ForegroundColor Yellow

Write-Host ">>> Build successful. Starting Upload..." -ForegroundColor Cyan

# Run upload using Python script
$BinPath = "$ProjectRoot\$BuildDir\mus4.ino.bin"
if (-not (Test-Path $BinPath)) {
    Write-Error "Binary file not found: $BinPath"
}

# Call upload
python "$ProjectRoot\arduino-cli.py" -u -i "$BinPath"

Write-Host ">>> All Done!" -ForegroundColor Green
