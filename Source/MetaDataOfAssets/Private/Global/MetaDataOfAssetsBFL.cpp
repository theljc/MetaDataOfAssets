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
	if (InAssets.IsEmpty())
	{
		return TArray<FName>();
	}

	TSet<FName> Result;
	TSet<FName> SetToCompare;

	if (!IsValid(InAssets[0])) 	return TArray<FName>();

	// 以第一个资产中的元数据为基准进行比较
	TMap<FName, FString> MetadataTagValues = UEditorAssetLibrary::GetMetadataTagValues(InAssets[0]);
	TArray<FName> MetadataKeys;
	MetadataTagValues.GetKeys(MetadataKeys);
	SetToCompare.Append(MetadataKeys);

	// 遍历所有资产
	for (UObject* Asset : InAssets)
	{
		if (!IsValid(Asset)) return TArray<FName>();

		MetadataTagValues.Empty();
		MetadataKeys.Empty();
		Result.Empty();
		
		MetadataTagValues = UEditorAssetLibrary::GetMetadataTagValues(Asset);
		MetadataTagValues.GetKeys(MetadataKeys);
		Result.Append(MetadataKeys);

		// Intersect 计算两个集合的交集
		TSet<FName> TempSet = Result.Intersect(SetToCompare);

		// 交集为空则返回空数组
		if (TempSet.IsEmpty()) return TArray<FName>();

		// 交集的结果作为新的基准进行下一次比较
		Result = TempSet;
	}

	// 集合转换为数组
	return Result.Array();
}
