$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
function Get-ContractHash([string]$LiteralPath) {
    $stream = [IO.File]::OpenRead($LiteralPath)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '') }
    finally { $algorithm.Dispose(); $stream.Dispose() }
}
$vendor = Join-Path $repoRoot 'third_party/reader-consumer'
$protoRoot = Join-Path $repoRoot 'protocol/sources/pubg_reader'
$descriptorPin = '4E5F26FED497F86A37D686DF443BE54D42DCC5E1EEEAE79143A19F209E5F6F92'
$manifestPin = 'AF09AE0F45189FF7B544E1CA96C3C0DAF663E43B8DE0C1EA751D5D063B4E463B'
$sourcePin = 'D10E21B2F7AE918CB0316ADB9CB804378C11441300B04B4BF520EC0EDF273288'
$propsPin = '1F97D270E478FE99CB2F999240BC277B2438884EB38F3E47F8373940B30D6DF3'
if ((Get-ContractHash (Join-Path $vendor 'SDK_MANIFEST.json')) -ne $manifestPin -or
    (Get-ContractHash (Join-Path $vendor 'SOURCE_MANIFEST.json')) -ne $sourcePin -or
    (Get-ContractHash (Join-Path $vendor 'client-sources.props')) -ne $propsPin) {
    throw 'Reader SDK/source/client compilation manifest differs from the frozen v3 handoff.'
}
$sdk = Get-Content -LiteralPath (Join-Path $vendor 'SDK_MANIFEST.json') -Raw | ConvertFrom-Json
if ($sdk.protocol_version -ne 3 -or $sdk.clock_contract_version -ne 1 -or $sdk.legacy_compatibility -or
    $sdk.schema_descriptor_sha256 -ne $descriptorPin -or $sdk.supported_features.Count -ne 1 -or $sdk.supported_features[0] -ne 0) {
    throw 'Unexpected Reader v3 contract.'
}
foreach ($entry in $sdk.files) {
    if ((Get-ContractHash (Join-Path $vendor $entry.path)) -ne $entry.sha256) { throw "Frozen SDK file changed: $($entry.path)" }
    if ($entry.path.StartsWith('protocol/')) {
        if ((Get-ContractHash (Join-Path $protoRoot $entry.path.Substring(9))) -ne $entry.sha256) {
            throw "Public protocol mirror changed: $($entry.path)"
        }
    }
}
foreach ($path in @((Join-Path $vendor 'protocol-descriptor.pb'), (Join-Path $protoRoot 'protocol-descriptor.pb'))) {
    if ((Get-ContractHash $path) -ne $descriptorPin) { throw 'Reader descriptor pin mismatch.' }
}
$sourceRoot = Join-Path $vendor 'src'
$snapshot = @(Get-Content -LiteralPath (Join-Path $vendor 'SOURCE_MANIFEST.json') -Raw | ConvertFrom-Json | ForEach-Object { $_ })
if (@(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File).Count -ne $snapshot.Count) { throw 'SDK source closure changed.' }
foreach ($entry in $snapshot) {
    if ((Get-ContractHash (Join-Path $sourceRoot $entry.path)) -ne $entry.sha256) { throw "Frozen source changed: $($entry.path)" }
}
$clients = @($sdk.client_cpp_sources | Sort-Object)
$providers = @($sdk.provider_cpp_sources | Sort-Object)
$allCpp = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -Filter '*.cpp' -File | ForEach-Object { 'src/' + $_.FullName.Substring($sourceRoot.Length + 1).Replace('\', '/') } | Sort-Object)
$listed = @(Get-Content -LiteralPath (Join-Path $vendor 'consumer-sources.txt') | Sort-Object)
if ($clients.Count -ne 17 -or $providers.Count -ne 11 -or
    @(Compare-Object $clients $listed).Count -or @(Compare-Object $clients @($sdk.cpp_sources | Sort-Object)).Count -or
    @(Compare-Object $allCpp @(($clients + $providers) | Sort-Object)).Count) { throw 'Client/provider compilation closure changed.' }
$props = [xml](Get-Content -LiteralPath (Join-Path $vendor 'client-sources.props') -Raw)
$compiled = @($props.Project.ItemGroup.ClCompile | ForEach-Object { $_.Include.Replace('$(RepoRoot)third_party\reader-consumer\', '').Replace('\', '/') } | Sort-Object)
if (@(Compare-Object $compiled $clients).Count) { throw 'MSBuild compiles a non-client SDK source.' }
$schema = Get-Content -LiteralPath (Join-Path $protoRoot 'protocol-schema-manifest.json') -Raw | ConvertFrom-Json
if ($schema.wire_protocol_version -ne 3 -or $schema.descriptor_sha256 -ne $descriptorPin -or $schema.schemas.Count -ne 12) {
    throw 'Reader protocol schema manifest changed.'
}
$header = Get-Content -LiteralPath (Join-Path $sourceRoot 'synchronization/codec/schema.hpp') -Raw
$adapter = Get-Content -LiteralPath (Join-Path $repoRoot 'src/adapters/sources/pubg_reader/contract.hpp') -Raw
if ($header -notmatch "(?i)CodecSchemaDescriptorSha256: $descriptorPin" -or $adapter -notmatch "(?i)$descriptorPin") {
    throw 'Reader generated schema/adapter descriptor mismatch.'
}
Write-Output "PASS: Reader Consumer v3 / 99 SDK files / 86 source files / 17 client CPP / 11 excluded provider CPP / 12 proto"
