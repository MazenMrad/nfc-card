$ErrorActionPreference = "Stop"
Set-Location -LiteralPath $PSScriptRoot
$compilerCommand = Get-Command x86_64-w64-mingw32-clang++.exe -ErrorAction Stop
$compilerItem = Get-Item -LiteralPath $compilerCommand.Source
$compilerPath = if ($compilerItem.ResolvedTarget) { $compilerItem.ResolvedTarget } else { $compilerItem.FullName }

$sources = @(
    "tests/test_main.cpp",
    "src/nfc_result.cpp",
    "src/reader_profile.cpp",
    "src/ntag215_device.cpp",
    "src/nfc_controller.cpp",
    "src/win_pcsc_transport.cpp"
)

& $compilerPath -std=c++20 -Wall -Wextra -Werror -static -static-libgcc -static-libstdc++ -I src @sources -lwinscard -o tests/nfc_tests.exe
if ($LASTEXITCODE -ne 0) {
    throw "Native test compilation failed"
}

& "$PSScriptRoot/tests/nfc_tests.exe"
if ($LASTEXITCODE -ne 0) {
    throw "Native tests failed"
}
