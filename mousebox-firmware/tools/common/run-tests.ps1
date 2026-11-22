param(
 [string]$CompilerPath = '',
 [ValidateSet('all','core','transport','simulation','tools')][string]$Suite = 'all'
)
. (Join-Path $PSScriptRoot '../toolchain.ps1')
. (Join-Path $PSScriptRoot 'target.ps1')
$target = Read-FirmwareTarget $taskRoot 'leonardo_max3421e_c54d'
$generated = Join-Path $taskRoot 'build/tests/generated'
Write-FirmwareTargetHeader $taskRoot $target $generated
$compiler = if ($CompilerPath) { (Resolve-Path -LiteralPath $CompilerPath).Path } else { (Get-Command g++ -ErrorAction Stop).Source }
$output = Join-Path $taskRoot 'build/tests'
$sources = @()
if ($Suite -in @('all','core')) { $sources += @('tests/control_tests.cpp','tests/feedback_tests.cpp','tests/rate_tests.cpp','tests/protocol_tests.cpp','tests/button_pulse_tests.cpp') }
if ($Suite -in @('all','transport')) { $sources += 'tests/transport_tests.cpp' }
if ($Suite -in @('all','simulation')) { $sources += 'targets/simulated_c54d/main.cpp' }
foreach ($source in $sources) {
 $exe = Join-Path $output (([IO.Path]::GetFileNameWithoutExtension($source)) + '.exe')
 $includes = @(('-I'+$generated), ('-I'+(Join-Path $taskRoot 'protocol/include')))
 if ($source -eq 'tests/transport_tests.cpp') { $includes += '-I'+(Join-Path $taskRoot 'tests/stubs') }
 & $compiler -std=c++11 -pedantic -Wall -Wextra -Werror @includes (Join-Path $taskRoot $source) -o $exe
 if ($LASTEXITCODE -ne 0) { throw ('Test compile failed: '+$source) }
 & $exe
 if ($LASTEXITCODE -ne 0) { throw ('Test failed: '+$source) }
}
if ($Suite -in @('all','tools')) {
 & (Join-Path $taskRoot 'tests/target-tests.ps1')
 & (Join-Path $taskRoot 'tests/artifact-tests.ps1')
 & (Join-Path $taskRoot 'tests/avr109-tests.ps1')
 & (Join-Path $taskRoot 'tests/device-location-tests.ps1')
}
