// Copyright 2026 ljc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UEUSS_MetaDataManager;
class UEUSS_WidgetManager;


namespace MetaDataOfAssetsGlobal
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
	
	UEUSS_WidgetManager* GetWidgetManagerSafe();
	UEUSS_MetaDataManager* GetMetaDataManagerSafe();
	
}
