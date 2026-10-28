// Copyright 2026 ljc

#include "Command/MetaDataOfAssetsCommands.h"

#include "Style/MetaDataOfAssetsStyle.h"

#define LOCTEXT_NAMESPACE "MetaDataOfAssetsCommands"

FMetaDataOfAssetsCommands::FMetaDataOfAssetsCommands() : TCommands<FMetaDataOfAssetsCommands>(
	"MetaDataOfAssets",														// 命令集名称
	LOCTEXT("MetaDataOfAssetsCommands", "MetaDataOfAssets Shortcut Key"),	// 显示名称
	NAME_None,																// 父命令集
	FMetaDataOfAssetsStyle::GetStyleSetName())								// 关联的样式集
{
	
}

void FMetaDataOfAssetsCommands::RegisterCommands()
{
	// Editor Preferences->Keyboard Shortcuts->MetaDataOfAssets Shortcut Key
	// 编辑器偏好设置->快捷按键->资产元数据快捷键
	UI_COMMAND(Command_OpenMain, "Open Main Widget", "Open Main Widget as a Floating Window", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::Z));
	UI_COMMAND(Command_CloseAll, "Close All Widgets", "Close All Opened Widgets", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::Z));
}

#undef LOCTEXT_NAMESPACE
