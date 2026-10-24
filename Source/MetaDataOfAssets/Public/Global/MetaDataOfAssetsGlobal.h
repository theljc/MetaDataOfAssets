#pragma once

#include "CoreMinimal.h"


namespace MetaDataOfAssetsConstant
{
	// 自定义控制台命令
	inline const FString& GetEnableDebugConsoleCommand()
	{
		static const FString EnableConsoleCmd(TEXT("MetaDataOfAssets.EnableDebug"));
		return EnableConsoleCmd;
	}

	inline const FString& GetDisableDebugConsoleCommand()
	{
		static const FString DisableConsoleCmd(TEXT("MetaDataOfAssets.DisableDebug"));
		return DisableConsoleCmd;
	}
	
}
