// Copyright 2026 ljc

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MetaDataOfAssetsBFL.generated.h"


/**
 * 本插件使用的蓝图函数库
 */
UCLASS()
class METADATAOFASSETS_API UMetaDataOfAssetsBFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	// 判断字符串是否为空
	UFUNCTION(BlueprintPure, Category = "MetaDataOfAssets")
	static bool IsStringBlank(const FString& InStr);

	// 判断命名是否为空
	UFUNCTION(BlueprintPure, Category = "MetaDataOfAssets")
	static bool IsNameBlank(const FName& InName);
	
};
