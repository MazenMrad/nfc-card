param(
    [ValidateSet("template_debug", "template_release", "all")]
    [string]$Target = "all",
    [int]$Jobs = 4
)

$ErrorActionPreference = "Stop"
Set-Location -LiteralPath $PSScriptRoot
$compilerCommand = Get-Command x86_64-w64-mingw32-clang++.exe -ErrorAction Stop
$compilerItem = Get-Item -LiteralPath $compilerCommand.Source
$compilerPath = if ($compilerItem.ResolvedTarget) { $compilerItem.ResolvedTarget } else { $compilerItem.FullName }
$compilerDirectory = Split-Path -Parent $compilerPath
$env:PATH = "$compilerDirectory;$env:PATH"

$targets = if ($Target -eq "all") { @("template_debug", "template_release") } else { @($Target) }
foreach ($buildTarget in $targets) {
    python -m SCons platform=windows arch=x86_64 target=$buildTarget "--jobs=$Jobs"
    if ($LASTEXITCODE -ne 0) {
        throw "SCons failed for $buildTarget"
    }
}
