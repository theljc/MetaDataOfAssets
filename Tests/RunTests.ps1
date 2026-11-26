# Copyright 2026 ljc. All rights reserved.

# 运行示例：
# powershell -ExecutionPolicy Bypass -File .\Tests\RunTests.ps1 -EngineRoot "引擎安装路径" -TestFilter "MetaDataOfAssets.Unit"

# [CmdletBinding()] 启用 PowerShell 通用参数和更严格的脚本绑定行为
[CmdletBinding()]
param(
	# 引擎根目录，用于拼接 UnrealEditor-Cmd.exe 的路径
	[string]$EngineRoot = "E:\UE_5.5",

	# 项目 .uproject 路径；为空时自动在项目根目录查找唯一的 uproject
	[string]$ProjectPath = "",

	# Automation Test 名称前缀，默认运行全部 MetaDataOfAssets.Unit 测试
	[string]$TestFilter = "MetaDataOfAssets.Unit",

	# 默认使用 -nullrhi 执行无界面逻辑测试；需要 Slate 控件测试时传入该开关
	[switch]$UseRHI
)

# 任何未处理的 PowerShell 错误都立即终止脚本
$ErrorActionPreference = "Stop"

# $MyInvocation.MyCommand.Path 是当前脚本路径
# Split-Path -Parent 依次从 Tests 目录反推出插件根目录和项目根目录
$TestsDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = Split-Path -Parent $TestsDirectory
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PluginRoot)

# 未显式传 ProjectPath 时，只允许自动发现一个 .uproject
if ([string]::IsNullOrWhiteSpace($ProjectPath))
{
	$ProjectCandidates = @(
		Get-ChildItem -LiteralPath $ProjectRoot -Filter "*.uproject" -File
	)
	if ($ProjectCandidates.Count -ne 1)
	{
		throw "Expected exactly one .uproject in '$ProjectRoot'. Pass -ProjectPath explicitly."
	}

	$ProjectPath = $ProjectCandidates[0].FullName
}

# Join-Path 用于安全拼接路径；Test-Path -PathType Leaf 用于确认目标是文件
$EditorCommand = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path -LiteralPath $EditorCommand -PathType Leaf))
{
	throw "UnrealEditor-Cmd.exe was not found at '$EditorCommand'."
}

if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf))
{
	throw "The project file was not found at '$ProjectPath'."
}

# 报告写入插件自己的 Saved 目录
# New-Item -Force 在目录已存在时不会报错，并会补齐缺少的父目录
$ReportDirectory = Join-Path $PluginRoot "Saved\Automation\MetaDataOfAssets"
New-Item -ItemType Directory -Path $ReportDirectory -Force | Out-Null
$ReportFile = Join-Path $ReportDirectory "index.json"

# 记录旧报告时间，稍后要求本次执行必须刷新 index.json，避免把旧结果误认为成功
$PreviousReportWriteTime = [DateTime]::MinValue
if (Test-Path -LiteralPath $ReportFile -PathType Leaf)
{
	$PreviousReportWriteTime = (Get-Item -LiteralPath $ReportFile).LastWriteTimeUtc
}

# 数组中每个元素都会作为 UnrealEditor-Cmd.exe 的一个独立命令行参数
$EditorArguments = @(
	$ProjectPath,
	"-ExecCmds=Automation RunTests $TestFilter",
	"-TestExit=Automation Test Queue Empty",
	"-unattended",
	"-nop4",
	"-nosplash",
	"-NoSound",
	"-log",
	"-ReportExportPath=$ReportDirectory"
)

# 默认关闭 RHI
if (-not $UseRHI)
{
	$EditorArguments += "-nullrhi"
}

Write-Host "Running automation tests: $TestFilter"
Write-Host "Project: $ProjectPath"
Write-Host "Reports: $ReportDirectory"

# PowerShell 的 & 是调用运算符；@EditorArguments 会把数组展开为多个命令行参数
& $EditorCommand @EditorArguments
# $LASTEXITCODE 保存上一条外部程序执行的退出码，0 通常表示进程正常退出
$ExitCode = $LASTEXITCODE

if ($ExitCode -ne 0)
{
	throw "Automation tests failed with Editor exit code $ExitCode."
}

if (-not (Test-Path -LiteralPath $ReportFile -PathType Leaf))
{
	throw "The automation report was not created at '$ReportFile'."
}

# 检查报告时间，确保脚本读取的是本次运行生成的报告
$ReportItem = Get-Item -LiteralPath $ReportFile
if ($ReportItem.LastWriteTimeUtc -le $PreviousReportWriteTime)
{
	throw "The automation report was not refreshed for this run."
}

# Get-Content -Raw 一次读取整个 JSON；ConvertFrom-Json 将 JSON 转成 PowerShell 对象
$Report = Get-Content -LiteralPath $ReportFile -Raw | ConvertFrom-Json
$TotalTests = [int]$Report.succeeded +
	[int]$Report.succeededWithWarnings +
	[int]$Report.failed +
	[int]$Report.notRun

Write-Host "Passed: $($Report.succeeded)"
Write-Host "Passed with warnings: $($Report.succeededWithWarnings)"
Write-Host "Failed: $($Report.failed)"
Write-Host "Not run: $($Report.notRun)"

if ($TotalTests -eq 0)
{
	throw "The test filter '$TestFilter' did not match any automation tests."
}

# 管道符 | 将测试对象依次传给 Where-Object 和 ForEach-Object
# $_ 表示当前管道对象，fullTestPath 是测试的完整 Automation 名称
if ([int]$Report.failed -ne 0 -or [int]$Report.notRun -ne 0)
{
	$FailedTests = @(
		$Report.tests |
			Where-Object { $_.state -eq "Fail" -or $_.state -eq "NotRun" } |
			ForEach-Object { $_.fullTestPath }
	)
	throw "Automation tests failed: $($FailedTests -join ', ')"
}

Write-Host "Automation tests completed successfully."

# exit 0 向调用方明确报告成功
exit 0
