#include "Command/MetadataPluginCommands.h"

#include "Style/MetaDataPluginStyle.h"

#define LOCTEXT_NAMESPACE "MetadataOfAssetsCommands"

FMetadataOfAssetsCommands::FMetadataOfAssetsCommands() : TCommands<FMetadataOfAssetsCommands>(
	"MetadataOfAssets",														// 命令集名称
	LOCTEXT("MetadataOfAssetsCommands", "MetadataPlugin Shortcut Key"),		// 显示名称
	NAME_None,																// 父命令集
	FMetadataOfAssetsStyle::GetStyleSetName())								// 关联的样式集
{
	
}

void FMetadataOfAssetsCommands::RegisterCommands()
{
	// Editor Preferences->Keyboard Shortcuts->MetaData Plugin Shortcut
	// 编辑器偏好设置->快捷按键->元数据插件快捷键
	UI_COMMAND(Command_OpenMain, "Open Main Widget", "Open Main Widget as a Floating Window", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::Z));
	UI_COMMAND(Command_CloseAll, "Close All Widgets", "Close All Opened Widgets", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::Z));
}

#undef LOCTEXT_NAMESPACE
