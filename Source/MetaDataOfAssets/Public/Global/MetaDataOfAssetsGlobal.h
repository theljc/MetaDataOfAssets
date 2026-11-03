// Copyright 2026 ljc. All rights reserved.

#pragma once

#include "CoreMinimal.h"


namespace MetaDataOfAssetsConstant
{
	// 自定义控制台命令
	inline const TCHAR* GetEnableDebugConsoleCommand()
	{
		return TEXT("MetaDataOfAssets.EnableDebug");
	}

	inline const TCHAR* GetDisableDebugConsoleCommand()
	{
		return TEXT("MetaDataOfAssets.DisableDebug");
	}
}
