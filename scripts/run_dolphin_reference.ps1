param(
    [string]$DolphinPath = "",
    [string]$DolPath = "",
    [string]$OutputPath = "",
    [int]$Port = 55020,
    [int]$ConnectTimeoutSeconds = 12,
    [int]$CaptureTimeoutSeconds = 20
)

$ErrorActionPreference = "Stop"

function Find-Dolphin {
    if ($DolphinPath -and (Test-Path -LiteralPath $DolphinPath)) {
        return (Resolve-Path -LiteralPath $DolphinPath).Path
    }

    if ($env:DOLPHIN_PATH -and (Test-Path -LiteralPath $env:DOLPHIN_PATH)) {
        return (Resolve-Path -LiteralPath $env:DOLPHIN_PATH).Path
    }

    $cmd = Get-Command Dolphin.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        (Join-Path $env:ProgramFiles "Dolphin Emulator\Dolphin.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "Dolphin Emulator\Dolphin.exe")
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    $downloads = Join-Path $HOME "Downloads"
    if (Test-Path -LiteralPath $downloads) {
        $found = Get-ChildItem -LiteralPath $downloads -Recurse -File -Filter Dolphin.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "dolphin" } |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    throw "Could not find Dolphin.exe. Pass -DolphinPath or set DOLPHIN_PATH."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if (!$DolPath) {
    $DolPath = Join-Path $repoRoot "tests\dolphin\test_opcodes.dol"
}
if (!$OutputPath) {
    $OutputPath = Join-Path $repoRoot "build\dolphin_cpu_reference.txt"
}

$DolphinPath = Find-Dolphin
$DolPath = (Resolve-Path -LiteralPath $DolPath).Path

$outDir = Split-Path -Parent $OutputPath
if ($outDir) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}
Remove-Item -LiteralPath $OutputPath -Force -ErrorAction SilentlyContinue

$args = @(
    "--batch",
    "--logger",
    "-C", "Dolphin.Core.CPUCore=0",
    "-C", "Dolphin.Core.SlotB=7",
    "--exec=$DolPath"
)

$process = Start-Process -FilePath $DolphinPath -ArgumentList $args -WindowStyle Hidden -PassThru

try {
    $client = $null
    $deadline = (Get-Date).AddSeconds($ConnectTimeoutSeconds)

    while ((Get-Date) -lt $deadline -and $null -eq $client) {
        $candidate = [System.Net.Sockets.TcpClient]::new()
        try {
            $async = $candidate.BeginConnect("127.0.0.1", $Port, $null, $null)
            if ($async.AsyncWaitHandle.WaitOne(250)) {
                $candidate.EndConnect($async)
                $client = $candidate
            } else {
                $candidate.Close()
            }
        } catch {
            $candidate.Close()
            Start-Sleep -Milliseconds 150
        }
    }

    if ($null -eq $client) {
        throw "USB Gecko TCP port $Port did not accept a connection."
    }

    $stream = $client.GetStream()
    $stream.ReadTimeout = 500
    $file = [System.IO.File]::Open($OutputPath, [System.IO.FileMode]::Create,
                                  [System.IO.FileAccess]::Write, [System.IO.FileShare]::Read)
    $buffer = New-Object byte[] 4096
    $captureDeadline = (Get-Date).AddSeconds($CaptureTimeoutSeconds)
    $seenText = ""

    try {
        while ((Get-Date) -lt $captureDeadline) {
            try {
                $count = $stream.Read($buffer, 0, $buffer.Length)
                if ($count -gt 0) {
                    $file.Write($buffer, 0, $count)
                    $file.Flush()
                    $chunk = [System.Text.Encoding]::ASCII.GetString($buffer, 0, $count)
                    $seenText += $chunk
                    if ($seenText.Contains("CPUREF_END")) {
                        break
                    }
                }
            } catch [System.IO.IOException] {
                Start-Sleep -Milliseconds 100
            }
        }
    } finally {
        $file.Close()
        $client.Close()
    }

    if (!(Test-Path -LiteralPath $OutputPath)) {
        throw "Dolphin reference capture did not create $OutputPath."
    }

    $result = Get-Content -LiteralPath $OutputPath -Raw
    if (!$result.Contains("CPUREF_END")) {
        throw "Dolphin reference capture timed out before CPUREF_END."
    }

    if ($result.Contains(",FAIL")) {
        throw "Dolphin reference reported at least one failing vector. See $OutputPath."
    }

    Write-Output "captured Dolphin reference: $OutputPath"
} finally {
    if ($process -and !$process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}
