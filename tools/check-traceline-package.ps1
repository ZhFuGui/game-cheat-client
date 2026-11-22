param([switch]$IncludeScenes)
$ErrorActionPreference='Stop'
function Get-TaskDigest([string]$Path) {
 $taskAlgorithm=[Security.Cryptography.SHA256]::Create()
 $taskStream=[IO.File]::OpenRead($Path)
 try { return [BitConverter]::ToString($taskAlgorithm.ComputeHash($taskStream)).Replace('-','').ToLowerInvariant() }
 finally { $taskStream.Dispose(); $taskAlgorithm.Dispose() }
}
$taskRepo=Split-Path $PSScriptRoot -Parent
$taskPublic=Join-Path $taskRepo 'third_party/traceline'
$taskBundle=Join-Path $taskRepo 'data/games/cs2/cover'
$taskLock=Get-Content -LiteralPath (Join-Path $taskPublic 'manifest.json') -Raw | ConvertFrom-Json
$taskCatalogFile=Join-Path $taskBundle 'manifest.json'
if ((Get-TaskDigest $taskCatalogFile) -ne $taskLock.catalogSha256) { throw 'traceline场景清单与锁定版本不一致' }
$taskCatalog=Get-Content -LiteralPath $taskCatalogFile -Raw | ConvertFrom-Json
if ($taskCatalog.schema -ne 1 -or $taskCatalog.abi -ne $taskLock.abi -or $taskCatalog.scope -ne 'static-physics' -or $taskCatalog.breakables -ne 'intact') { throw 'traceline包版本不兼容' }
foreach($taskHeader in $taskLock.headers) {
 $taskFile=Join-Path $taskPublic $taskHeader.file
 if ((Get-TaskDigest $taskFile) -ne $taskHeader.sha256) { throw 'traceline公开头文件不匹配' }
}
$taskFiles=@($taskCatalog.library)
if($taskCatalog.ballistics) { $taskFiles+=@($taskCatalog.ballistics) }
if($IncludeScenes) { foreach($taskMap in $taskCatalog.maps) { $taskFiles+=@($taskMap.scenes) } }
foreach($taskEntry in $taskFiles) {
 if($taskEntry.file -match '(^|/)\.\.(/|$)|[:\\]' -or [IO.Path]::IsPathRooted($taskEntry.file)) { throw 'traceline资源路径无效' }
 $taskFile=Join-Path $taskBundle $taskEntry.file
 if ((Get-Item -LiteralPath $taskFile).Length -ne $taskEntry.bytes -or (Get-TaskDigest $taskFile) -ne $taskEntry.sha256) { throw "traceline资源校验失败：$($taskEntry.file)" }
}
if($taskCatalog.library.sha256 -ne $taskLock.librarySha256) { throw 'traceline DLL与头文件版本不匹配' }
$taskCacheCount=0
foreach($taskMap in $taskCatalog.maps) { $taskCacheCount+=@($taskMap.scenes).Count }
$taskActual=@(Get-ChildItem -LiteralPath $taskBundle -Recurse -File)
if($taskActual.Count -ne ($taskCacheCount+2+[int][bool]$taskCatalog.ballistics) -or @($taskActual | Where-Object { $_.Extension -notin '.cs2tl','.cs2tlp','.dll','.json' }).Count) { throw 'traceline包含未登记文件或地图源文件' }
Write-Output "PASS: traceline ABI $($taskCatalog.abi), $(@($taskCatalog.maps).Count) maps, $taskCacheCount collision caches, public API and DLL pinned"
