param([string]$DriverInf = '')
$ErrorActionPreference = 'Stop'
$firmwareRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
if (-not $DriverInf) { $DriverInf = Join-Path $firmwareRoot 'build/drivers/cp210x/microsoft-package/silabser.inf' }
$inf = (Resolve-Path -LiteralPath $DriverInf).Path
$package = Split-Path $inf -Parent
$signature = Get-AuthenticodeSignature -LiteralPath (Join-Path $package 'silabser.cat')
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Windows Hardware Compatibility Publisher') {
    throw '驱动目录签名未通过微软硬件兼容发布者校验。'
}
if ((Get-Content -LiteralPath $inf -Raw) -notmatch 'USB\\VID_10C4&PID_EA60') {
    throw '该驱动没有声明当前 CP2102 的 USB 身份。'
}
$principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $arguments = '-NoProfile -File "' + $PSCommandPath + '" -DriverInf "' + $inf + '"'
    $process = Start-Process -FilePath (Join-Path $PSHOME 'pwsh.exe') -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -PassThru
    Write-Output "驱动安装进程已启动：PID $($process.Id)。完成后核对安装日志和设备身份。"
    return
}
$log = Join-Path $firmwareRoot 'build/drivers/cp210x/install.log'
& (Join-Path $env:WINDIR 'System32/pnputil.exe') /add-driver $inf /install 2>&1 | Out-File -LiteralPath $log -Encoding utf8
$result = $LASTEXITCODE
"ExitCode=$result" | Add-Content -LiteralPath $log -Encoding utf8
if ($result -ne 0 -and $result -ne 3010) { throw "驱动安装失败：$result。详见 $log" }
