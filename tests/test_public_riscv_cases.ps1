Param(
    [string]$CompilerPath = "",
    [string]$ProjectRoot = "",
    [string]$Distro = "Ubuntu-24.04",
    [string[]]$Cases = @(
        "public\performance_easy\03_sort1.sy",
        "public\performance_easy\fft0.sy"
    )
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    $candidate1 = Join-Path $ProjectRoot "build\compiler.exe"
    $candidate2 = Join-Path $ProjectRoot "build\compiler"
    if (Test-Path $candidate1) {
        $CompilerPath = $candidate1
    } elseif (Test-Path $candidate2) {
        $CompilerPath = $candidate2
    } else {
        Write-Host "[ERROR] compiler binary not found. Please pass -CompilerPath."
        exit 2
    }
}

function Normalize-Text {
    param([string]$Text)
    $normalized = $Text -replace "`r", ""
    return $normalized.TrimEnd("`n", "`t")
}

function Split-ExpectedResult {
    param([string]$CombinedText)
    $text = $CombinedText -replace "`r", ""
    $trimmed = $text.TrimEnd("`n", "`t")
    $lastNewline = $trimmed.LastIndexOf("`n")
    if ($lastNewline -lt 0) {
        return @{
            Stdout = ""
            ExitCode = $trimmed.Trim()
        }
    }

    return @{
        Stdout = $trimmed.Substring(0, $lastNewline + 1)
        ExitCode = $trimmed.Substring($lastNewline + 1).Trim()
    }
}

