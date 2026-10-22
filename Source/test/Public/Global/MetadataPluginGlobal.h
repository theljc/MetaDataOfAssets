#pragma once

#include "CoreMinimal.h"


namespace MetadataOfAssetsConstant
{
	// 字符串表 ID
	// inline const FName& GetTableID()
	// {
	// 	static const FName ID("MetadataPluginTableID");
	// 	return ID;
	// }

	// 字符串表命名空间
	// inline const FTextKey& GetNamespace()
	// {
	// 	static const FTextKey Namespace("MetadataPluginNamespace");
	// 	return Namespace;
	// }

	// 字符串表文件路径
	// inline const FString& GetLocalizationFilePath()
	// {
	// 	static const FString LocalizationFilePath("MetaDataPlugin/Content/Localization/MetadataPlugin.csv");
	// 	return LocalizationFilePath;
	// }

	// 自定义控制台命令
	inline const FString& GetEnableDebugConsoleCommand()
	{
		static const FString EnableConsoleCmd(TEXT("MetaDataPlugin.EnableDebug"));
		return EnableConsoleCmd;
	}

	inline const FString& GetDisableDebugConsoleCommand()
	{
		static const FString DisableConsoleCmd(TEXT("MetaDataPlugin.DisableDebug"));
		return DisableConsoleCmd;
	}
	
}
