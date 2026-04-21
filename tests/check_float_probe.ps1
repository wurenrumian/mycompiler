Param(
    [string]$CompilerPath = ".\\build\\compiler.exe",
    [string]$WorkDir = "."
)

$ErrorActionPreference = "Stop"

Push-Location $WorkDir
try {
    if (!(Test-Path $CompilerPath)) {
        Write-Host "[FAIL] compiler not found: $CompilerPath"
        exit 2
    }

    @'
float a = 3.7;
int main() {
    float x = 1.1;
    float y = 2.2;
    float z = x * y + 0.5;
    int k = z;
    putfloat(z);
    putch(10);
    putint(k);
    putch(10);
    return 0;
}
'@ | Set-Content -Path "__tmp_float_probe.sy" -NoNewline

    & $CompilerPath -S -o "__tmp_float_probe.s" "__tmp_float_probe.sy"
    if ($LASTEXITCODE -ne 0 -or !(Test-Path "__tmp_float_probe.s")) {
        Write-Host "[FAIL] failed to generate assembly"
        exit 1
    }

    $badPattern = "fcvt\.d\.s|fcvt\.s\.d|fadd\.d|fmul\.d|fdiv\.d|fsub\.d"
    $bad = Select-String -Path "__tmp_float_probe.s" -Pattern $badPattern
    if ($bad) {
        Write-Host "[FAIL] still has double-path ops:"
        $bad | ForEach-Object { Write-Host ("{0}:{1}: {2}" -f $_.Path, $_.LineNumber, $_.Line.Trim()) }
        exit 1
    }

    Write-Host "[PASS] no double-path ops in float probe"
    $goodPattern = "fmadd\.s|fadd\.s|fmul\.s|fdiv\.s|fsub\.s|fcvt\.w\.s"
    $good = Select-String -Path "__tmp_float_probe.s" -Pattern $goodPattern
    if ($good) {
        $good | ForEach-Object { Write-Host ("{0}:{1}: {2}" -f $_.Path, $_.LineNumber, $_.Line.Trim()) }
    }
    exit 0
}
finally {
    Pop-Location
}
