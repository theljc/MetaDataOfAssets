// Copyright 2026 ljc

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * 单例模式
 */
class FMetaDataOfAssetsStyle
{
public:
	// 初始化，在模块启动时调用
	static void Initialize();

	// 关闭模块时调用
	static void Shutdown();

	// 返回 style 的名字
	static FName GetStyleSetName();

	// 获得插件的资源目录
	static FString GetPluginResourcesDir(const FString& ModuleName);

	// 获得 Style 单例
	static FSlateStyleSet& Get();

private:

	// 图标大小
	static const FVector2D Icon128X128;
	
	// 创建单例
	static TSharedRef<FSlateStyleSet> CreateStyle();

	// 保存单例对象
	static TSharedPtr<FSlateStyleSet> StyleInstance;

	// 禁止实例化
	FMetaDataOfAssetsStyle() = delete;
	FMetaDataOfAssetsStyle(const FMetaDataOfAssetsStyle&) = delete;
	FMetaDataOfAssetsStyle& operator=(const FMetaDataOfAssetsStyle&) = delete;
	
};
