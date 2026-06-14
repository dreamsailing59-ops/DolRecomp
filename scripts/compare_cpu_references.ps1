param(
    [string]$DolphinReference = "",
    [string]$PcReference = "",
    [string]$PcReferenceExe = "",
    [switch]$UseExistingDolphinReference
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (!$DolphinReference) {
    $DolphinReference = Join-Path $repoRoot "build\dolphin_cpu_reference.txt"
}
if (!$PcReference) {
    $PcReference = Join-Path $repoRoot "build\pc_cpu_reference.txt"
}
if (!$PcReferenceExe) {
    $candidate = Join-Path $repoRoot "build\test_pc_reference.exe"
    if (!(Test-Path -LiteralPath $candidate)) {
        $candidate = Join-Path $repoRoot "build\test_pc_reference"
    }
    $PcReferenceExe = $candidate
}

if (!$UseExistingDolphinReference -or !(Test-Path -LiteralPath $DolphinReference)) {
    & (Join-Path $PSScriptRoot "run_dolphin_reference.ps1") -OutputPath $DolphinReference
}

& $PcReferenceExe | Tee-Object -FilePath $PcReference | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "PC reference failed. See $PcReference."
}

function Read-ReferenceRows($path, $prefix) {
    $rows = @{}
    foreach ($line in Get-Content -LiteralPath $path) {
        if (!$line.StartsWith($prefix)) {
            continue
        }

        $parts = $line.Split(",")
        if ($parts.Count -lt 5) {
            continue
        }

        $name = $parts[1]
        $got = $parts[2]
        $status = $parts[4]
        $rows[$name] = [pscustomobject]@{
            Name = $name
            Got = $got
            Status = $status
            Line = $line
        }
    }
    return $rows
}

$dolphin = Read-ReferenceRows $DolphinReference "CPUREF,"
$pc = Read-ReferenceRows $PcReference "PCREF,"

$failures = @()
foreach ($row in $dolphin.Values) {
    if ($row.Status -ne "PASS") {
        $failures += "Dolphin reference failed: $($row.Line)"
    }
}
foreach ($row in $pc.Values) {
    if ($row.Status -ne "PASS") {
        $failures += "PC reference failed: $($row.Line)"
    }
}

$common = @()
foreach ($name in $dolphin.Keys) {
    if ($pc.ContainsKey($name)) {
        $common += $name
        if ($dolphin[$name].Got -ne $pc[$name].Got) {
            $failures += "Mismatch ${name}: Dolphin $($dolphin[$name].Got), PC $($pc[$name].Got)"
        }
    }
}

if ($common.Count -eq 0) {
    throw "No common reference rows to compare."
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    throw "$($failures.Count) reference comparison failure(s)."
}

Write-Output "matched $($common.Count) CPU reference rows between Dolphin and PC"
