// Copyright 2026 ljc. All rights reserved.

#include "MetaDataOfAssets.h"
#include "MetaData/EUSS_MetaDataManager.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "Command/MetaDataOfAssetsCommands.h"
#include "Style/MetaDataOfAssetsStyle.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widget/EUSS_WidgetManager.h"
#include "Global/MetaDataOfAssetsGlobal.h"
#include "Interfaces/IMainFrameModule.h"
// #include "Internationalization/StringTableRegistry.h"
// #include "Log/MetaDataOfAssetsLogChannel.h"


#define LOCTEXT_NAMESPACE "FMetaDataOfAssetsModule"

namespace
{
	UEUSS_MetaDataManager* GetMetaDataManagerSafe()
	{
		return GEditor
			? GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>()
			: nullptr;
	}

	UEUSS_WidgetManager* GetWidgetManagerSafe()
	{
		return GEditor
			? GEditor->GetEditorSubsystem<UEUSS_WidgetManager>()
			: nullptr;
	}
}

void FMetaDataOfAssetsModule::StartupModule()
{
	// 注册命令
	FMetaDataOfAssetsCommands::Register();
	// 初始化样式集
	FMetaDataOfAssetsStyle::Initialize();

	
	// 从 <Plugin>/Config/DefaultMetaDataOfAssets.ini 加载配置
	UMetaDataOfAssetsSettings* PluginSettings = GetMutableDefault<UMetaDataOfAssetsSettings>();
	PluginSettings->LoadFromPluginConfig();

	// 手动注册插件配置到项目设置页
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"));
	ISettingsSectionPtr SettingsSection = SettingsModule.RegisterSettings(
		PluginSettings->GetContainerName(),
		PluginSettings->GetCategoryName(),
		PluginSettings->GetSectionName(),
		PluginSettings->GetSectionText(),
		PluginSettings->GetSectionDescription(),
		PluginSettings);

	if (SettingsSection.IsValid())
	{
		// 缓存设置页标识，供 ShutdownModule 注销时使用
		SettingsContainerName = PluginSettings->GetContainerName();
		SettingsCategoryName = PluginSettings->GetCategoryName();
		SettingsSectionName = PluginSettings->GetSectionName();
		bSettingsRegistered = true;

		// 项目设置页的改动保存到插件自己的 ini 配置文件
		SettingsSection->OnSave().BindLambda([]() -> bool
		{
			return GetMutableDefault<UMetaDataOfAssetsSettings>()->TryUpdateDefaultConfigFile(
				UMetaDataOfAssetsSettings::GetConfigFilePath());
		});
	}

	
	// 注册控制台命令
	RegisterCustomConsoleCommand();
	
	// 绑定快捷键和对应的操作
	BindCommandActions();
	

	// 获得关卡编辑器模块
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	// 扩展关卡编辑器的工具栏
	ToolbarExtender = MakeShareable(new FExtender);

	// 将按钮添加在名为 "Play" 的工具栏区块之后
	ToolbarExtender->AddToolBarExtension("Play"
		, EExtensionHook::After
		, PluginCommands
		, FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& Builder)
			{
				Builder.AddToolBarButton(FMetaDataOfAssetsCommands::Get().Command_OpenMain);
			}));

	// 使按钮出现在工具栏
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	
	// 获得资产注册表模块
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// 绑定资产重命名事件，移动资产和重命名资产都会触发
	AssetRenamedDelegateHandle = AssetRegistry.OnAssetRenamed().AddRaw(
		this,
		&FMetaDataOfAssetsModule::OnAssetMoved);
	
	// 绑定具体资产的删除事件
	AssetRemovedDelegateHandle = AssetRegistry.OnAssetRemoved().AddRaw(
		this,
		&FMetaDataOfAssetsModule::OnAssetRemoved);
	
	// 编辑器初始化完成的回调
	EditorInitializedDelegateHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
		this,
		&FMetaDataOfAssetsModule::OnEditorInitialized);

	// 关卡切换前关闭所有控件，避免控件持有旧 World 导致世界泄漏
	MapLoadDelegateHandle = FEditorDelegates::OnMapLoad.AddRaw(
		this,
		&FMetaDataOfAssetsModule::OnMapLoad);
}

