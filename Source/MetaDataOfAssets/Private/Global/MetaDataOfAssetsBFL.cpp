// Copyright 2026 ljc. All rights reserved.

#include "Global/MetaDataOfAssetsBFL.h"
#include "EditorAssetLibrary.h"
#include "Log/MetaDataOfAssetsLogChannel.h"


bool UMetaDataOfAssetsBFL::IsStringBlank(const FString& InStr)
{
	for (const TCHAR Char : InStr)
	{
		// IsWhitespace 会判断"空格"等特殊字符
		if (!FChar::IsWhitespace(Char))
		{
			return false;
		}
	}
	
	return true;
}

bool UMetaDataOfAssetsBFL::IsNameBlank(const FName& InName)
{
	const FString NameStr = InName.ToString();
	return InName.IsNone() || IsStringBlank(NameStr);
}

TArray<FName> UMetaDataOfAssetsBFL::CalcIntersectionOfMetaData(const TArray<UObject*>& InAssets)
{
	if (InAssets.IsEmpty()) return TArray<FName>();

	TSet<FName> Result;
	for (int32 AssetIndex = 0; AssetIndex < InAssets.Num(); ++AssetIndex)
	{
		UObject* Asset = InAssets[AssetIndex];
		if (!IsValid(Asset)) return TArray<FName>();

		const TMap<FName, FString> MetadataTagValues = UEditorAssetLibrary::GetMetadataTagValues(Asset);
		TArray<FName> MetadataKeys;
		MetadataTagValues.GetKeys(MetadataKeys);

		if (AssetIndex == 0)
		{
			Result.Append(MetadataKeys);
			continue;
		}

		// 每次都使用上一次的交集结果继续比较，保证得到所有资产的 N 路交集
		Result = Result.Intersect(TSet<FName>(MetadataKeys));
		if (Result.IsEmpty()) return TArray<FName>();
	}

	// 集合转换为数组
	return Result.Array();
}

TMap<int32, UObject*> UMetaDataOfAssetsBFL::SortMapByKeyAsc(const TMap<int32, UObject*>& SourceMap)
{
	TMap<int32, UObject*> Result = SourceMap;
	Result.KeySort([](const int32 A, const int32 B)
	{
		return A < B;
	});
	
	return Result;
}

void UMetaDataOfAssetsBFL::SortMapByKeyAsc2(const TMap<int32, UObject*>& SourceMap, TMap<int32, UObject*>& OutMap)
{
	checkf(false, TEXT("SortMapByKeyAsc2"));
}

DEFINE_FUNCTION(UMetaDataOfAssetsBFL::execSortMapByKeyAsc2)
{
	// 读取 SourceMap
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	FMapProperty* SourceMapProp = CastField<FMapProperty>(Stack.MostRecentProperty);
	void* SourceMapAddr = Stack.MostRecentPropertyAddress;

	// 读取 OutMap
	Stack.StepCompiledIn<FMapProperty>(nullptr);
	FMapProperty* OutMapProp = CastField<FMapProperty>(Stack.MostRecentProperty);
	void* OutMapAddr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (!SourceMapProp || !OutMapProp || !SourceMapAddr || !OutMapAddr)
	{
		return;
	}

	GenericSortMapByKey(SourceMapProp, SourceMapAddr, OutMapAddr, /*bAscending=*/true);
}

void UMetaDataOfAssetsBFL::GenericSortMapByKey(FMapProperty* MapProp, const void* SourceMapAddr, void* OutMapAddr,
	bool bAscending)
{
	// 键必须是 int
	if (!MapProp->KeyProp || !CastField<FIntProperty>(MapProp->KeyProp))
	{
		UE_LOG(LogMetaDataOfAssets, Error,
			TEXT("[%hs]：键类型必须是 int, 当前类型：%s"),
			__FUNCTION__, MapProp->KeyProp ? *MapProp->KeyProp->GetCPPType() : TEXT("null"));
		
		return;
	}

	// 用 FScriptMapHelper 访问 Source Map
	FScriptMapHelper SourceMapHelper(MapProp, SourceMapAddr);

	/** 从 MapHelper 获得所有键值对 */
	struct FKV { int32 Key; const uint8* ValuePtr; };
	TArray<FKV> Pairs;
	Pairs.Reserve(SourceMapHelper.Num());

	for (int32 i = 0; i < SourceMapHelper.Num(); ++i)
	{
		const int32 Key = *reinterpret_cast<const int32*>(SourceMapHelper.GetKeyPtr(i));
		const uint8* ValuePtr = SourceMapHelper.GetValuePtr(i);
		Pairs.Add(FKV(Key, ValuePtr));
	}

	// 排序
	if (bAscending)
	{
		Pairs.Sort([](const FKV& A, const FKV& B) { return A.Key < B.Key; });
	}
	else
	{
		Pairs.Sort([](const FKV& A, const FKV& B) { return A.Key > B.Key; });
	}

	// Pairs 中保存了排序后的键值对，写回 OutMapHelper
	FScriptMapHelper OutMapHelper(MapProp, OutMapAddr);
	// 写回前清空
	OutMapHelper.EmptyValues();

	const FProperty* KeyProp = MapProp->KeyProp;
	const FProperty* ValueProp = MapProp->ValueProp;

	for (const FKV& Pair : Pairs)
	{
		const int32 NewIndex = OutMapHelper.AddDefaultValue_Invalid_NeedsRehash();
		uint8* NewKeyPtr = static_cast<uint8*>(OutMapHelper.GetKeyPtr(NewIndex));
		uint8* NewValuePtr = static_cast<uint8*>(OutMapHelper.GetValuePtr(NewIndex));

		KeyProp->CopyCompleteValue(NewKeyPtr, &Pair.Key);
		ValueProp->CopyCompleteValue(NewValuePtr, Pair.ValuePtr);
	}

	// 重建哈希，使迭代顺序与插入顺序一致
	OutMapHelper.Rehash();
}
