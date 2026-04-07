$ErrorActionPreference = "Stop"

$SevenZip = "7z"
$OutputZip = "src.zip"

# Files required to build/run the compiler core.
$Files = @(
    "include\\Ast.h",
    "include\\common.h",
    "include\\Codegen.h",
    "include\\Lexer.h",
    "include\\ParserFrontend.h",
    "include\\IRBuilder.h",
    "include\\Semantic.h",
    "include\\Stream.h",
    "include\\Token.h",
    "src\\Ast.cpp",
    "src\\Codegen.cpp",
    "src\\Lexer.cpp",
    "src\\IRBuilder.cpp",
    "src\\ParserFrontend.cpp",
    "src\\Semantic.cpp",
    "src\\Token.cpp",
    "src\\parser_main.cpp",
    "build\\sysy.tab.cc",
    "build\\sysy.tab.h"
)

function Require-SevenZip {
    try {
        & $SevenZip i | Out-Null
    }
    catch {
        throw "7z not found. Install 7-Zip and ensure '7z' is in PATH."
    }
}

function Collect-Files {
    param([string[]]$FilePaths)

    $files = @()
    foreach ($path in $FilePaths) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $files += Get-Item -LiteralPath $path
        }
        else {
            Write-Warning "Missing required file: $path"
        }
    }

    $files | Sort-Object -Property FullName -Unique | Sort-Object -Property FullName
}

Require-SevenZip

$filesToPack = Collect-Files -FilePaths $Files
if ($filesToPack.Count -eq 0) {
    throw "No files matched. Nothing to pack."
}

if (Test-Path $OutputZip) {
    Remove-Item $OutputZip -Force
}

$tempDir = Join-Path $PWD ".pack_src_tmp"
if (Test-Path $tempDir) {
    Remove-Item $tempDir -Recurse -Force
}
New-Item -Path $tempDir -ItemType Directory | Out-Null

try {
    $nameCount = @{}

    foreach ($file in $filesToPack) {
        $baseName = $file.Name

        if ($nameCount.ContainsKey($baseName)) {
            $nameCount[$baseName] += 1
            $targetName = "{0}_{1}" -f $nameCount[$baseName], $baseName
        }
        else {
            $nameCount[$baseName] = 0
            $targetName = $baseName
        }

        Copy-Item -Path $file.FullName -Destination (Join-Path $tempDir $targetName)
    }

    Push-Location $tempDir
    try {
        & $SevenZip a -tzip "..\\$OutputZip" * | Out-Null
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

Write-Host "Created $OutputZip with $($filesToPack.Count) source files (flat at archive root)."
