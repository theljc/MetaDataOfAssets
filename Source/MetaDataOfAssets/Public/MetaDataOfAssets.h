// Copyright 2026 ljc. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"

class FUICommandList;
class FExtender;
struct FCanLoadMap;


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
	void OnPackageDeleted(UPackage* InPackage);

	// 编辑器初始化完成的回调
	void OnEditorInitialized(double Double);

	// 关卡加载前关闭所有控件
	void OnMapLoad(const FString& InFilename, FCanLoadMap& OutCanLoadMap);

	// 关闭所有创建的窗口
	void RemoveAllWidget();

	// 绑定快捷键对应的操作
	void BindCommandActions();

	// 注册自定义控制台命令
	void RegisterCustomConsoleCommand();

	// 开启和关闭调试模式
	void DisableDebug();
	void EnableDebug();

private:
	
	// 绑定命令和具体的操作
	TSharedPtr<FUICommandList> PluginCommands;

	// 关卡编辑器工具栏扩展
	TSharedPtr<FExtender> ToolbarExtender;

	// 回调事件句柄
	FDelegateHandle AssetRemovedDelegateHandle;
	FDelegateHandle AssetRenamedDelegateHandle;
	FDelegateHandle EditorInitializedDelegateHandle;
	FDelegateHandle MapLoadDelegateHandle;

	// 设置页注册信息，关闭模块时不再访问 Settings 的 CDO
	FName SettingsContainerName;
	FName SettingsCategoryName;
	FName SettingsSectionName;
	bool bSettingsRegistered = false;

};
