// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetadataOfAssets.h"

// #include "BlueprintEditorLibrary.h"
// #include "EditorUtilityWidgetBlueprint.h"
#include "MetaData/EUSS_MetaDataManager.h"
#include "LevelEditor.h"
#include "Command/MetadataPluginCommands.h"
#include "Style/MetaDataPluginStyle.h"
#include "Settings/MetaDataPluginSettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widget/EUSS_WidgetManager.h"

// 属性自定义
#include "PropertyEditorModule.h"
#include "Global/MetadataPluginGlobal.h"
#include "Interfaces/IMainFrameModule.h"
#include "Internationalization/StringTableRegistry.h"
// #include "Settings/MetaDataPluginSettings.h"
// #include "Task/FAssetScanTask.h"
// #include "ThumbnailRendering/ThumbnailManager.h"
// #include "../Public/Global/MetadataPluginGlobal.h"
// #include "Widgets/Layout/SConstraintCanvas.h"

#define LOCTEXT_NAMESPACE "FtestModule"

void FMetadataOfAssetsModule::StartupModule()
{
	// 从 TODO:<Plugin>/Config/MetaDataPluginConfig.ini 读取配置
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

	// 注册命令
	FMetadataOfAssetsCommands::Register();
	// 初始化样式集
	FMetadataOfAssetsStyle::Initialize();
	
	// 2. 注册 Tab 的生成器（通常在 StartupModule 中）
	// FGlobalTabmanager::Get()->RegisterTabSpawner(MyTabName, FOnSpawnTab::CreateRaw(this, &FtestModule::SpawnMyTab))
		// .SetDisplayName(FText::FromString("My Tab"));

	// 加载 csv 文件
	FStringTableRegistry::Get().Internal_LocTableFromFile(
		MetadataOfAssetsConstant::GetTableID(),
		MetadataOfAssetsConstant::GetNamespace(),
		FPaths::Combine(FPaths::ProjectPluginsDir(), MetadataOfAssetsConstant::GetLocalizationFilePath()),
		FPaths::ProjectPluginsDir()
	);
	
	// 创建命令列表
	PluginCommands = MakeShareable(new FUICommandList);
	
	// 用 MainFrame 模块，确保聚焦到 EUW 时也能触发快捷键
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
    
	// 获取 MainFrame 的命令列表
	PluginCommands = MainFrameModule.GetMainFrameCommandBindings();
	
	// 绑定快捷键和回调函数
	BindCommandActions();
	
	// 订阅项目设置变更 — 实现快捷键热重载（修改后立即生效，无需重启编辑器）
	// SettingsChangedHandle = GetMutableDefault<UtestSettings>()->OnSettingChanged().AddRaw(
	// 	this, &FtestModule::OnSettingsChanged);
	
	// 注册 FTestPluginChord 的属性自定义显示（Keyboard Shortcuts 风格）
	// FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// PropertyEditor.RegisterCustomPropertyTypeLayout(
	// 	FTestPluginChord::StaticStruct()->GetFName(),
		// FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTestPluginChordCustomization::MakeInstance));
	
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
	
	// 把自定义的命令，添加到关卡编辑器的全局命令列表中
	// TSharedRef<FUICommandList> LevelEditorCommandList = LevelEditorModule.GetGlobalLevelEditorActions();
	// LevelEditorCommandList->Append(PluginCommands.ToSharedRef());
	
	// 获取全局缩略图池
	// TSharedPtr<FAssetThumbnailPool> ThumbnailPool = UThumbnailManager::Get().GetSharedThumbnailPool(); // 或通过其他方式获取
	// //
	// if (ThumbnailPool.IsValid())
	// {
	// 	// 绑定渲染成功事件
	// 	ThumbnailPool->OnThumbnailRendered().AddLambda([](const FAssetData& AssetData)
	// 	{
	// 		// 缩略图渲染完成，可以执行刷新UI等操作
	// 		UE_LOG(LogTemp, Warning, TEXT("Thumbnail rendered for: %s"), *AssetData.AssetName.ToString());
	// 	});
	//
	// 	// 绑定渲染失败事件
	// 	ThumbnailPool->OnThumbnailRenderFailed().AddLambda([](const FAssetData& AssetData)
	// 	{
	// 		// 缩略图渲染失败，可以显示占位图或进行错误处理
	// 		UE_LOG(LogTemp, Error, TEXT("Thumbnail render failed for: %s"), *AssetData.AssetName.ToString());
	// 	});
	// }
	
	// 获得资产注册表模块
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	// 绑定资产删除事件
	AssetRegistry.OnAssetRemoved().AddRaw(this, &FMetadataOfAssetsModule::OnAssetRemoved);
	// 移动资产时也会触发重命名事件
	AssetRegistry.OnAssetRenamed().AddRaw(this, &FMetadataOfAssetsModule::OnAssetMoved);
	// 资产保存时，扫描并更新元数据
	// UPackage::PackageSavedWithContextEvent.AddRaw(this, &FMetadataOfAssetsModule::OnAssetSaved);
	
	// 引擎初始化完成后的回调
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FMetadataOfAssetsModule::OnPostEngineInit);

}

void FMetadataOfAssetsModule::ShutdownModule()
{
	// 取消订阅项目设置变更
	// if (SettingsChangedHandle.IsValid())
	// {
	// 	GetMutableDefault<UtestSettings>()->OnSettingChanged().Remove(SettingsChangedHandle);
	// 	SettingsChangedHandle.Reset();
	// }

	// 取消注册属性自定义
	// if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	// {
	// 	FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// 	PropertyEditor.UnregisterCustomPropertyTypeLayout(FTestPluginChord::StaticStruct()->GetFName());
	// }
	
	FMetadataOfAssetsCommands::Unregister();
	FMetadataOfAssetsStyle::Shutdown();
}

