// Copyright 2026 ljc. All rights reserved.

/**
 * 使用 powershell
 * cd "本插件根目录"
 * powershell -ExecutionPolicy Bypass -File .\Tests\RunTests.ps1 -EngineRoot "引擎的安装路径" -TestFilter "MetaDataOfAssets.Unit.BFL"
 */

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/DataTable.h"
#include "Global/MetaDataOfAssetsBFL.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

namespace MetaDataOfAssetsTest
{
	class FTransientMetadataObjects
	{
	public:
		FTransientMetadataObjects()
		{
			const FString PackageName = FString::Printf(
				TEXT("/Temp/MetaDataOfAssetsTests/%s"),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits));
			Package = CreatePackage(*PackageName);
			Package->SetFlags(RF_Transient);
		}

		~FTransientMetadataObjects()
		{
			if (Package)
			{
				Package->MarkAsGarbage();
			}
		}

		UObject* CreateObject()
		{
			return NewObject<UDataTable>(
				Package,
				MakeUniqueObjectName(Package, UDataTable::StaticClass(), TEXT("TestObject")),
				RF_Transient);
		}

		void SetMetadata(UObject* Object, const FName Key, const FString& Value)
		{
			Package->GetMetaData()->SetValue(Object, Key, *Value);
		}

	private:
		UPackage* Package = nullptr;
	};

	void TestContainsExactly(
		FAutomationTestBase& Test,
		const TCHAR* Context,
		const TArray<FName>& Actual,
		const TArray<FName>& Expected)
	{
		// 测试数量是否正确
		Test.TestEqual(
			FString::Printf(TEXT("%s 计数"), Context),
			Actual.Num(),
			Expected.Num());

		// 测试每个键是否都包含在内
		for (const FName& Key : Expected)
		{
			Test.TestTrue(
				FString::Printf(TEXT("%s：包含 %s"), Context, *Key.ToString()),
				Actual.Contains(Key));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMetaDataOfAssetsBFLStringValidationTest,
	"MetaDataOfAssets.Unit.BFL.StringValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMetaDataOfAssetsBFLStringValidationTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("空字符串应为空"),
		UMetaDataOfAssetsBFL::IsStringBlank(FString()));
	TestTrue(
		TEXT("仅包含空白字符的字符串应为空"),
		UMetaDataOfAssetsBFL::IsStringBlank(TEXT(" \t\r\n")));
	TestFalse(
		TEXT("包含文本的字符串不应为空"),
		UMetaDataOfAssetsBFL::IsStringBlank(TEXT("Metadata")));

	TestTrue(
		TEXT("NAME_None 应为空"),
		UMetaDataOfAssetsBFL::IsNameBlank(NAME_None));
	TestTrue(
		TEXT("仅包含空白字符的名称应为空"),
		UMetaDataOfAssetsBFL::IsNameBlank(FName(TEXT("  "))));
	TestFalse(
		TEXT("可见名称不应为空"),
		UMetaDataOfAssetsBFL::IsNameBlank(FName(TEXT("Shared.Key"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMetaDataOfAssetsBFLIntersectionTest,
	"MetaDataOfAssets.Unit.BFL.MetadataIntersection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMetaDataOfAssetsBFLIntersectionTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("空资产列表不应有共同的元数据"),
		UMetaDataOfAssetsBFL::CalcIntersectionOfMetaData(TArray<UObject*>()).IsEmpty());

	TArray<UObject*> InvalidAssets;
	InvalidAssets.Add(nullptr);
	TestTrue(
		TEXT("无效资产不应有共同的元数据"),
		UMetaDataOfAssetsBFL::CalcIntersectionOfMetaData(InvalidAssets).IsEmpty());

	MetaDataOfAssetsTest::FTransientMetadataObjects Objects;
	UObject* AssetA = Objects.CreateObject();
	UObject* AssetB = Objects.CreateObject();
	UObject* AssetC = Objects.CreateObject();

	Objects.SetMetadata(AssetA, TEXT("Shared"), TEXT("A"));
	Objects.SetMetadata(AssetA, TEXT("OnlyA"), TEXT("A"));
	Objects.SetMetadata(AssetA, TEXT("AnotherShared"), TEXT("A"));

	Objects.SetMetadata(AssetB, TEXT("Shared"), TEXT("B"));
	Objects.SetMetadata(AssetB, TEXT("OnlyB"), TEXT("B"));
	Objects.SetMetadata(AssetB, TEXT("AnotherShared"), TEXT("B"));

	Objects.SetMetadata(AssetC, TEXT("Shared"), TEXT("C"));
	Objects.SetMetadata(AssetC, TEXT("OnlyC"), TEXT("C"));

	// 测试单个资产
	TArray<UObject*> SingleAsset;
	SingleAsset.Add(AssetA);
	MetaDataOfAssetsTest::TestContainsExactly(
		*this,
		TEXT("单个资产"),
		UMetaDataOfAssetsBFL::CalcIntersectionOfMetaData(SingleAsset),
		{ FName(TEXT("Shared")), FName(TEXT("OnlyA")), FName(TEXT("AnotherShared")) });

	// 测试三个资产的交集
	TArray<UObject*> ThreeAssets;
	ThreeAssets.Add(AssetA);
	ThreeAssets.Add(AssetB);
	ThreeAssets.Add(AssetC);
	MetaDataOfAssetsTest::TestContainsExactly(
		*this,
		TEXT("三个资产"),
		UMetaDataOfAssetsBFL::CalcIntersectionOfMetaData(ThreeAssets),
		{ FName(TEXT("Shared")) });

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
