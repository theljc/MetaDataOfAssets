#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetaDataPluginSettings.generated.h"


USTRUCT(BlueprintType)
struct FScanDirectorySetting
{
	GENERATED_BODY()

	// 扫描路径
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ContentDir, DisplayName = "Directory"))
	FDirectoryPath Directory;

	// 是否递归
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "Recursive"))
	bool bRecursive = false;
};

/**
 * 插件的项目配置
 * TODO:Config=MetaDataPlugin
 */
UCLASS(Config=MetaDataPlugin)
class TEST_API UMetaDataPluginSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMetaDataPluginSettings() {};
	
	// 获得插件的配置文件路径：TODO:<Plugin>/Config/MetaDataPluginConfig.ini
	static FString GetConfigFilePath();

	// 从插件自己的 ini 读取配置，插件模块启动时调用
	void LoadFromPluginConfig();

#if WITH_EDITOR
	// 项目设置页由插件自己注册，不使用 UDeveloperSettings 的自动注册
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif

	// DeleteMode
	// 用于确定是否要自动删除资产注册表中的 MetaData Tag，修改配置需要重启编辑器
	UPROPERTY(Config, EditAnywhere, meta=(ConfigRestartRequired=true))
	bool bAllowManageAssetRegistryTagsDelete = false;

	// DeleteMode
	// 扫描路径
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "ScanDirectorySetting", EditCondition = "bAllowManageAssetRegistryTagsDelete"))
	TArray<FScanDirectorySetting> ScanDirectory;

	// DeleteMode
	// 每次扫描的资产数量
	// UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Batch Size", EditCondition = "bAllowManageAssetRegistryTagsDelete", ClampMin = "1", ClampMax = "32"))
	// int32 BatchSize = 32;

	// UPROPERTY(Config, EditAnywhere)
	// TSoftObjectPtr<UEditorUtilityWidgetBlueprint> EUWBP_MetaData;

	
	// 保存主控件对应的资产路径
	UPROPERTY(Config)
	FString MainWidgetPath;

	// 更新配置文件
	void SaveMainWidgetPath(const FString& InMainWidgetPath);
	
	// Project Settings -> Plugins -> MetaDataPlugin
	// 项目设置 -> 插件 -> 元数据插件
	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("MetaDataPlugin", "MetaDataPluginSettingsSection", "MetaDataPlugin"); }
#endif
	
};

