[CmdletBinding(SupportsShouldProcess)]
param([string]$ParentDirectory = 'G:\')

$ErrorActionPreference = 'Stop'
$parent = Get-Item -LiteralPath $ParentDirectory
if ($parent -isnot [IO.DirectoryInfo] -or ($parent.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw "请选择实际的父目录。当前路径为 $ParentDirectory"
}

$sourcePath = Join-Path $parent.FullName 'mousebox-aimbot'
$targetPath = Join-Path $parent.FullName 'game-cheat-client'
$sourcePrefix = $sourcePath + '\'
$targetPrefix = $targetPath + '\'
$sourceExists = Test-Path -LiteralPath $sourcePath
$targetExists = Test-Path -LiteralPath $targetPath

if ($sourceExists -and $targetExists) {
    throw "目标路径已被占用。请检查 $targetPath"
}
if (!$sourceExists -and !$targetExists) {
    throw "找不到待迁移的项目目录 $sourcePath"
}

$projectPath = if ($sourceExists) { $sourcePath } else { $targetPath }
$project = Get-Item -LiteralPath $projectPath -Force
if ($project -isnot [IO.DirectoryInfo] -or $project.FullName -ine $projectPath -or
    ($project.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
    !(Test-Path -LiteralPath (Join-Path $projectPath '.git') -PathType Container) -or
    !(Test-Path -LiteralPath (Join-Path $projectPath 'game-cheat-client.sln') -PathType Leaf)) {
    throw "请确认 $projectPath 是已更新源码的项目根目录。"
}

$package = Get-Content -LiteralPath (Join-Path $projectPath 'package.json') -Encoding UTF8 -Raw | ConvertFrom-Json
if ($package.name -cne 'game-cheat-client') {
    throw "请先更新 $projectPath 中的项目源码。"
}

if ($sourceExists) {
    if (!$PSCmdlet.ShouldProcess($sourcePath, "改名为 $targetPath")) { return }
    if ($PSScriptRoot.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase) -or $PSScriptRoot -ieq $sourcePath) {
        throw '请将脚本复制到旧项目目录之外再运行。'
    }

    $running = @(Get-Process | Where-Object {
        $_.Path -and $_.Path.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase)
    })
    if ($running.Count) {
        throw "请先退出项目程序后再运行脚本。进程名 $($running.Name | Sort-Object -Unique)"
    }

    Set-Location -LiteralPath $parent.FullName
    [Environment]::CurrentDirectory = $parent.FullName
    try {
        Rename-Item -LiteralPath $sourcePath -NewName 'game-cheat-client'
    } catch {
        throw "目录迁移失败。请关闭占用旧目录的 Codex、Visual Studio 和终端，再重试。$($_.Exception.Message)"
    }
}
Write-Host "项目目录 $targetPath"

$desktopModules = Join-Path $targetPath 'apps\desktop\node_modules'
$shared = Join-Path $desktopModules '@mousebox\shared\package.json'
if ((Test-Path -LiteralPath $desktopModules -PathType Container) -and !(Test-Path -LiteralPath $shared) -and
    $PSCmdlet.ShouldProcess($targetPath, '重新建立 pnpm 依赖链接')) {
    Push-Location -LiteralPath $targetPath
    try {
        & pnpm.cmd install --offline --frozen-lockfile --force
        if ($LASTEXITCODE -ne 0) { throw "pnpm 退出码 $LASTEXITCODE" }
    } catch {
        Write-Warning "依赖链接待修复。可在新目录运行 pnpm install --frozen-lockfile --force。$($_.Exception.Message)"
    } finally {
        Pop-Location
    }
}

if ($parent.FullName -ine 'G:\') { return }
$desktop = [Environment]::GetFolderPath([Environment+SpecialFolder]::DesktopDirectory)
$linkPath = Join-Path $desktop 'mousebox-aimbot.lnk'
if (!(Test-Path -LiteralPath $linkPath -PathType Leaf)) { return }

try {
    $shell = New-Object -ComObject WScript.Shell
    $link = $shell.CreateShortcut($linkPath)
    $pointsAtOldRoot = $link.TargetPath.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase)
    if (!$pointsAtOldRoot -and !$link.TargetPath.StartsWith($targetPrefix, [StringComparison]::OrdinalIgnoreCase)) { return }
    if (!$PSCmdlet.ShouldProcess($linkPath, '更新桌面快捷方式')) { return }

    foreach ($property in @('TargetPath', 'WorkingDirectory', 'IconLocation')) {
        $path = $link.$property
        if ($path.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase)) {
            $link.$property = $targetPrefix + $path.Substring($sourcePrefix.Length)
        } elseif ($property -eq 'WorkingDirectory' -and $path -ieq $sourcePath) {
            $link.WorkingDirectory = $targetPath
        }
    }

    if (!(Test-Path -LiteralPath $link.TargetPath -PathType Leaf)) {
        throw "快捷方式目标未找到。当前目标为 $($link.TargetPath)"
    }
    $link.Save()
    $newLinkPath = Join-Path $desktop 'game-cheat-client.lnk'
    if (!(Test-Path -LiteralPath $newLinkPath)) {
        Rename-Item -LiteralPath $linkPath -NewName 'game-cheat-client.lnk'
    }
} catch {
    Write-Warning "目录已迁移。请检查桌面快捷方式。$($_.Exception.Message)"
}
