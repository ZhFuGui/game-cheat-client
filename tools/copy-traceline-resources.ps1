param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference='Stop'
$taskRepo=Split-Path $PSScriptRoot -Parent
$taskSource=Join-Path $taskRepo 'data/games/cs2/cover'
$taskCatalog=Get-Content -LiteralPath (Join-Path $taskSource 'manifest.json') -Raw | ConvertFrom-Json
$taskEntries=@('manifest.json',$taskCatalog.library.file)
if($taskCatalog.ballistics) { $taskEntries+=$taskCatalog.ballistics.file }
foreach($taskMap in $taskCatalog.maps) { foreach($taskScene in $taskMap.scenes) { $taskEntries+=$taskScene.file } }
$taskCopied=0
foreach($taskRelative in $taskEntries) {
 $taskFrom=Get-Item -LiteralPath (Join-Path $taskSource $taskRelative)
 $taskTo=Join-Path $Destination $taskRelative
 $taskExisting=Get-Item -LiteralPath $taskTo -ErrorAction SilentlyContinue
 if($taskExisting -and $taskExisting.Length -eq $taskFrom.Length -and $taskExisting.LastWriteTimeUtc -eq $taskFrom.LastWriteTimeUtc) { continue }
 New-Item -ItemType Directory -Path (Split-Path $taskTo -Parent) -Force | Out-Null
 Copy-Item -LiteralPath $taskFrom.FullName -Destination $taskTo -Force
 $taskCopied++
}
Write-Output "碰撞资源：复制 $taskCopied，跳过 $($taskEntries.Count-$taskCopied)；日常不重读场景内容"