function To-WslPath {
    param([string]$WindowsPath)
    $full = [IO.Path]::GetFullPath($WindowsPath)
    $drive = $full.Substring(0, 1).ToLowerInvariant()
    $rest = $full.Substring(2).Replace("\", "/")
    return "/mnt/$drive$rest"
}

function Invoke-Wsl {
    param([string]$Command)
    & wsl.exe -d $Distro -- bash -lc $Command
    return $LASTEXITCODE
}

function Show-TextSummary {
    param(
        [string]$Label,
        [string]$Text
    )
    $normalized = $Text -replace "`r", ""
    $firstLine = ($normalized -split "`n", 2)[0]
    if ($firstLine.Length -gt 160) {
        $firstLine = $firstLine.Substring(0, 160) + "..."
    }
    Write-Host ("  " + $Label + " length = " + $Text.Length)
    Write-Host ("  " + $Label + " first line = " + $firstLine)
}

$wslProjectRoot = To-WslPath $ProjectRoot
$toolCheck = Invoke-Wsl "command -v riscv64-linux-gnu-gcc >/dev/null && command -v qemu-riscv64 >/dev/null"
if ($toolCheck -ne 0) {
    Write-Host "[ERROR] WSL tools missing. Need riscv64-linux-gnu-gcc and qemu-riscv64 in $Distro."
    exit 2
}

$caseFiles = @()
foreach ($casePath in $Cases) {
    $resolved = if ([IO.Path]::IsPathRooted($casePath)) {
        $casePath
    } else {
        Join-Path $ProjectRoot $casePath
    }
    if (!(Test-Path $resolved)) {
        Write-Host ("[ERROR] case not found: " + $casePath)
        exit 2
    }
    $caseFiles += Get-Item $resolved
}

$outputRoot = Join-Path $ProjectRoot "build\riscv_run_out"
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
$runnerPath = Join-Path $outputRoot "run_case.sh"
$runnerText = @'
#!/usr/bin/env bash
set +e
exe="$1"
input="$2"
stdout="$3"
stderr="$4"
exitfile="$5"
if [ "$input" = "-" ]; then
    qemu-riscv64 "$exe" > "$stdout" 2> "$stderr"
else
    qemu-riscv64 "$exe" < "$input" > "$stdout" 2> "$stderr"
fi
status=$?
printf '%s' "$status" > "$exitfile"
exit 0
'@
[IO.File]::WriteAllText($runnerPath, $runnerText.Replace("`r`n", "`n"))
$wslRunner = To-WslPath $runnerPath

$failed = 0
$passed = 0

foreach ($case in $caseFiles) {
    $relative = $case.FullName.Substring($ProjectRoot.Length).TrimStart('\', '/')
    $baseName = [IO.Path]::GetFileNameWithoutExtension($case.Name)
    $caseOutDir = Join-Path $outputRoot ($relative.Replace('\', '_').Replace('/', '_').Replace('.sy', ''))
    New-Item -ItemType Directory -Force -Path $caseOutDir | Out-Null

    $asmPath = Join-Path $caseOutDir ($baseName + ".s")
    $exePath = Join-Path $caseOutDir ($baseName + ".riscv")
    $stdoutPath = Join-Path $caseOutDir ($baseName + ".stdout")
    $stderrPath = Join-Path $caseOutDir ($baseName + ".stderr")
    $exitPath = Join-Path $caseOutDir ($baseName + ".exit")
    $actualPath = Join-Path $caseOutDir ($baseName + ".actual")
    $actualCombinedPath = Join-Path $caseOutDir ($baseName + ".combined.actual")
    $inputPath = [IO.Path]::ChangeExtension($case.FullName, ".in")
    $expectedPath = [IO.Path]::ChangeExtension($case.FullName, ".out")

    Write-Host ("[RUN] " + $relative)
    Remove-Item -Force -ErrorAction SilentlyContinue $asmPath, $exePath, $stdoutPath, $stderrPath, $exitPath, $actualPath, $actualCombinedPath

    & $CompilerPath -S -o $asmPath $case.FullName
    if ($LASTEXITCODE -ne 0) {
        Write-Host ("[FAIL] compiler exit code = " + $LASTEXITCODE)
        $failed++
        continue
    }

    $wslAsm = To-WslPath $asmPath
    $wslExe = To-WslPath $exePath
    $linkCommand = "cd '$wslProjectRoot' && riscv64-linux-gnu-gcc -static '$wslAsm' sylib.c -o '$wslExe'"
    if ((Invoke-Wsl $linkCommand) -ne 0) {
        Write-Host "[FAIL] riscv64-linux-gnu-gcc link failed"
        $failed++
        continue
    }

    $wslStdout = To-WslPath $stdoutPath
    $wslStderr = To-WslPath $stderrPath
    $wslExit = To-WslPath $exitPath
    if (Test-Path $inputPath) {
        $wslInput = To-WslPath $inputPath
    } else {
        $wslInput = "-"
    }
    $runCommand = "cd '$wslProjectRoot' && bash '$wslRunner' '$wslExe' '$wslInput' '$wslStdout' '$wslStderr' '$wslExit'"
    Invoke-Wsl $runCommand | Out-Null

    $stdout = if (Test-Path $stdoutPath) { Get-Content $stdoutPath -Raw } else { "" }
    $stderr = if (Test-Path $stderrPath) { Get-Content $stderrPath -Raw } else { "" }
    $exitCode = if (Test-Path $exitPath) { (Get-Content $exitPath -Raw).Trim() } else { "" }
    Set-Content -NoNewline -Path $actualPath -Value $stdout

    if ($stdout.Length -eq 0) {
        $actualCombined = $exitCode
    } elseif ($stdout.EndsWith("`n")) {
        $actualCombined = $stdout + $exitCode
    } else {
        $actualCombined = $stdout + "`n" + $exitCode
    }
    Set-Content -NoNewline -Path $actualCombinedPath -Value $actualCombined

    Write-Host ("  exit code = " + $exitCode)
    Show-TextSummary "stdout" $stdout
    if ($stderr.Length -gt 0) {
        Show-TextSummary "stderr" $stderr
    }

    if (!(Test-Path $expectedPath)) {
        Write-Host ("[WARN] expected file not found: " + $expectedPath)
        $passed++
        continue
    }

    $expected = Get-Content $expectedPath -Raw
    $expectedParts = Split-ExpectedResult $expected
    $stdoutOk = (Normalize-Text $stdout) -eq (Normalize-Text $expectedParts.Stdout)
    $exitOk = $exitCode -eq $expectedParts.ExitCode

    if (!$stdoutOk) {
        Write-Host "[FAIL] stdout mismatch"
        Show-TextSummary "expected stdout" $expectedParts.Stdout
        Write-Host ("  actual file = " + $actualPath)
        $failed++
        continue
    }
    if (!$exitOk) {
        Write-Host ("[FAIL] exit code mismatch, expected " + $expectedParts.ExitCode + ", actual " + $exitCode)
        Write-Host ("  actual file = " + $actualCombinedPath)
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
