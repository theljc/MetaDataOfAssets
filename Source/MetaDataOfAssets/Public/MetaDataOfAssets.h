// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"

class FUICommandList;


class FMetaDataOfAssetsModule : public IModuleInterface
{
	
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// 点击插件的按钮
	void OnButtonClicked();

	// 资产移动回调
	void OnAssetMoved(const FAssetData& AssetData, const FString& OldObjectPath);

	// 资产删除回调
	void OnAssetRemoved(const FAssetData& AssetData);

	// 关闭所有创建的窗口
	void RemoveAllWidget();

	// 绑定快捷键对应的操作
	void BindCommandActions();

	// 注册自定义控制台命令
	void RegisterCustomConsoleCommand();
	void DisableDebug();
	void EnableDebug();

private:
	
	// 绑定命令和具体的操作
	TSharedPtr<FUICommandList> PluginCommands;

};
