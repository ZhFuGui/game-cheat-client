function Get-ContentDigest([string]$LiteralPath) {
    $algorithm = [Security.Cryptography.SHA256]::Create()
    $stream = $null
    try {
        $stream = [IO.File]::OpenRead($LiteralPath)
        return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '')
    } finally {
        if ($stream) { $stream.Dispose() }
        $algorithm.Dispose()
    }
}
function Copy-ChangedFile([string]$Source, [string]$Destination) {
    $sourceFile = Get-Item -LiteralPath $Source
    $sourceDigest = Get-ContentDigest $Source
    $destinationFile = Get-Item -LiteralPath $Destination -ErrorAction SilentlyContinue
    if ($destinationFile -and $destinationFile.Length -eq $sourceFile.Length -and
        (Get-ContentDigest $Destination) -eq $sourceDigest) {
        return @{copied=$false;sha256=$sourceDigest}
    }
    New-Item -ItemType Directory -Path (Split-Path $Destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    return @{copied=$true;sha256=$sourceDigest}
}
function Write-ChangedText([string]$Destination, [string]$Text) {
    if ((Test-Path -LiteralPath $Destination) -and [IO.File]::ReadAllText($Destination) -ceq $Text) { return $false }
    [IO.File]::WriteAllText($Destination, $Text, (New-Object Text.UTF8Encoding($false)))
    return $true
}
