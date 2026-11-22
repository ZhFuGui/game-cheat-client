function Resolve-FirmwarePath {
    param([string]$Root, [string]$Relative)
    $base = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $value = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    if (!$value.StartsWith($base, [StringComparison]::OrdinalIgnoreCase)) { throw 'Firmware path escapes workspace.' }
    return $value
}

function Read-FirmwareTarget {
    param([string]$Root, [string]$Id)
    if ($Id -notmatch '^[a-z0-9_]+$') { throw 'Invalid target ID.' }
    $path = Resolve-FirmwarePath $Root ('targets/' + $Id + '/target.json')
    $target = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    if ($target.schemaVersion -ne 1 -or $target.id -cne $Id -or $target.version -notmatch '^\d+\.\d+\.\d+$') {
        throw 'Invalid target manifest identity/version.'
    }
    if ($target.builder -notin @('arduino-avr', 'simulation')) { throw 'Unsupported target builder.' }
    $caps = $target.capabilities
    if ($caps.baudRate -lt 9600 -or $caps.baudRate -gt 4000000 -or $caps.maxStepCounts -lt 1 -or
        $caps.maxStepCounts -gt 32767 -or $caps.minIntervalMs -lt 1 -or $caps.minIntervalMs -gt 50 -or
        $caps.maxBudget -lt 1 -or $caps.maxBudget -gt 65535) { throw 'Invalid target capabilities.' }
    # Existing feedback flags encode fixed limits. Unknown values require an explicit protocol extension.
    if ($caps.legacyFlags -in @(12,28)) {
        if ($caps.maxStepCounts -ne 100 -or $caps.minIntervalMs -ne 2 -or $caps.maxBudget -ne 200) {
            throw 'Legacy high-rate capability mismatch.'
        }
    } else { throw 'No supported legacy capability mapping for this target.' }
    if (2 * 24 * 10 * 1000 / $caps.baudRate -gt $caps.minIntervalMs) { throw 'Control channel has insufficient bandwidth.' }
    if ($target.builder -eq 'arduino-avr') {
        if ($target.fqbn -cne 'mousebox:avr:leonardo_c54d' -or $target.flash.backend -cne 'avr109' -or
            $target.flash.mcu -cne 'atmega32u4' -or $target.flash.applicationBytes -ne 28672 -or
            $target.flash.flashBytes -ne 32768 -or $target.flash.eepromBytes -ne 1024) {
            throw 'Unsupported AVR board/memory layout.'
        }
        $null = Get-Item -LiteralPath (Resolve-FirmwarePath $Root $target.composition)
    }
    return $target
}

function Write-FirmwareTargetHeader {
    param([string]$Root, $Target, [string]$Directory)
    $null = New-Item -ItemType Directory -Path $Directory -Force
    $caps = $Target.capabilities
    $include = (Resolve-FirmwarePath $Root 'core/contracts/capabilities.hpp').Replace('\', '/')
    $text = @"
#pragma once
// Generated from targets/$($Target.id)/target.json. Do not edit.
#include "$include"
namespace mousebox_generated {
typedef mousebox_firmware::MotionLimits<$($caps.maxStepCounts), $($caps.minIntervalMs), $($caps.maxBudget)> Limits;
struct Capabilities {
    static const uint32_t BaudRate = $($caps.baudRate)UL;
    enum { FeedbackFlags = $($caps.legacyFlags) };
};
}
"@
    [IO.File]::WriteAllText((Join-Path $Directory 'target_capabilities.hpp'), $text, [Text.UTF8Encoding]::new($false))
}

function Assert-FirmwareDependencies {
    param([string]$Root)
    $lock = Get-Content -LiteralPath (Join-Path $Root 'third_party/dependencies.lock.json') -Raw | ConvertFrom-Json
    foreach ($dependency in $lock.dependencies) {
        $dependencyRoot = Resolve-FirmwarePath $Root ('third_party/' + $dependency.root)
        $actual = @(Get-ChildItem -LiteralPath $dependencyRoot -File -Recurse)
        if ($actual.Count -ne @($dependency.files).Count) { throw ('Unpinned upstream files: ' + $dependency.id) }
        foreach ($file in $dependency.files) {
            $path = Resolve-FirmwarePath $Root ('third_party/' + $dependency.root + '/' + $file.path)
            if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $file.sha256) {
                throw ('Pinned upstream differs: ' + $dependency.id + '/' + $file.path)
            }
        }
    }
    foreach ($file in $lock.overlays.files) {
        $base = Resolve-FirmwarePath $Root ('third_party/upstream/arduino-avr-1.8.7/' + $file.basePath)
        if ((Get-FileHash -LiteralPath $base -Algorithm SHA256).Hash -ine $file.baseSha256) { throw 'AVR overlay base mismatch.' }
        $path = Resolve-FirmwarePath $Root ('third_party/' + $lock.overlays.root + '/' + $file.path)
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $file.sha256) {
            throw ('Pinned AVR overlay differs: ' + $file.path)
        }
    }
    return $lock
}
