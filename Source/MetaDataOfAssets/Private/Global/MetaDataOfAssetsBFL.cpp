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
	TSet<FName> Result;
	TSet<FName> SetToCompare;
	
	if (IsValid(InAssets[0]))
	{
		TMap<FName, FString> MetadataTagValues = UEditorAssetLibrary::GetMetadataTagValues(InAssets[0]);
		TArray<FName> MetadataKeys;
		MetadataTagValues.GetKeys(MetadataKeys);
		SetToCompare.Append(MetadataKeys);
	}
	else
	{
		return TArray<FName>();
	}

	for (UObject* Asset : InAssets)
	{
		if (IsValid(Asset))
		{
			Result.Empty();

			TMap<FName, FString> MetadataTagValues = UEditorAssetLibrary::GetMetadataTagValues(Asset);

			TArray<FName> MetadataKeys;
			MetadataTagValues.GetKeys(MetadataKeys);
			Result.Append(MetadataKeys);
			
			TSet<FName> TempSet = Result.Intersect(SetToCompare);
			if (TempSet.IsEmpty()) return TArray<FName>();

			Result = TempSet;
		}
		else
		{
			return TArray<FName>();
		}
	}

	return Result.Array();
}