void FMetaDataOfAssetsModule::ShutdownModule()
{
	// 解除命令映射绑定
	if (PluginCommands.IsValid())
	{
		PluginCommands->UnmapAction(FMetaDataOfAssetsCommands::Get().Command_OpenMain);
		PluginCommands->UnmapAction(FMetaDataOfAssetsCommands::Get().Command_CloseAll);
		PluginCommands.Reset();
	}

	// 从关卡编辑器工具栏移除
	if (FLevelEditorModule* LevelEditorModule =
		FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
	{
		TSharedPtr<FExtensibilityManager> ToolbarManager =
			LevelEditorModule->GetToolBarExtensibilityManager();
		if (ToolbarManager.IsValid() && ToolbarExtender.IsValid())
		{
			ToolbarManager->RemoveExtender(ToolbarExtender);
		}
	}
	ToolbarExtender.Reset();

	// 解除 AssetRegistry 事件绑定
	if (FAssetRegistryModule* AssetRegistryModule =
		FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		if (AssetRemovedDelegateHandle.IsValid())
		{
			AssetRegistry.OnAssetRemoved().Remove(AssetRemovedDelegateHandle);
		}
		if (AssetRenamedDelegateHandle.IsValid())
		{
			AssetRegistry.OnAssetRenamed().Remove(AssetRenamedDelegateHandle);
		}
	}
	AssetRemovedDelegateHandle.Reset();
	AssetRenamedDelegateHandle.Reset();

	// 解除编辑器初始化完成事件绑定
	if (EditorInitializedDelegateHandle.IsValid())
	{
		FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedDelegateHandle);
		EditorInitializedDelegateHandle.Reset();
	}

	if (MapLoadDelegateHandle.IsValid())
	{
		FEditorDelegates::OnMapLoad.Remove(MapLoadDelegateHandle);
		MapLoadDelegateHandle.Reset();
	}

	// 从项目设置中移除本插件的设置页
	if (ISettingsModule* SettingsModule =
		FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
	{
		if (bSettingsRegistered)
		{
			SettingsModule->UnregisterSettings(
				SettingsContainerName,
				SettingsCategoryName,
				SettingsSectionName);
		}
	}
	
	bSettingsRegistered = false;

	// 取消注册控制台命令
	IConsoleManager::Get().UnregisterConsoleObject(MetaDataOfAssetsConstant::GetDisableDebugConsoleCommand());
	IConsoleManager::Get().UnregisterConsoleObject(MetaDataOfAssetsConstant::GetEnableDebugConsoleCommand());

	FMetaDataOfAssetsCommands::Unregister();
	FMetaDataOfAssetsStyle::Shutdown();
}

void FMetaDataOfAssetsModule::BindCommandActions()
{
	// 创建命令列表
	PluginCommands = MakeShareable(new FUICommandList);
	
	// 用 MainFrame 模块，确保聚焦不到编辑器主窗口时也能触发快捷键
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
    
	// 获取 MainFrame 的命令列表
	PluginCommands = MainFrameModule.GetMainFrameCommandBindings();
	
	if (!PluginCommands.IsValid())
	{
		return;
	}
	
	// 绑定执行函数
	PluginCommands->MapAction(
		FMetaDataOfAssetsCommands::Get().Command_OpenMain,
		FExecuteAction::CreateRaw(this, &FMetaDataOfAssetsModule::OnButtonClicked));

	PluginCommands->MapAction(
		FMetaDataOfAssetsCommands::Get().Command_CloseAll,
		FExecuteAction::CreateRaw(this, &FMetaDataOfAssetsModule::RemoveAllWidget));
	
}

void FMetaDataOfAssetsModule::RegisterCustomConsoleCommand()
{
	IConsoleManager::Get().RegisterConsoleCommand(
		MetaDataOfAssetsConstant::GetDisableDebugConsoleCommand(),
		TEXT("Disable UE_LOG and PrintString of the MetaDataPlugin"),
		FConsoleCommandDelegate::CreateRaw(this, &FMetaDataOfAssetsModule::DisableDebug),
		ECVF_Default
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		MetaDataOfAssetsConstant::GetEnableDebugConsoleCommand(),
		TEXT("Enable UE_LOG and PrintString of the MetaDataPlugin"),
		FConsoleCommandDelegate::CreateRaw(this, &FMetaDataOfAssetsModule::EnableDebug),
		ECVF_Default
	);
}

void FMetaDataOfAssetsModule::DisableDebug()
{
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (WidgetManager)
	{
		WidgetManager->DisableDebug();
	}
}

void FMetaDataOfAssetsModule::EnableDebug()
{
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (WidgetManager)
	{
		WidgetManager->EnableDebug();
	}
}

void FMetaDataOfAssetsModule::RemoveAllWidget()
{
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (WidgetManager)
	{
		WidgetManager->CloseAllWidgets();
	}
}

void FMetaDataOfAssetsModule::OnButtonClicked()
{
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (WidgetManager)
	{
		// 点击按钮时，创建主窗口
		WidgetManager->CreateMainWidget();
	}
}

void FMetaDataOfAssetsModule::OnAssetMoved(const FAssetData& AssetData, const FString& OldObjectPath)
{
	UEUSS_MetaDataManager* MetaDataManager = GetMetaDataManagerSafe();
	if (MetaDataManager)
	{
		MetaDataManager->OnAssetMoved(AssetData, OldObjectPath);
	}
	
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (WidgetManager)
	{
		WidgetManager->OnAssetMoved(AssetData, OldObjectPath);
	}
	
}

void FMetaDataOfAssetsModule::OnAssetRemoved(const FAssetData& AssetData)
{
	UEUSS_MetaDataManager* MetaDataManager = GetMetaDataManagerSafe();
	if (MetaDataManager)
	{
		MetaDataManager->OnAssetRemoved(AssetData);
	}
}

void FMetaDataOfAssetsModule::OnMapLoad(const FString& InFilename, FCanLoadMap& OutCanLoadMap)
{
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (WidgetManager)
	{
		WidgetManager->CloseAllWidgets();
	}
}

void FMetaDataOfAssetsModule::OnEditorInitialized(double Double)
{
	UEUSS_MetaDataManager* MetaDataManager = GetMetaDataManagerSafe();
	UEUSS_WidgetManager* WidgetManager = GetWidgetManagerSafe();
	if (MetaDataManager && WidgetManager)
	{
		WidgetManager->RegisterMainWidgetTab();
		MetaDataManager->SyncAssetsInDirectory();
	}
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetaDataOfAssetsModule, MetaDataOfAssets)
