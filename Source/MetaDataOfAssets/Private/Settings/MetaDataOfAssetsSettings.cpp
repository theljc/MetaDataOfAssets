#include "Settings/MetaDataOfAssetsSettings.h"

#include "Interfaces/IPluginManager.h"
#include "Log/MetaDataOfAssetsLogChannel.h"
#include "Misc/Paths.h"


FString UMetaDataOfAssetsSettings::GetConfigFilePath()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("MetaDataOfAssets"));
	const FString ConfigDir = Plugin.IsValid() ? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config")) : FPaths::ProjectConfigDir();

	return FPaths::ConvertRelativePathToFull(FPaths::Combine(ConfigDir, TEXT("DefaultMetaDataOfAssets.ini")));
}

void UMetaDataOfAssetsSettings::LoadFromPluginConfig()
{
	const FString ConfigFilePath = GetConfigFilePath();
	if (!FPaths::FileExists(ConfigFilePath))
	{
		UE_LOG(LogMetaDataOfAssets, Warning, TEXT("[%hs] : 没有找到插件配置文件"), __FUNCTION__);
		return;
	}

	// 加载配置
	LoadConfig(UMetaDataOfAssetsSettings::StaticClass(), *ConfigFilePath);
	UE_LOG(LogMetaDataOfAssets, Log, TEXT("[%hs] : 已从插件配置文件加载：%s"), __FUNCTION__, *ConfigFilePath);
}

void UMetaDataOfAssetsSettings::SetEnableDebug(bool bInEnableDebug)
{
	bEnableDebug = bInEnableDebug;
	// 更新配置文件
	TryUpdateDefaultConfigFile(GetConfigFilePath());
}

void UMetaDataOfAssetsSettings::SetMainWidgetPath(const FString& InMainWidgetPath)
{
	MainWidgetPath = InMainWidgetPath;
	// 更新配置文件
	TryUpdateDefaultConfigFile(GetConfigFilePath());
}
