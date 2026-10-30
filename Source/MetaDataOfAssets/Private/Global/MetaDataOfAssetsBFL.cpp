// Copyright 2026 ljc

#include "Global/MetaDataOfAssetsBFL.h"


bool UMetaDataOfAssetsBFL::IsStringBlank(const FString& InStr)
{
	for (const TCHAR Char : InStr)
	{
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
	return InName.IsNone() && IsStringBlank(NameStr);
}
