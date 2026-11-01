// Copyright 2026 ljc. All rights reserved.

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

	/**
	 * 计算资产中所有元数据的交集
	 *
	 * @param InAssets 资产数组
	 * @return 共同的元数据
	 */
	UFUNCTION(BlueprintPure, Category = "MetaDataOfAssets")
	static TArray<FName> CalcIntersectionOfMetaData(const TArray<UObject*>& InAssets);
	
};
