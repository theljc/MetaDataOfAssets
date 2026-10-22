// WidgetInterface.h — 提供所有控件都需要的通用接口

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StructUtils/InstancedStruct.h"
#include "WidgetInterface_MetaDataPlugin.generated.h"


UINTERFACE()
class TEST_API UWidgetInterface_MetaDataPlugin : public UInterface
{
	GENERATED_BODY()
};

/**
 * 控件在初始化或被关闭时会调用 OnInitialize 和 OnDeinitialize
 * 提供 OnOpen 和 OnClose 用于蓝图
 */
class TEST_API IWidgetInterface_MetaDataPlugin
{
	GENERATED_BODY()

public:
	
	/**
	 * OnInitialize 时被调用，在蓝图中使用
	 * 
	 * @param Params 创建时需要的参数
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MetaDataPlugin", meta = (DisplayName = "OnWidgetOpened"))
	void OnOpen(const FInstancedStruct& Params);

	/**
	 * OnDeinitialize 时被调用，在蓝图中使用
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MetaDataPlugin", meta = (DisplayName = "OnWidgetClosed"))
	void OnClose();
	
	// Construct 时的初始化
	virtual void OnInitialize(const FInstancedStruct& Params);

	// Destruct 时的清理
	virtual void OnDeinitialize();
	
};
