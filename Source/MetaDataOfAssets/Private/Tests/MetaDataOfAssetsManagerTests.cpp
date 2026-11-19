// Copyright 2026 ljc. All rights reserved.

/**
 * 使用 powershell
 * cd "本插件根目录"
 * powershell -ExecutionPolicy Bypass -File .\Tests\RunTests.ps1 -EngineRoot "引擎的安装路径" -TestFilter "MetaDataOfAssets.Unit.Settings"
 */

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MetaData/EUSS_MetaDataManager.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "UObject/Package.h"

namespace MetaDataOfAssetsTest
{
	FScanDirectorySetting MakeScanSetting(const TCHAR* Path, const bool bRecursive)
	{
		FScanDirectorySetting Setting;
		Setting.Directory.Path = Path;
		Setting.bRecursive = bRecursive;
		return Setting;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMetaDataOfAssetsNormalizeScanPathsTest,
	"MetaDataOfAssets.Unit.Settings.NormalizeScanPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMetaDataOfAssetsNormalizeScanPathsTest::RunTest(const FString& Parameters)
{
	UEUSS_MetaDataManager* Manager =
		NewObject<UEUSS_MetaDataManager>(GetTransientPackage());
	if (!Manager)
	{
		AddError(TEXT("创建 UEUSS_MetaDataManager 失败"));
		return false;
	}

	TArray<FScanDirectorySetting> DuplicateSettings;
	DuplicateSettings.Add(MetaDataOfAssetsTest::MakeScanSetting(
		TEXT("/Game/MetaDataOfAssetsTests"),
		false));
	DuplicateSettings.Add(MetaDataOfAssetsTest::MakeScanSetting(
		TEXT("/game/metadataofassetstests/"),
		true));
	DuplicateSettings.Add(MetaDataOfAssetsTest::MakeScanSetting(
		TEXT("/Game/MetaDataOfAssetsTests/Nested"),
		false));

	// 测试三个路径
	const TArray<FScanDirectorySetting> DuplicateResult =
		Manager->GetNormalizedPathSettings(DuplicateSettings);
	TestEqual(
		TEXT("重复路径需要被合并为一个有效的扫描根目录"),
		DuplicateResult.Num(),
		1);
	if (DuplicateResult.Num() == 1)
	{
		// 测试合并后的路径、递归是否正确
		TestTrue(
			TEXT("合并重复路径时，递归应该被设置"),
			DuplicateResult[0].bRecursive);
		TestEqual(
			TEXT("规范化路径时，路径应该是 /Game/MetaDataOfAssetsTests"),
			DuplicateResult[0].Directory.Path,
			FString(TEXT("/Game/MetaDataOfAssetsTests")));
	}

	TArray<FScanDirectorySetting> NonRecursiveSettings;
	NonRecursiveSettings.Add(MetaDataOfAssetsTest::MakeScanSetting(
		TEXT("/Game/MetaDataOfAssetsTests"),
		false));
	NonRecursiveSettings.Add(MetaDataOfAssetsTest::MakeScanSetting(
		TEXT("/Game/MetaDataOfAssetsTests/Nested"),
		false));

	// 测试两个非递归路径
	const TArray<FScanDirectorySetting> NonRecursiveResult =
		Manager->GetNormalizedPathSettings(NonRecursiveSettings);
	TestEqual(
		TEXT("非递归父目录不应该覆盖子目录"),
		NonRecursiveResult.Num(),
		2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
