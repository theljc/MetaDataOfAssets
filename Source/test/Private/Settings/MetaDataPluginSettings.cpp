#include "Settings/MetaDataPluginSettings.h"

#include "Interfaces/IPluginManager.h"
#include "Log/MetaDataPluginLogChannel.h"
#include "Misc/Paths.h"


FString UMetaDataPluginSettings::GetConfigFilePath()
{
	// TODO:插件重命名
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("test"));
	const FString ConfigDir = Plugin.IsValid() ? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config")) : FPaths::ProjectConfigDir();

	// TODO:配置文件重命名
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(ConfigDir, TEXT("MetaDataPlugin.ini")));
}

void UMetaDataPluginSettings::LoadFromPluginConfig()
{
	const FString ConfigFilePath = GetConfigFilePath();
	if (!FPaths::FileExists(ConfigFilePath))
	{
		UE_LOG(LogMetaDataPlugin, Warning, TEXT("[%hs] : 没有找到插件配置文件"), __FUNCTION__);
		return;
	}

	// 加载配置
	LoadConfig(UMetaDataPluginSettings::StaticClass(), *ConfigFilePath);
	UE_LOG(LogMetaDataPlugin, Log, TEXT("[%hs] : 已从插件配置文件加载：%s"), __FUNCTION__, *ConfigFilePath);
}

void UMetaDataPluginSettings::SetEnableDebug(bool bInEnableDebug)
{
	bEnableDebug = bInEnableDebug;
	// 更新配置文件
	TryUpdateDefaultConfigFile(GetConfigFilePath());
}

void UMetaDataPluginSettings::SetMainWidgetPath(const FString& InMainWidgetPath)
{
	MainWidgetPath = InMainWidgetPath;
	// 更新配置文件
	TryUpdateDefaultConfigFile(GetConfigFilePath());
}
