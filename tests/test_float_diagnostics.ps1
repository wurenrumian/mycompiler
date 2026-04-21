Param(
    [string]$CompilerPath = "",
    [string]$ProjectRoot = "",
    [string]$CasePattern = "float|fft|dct|rotate|light|color|transpose|mm|spmv|crypto"
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
        Write-Host "[ERROR] compiler binary not found. pass -CompilerPath."
        exit 2
    }
}

if (!(Test-Path $CompilerPath)) {
    Write-Host "[ERROR] compiler not found: $CompilerPath"
    exit 2
}

$publicRoots = @(
    (Join-Path $ProjectRoot "public\\functional_easy"),
    (Join-Path $ProjectRoot "public\\functional_hard"),
    (Join-Path $ProjectRoot "public\\performance_easy")
)

$cases = @()
foreach ($dir in $publicRoots) {
    if (Test-Path $dir) {
        $cases += Get-ChildItem -Path $dir -Recurse -File -Filter *.sy
    }
}

if ($cases.Count -eq 0) {
    Write-Host "[WARN] no .sy files found under public/*"
    exit 0
}

$filtered = $cases | Where-Object { $_.BaseName -match $CasePattern -or $_.Name -match $CasePattern }
if ($filtered.Count -eq 0) {
    $filtered = $cases
}

$outDir = Join-Path $ProjectRoot "build\\float_diag"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$total = 0
$compileFail = 0
$suspicious = 0

Write-Host ("[INFO] selected cases: " + $filtered.Count)
Write-Host ("[INFO] case pattern: " + $CasePattern)

foreach ($case in $filtered) {
    $total++
    $asm = Join-Path $outDir ($case.BaseName + ".s")

    & $CompilerPath -S -o $asm $case.FullName
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -or !(Test-Path $asm)) {
        Write-Host ("[COMPILE_FAIL] " + $case.FullName)
        $compileFail++
        continue
    }

    $lines = Get-Content $asm
    $fcvtFloatToInt = $lines | Where-Object { $_ -match "fcvt\.(w|wu|l|lu)\.s" }
    $fcvtFloatToIntNoRtz = $fcvtFloatToInt | Where-Object { $_ -notmatch "rtz" }

    if ($fcvtFloatToIntNoRtz.Count -gt 0) {
        $suspicious++
        Write-Host ("[SUSPECT] " + $case.Name + " has float->int fcvt without rtz")
        $fcvtFloatToIntNoRtz | Select-Object -First 5 | ForEach-Object { Write-Host ("          " + $_.Trim()) }
    }
}

Write-Host ""
Write-Host ("[SUMMARY] total=" + $total + ", compile_fail=" + $compileFail + ", suspect_cases=" + $suspicious)
if ($suspicious -eq 0) {
    Write-Host "[SUMMARY] no obvious non-rtz float->int conversion found in generated assembly"
}

exit 0