// ==================== 快捷键热重载 ====================

// void FtestModule::OnSettingsChanged(UObject* SettingsObject, FPropertyChangedEvent& PropertyChangedEvent)
// {
// 	// 项目设置中快捷键被修改 → 立即重新绑定
// 	UE_LOG(LogTemp, Log, TEXT("[FtestModule] 项目设置快捷键已变更，热重载中..."));
// 	UpdateShortcutBindings();
// }

// 3. 实现 Spawn 回调，返回一个 SDockTab
// TSharedRef<SDockTab> FtestModule::SpawnMyTab(const FSpawnTabArgs& Args)
// {
// 	UE_LOG(LogTemp, Warning, TEXT("%s"), *Args.GetTabId().TabType.ToString());
// 	
// 	return SNew(SDockTab)
// 		.Label(FText::FromString("New Tab"))
// 		[
// 			SNew(SConstraintCanvas)
// 			+ SConstraintCanvas::Slot()
// 			.Alignment(0.5f)
// 			.Anchors(0.5f)
// 			.Offset(FMargin(0.0f, 0.0f, 300.f, 100.f))
// 			.AutoSize(true)
// 			[
// 				SNew(STextBlock)
// 				.Text(FText::FromString("This is a Tab content!"))
// 			]
// 		];
// }

// void FtestModule::TestAction()
// {
// 	TSharedPtr<SDockTab> NewTab = FGlobalTabmanager::Get()->TryInvokeTab(MyTabName);
// }

void FMetadataOfAssetsModule::BindCommandActions()
{
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

void FMetadataOfAssetsModule::ScanAssetsWithConfiguration()
{
	// UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
	// if (MetaDataManager)
	// {
	// 	MetaDataManager->SyncAssetsInDirectory();
	// }
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
	// 点击按钮时，获得子系统，并创建主窗口
	UEUSS_WidgetManager* WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
	if (WidgetManager)
	{
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
	
	// 获取配置文件路径和主控件资产路径
	// FString ConfigPath = MetadataOfAssetsUtils::GetNormalizedConfigIniPath();
	// FString WidgetPath = MetadataOfAssetsUtils::GetWidgetPathFromConfigFile();

	// UMetaDataPluginSettings* Settings = MetadataOfAssetsUtils::GetMetaDataPluginSettings();
	
	// 没有值时，表示未移动过资产，需要设置一个默认值，用于后续判断移动的资产是否是主控件的资产
	// if (WidgetPath.IsEmpty())
	// {
	// 	WidgetPath = TEXT("/test/元数据Widget/EUWBP_MetaData.EUWBP_MetaData");
	// }
	
	// 判断移动的是否是主控件的资产
	// if (Settings->MainWidgetPath == OldObjectPath)
	// {
	// 	// 保存新的路径
	// 	// GConfig->SetString(TEXT("MetaDataPluginConfig"), TEXT("CurrentAssetPath"), *AssetData.GetSoftObjectPath().ToString(), ConfigPath);
	//
	// 	// 刷新配置文件
	// 	// GConfig->Flush(true, ConfigPath);
	//
	// 	// 保存主控件资产的路径
	// 	Settings->SaveMainWidgetPath(*AssetData.GetSoftObjectPath().ToString());
	// }
	
}

void FMetadataOfAssetsModule::OnAssetRemoved(const FAssetData& AssetData)
{
	UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
	if (MetaDataManager)
	{
		MetaDataManager->OnAssetRemoved(AssetData);
	}
}

// void FMetadataOfAssetsModule::OnAssetSaved(const FString& String, UPackage* Package, FObjectPostSaveContext Context)
// {
	// UEUSS_MetaDataManager* MetaDataManager = GEditor->GetEditorSubsystem<UEUSS_MetaDataManager>();
	// if (MetaDataManager)
	// {
	// 	MetaDataManager->SyncAsset(Package->FindAssetInPackage());
	// }
// }

// void FMetadataOfAssetsModule::LoadMainEUWBP()
// {
// 	UEUSS_WidgetManager* EUSS_WidgetManager = GEditor->GetEditorSubsystem<UEUSS_WidgetManager>();
// 	if (EUSS_WidgetManager)
// 	{
// 		// const FString Path = FPaths::ProjectDir() + TEXT("Config/DefaultGame.ini");
//
// 		// 读取配置文件的主窗口资产的路径
// 		// GConfig->GetString(TEXT("MetaDataPluginConfig"),
// 		//                    TEXT("CurrentAssetPath"),
// 		//                    CurrentPath,
// 		//                    Path  // 指向项目的 DefaultGame.ini
// 		// );
//
// 		// 设置一个默认值
// 		// FSoftObjectPath EUWBP_Path(TEXT("/test/元数据Widget/EUWBP_MetaData.EUWBP_MetaData"));
//
// 		// 判断是否读取到了配置文件中的资产路径
// 		// if (!CurrentPath.IsEmpty())
// 		// {
// 		// 	// 读取成功，则将配置文件中的资产路径设置为主窗口的资产路径，失败则使用默认值
// 		// 	EUWBP_Path = FSoftObjectPath(CurrentPath);
// 		// }
//
// 		// 赋值给软引用对象
// 		// EUSS_WidgetManager->Set_EUWBP(TSoftObjectPtr<UEditorUtilityWidgetBlueprint>(EUWBP_Path));
//
// 		// 加载资产才能使软引用一开始就生效
// 		// EUSS_WidgetManager->GetMainWidgetSoftPtr().LoadSynchronous();
// 		
// 	}
// }

void FMetadataOfAssetsModule::OnPostEngineInit()
{
	// LoadMainEUWBP();

	// DeleteMode
	ScanAssetsWithConfiguration();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetadataOfAssetsModule, test)
