// WidgetInterface.h — 提供所有控件都需要的通用接口

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StructUtils/InstancedStruct.h"
#include "WidgetInterface_MetaDataOfAssets.generated.h"


UINTERFACE()
class METADATAOFASSETS_API UWidgetInterface_MetaDataOfAssets : public UInterface
{
	GENERATED_BODY()
};

/**
 * 控件在初始化或被关闭时会调用 OnInitialize 和 OnDeinitialize
 * 提供 OnOpen 和 OnClose 用于蓝图
 */
class METADATAOFASSETS_API IWidgetInterface_MetaDataOfAssets
{
	GENERATED_BODY()

public:
	
	/**
	 * OnInitialize 时被调用，在蓝图中使用
	 * 
	 * @param Params 创建时需要的参数
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MetaDataOfAssets", meta = (DisplayName = "OnWidgetOpened"))
	void OnOpen(const FInstancedStruct& Params);

	/**
	 * OnDeinitialize 时被调用，在蓝图中使用
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MetaDataOfAssets", meta = (DisplayName = "OnWidgetClosed"))
	void OnClose();
	
	// Construct 时的初始化
	virtual void OnInitialize(const FInstancedStruct& Params);

	// Destruct 时的清理
	virtual void OnDeinitialize();
	
};
