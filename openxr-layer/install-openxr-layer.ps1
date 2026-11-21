# Registers the MultiTron OpenXR API layer for the current user (no admin needed).
# Run from the folder containing XR_APILAYER_MULTITRON_stick.json.
$manifest = Join-Path $PSScriptRoot 'XR_APILAYER_MULTITRON_stick.json'
if (-not (Test-Path $manifest)) { throw "Manifest not found: $manifest" }
$key = 'HKCU:\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit'
New-Item -Path $key -Force | Out-Null
New-ItemProperty -Path $key -Name $manifest -Value 0 -PropertyType DWord -Force | Out-Null
Write-Host "Registered: $manifest"
Write-Host "Restart the VR game for it to take effect."
