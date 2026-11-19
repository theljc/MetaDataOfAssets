// Copyright 2026 ljc. All rights reserved.

/**
 * 使用 powershell
 * cd "本插件根目录"
 * powershell -ExecutionPolicy Bypass -File .\Tests\RunTests.ps1 -EngineRoot "引擎的安装路径" -TestFilter "MetaDataOfAssets.Unit.Settings"
 */

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/Paths.h"
#include "Settings/MetaDataOfAssetsSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMetaDataOfAssetsConfigPathTest,
	"MetaDataOfAssets.Unit.Settings.ConfigPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMetaDataOfAssetsConfigPathTest::RunTest(const FString& Parameters)
{
	const FString ConfigPath = UMetaDataOfAssetsSettings::GetConfigFilePath();

	TestFalse(TEXT("插件配置文件路径不应为空"), ConfigPath.IsEmpty());
	TestFalse(TEXT("插件配置文件路径是绝对路径"), FPaths::IsRelative(ConfigPath));
	TestEqual(
		TEXT("插件应该使用 DefaultMetaDataOfAssets.ini 配置文件名"),
		FPaths::GetCleanFilename(ConfigPath),
		FString(TEXT("DefaultMetaDataOfAssets.ini")));
	TestEqual(
		TEXT("插件配置文件应该存储在 Config 目录下"),
		FPaths::GetCleanFilename(FPaths::GetPath(ConfigPath)),
		FString(TEXT("Config")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
