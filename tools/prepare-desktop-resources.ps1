param([ValidateSet('Release')][string]$Configuration='Release', [switch]$FullVerification)
$ErrorActionPreference='Stop'
$repoRoot=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'lib/files.ps1')
& (Join-Path $PSScriptRoot 'check-traceline-package.ps1') -IncludeScenes:$FullVerification
$nativeSource=Join-Path $repoRoot "out/x64/$Configuration/mousebox-native.exe"
if(!(Test-Path -LiteralPath $nativeSource)){throw '请先构建 Release 原生宿主'}
$inputSource=Join-Path $repoRoot "out/x64/$Configuration/mousebox-input.node"
if(!(Test-Path -LiteralPath $inputSource)){throw '请先构建 Release 前台原生输入模块'}
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPaths=@(& $vswhere -all -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if(!$vsPaths.Count){throw '未找到 Visual Studio C++ 运行库目录'}
$versions=@(foreach($vsPath in $vsPaths){
 $redist=Join-Path $vsPath 'VC/Redist/MSVC'
 if(Test-Path -LiteralPath $redist){Get-ChildItem -LiteralPath $redist -Directory | Where-Object Name -Match '^\d+\.\d+\.\d+'}
}) | Sort-Object { [version]$_.Name } -Descending
$crtPath=$null
foreach($version in $versions){$candidate=Join-Path $version.FullName 'x64/Microsoft.VC143.CRT';if(Test-Path -LiteralPath $candidate){$crtPath=$candidate;break}}
if(!$crtPath){throw '未找到 Microsoft.VC143.CRT；请安装 VS C++ 桌面开发运行库'}
$destination=Join-Path $repoRoot 'out/desktop-native'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
$sources=@((Get-Item -LiteralPath $nativeSource), (Get-Item -LiteralPath $inputSource))
$sources+=@(Get-ChildItem -LiteralPath $crtPath -Filter '*.dll' -File)
$manifest=@(); $copied=0
foreach($source in ($sources | Sort-Object Name)) {
 $result=Copy-ChangedFile $source.FullName (Join-Path $destination $source.Name)
 if($result.copied){$copied++}
 $manifest+=@([ordered]@{name=$source.Name;sha256=$result.sha256})
}
$oldLicense=Join-Path $destination 'LICENSE-NODE-API.txt'
if(Test-Path -LiteralPath $oldLicense){Remove-Item -LiteralPath $oldLicense}
$updated=Write-ChangedText (Join-Path $destination 'runtime-manifest.json') (($manifest | ConvertTo-Json -Compress)+"`n")
Write-Output "原生运行资源：复制 $copied，跳过 $($sources.Count-$copied)，清单更新 $updated"
