$manifest = Join-Path $PSScriptRoot 'XR_APILAYER_MULTITRON_stick.json'
$key = 'HKCU:\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit'
Remove-ItemProperty -Path $key -Name $manifest -ErrorAction SilentlyContinue
Write-Host "Unregistered: $manifest"
