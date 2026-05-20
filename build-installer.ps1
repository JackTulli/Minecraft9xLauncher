#requires -Version 5.0
<#
Builds the Minecraft9xLauncher installer.

Steps:
  1. Compile installer.cpp + installer.rc with OpenWatcom 1.9 (./WATCOM).
  2. Stage the payload tree under .\dist\data\ from the working tree, excluding
     user data (saves, crash-reports, logs, options.txt, etc.).
  3. Copy the freshly-built launcher into the payload.
  4. Drop the installer exe at .\dist\Minecraft9xLauncher-Setup.exe.

Usage: .\build-installer.ps1
       .\build-installer.ps1 -NoLauncherRebuild   # skip launcher compile
#>

param(
    [switch]$NoLauncherRebuild
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$WATCOM   = (Resolve-Path .\WATCOM).Path
$env:PATH = "$WATCOM\binnt;$env:PATH"
$env:INCLUDE = "$WATCOM\h;$WATCOM\h\nt"

function Run([string]$desc, [scriptblock]$block) {
    Write-Host ">> $desc" -ForegroundColor Cyan
    & $block
    if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne $null) {
        throw "$desc failed (exit $LASTEXITCODE)"
    }
}

# -------- 1. (re)build the launcher first --------
if (-not $NoLauncherRebuild) {
    Run "Compiling launcher.cpp" {
        Remove-Item -ErrorAction SilentlyContinue launcher.obj
        & wpp386.exe launcher.cpp -w4 -e25 -zq -od -d2 -6r -bt=nt `
            -fo="launcher.obj" -mf -xs -xr | Out-Null
    }
    Run "Linking launcher" {
        "FIL launcher.obj" | Out-File -Encoding ascii Minecraft9xLauncher.lk1
        & wlink.exe name Minecraft9xLauncher d all sys nt_win `
            op m op maxe=25 op q op symf "@Minecraft9xLauncher.lk1" | Out-Null
    }
    Run "Binding launcher icon" {
        & wrc.exe -q -bt=nt launcher.rc Minecraft9xLauncher.exe | Out-Null
    }
}

# -------- 2. compile installer.cpp --------
Run "Compiling installer.cpp" {
    Remove-Item -ErrorAction SilentlyContinue installer.obj
    & wpp386.exe installer.cpp -w4 -e25 -zq -od -d2 -6r -bt=nt `
        -fo="installer.obj" -mf -xs -xr | Out-Null
}

Run "Linking installer" {
    "FIL installer.obj" | Out-File -Encoding ascii installer.lk1
    & wlink.exe name installer d all sys nt_win `
        op m op maxe=25 op q op symf `
        library shell32.lib `
        library ole32.lib `
        library uuid.lib `
        library comctl32.lib `
        "@installer.lk1" | Out-Null
}

Run "Binding installer resources" {
    & wrc.exe -q -bt=nt installer.rc installer.exe | Out-Null
}

# -------- 3. stage payload under .\dist\data\ --------
$Dist    = Join-Path $PSScriptRoot 'dist'
$Payload = Join-Path $Dist 'data'

Write-Host ">> Staging payload under $Payload" -ForegroundColor Cyan
if (Test-Path $Dist) { Remove-Item $Dist -Recurse -Force }
New-Item -ItemType Directory -Path $Payload | Out-Null

# Top-level files we want in the payload.
$TopFiles = @(
    'Minecraft9xLauncher.exe',
    'LICENSE',
    'release_notes.txt',
    'README.md'
)
foreach ($f in $TopFiles) {
    if (Test-Path $f) { Copy-Item $f $Payload }
}

# Bundle a fresh settings.txt with safe defaults (not the dev's local one).
@'
version=beta1.7.3
username=Player
ram_max_mb=1024
ram_min_mb=256
nosound=0
gl_mode=default
java=java
'@ | Out-File -Encoding ascii (Join-Path $Payload 'settings.txt')

# Mirror resources\ with exclusions.
$srcRes = Join-Path $PSScriptRoot 'resources'
$dstRes = Join-Path $Payload 'resources'
$excludeDirs  = @('crash-reports','logs','saves','stats')
$excludeFiles = @('options.txt','usercache.json','log.txt','.gl_manifest','servers.dat','servers.dat_old','realms_persistence.json')

Write-Host ">> Mirroring resources\ (excluding user data)" -ForegroundColor Cyan
Get-ChildItem $srcRes -Recurse -Force | ForEach-Object {
    $rel = $_.FullName.Substring($srcRes.Length + 1)
    $segs = $rel -split '\\'
    foreach ($s in $segs) {
        if ($excludeDirs -contains $s) { return }
    }
    if ($_.PSIsContainer) {
        $dst = Join-Path $dstRes $rel
        if (-not (Test-Path $dst)) { New-Item -ItemType Directory -Path $dst | Out-Null }
    } else {
        if ($excludeFiles -contains $_.Name) { return }
        if ($_.Extension -eq '.bat') { return }
        $dst = Join-Path $dstRes $rel
        $parent = Split-Path $dst -Parent
        if (-not (Test-Path $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
        Copy-Item $_.FullName $dst -Force
    }
}

# -------- 4. pack payload + append to installer.exe --------
$SetupExe = Join-Path $Dist 'Minecraft9xLauncher-Setup.exe'
Copy-Item (Join-Path $PSScriptRoot 'installer.exe') $SetupExe

$payloadCount = (Get-ChildItem $Payload -Recurse -File).Count
$payloadBytes = (Get-ChildItem $Payload -Recurse -File | Measure-Object Length -Sum).Sum

Run "Appending payload to installer ($payloadCount files, $('{0:N0}' -f $payloadBytes) bytes)" {
    $fs = [System.IO.File]::Open($SetupExe, [System.IO.FileMode]::Append)
    try {
        $bw = New-Object System.IO.BinaryWriter($fs)
        $archiveStart = $fs.Position
        $count = 0
        # Drive the enumeration directly so we don't load everything into memory.
        Get-ChildItem $Payload -Recurse -File | ForEach-Object {
            $rel = $_.FullName.Substring($Payload.Length + 1) -replace '\\', '/'
            $relBytes = [System.Text.Encoding]::UTF8.GetBytes($rel)
            if ($relBytes.Length -ge 65535) {
                throw "Path too long for archive: $rel"
            }
            $bw.Write([UInt16]$relBytes.Length)
            $bw.Write($relBytes)
            $bw.Write([UInt32]$_.Length)
            $src = [System.IO.File]::OpenRead($_.FullName)
            try { $src.CopyTo($fs) } finally { $src.Close() }
            $count++
        }
        $payloadSize  = $fs.Position - $archiveStart
        $payloadLow   = [UInt32]($payloadSize -band 0xFFFFFFFF)
        $payloadHigh  = [UInt32]([math]::Floor($payloadSize / 4294967296))

        # 24-byte footer: u32 count, u32 sizeLow, u32 sizeHigh, char[12] magic
        $bw.Write([UInt32]$count)
        $bw.Write($payloadLow)
        $bw.Write($payloadHigh)
        $bw.Write([System.Text.Encoding]::ASCII.GetBytes("MC9XSFXEND" + [char]0 + [char]0))
    } finally {
        $fs.Close()
    }
}

# Clean up the staging directory now that everything is inside the exe.
Remove-Item $Payload -Recurse -Force

# -------- summary --------
$finalSize = (Get-Item $SetupExe).Length
Write-Host ""
Write-Host "Setup built (single-file)." -ForegroundColor Green
Write-Host ("  {0}  ({1:N0} bytes, {2} embedded files)" -f $SetupExe, $finalSize, $payloadCount)
