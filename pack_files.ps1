# 打包脚本 - 将项目关键文件打包为 ZIP
# 使用方法: .\pack_files.ps1

$ErrorActionPreference = "Stop"

# 7z 可执行文件路径（请根据实际情况修改）
$SevenZipPath = "7z"

# 输出 ZIP 文件名
$OutputZip = "source_package.zip"

# 需要打包的文件列表
$FilesToPack = @(
	"src\Lexer.cpp",
	"src\parser_main.cpp",
	"src\Token.cpp",
	"include\common.h",
	"include\Lexer.h",
	"include\Stream.h",
	"include\Token.h",
	"build\sysy.tab.cc",
	"build\sysy.tab.h"
)

Write-Host "开始打包文件..." -ForegroundColor Cyan

# 检查 7z 是否可用
try {
	& $SevenZipPath | Out-Null
}
catch {
	Write-Error "未找到 7z 命令。请确保 7-Zip 已安装并添加到系统 PATH 中。"
	Write-Host "`n您也可以手动修改脚本中的 `$SevenZipPath 变量指向 7z.exe 的完整路径。" -ForegroundColor Yellow
	exit 1
}

# 验证所有文件是否存在
$AllFilesExist = $true
foreach ($file in $FilesToPack) {
	if (Test-Path $file) {
		Write-Host "✓ 找到: $file"
	}
 else {
		Write-Warning "文件不存在: $file"
		$AllFilesExist = $false
	}
}

if (-not $AllFilesExist) {
	Write-Host "`n警告：部分文件不存在，但仍将尝试打包存在的文件。" -ForegroundColor Yellow
}

# 使用 7z 直接打包（不经过临时文件夹，所有文件平放在根目录）
Write-Host "`n正在创建 ZIP 文件: $OutputZip" -ForegroundColor Cyan

# 如果输出文件已存在，先删除
if (Test-Path $OutputZip) {
	Remove-Item $OutputZip -Force
	Write-Host "已删除旧的压缩文件" -ForegroundColor Gray
}

# 将所有文件复制到当前目录的临时位置，然后打包到根目录
$TempFiles = @()
foreach ($file in $FilesToPack) {
	if (Test-Path $file) {
		$fileName = Split-Path $file -Leaf
		# 如果文件名冲突，添加前缀
		$destFileName = $fileName
		$counter = 1
		while (Test-Path $destFileName) {
			$destFileName = "${counter}_$fileName"
			$counter++
		}
		Copy-Item $file $destFileName
		$TempFiles += $destFileName
		Write-Host "✓ 已添加: $file -> $destFileName"
	}
}

# 打包所有临时文件（都在当前目录，所以会平放在 ZIP 根目录）
& $SevenZipPath a -tzip $OutputZip $TempFiles | Out-Null

# 清理临时文件
foreach ($tempFile in $TempFiles) {
	if (Test-Path $tempFile) {
		Remove-Item $tempFile -Force
	}
}

if ($LASTEXITCODE -eq 0) {
	Write-Host "`n打包完成！输出文件: $OutputZip" -ForegroundColor Green
    
	# 显示文件信息
	if (Test-Path $OutputZip) {
		$size = [math]::Round((Get-Item $OutputZip).Length / 1KB, 2)
		Write-Host "文件大小: $size KB" -ForegroundColor Cyan
        
		# 显示压缩包内容
		Write-Host "`n压缩包内容:" -ForegroundColor Cyan
		& $SevenZipPath l $OutputZip | Select-Object -Skip 6 | Select-Object -First ($FilesToPack.Count + 2)
	}
}
else {
	Write-Error "打包失败，7z 返回错误代码: $LASTEXITCODE"
}

Write-Host "`n操作完成！" -ForegroundColor Green
