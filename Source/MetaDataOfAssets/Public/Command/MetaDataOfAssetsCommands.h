// Copyright 2026 ljc

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * 注册 Command_OpenMain 和 Command_CloseAll 两个命令
 */
class METADATAOFASSETS_API FMetaDataOfAssetsCommands : public TCommands<FMetaDataOfAssetsCommands>
{
	
public:
	
	FMetaDataOfAssetsCommands();

	// 注册命令
	virtual void RegisterCommands() override;

	// 打开主窗口命令
	TSharedPtr<FUICommandInfo> Command_OpenMain;
	
	// 关闭所有控件命令
	TSharedPtr<FUICommandInfo> Command_CloseAll;
	
};
