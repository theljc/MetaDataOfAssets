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
	
	// 判断命名是否有效，由于 FName 可能超过长度限制，所以用 FString 接收
	UFUNCTION(BlueprintPure, Category = "MetaDataOfAssets")
	static bool IsNameValid(const FString& InStr);
	
	// 判断字符串是否超过 FName 的长度限制
	static bool IsStringOutOfNameSize(const FString& InStr);

	// 判断字符串的字符是否有效
	static bool IsStringCharValid(const FString& InStr);
	
	/**
	 * 计算资产中所有元数据的交集
	 *
	 * @param InAssets 资产数组
	 * @return 共同的元数据
	 */
	UFUNCTION(BlueprintPure, Category = "MetaDataOfAssets")
	static TArray<FName> CalcIntersectionOfMetaData(const TArray<UObject*>& InAssets);
	
	// 按 Key 升序排序 Map
	UFUNCTION(BlueprintCallable, Category = "MetaDataOfAssets")
	static TMap<int32, UObject*> SortMapByKeyAsc(const TMap<int32, UObject*>& SourceMap);
	
};
