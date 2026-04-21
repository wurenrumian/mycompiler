Param(
    [string]$CompilerPath = "",
    [string]$ProjectRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    $candidate1 = Join-Path $ProjectRoot "build\\compiler.exe"
    $candidate2 = Join-Path $ProjectRoot "build\\compiler"
    if (Test-Path $candidate1) {
        $CompilerPath = $candidate1
    } elseif (Test-Path $candidate2) {
        $CompilerPath = $candidate2
    } else {
        Write-Host "[ERROR] compiler binary not found. Please pass -CompilerPath."
        exit 2
    }
}

if (!(Test-Path $CompilerPath)) {
    Write-Host "[ERROR] compiler not found: $CompilerPath"
    exit 2
}

$publicDirs = @(
    (Join-Path $ProjectRoot "public\\functional_easy"),
    (Join-Path $ProjectRoot "public\\functional_hard"),
    (Join-Path $ProjectRoot "public\\performance_easy")
)

$cases = @()
foreach ($dir in $publicDirs) {
    if (Test-Path $dir) {
        $cases += Get-ChildItem -Path $dir -Recurse -File -Filter *.sy
    }
}

if ($cases.Count -eq 0) {
    Write-Host "[WARN] no .sy files found under public/*"
    exit 0
}

$outputRoot = Join-Path $ProjectRoot "build\\public_test_out"
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

$failed = 0
$passed = 0

foreach ($case in $cases) {
    $relative = $case.FullName.Substring($ProjectRoot.Length).TrimStart('\', '/')
    $outFile = Join-Path $outputRoot ($case.BaseName + ".s")

    Write-Host ("[RUN] " + $relative)
    & $CompilerPath -S -o $outFile $case.FullName
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        Write-Host ("[FAIL] compiler exit code = " + $exitCode)
        $failed++
        continue
    }

    if (!(Test-Path $outFile)) {
        Write-Host "[FAIL] output .s file missing"
        $failed++
        continue
    }

    $size = (Get-Item $outFile).Length
    if ($size -le 0) {
        Write-Host "[FAIL] output .s file is empty"
        $failed++
        continue
    }

    Write-Host "[PASS]"
    $passed++
}

Write-Host ""
Write-Host ("[SUMMARY] passed = " + $passed + ", failed = " + $failed)

if ($failed -ne 0) {
    exit 1
}

exit 0
