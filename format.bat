@echo off
setlocal
set "AXIOM_ROOT=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((Get-Content -Raw '%~f0') -replace '(?s)^.*?\r?\n#_PS_START_\r?\n','')"
exit /b %ERRORLEVEL%
#_PS_START_
# clang-format runner for the Axiom + vku projects.
# Formats the same project-owned sources as the repository reformat:
# vendored code, nested submodules and build output are excluded.

$ErrorActionPreference = 'Stop'

$root = $env:AXIOM_ROOT.TrimEnd('\')
$cfg  = Join-Path $root '.clang-format'
if (-not (Test-Path -LiteralPath $cfg)) { throw "clang-format config not found: $cfg" }

$clangFormat = (Get-Command clang-format.exe -ErrorAction SilentlyContinue).Source
if (-not $clangFormat) {
    $candidates = @(
        (Join-Path $env:ProgramFiles 'LLVM\bin\clang-format.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'LLVM\bin\clang-format.exe')
    )
    foreach ($cand in $candidates) {
        if (Test-Path -LiteralPath $cand) { $clangFormat = $cand; break }
    }
}
if (-not $clangFormat) {
    $vs = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio' -Directory -ErrorAction SilentlyContinue
    foreach ($dir in $vs) {
        $found = Get-ChildItem $dir.FullName -Recurse -Filter clang-format.exe -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($found) { $clangFormat = $found.FullName; break }
    }
}
if (-not $clangFormat) { throw 'clang-format.exe not found on PATH, in LLVM, or in Visual Studio.' }

$extPattern = '\.(h|hpp|c|cc|cpp|cxx|inl)$'
$files = New-Object System.Collections.Generic.List[string]

foreach ($file in git -C $root ls-files 'axiom/**' 'examples/**' 'tests/**' 'lab/**') {
    if ($file -match $extPattern) { $files.Add((Join-Path $root $file)) }
}

$vkuRoot = Join-Path $root 'submodules\vku'
if (Test-Path -LiteralPath (Join-Path $vkuRoot '.git')) {
    $vendorPattern = '(^|/)(Im3D|ImGui|ThirdParty|third-party|third_party)/'
    foreach ($file in git -C $vkuRoot ls-files 'vku/**' 'backends/**' 'examples/**' 'tools/**') {
        if ($file -match $extPattern -and $file -notmatch $vendorPattern) {
            $files.Add((Join-Path $vkuRoot $file))
        }
    }
} else {
    Write-Warning "vku submodule not found at $vkuRoot; skipping."
}

$files = @($files | Sort-Object -Unique)
if ($files.Count -eq 0) { Write-Host 'No source files found.'; return }

Write-Host ("clang-format : {0}" -f $clangFormat)
Write-Host ("style        : {0}" -f $cfg)
Write-Host ("formatting   : {0} files" -f $files.Count)

& $clangFormat -i "--style=file:$cfg" @files
if ($LASTEXITCODE -ne 0) { throw "clang-format failed with exit code $LASTEXITCODE" }

Write-Host 'Done.'
