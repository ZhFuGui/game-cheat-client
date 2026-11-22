param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$IncludeLocalTools,
    [ValidateRange(1, 32)][int]$CompilerProcesses = 4
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw '未找到 Visual Studio Installer，请安装 Visual Studio C++ 桌面开发组件。' }
$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw '未找到 MSBuild。' }
$solution = if ($IncludeLocalTools) { 'mousebox-aimbot.local.sln' } else { 'mousebox-aimbot.sln' }
if (!(Test-Path -LiteralPath (Join-Path $repoRoot $solution))) { throw "未找到解决方案：$solution" }
$lockFolder = Join-Path $repoRoot 'out/workflow'
New-Item -ItemType Directory -Path $lockFolder -Force | Out-Null
$buildLock = [IO.File]::Open((Join-Path $lockFolder "native-$Configuration.lock"), 'OpenOrCreate', 'ReadWrite', 'None')
try {
& $msbuild (Join-Path $repoRoot $solution) /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" /p:Platform=x64 "/p:CL_MPCount=$CompilerProcesses"
if ($LASTEXITCODE -ne 0) { throw "VS 工程编译失败：$LASTEXITCODE" }
& (Join-Path $PSScriptRoot 'copy-traceline-resources.ps1') -Destination (Join-Path $repoRoot "out/x64/$Configuration/data/games/cs2/cover")
} finally { $buildLock.Dispose() }
