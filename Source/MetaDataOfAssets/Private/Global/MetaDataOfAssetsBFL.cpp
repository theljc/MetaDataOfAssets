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
