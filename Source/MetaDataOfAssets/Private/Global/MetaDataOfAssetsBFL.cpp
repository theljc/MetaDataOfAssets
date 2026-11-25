// Copyright 2026 ljc. All rights reserved.

#include "Global/MetaDataOfAssetsBFL.h"
#include "EditorAssetLibrary.h"


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

	TSet<const UObject*> ExistAssets;
	ExistAssets.Reserve(InAssets.Num());

	// 最终交集的结果
	TSet<FName> Result;
	
	// 遍历资产
	for (UObject* Asset : InAssets)
	{
		if (!IsValid(Asset)) return TArray<FName>();

		// 资产已存在则跳过
		bool bAlreadyExist = false;
		ExistAssets.Add(Asset, &bAlreadyExist);
		if (bAlreadyExist) continue;

		// 获取资产的元数据标签值
		const TMap<FName, FString> MetadataTagValues = UEditorAssetLibrary::GetMetadataTagValues(Asset);
		
		// 如果 Result 为空，表示首次遍历
		if (Result.IsEmpty())
		{
			Result.Reserve(MetadataTagValues.Num());
			// 添加到 Result
			for (const TPair<FName, FString>& MetadataTag : MetadataTagValues)
			{
				Result.Add(MetadataTag.Key);
			}

			// 如果 Result 为空表示第一个资产就没有元数据，则交集一定为空
			if (Result.IsEmpty()) return TArray<FName>();
			continue;
		}

		// 遍历 Result
		for (auto It = Result.CreateIterator(); It; ++It)
		{
			// 如果当前遍历的资产的元数据中不包含 Result 中的标签则从 Result 中移除（表示交集中不应该包含该标签）
			if (!MetadataTagValues.Contains(*It))
			{
				It.RemoveCurrent();
			}
		}

		// 如果交集为空则直接返回
		if (Result.IsEmpty()) return TArray<FName>();
	}

	// TSet 转 TArray
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
