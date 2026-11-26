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
	FMetaDataOfAssetsBFLNameValidationTest,
	"MetaDataOfAssets.Unit.BFL.NameValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMetaDataOfAssetsBFLNameValidationTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("大小写字母、数字和下划线组合应有效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("MetaData_01")));
	TestTrue(
		TEXT("仅包含大写字母应有效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("METADATA")));
	TestTrue(
		TEXT("仅包含小写字母应有效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("metadata")));
	TestTrue(
		TEXT("仅包含数字应有效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("123456")));
	TestTrue(
		TEXT("下划线开头应有效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("_Internal01")));

	TestFalse(
		TEXT("空字符串应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(FString()));
	TestFalse(
		TEXT("点号应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Shared.Key")));
	TestFalse(
		TEXT("短横线应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta-Data")));
	TestFalse(
		TEXT("空格应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta Data")));
	TestFalse(
		TEXT("制表符应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta\tData")));
	TestFalse(
		TEXT("换行符应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta\nData")));
	TestFalse(
		TEXT("中文应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("名称_01")));
	TestFalse(
		TEXT("斜杠应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta/Data")));
	TestFalse(
		TEXT("冒号应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta:Data")));
	TestFalse(
		TEXT("Emoji 应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TEXT("Meta😀Data")));

	const FString MaxLengthName = FString::ChrN(NAME_SIZE - 1, TEXT('A'));
	const FString TooLongName = FString::ChrN(NAME_SIZE, TEXT('A'));
	
	TestTrue(
		TEXT("最大允许长度应有效"),
		UMetaDataOfAssetsBFL::IsNameValid(MaxLengthName));
	TestFalse(
		TEXT("超过最大允许长度应无效"),
		UMetaDataOfAssetsBFL::IsNameValid(TooLongName));

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
