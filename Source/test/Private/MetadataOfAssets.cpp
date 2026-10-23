// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetadataOfAssets.h"

#include "MetaData/EUSS_MetaDataManager.h"
#include "LevelEditor.h"
#include "Command/MetadataPluginCommands.h"
#include "Style/MetaDataPluginStyle.h"
#include "Settings/MetaDataPluginSettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widget/EUSS_WidgetManager.h"

#include "PropertyEditorModule.h"
#include "Global/MetadataPluginGlobal.h"
#include "Interfaces/IMainFrameModule.h"
#include "Internationalization/StringTableRegistry.h"


#define LOCTEXT_NAMESPACE "FtestModule"

void FMetadataOfAssetsModule::StartupModule()
{
	// 注册命令
	FMetadataOfAssetsCommands::Register();
	// 初始化样式集
	FMetadataOfAssetsStyle::Initialize();

	
	// TODO:从 <Plugin>/Config/MetaDataPluginConfig.ini 读取配置
	UMetaDataPluginSettings* PluginSettings = GetMutableDefault<UMetaDataPluginSettings>();
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
			return GetMutableDefault<UMetaDataPluginSettings>()->TryUpdateDefaultConfigFile(
				UMetaDataPluginSettings::GetConfigFilePath());
		});
	}
	

	// FStringTableRegistry::Get().Internal_LocTableFromFile(
	// 	MetadataOfAssetsConstant::GetTableID(),
	// 	MetadataOfAssetsConstant::GetNamespace(),
	// 	FPaths::Combine(FPaths::ProjectPluginsDir(), MetadataOfAssetsConstant::GetLocalizationFilePath()),
	// 	FPaths::ProjectPluginsDir()
	// );

	// 加载 csv 文件
	LOCTABLE_FROMFILE_GAME(
		"MetadataPluginTableID",
		"MetadataPluginNamespace",
		"../Plugins/MetaDataPlugin/Content/Localization/MetadataPlugin.csv"
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
				Builder.AddToolBarButton(FMetadataOfAssetsCommands::Get().Command_OpenMain);
				// Builder.AddToolBarButton(FTestCommands::Get().Command_CloseAll);
			}));

	// 使按钮出现在工具栏
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	
	// 获得资产注册表模块
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	// 绑定资产删除事件
	AssetRegistry.OnAssetRemoved().AddRaw(this, &FMetadataOfAssetsModule::OnAssetRemoved);
	// 移动资产时也会触发重命名事件
	AssetRegistry.OnAssetRenamed().AddRaw(this, &FMetadataOfAssetsModule::OnAssetMoved);

}

void FMetadataOfAssetsModule::ShutdownModule()
{
	IConsoleManager::Get().UnregisterConsoleObject(*MetadataOfAssetsConstant::GetDisableDebugConsoleCommand());
	IConsoleManager::Get().UnregisterConsoleObject(*MetadataOfAssetsConstant::GetEnableDebugConsoleCommand());
	FMetadataOfAssetsCommands::Unregister();
	FMetadataOfAssetsStyle::Shutdown();
}

void FMetadataOfAssetsModule::BindCommandActions()
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
		FMetadataOfAssetsCommands::Get().Command_OpenMain,
		FExecuteAction::CreateRaw(this, &FMetadataOfAssetsModule::OnButtonClicked));

	PluginCommands->MapAction(
		FMetadataOfAssetsCommands::Get().Command_CloseAll,
		FExecuteAction::CreateRaw(this, &FMetadataOfAssetsModule::RemoveAllWidget));
	
}

void FMetadataOfAssetsModule::RegisterCustomConsoleCommand()
{
	IConsoleManager::Get().RegisterConsoleCommand(
		*MetadataOfAssetsConstant::GetDisableDebugConsoleCommand(),
		TEXT("Disable UE_LOG and PrintString of the MetaDataPlugin"),
		FConsoleCommandDelegate::CreateRaw(this, &FMetadataOfAssetsModule::DisableDebug),
		ECVF_Default
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		*MetadataOfAssetsConstant::GetEnableDebugConsoleCommand(),
		TEXT("Enable UE_LOG and PrintString of the MetaDataPlugin"),
		FConsoleCommandDelegate::CreateRaw(this, &FMetadataOfAssetsModule::EnableDebug),
		ECVF_Default
	);
}

void FMetadataOfAssetsModule::DisableDebug()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->DisableDebug();
	}
}

void FMetadataOfAssetsModule::EnableDebug()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->EnableDebug();
	}
}

void FMetadataOfAssetsModule::RemoveAllWidget()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->CloseAllWidgets();
	}
}

void FMetadataOfAssetsModule::OnButtonClicked()
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		// 点击按钮时，创建主窗口
		WidgetManager->CreateMainWidget();
	}
}

void FMetadataOfAssetsModule::OnAssetMoved(const FAssetData& AssetData, const FString& OldObjectPath)
{
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
		WidgetManager->OnMainWidgetMoved(AssetData, OldObjectPath);
	}
}

void FMetadataOfAssetsModule::OnAssetRemoved(const FAssetData& AssetData)
{
	UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
	if (MetaDataManager)
	{
		MetaDataManager->OnAssetRemoved(AssetData);
	}
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetadataOfAssetsModule, test)
