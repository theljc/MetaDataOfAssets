// Copyright 2026 ljc. All rights reserved.

#include "Global/MetaDataOfAssetsBFL.h"
#include "EditorAssetLibrary.h"


bool UMetaDataOfAssetsBFL::IsNameValid(const FString& InStr)
{
	if (InStr.IsEmpty()
		|| IsStringOutOfNameSize(InStr)
		|| !IsStringCharValid(InStr)) return false;

	return true;
}

bool UMetaDataOfAssetsBFL::IsStringOutOfNameSize(const FString& InStr)
{
	return InStr.Len() > NAME_SIZE - 1;
}

bool UMetaDataOfAssetsBFL::IsStringCharValid(const FString& InStr)
{
	// 遍历字符串
	for (const TCHAR Char : InStr)
	{
		// 有效字符为：大小写字母、数字、下划线
		const bool bIsUppercaseLetter = Char >= 'A' && Char <= 'Z';
		const bool bIsLowercaseLetter = Char >= 'a' && Char <= 'z';
		const bool bIsDigit = Char >= '0' && Char <= '9';
		const bool bIsUnderscore = Char == '_';

		if (!bIsUppercaseLetter &&
			!bIsLowercaseLetter &&
			!bIsDigit &&
			!bIsUnderscore)
		{
			return false;
		}
	}

	return true;
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
