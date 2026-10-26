#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetaDataOfAssetsSettings.generated.h"


USTRUCT(BlueprintType)
struct FScanDirectorySetting
{
	GENERATED_BODY()

	// 扫描路径
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MetaDataOfAssets", meta = (ContentDir, DisplayName = "Directory"))
	FDirectoryPath Directory;

	// 是否递归
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MetaDataOfAssets", meta = (DisplayName = "Recursive"))
	bool bRecursive = false;
};


/**
 * 插件的项目配置
 */
UCLASS(Config=MetaDataOfAssets)
class METADATAOFASSETS_API UMetaDataOfAssetsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMetaDataOfAssetsSettings() {};
	
	// 用于确定是否要自动删除资产注册表中的 MetaData Tag，修改配置需要重启编辑器
	UPROPERTY(Config, EditAnywhere, Category = "MetaDataOfAssets", meta = (ConfigRestartRequired = true))
	bool bAllowManageAssetRegistryTagsDelete = false;

	// 扫描配置
	UPROPERTY(Config, EditAnywhere, Category = "MetaDataOfAssets", meta = (EditCondition = "bAllowManageAssetRegistryTagsDelete"))
	TArray<FScanDirectorySetting> ScanSettings;

	// 是否启用调试
	UPROPERTY(Config)
	bool bEnableDebug = true;
	
	// 保存主控件对应的资产路径
	UPROPERTY(Config)
	FString MainWidgetPath;

	// 设置是否启用打印字符串
	void SetEnableDebug(bool bInEnablePrintString);
	
	// 更新配置文件
	void SetMainWidgetPath(const FString& InMainWidgetPath);
	
	// 获得插件的配置文件路径：<Plugin>/Config/DefaultMetaDataOfAssets.ini
	static FString GetConfigFilePath();

	// 从插件自己的 ini 读取配置，插件模块启动时调用
	void LoadFromPluginConfig();

	// Project Settings -> Plugins -> MetaDataOfAssets
	// 项目设置 -> 插件 -> 资产元数据
	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
	
#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("MetaDataOfAssets", "MetaDataOfAssetsSettingsSection", "MetaDataOfAssets"); }

	// 项目设置页由插件自己注册，不使用 UDeveloperSettings 的自动注册
	virtual bool SupportsAutoRegistration() const override { return false; }
#endif
	
};

