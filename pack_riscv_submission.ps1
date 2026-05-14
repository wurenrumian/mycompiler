$ErrorActionPreference = "Stop"

$SevenZip = "7z"
$OutputZip = "submission_riscv.zip"

$CommonFiles = @(
    "include\Ast.h",
    "include\Codegen.h",
    "include\common.h",
    "include\Ir.h",
    "include\IrBuilder.h",
    "include\IrPrinter.h",
    "include\IrText.h",
    "include\Lexer.h",
    "include\ParserFrontend.h",
    "include\RiscVBackend.h",
    "include\Semantic.h",
    "include\Stream.h",
    "include\Token.h",
    "src\Ast.cpp",
    "src\Codegen.cpp",
    "src\Ir.cpp",
    "src\IrBuilder.cpp",
    "src\IrPrinter.cpp",
    "src\IrText.cpp",
    "src\Lexer.cpp",
    "src\ParserFrontend.cpp",
    "src\RiscVBackend.cpp",
    "src\Semantic.cpp",
    "src\Token.cpp",
    "src\sysy.y",
    "build\sysy.tab.cc",
    "build\sysy.tab.h",
    "sylib.c",
    "sylib.h"
)

$EntryFiles = @(
    "src\compiler_main.cpp"
)

function Require-SevenZip {
    try {
        & $SevenZip i | Out-Null
    }
    catch {
        throw "7z not found. Install 7-Zip and ensure '7z' is in PATH."
    }
}

function Collect-RequiredFiles {
    param([string[]]$Paths)

    $files = @()
    foreach ($path in $Paths) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Missing required file: $path"
        }
        $files += [PSCustomObject]@{
            RelativePath = $path
            FileInfo = Get-Item -LiteralPath $path
        }
    }

    return $files | Sort-Object -Property RelativePath -Unique
}

function Copy-FlatWithUniqueNames {
    param(
        [object[]]$Files,
        [string]$TargetDir
    )

    $nameCount = @{}
    foreach ($entry in $Files) {
        $file = $entry.FileInfo
        $baseName = $file.Name

        if ($nameCount.ContainsKey($baseName)) {
            $nameCount[$baseName] += 1
            $targetName = "{0}_{1}" -f $nameCount[$baseName], $baseName
        }
        else {
            $nameCount[$baseName] = 0
            $targetName = $baseName
        }

        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $TargetDir $targetName)
    }
}

Require-SevenZip

$filesToPack = Collect-RequiredFiles -Paths ($CommonFiles + $EntryFiles)

if (Test-Path $OutputZip) {
    Remove-Item $OutputZip -Force
}

$tempDir = Join-Path $PWD ".pack_riscv_submission_tmp"
if (Test-Path $tempDir) {
    Remove-Item $tempDir -Recurse -Force
}
New-Item -Path $tempDir -ItemType Directory | Out-Null

try {
    Copy-FlatWithUniqueNames -Files $filesToPack -TargetDir $tempDir

    Push-Location $tempDir
    try {
        & $SevenZip a -tzip "..\$OutputZip" * | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "7z failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}
finally {
    if (Test-Path $tempDir) {
        Remove-Item $tempDir -Recurse -Force
    }
}

Write-Host "Created $OutputZip for final RISC-V submission with $($filesToPack.Count) files."
