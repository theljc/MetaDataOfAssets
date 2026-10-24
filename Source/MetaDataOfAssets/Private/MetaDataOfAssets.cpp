// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaDataOfAssets.h"

#include "MetaData/EUSS_MetaDataManager.h"
#include "LevelEditor.h"
#include "Command/MetaDataOfAssetsCommands.h"
#include "Style/MetaDataOfAssetsStyle.h"
#include "Settings/MetaDataOfAssetsSettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widget/EUSS_WidgetManager.h"

#include "PropertyEditorModule.h"
#include "Global/MetaDataOfAssetsGlobal.h"
#include "Interfaces/IMainFrameModule.h"
#include "Internationalization/StringTableRegistry.h"


#define LOCTEXT_NAMESPACE "FtestModule"

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
		// 项目设置页的改动保存到插件自己的 ini 配置文件
		SettingsSection->OnSave().BindLambda([]() -> bool
		{
			return GetMutableDefault<UMetaDataOfAssetsSettings>()->TryUpdateDefaultConfigFile(
				UMetaDataOfAssetsSettings::GetConfigFilePath());
		});
	}
	

	// 加载 csv 文件
	LOCTABLE_FROMFILE_GAME(
		"MetaDataPluginTableID",
		"MetaDataOfAssetsNamespace",
		"../Plugins/MetaDataOfAssets/Content/Localization/MetadataPlugin.csv"
	);

	
	// 注册控制台命令
	RegisterCustomConsoleCommand();
	
	// 绑定快捷键和对应的操作
	BindCommandActions();
	

	// 获得关卡编辑器模块
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	// 扩展关卡编辑器的工具栏
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	// 将按钮添加在名为 "Play" 的工具栏区块之后
	ToolbarExtender->AddToolBarExtension("Play"
		, EExtensionHook::After
		, PluginCommands
		, FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& Builder)
			{
				Builder.AddToolBarButton(FMetaDataOfAssetsCommands::Get().Command_OpenMain);
				// Builder.AddToolBarButton(FTestCommands::Get().Command_CloseAll);
			}));

	// 使按钮出现在工具栏
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	
	// 获得资产注册表模块
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	// 绑定资产删除事件
	AssetRegistry.OnAssetRemoved().AddRaw(this, &FMetaDataOfAssetsModule::OnAssetRemoved);
	// 移动资产时也会触发重命名事件
	AssetRegistry.OnAssetRenamed().AddRaw(this, &FMetaDataOfAssetsModule::OnAssetMoved);

}

void FMetaDataOfAssetsModule::ShutdownModule()
{
	// 取消注册控制台命令
	IConsoleManager::Get().UnregisterConsoleObject(*MetaDataOfAssetsConstant::GetDisableDebugConsoleCommand());
	IConsoleManager::Get().UnregisterConsoleObject(*MetaDataOfAssetsConstant::GetEnableDebugConsoleCommand());
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
		*MetaDataOfAssetsConstant::GetDisableDebugConsoleCommand(),
		TEXT("Disable UE_LOG and PrintString of the MetaDataPlugin"),
		FConsoleCommandDelegate::CreateRaw(this, &FMetaDataOfAssetsModule::DisableDebug),
		ECVF_Default
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		*MetaDataOfAssetsConstant::GetEnableDebugConsoleCommand(),
		TEXT("Enable UE_LOG and PrintString of the MetaDataPlugin"),
		FConsoleCommandDelegate::CreateRaw(this, &FMetaDataOfAssetsModule::EnableDebug),
		ECVF_Default
	);
}

void FMetaDataOfAssetsModule::DisableDebug()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->DisableDebug();
	}
}

void FMetaDataOfAssetsModule::EnableDebug()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->EnableDebug();
	}
}

void FMetaDataOfAssetsModule::RemoveAllWidget()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->CloseAllWidgets();
	}
}

void FMetaDataOfAssetsModule::OnButtonClicked()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		// 点击按钮时，创建主窗口
		WidgetManager->CreateMainWidget();
	}
}

void FMetaDataOfAssetsModule::OnAssetMoved(const FAssetData& AssetData, const FString& OldObjectPath)
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->OnMainWidgetMoved(AssetData, OldObjectPath);
	}
}

void FMetaDataOfAssetsModule::OnAssetRemoved(const FAssetData& AssetData)
{
	UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
	if (MetaDataManager)
	{
		MetaDataManager->OnAssetRemoved(AssetData);
	}
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetaDataOfAssetsModule, MetaDataOfAssets)
