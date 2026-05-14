Param(
    [string]$ParserPath = "",
    [string]$ProjectRoot = "",
    [int]$StartCase = 1,
    [int]$EndCase = 40
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

if ([string]::IsNullOrWhiteSpace($ParserPath)) {
    $candidate1 = Join-Path $ProjectRoot "build\parser.exe"
    $candidate2 = Join-Path $ProjectRoot "build\parser"
    if (Test-Path $candidate1) {
        $ParserPath = $candidate1
    } elseif (Test-Path $candidate2) {
        $ParserPath = $candidate2
    } else {
        Write-Host "[ERROR] parser binary not found. Please pass -ParserPath."
        exit 2
    }
}

if (!(Test-Path $ParserPath)) {
    Write-Host "[ERROR] parser not found: $ParserPath"
    exit 2
}

$publicRoot = Join-Path $ProjectRoot "public\public"
if (!(Test-Path $publicRoot)) {
    Write-Host "[ERROR] public/public not found: $publicRoot"
    exit 2
}

$buildDir = Split-Path -Parent $ParserPath
function Normalize-Text {
    param([string]$Text)
    $normalized = $Text -replace "`r", ""
    return $normalized.TrimEnd("`n", "`t")
}

function Find-ToolNearLli {
    param([string]$ToolName)
    $pathEntries = $env:PATH -split ';'
    foreach ($entry in $pathEntries) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        $lliPath = Join-Path $entry "lli.exe"
        $toolPath = Join-Path $entry $ToolName
        if ((Test-Path $lliPath) -and (Test-Path $toolPath)) {
            return $toolPath
        }
    }
    return $ToolName
}

$clangPath = if ($env:LLVM_CLANG) { $env:LLVM_CLANG } else { Find-ToolNearLli "clang.exe" }
$llvmLinkPath = Find-ToolNearLli "llvm-link.exe"

function Invoke-NativeLogged {
    param(
        [string]$CommandLine,
        [string]$LogPath
    )
    cmd /c ($CommandLine + " > """ + $LogPath + """ 2>&1")
    return $LASTEXITCODE
}

$failed = @()
$passed = 0

Push-Location $buildDir
try {
    foreach ($caseId in $StartCase..$EndCase) {
        $sourcePath = Join-Path $publicRoot ("testfile" + $caseId + ".txt")
        $inputPath = Join-Path $publicRoot ("input" + $caseId + ".txt")
        $outputPath = Join-Path $publicRoot ("output" + $caseId + ".txt")
        if (!(Test-Path $sourcePath) -or !(Test-Path $outputPath)) {
            continue
        }

        Copy-Item -Force $sourcePath "testfile.txt"
        if (Test-Path "output.ll") { Remove-Item -Force "output.ll" }
        if (Test-Path "linked_output.ll") { Remove-Item -Force "linked_output.ll" }
        if (Test-Path "runtime_sylib.ll") { Remove-Item -Force "runtime_sylib.ll" }
        if (Test-Path "runtime_support.c") { Remove-Item -Force "runtime_support.c" }
        if (Test-Path "judge_stdout.log") { Remove-Item -Force "judge_stdout.log" }
        if (Test-Path "judge_input.txt") { Remove-Item -Force "judge_input.txt" }

        Write-Host ("[RUN] public/public case " + $caseId)
        & $ParserPath
        if ($LASTEXITCODE -ne 0) {
            Write-Host ("[FAIL] parser exit code = " + $LASTEXITCODE)
            $failed += $caseId
            continue
        }
        if (!(Test-Path "output.ll")) {
            Write-Host "[FAIL] output.ll missing"
            $failed += $caseId
            continue
        }

        @"
extern int scanf(const char *, ...);
extern int printf(const char *, ...);
extern int vprintf(const char *, __builtin_va_list);
extern int getchar(void);
int getint(void) { int x = 0; scanf("%d", &x); return x; }
int getch(void) { return getchar(); }
float getfloat(void) { float x = 0.0f; scanf("%a", &x); return x; }
int getarray(int a[]) { int n = 0; scanf("%d", &n); for (int i = 0; i < n; ++i) scanf("%d", &a[i]); return n; }
int getfarray(float a[]) { int n = 0; scanf("%d", &n); for (int i = 0; i < n; ++i) scanf("%a", &a[i]); return n; }
void putint(int a) { printf("%d", a); }
void putch(int a) { printf("%c", a); }
void putfloat(float a) { printf("%a", a); }
void putarray(int n, int a[]) { printf("%d:", n); for (int i = 0; i < n; ++i) printf(" %d", a[i]); printf("\n"); }
void putfarray(int n, float a[]) { printf("%d:", n); for (int i = 0; i < n; ++i) printf(" %a", a[i]); printf("\n"); }
void putf(char a[], ...) { __builtin_va_list args; __builtin_va_start(args, a); vprintf(a, args); __builtin_va_end(args); }
void _sysy_starttime(int lineno) { (void)lineno; }
void _sysy_stoptime(int lineno) { (void)lineno; }
"@ | Set-Content -Path "runtime_support.c"

        $clangCmd = '"' + $clangPath + '" -S -emit-llvm "runtime_support.c" -o runtime_sylib.ll -O0'
        if ((Invoke-NativeLogged $clangCmd "runtime_build.log") -ne 0) {
            Write-Host "[FAIL] runtime LLVM IR build failed"
            $failed += $caseId
            continue
        }

        $linkCmd = '"' + $llvmLinkPath + '" runtime_sylib.ll output.ll -S -o linked_output.ll'
        if ((Invoke-NativeLogged $linkCmd "runtime_link.log") -ne 0) {
            Write-Host "[FAIL] llvm-link failed"
            $failed += $caseId
            continue
        }

        $inputText = if (Test-Path $inputPath) { Get-Content $inputPath -Raw } else { "" }
        Set-Content -NoNewline -Path "judge_input.txt" -Value $inputText
        if ($inputText.Length -gt 0) {
            $inputText | lli linked_output.ll *> judge_stdout.log
        } else {
            cmd /c "lli linked_output.ll > judge_stdout.log 2>&1"
        }

        $actual = if (Test-Path "judge_stdout.log") { Get-Content "judge_stdout.log" -Raw } else { "" }
        $expected = Get-Content $outputPath -Raw
        $normalizedActual = Normalize-Text $actual
        $normalizedExpected = Normalize-Text $expected
        if ($normalizedActual -ne $normalizedExpected) {
            Write-Host "[FAIL] output mismatch"
            Write-Host "Expected:"
            Write-Host $normalizedExpected
            Write-Host "Actual:"
            Write-Host $normalizedActual
            $failed += $caseId
            continue
        }

        Write-Host "[PASS]"
        $passed++
    }
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host ("[SUMMARY] passed = " + $passed + ", failed = " + $failed.Count)
if ($failed.Count -gt 0) {
    Write-Host ("[FAILED_CASES] " + ($failed -join ","))
    exit 1
}

exit 0
