# Release a Hydro Hub firmware update over the air.
#
# What it does:
#   1. Compiles the sketch with arduino-cli (the Arduino IDE's bundled copy).
#   2. Uploads Hydro-Hub.ino.bin to the public Supabase "firmware" bucket.
#   3. Inserts a row in hydro_hub_firmware_releases.
#   Every online hub picks the release up on its next sync (~2s), flashes the
#   inactive OTA slot and reboots into it.
#
# Usage (from the repo root or anywhere):
#   powershell -ExecutionPolicy Bypass -File "Hydro Hub Device\tools\release-ota.ps1"
#
# Requirements:
#   - Bump FIRMWARE_VERSION in Hydro-Hub/config.h first (the script refuses to
#     re-release an existing version).
#   - SUPABASE_SERVICE_ROLE_KEY in "Water System Mobile App\.env".
#
# Safety: a hub that crashes on boot after a bad update can only be recovered
# over USB — this script therefore refuses to publish if the compile fails.

$ErrorActionPreference = "Stop"

$RepoRoot   = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)   # ...\Water-System
$SketchDir  = Join-Path $RepoRoot "Hydro Hub Device\Hydro-Hub"
$EnvFile    = Join-Path $RepoRoot "Water System Mobile App\.env"
$BuildDir   = Join-Path $env:TEMP "hydro-hub-ota-build"
$Cli        = Join-Path $env:LOCALAPPDATA "Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
# PSRAM must stay DISABLED: this board has no working PSRAM, and firmware
# built with PSRAM=opi boot-loops (INT_WDT) — caused the 1.2.1 brick.
$Fqbn       = "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=disabled,PartitionScheme=custom"
$SupabaseUrl = "https://aidpejxlofvdrtemurft.supabase.co"

# -- Read the version from config.h --
$configPath = Join-Path $SketchDir "config.h"
$versionMatch = Select-String -Path $configPath -Pattern 'FIRMWARE_VERSION\[\]\s*=\s*"([^"]+)"'
if (-not $versionMatch) { throw "FIRMWARE_VERSION not found in config.h" }
$Version = $versionMatch.Matches[0].Groups[1].Value
Write-Host "Releasing firmware version: $Version"

# -- Read the service role key from the app .env --
$keyLine = Select-String -Path $EnvFile -Pattern '^SUPABASE_SERVICE_ROLE_KEY=(.+)$'
if (-not $keyLine) { throw "SUPABASE_SERVICE_ROLE_KEY not found in $EnvFile" }
$ServiceKey = $keyLine.Matches[0].Groups[1].Value.Trim()

$headers = @{
  "apikey"        = $ServiceKey
  "Authorization" = "Bearer $ServiceKey"
}

# -- Refuse to re-release an existing version --
$existing = Invoke-RestMethod -Headers $headers -Uri "$SupabaseUrl/rest/v1/hydro_hub_firmware_releases?version=eq.$Version&select=version"
if ($existing.Count -gt 0) {
  throw "Version $Version already released. Bump FIRMWARE_VERSION in config.h first."
}

# -- Compile --
Write-Host "Compiling..."
& $Cli compile --fqbn $Fqbn --output-dir $BuildDir $SketchDir
if ($LASTEXITCODE -ne 0) { throw "Compile failed - not publishing." }

$BinPath = Join-Path $BuildDir "Hydro-Hub.ino.bin"
if (-not (Test-Path $BinPath)) { throw "Compiled binary not found: $BinPath" }
$sizeKb = [math]::Round((Get-Item $BinPath).Length / 1kb)
$sha256 = (Get-FileHash -Algorithm SHA256 $BinPath).Hash.ToLower()
Write-Host "Binary: $sizeKb KB  sha256: $sha256"

# -- Upload to the public firmware bucket --
$objectPath = "hydro-hub/Hydro-Hub-$Version.bin"
Write-Host "Uploading to storage: $objectPath"
$uploadHeaders = $headers.Clone()
$uploadHeaders["Content-Type"] = "application/octet-stream"
$uploadHeaders["x-upsert"] = "true"
Invoke-RestMethod -Method Post -Headers $uploadHeaders `
  -Uri "$SupabaseUrl/storage/v1/object/firmware/$objectPath" `
  -InFile $BinPath | Out-Null

$publicUrl = "$SupabaseUrl/storage/v1/object/public/firmware/$objectPath"

# -- Publish the release row --
Write-Host "Publishing release row..."
$releaseHeaders = $headers.Clone()
$releaseHeaders["Content-Type"] = "application/json"
$releaseHeaders["Prefer"] = "return=representation"
$body = @{
  version      = $Version
  firmware_url = $publicUrl
  sha256       = $sha256
  is_active    = $true
} | ConvertTo-Json
$row = Invoke-RestMethod -Method Post -Headers $releaseHeaders `
  -Uri "$SupabaseUrl/rest/v1/hydro_hub_firmware_releases" -Body $body

Write-Host ""
Write-Host "=== Released $Version ==="
Write-Host "URL: $publicUrl"
Write-Host "Every online hub will update within seconds of its next sync."
Write-Host "To pull this release:  update hydro_hub_firmware_releases set is_active=false where version='$Version';"
