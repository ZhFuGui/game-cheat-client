$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path

function Resolve-ArduinoCli {
    param([string]$ExplicitPath = '')
    if ($ExplicitPath) { return (Resolve-Path -LiteralPath $ExplicitPath).Path }
    $taskCommand = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($taskCommand) { return $taskCommand.Source }
    $taskCandidates = @(
        'D:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe',
        (Join-Path $env:ProgramFiles 'Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'),
        (Join-Path $env:LOCALAPPDATA 'Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe')
    )
    foreach ($taskCandidate in $taskCandidates) {
        if (Test-Path -LiteralPath $taskCandidate) { return $taskCandidate }
    }
    throw 'Arduino CLI not found. Supply -ArduinoCliPath explicitly.'
}

function Resolve-Avrdude {
    $taskData = if ($env:ARDUINO_DIRECTORIES_DATA) { $env:ARDUINO_DIRECTORIES_DATA } else { Join-Path $env:LOCALAPPDATA 'Arduino15' }
    $taskBase = Join-Path $taskData 'packages\arduino\tools\avrdude'
    $taskVersions = @(Get-ChildItem -LiteralPath $taskBase -Directory | Sort-Object Name -Descending)
    foreach ($taskVersion in $taskVersions) {
        $taskExe = Join-Path $taskVersion.FullName 'bin\avrdude.exe'
        $taskConfig = Join-Path $taskVersion.FullName 'etc\avrdude.conf'
        if ((Test-Path -LiteralPath $taskExe) -and (Test-Path -LiteralPath $taskConfig)) {
            return @{ Exe = $taskExe; Config = $taskConfig }
        }
    }
    throw 'AVRDUDE not found in the installed Arduino AVR platform.'
}

function Invoke-AvrdudeBounded {
    param(
        [Parameter(Mandatory = $true)] $Tool,
        [Parameter(Mandatory = $true)] [string[]] $Arguments,
        [ValidateRange(10, 60)] [int] $TimeoutSeconds = 45,
        [ValidatePattern('^[a-zA-Z0-9_-]+$')] [string] $LogName = 'device'
    )
    # Start-Process uses a native command line, not a shell. Quote paths with spaces;
    # reject embedded quotes/newlines and trailing slashes rather than misquote them.
    $taskQuoted = foreach ($taskArgument in $Arguments) {
        if ($taskArgument -match '["\r\n]' -or $taskArgument.EndsWith('\')) {
            throw 'Unsupported native argument.'
        }
        '"' + $taskArgument + '"'
    }
    $taskLogDirectory = Join-Path $taskRoot 'logs'
    New-Item -ItemType Directory -Path $taskLogDirectory -Force | Out-Null
    $taskLogPrefix = Join-Path $taskLogDirectory ($LogName + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    $taskOutputLog = $taskLogPrefix + '.stdout.log'
    $taskErrorLog = $taskLogPrefix + '.stderr.log'
    Write-Host "AVRDUDE log: $taskErrorLog"
    $taskProgrammer = Start-Process -FilePath $Tool.Exe -ArgumentList ($taskQuoted -join ' ') `
        -WindowStyle Hidden -RedirectStandardOutput $taskOutputLog -RedirectStandardError $taskErrorLog -PassThru
    $null = $taskProgrammer.Handle # Retain the exit-code handle on Windows PowerShell 5.1 too.
    try {
        if (!$taskProgrammer.WaitForExit($TimeoutSeconds * 1000)) {
            # A broken USB driver can keep a terminated process alive until unplug.
            # Request termination, but never wait indefinitely for that cancellation.
            try { $taskProgrammer.Kill() } catch { Write-Warning $_.Exception.Message }
            Write-Host (Get-Content -LiteralPath $taskErrorLog -Raw)
            throw "AVRDUDE timed out; verification is incomplete. Process $($taskProgrammer.Id); log $taskErrorLog. If it remains alive, disconnect this board before retrying."
        }
        $taskProgrammer.Refresh()
        foreach ($taskLog in @($taskOutputLog, $taskErrorLog)) {
            $taskLogText = Get-Content -LiteralPath $taskLog -Raw
            if ($taskLogText) { Write-Host $taskLogText }
        }
        if ($taskProgrammer.ExitCode -ne 0) {
            throw "AVRDUDE failed ($($taskProgrammer.ExitCode)); log $taskErrorLog."
        }
    } finally { $taskProgrammer.Dispose() }
}
