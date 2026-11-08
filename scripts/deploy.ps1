<#
.SYNOPSIS
    Copies the built Purple Tuba VST3 into the system VST3 folder.

.DESCRIPTION
    Run after a build. Copies build\PurpleTuba_artefacts\<Config>\VST3\Purple Tuba.vst3 to
    C:\Program Files\Common Files\VST3, replacing any previous copy. Writing there needs
    administrator rights, so if this session is not elevated the script re-launches itself
    elevated (a UAC prompt appears) and prints the elevated run's output when it finishes.

.PARAMETER Config
    Build configuration to deploy (Release or Debug). Default: Release.

.PARAMETER BuildDir
    CMake build directory. Default: <repo>\build.

.EXAMPLE
    .\scripts\deploy.ps1
    .\scripts\deploy.ps1 -Config Debug
#>
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Config = 'Release',

    [string]$BuildDir,

    # Internal: set when the script re-launches itself elevated; output goes to this file.
    [string]$LogFile
)

$ErrorActionPreference = 'Stop'

# Windows PowerShell 5.1 doesn't set $PSScriptRoot while evaluating parameter defaults.
if (-not $BuildDir) {
    $BuildDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'build'
}
$BuildDir = [IO.Path]::GetFullPath($BuildDir)

$bundleName  = 'Purple Tuba.vst3'
$source      = Join-Path $BuildDir "PurpleTuba_artefacts\$Config\VST3\$bundleName"
$destDir     = Join-Path $env:CommonProgramFiles 'VST3'
$destination = Join-Path $destDir $bundleName

function Write-Log([string]$message) {
    Write-Host $message
    if ($LogFile) { Add-Content -Path $LogFile -Value $message -Encoding UTF8 }
}

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    (New-Object Security.Principal.WindowsPrincipal $identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Path -LiteralPath $source)) {
    Write-Log "Build output not found: $source"
    Write-Log "Build first: cmake --build build --config $Config --target PurpleTuba_VST3"
    exit 1
}

if (-not (Test-IsAdmin)) {
    # Re-launch elevated and wait; the elevated window's output is relayed through a log file.
    $log = Join-Path ([IO.Path]::GetTempPath()) ("PurpleTuba-deploy-{0}.log" -f [guid]::NewGuid())
    $arguments = @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass',
        '-File', "`"$PSCommandPath`"",
        '-Config', $Config,
        '-BuildDir', "`"$BuildDir`"",
        '-LogFile', "`"$log`""
    )

    Write-Host "Requesting administrator rights to write to $destDir ..."
    try {
        $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $arguments -Verb RunAs -Wait -PassThru -WindowStyle Hidden
    }
    catch {
        Write-Host "Elevation was cancelled or failed: $($_.Exception.Message)"
        exit 1
    }

    if (Test-Path -LiteralPath $log) {
        Get-Content -LiteralPath $log | ForEach-Object { Write-Host $_ }
        Remove-Item -LiteralPath $log -Force
    }
    exit $process.ExitCode
}

try {
    if (-not (Test-Path -LiteralPath $destDir)) {
        New-Item -ItemType Directory -Path $destDir | Out-Null
    }

    if (Test-Path -LiteralPath $destination) {
        Remove-Item -LiteralPath $destination -Recurse -Force
    }

    Copy-Item -LiteralPath $source -Destination $destDir -Recurse -Force

    $binary = Join-Path $destination 'Contents\x86_64-win\Purple Tuba.vst3'
    $built  = (Get-Item -LiteralPath $binary).LastWriteTime
    Write-Log "Deployed $Config build to $destination (binary built $built)."
    exit 0
}
catch {
    Write-Log "Deploy failed: $($_.Exception.Message)"
    Write-Log "If a DAW has Purple Tuba loaded, close it (or remove the plug-in) and run this again."
    exit 1
}
