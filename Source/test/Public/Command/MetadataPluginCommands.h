#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * 注册 Command_OpenMain 和 Command_CloseAll 两个命令
 */
class TEST_API FMetadataOfAssetsCommands : public TCommands<FMetadataOfAssetsCommands>
{
public:
	FMetadataOfAssetsCommands();

	// 注册命令
	virtual void RegisterCommands() override;

	// 打开主窗口命令
	TSharedPtr<FUICommandInfo> Command_OpenMain;
	// 关闭所有控件命令
	TSharedPtr<FUICommandInfo> Command_CloseAll;
	
};
