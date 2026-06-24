# 临时调试脚本：触发 STA 重试并抓串口日志
param([string]$ComPort = 'COM20', [int]$WaitSeconds = 18)

$port = New-Object System.IO.Ports.SerialPort $ComPort, 115200, 'None', 8, 'One'
$port.ReadTimeout = 5000
try {
    $port.Open()
    Start-Sleep -Seconds 1
    $port.DiscardInBuffer()
    $port.WriteLine("WIFI_STA_APPLY")
    Write-Host "=== Waiting $WaitSeconds seconds for STA result... ==="
    Start-Sleep -Seconds $WaitSeconds
    $buf = $port.ReadExisting()
    $lines = $buf -split "[\r\n]+" | Where-Object { $_ -match 'wifi|STA|AP|chan|begin|connect|fail|reason|disconn' }
    $lines | Select-Object -Last 20
} finally {
    if ($port.IsOpen) { $port.Close() }
}
