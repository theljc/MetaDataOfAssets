#include "Style/MetaDataPluginStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Log/MetaDataPluginLogChannel.h"

TSharedPtr< FSlateStyleSet > FMetadataOfAssetsStyle::StyleInstance = NULL;
const FVector2D Icon128x128(128.0f, 128.0f);

void FMetadataOfAssetsStyle::Initialize()
{
	// 初始化时创建单例并注册
	if (!StyleInstance.IsValid())
	{
		StyleInstance = CreateStyle();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMetadataOfAssetsStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FMetadataOfAssetsStyle::GetStyleSetName()
{
	// 使用静态变量，避免每次调用都构造 FName
	static FName StyleSetName(TEXT("MetadataOfAssetsStyle"));
	return StyleSetName;
}

FSlateStyleSet& FMetadataOfAssetsStyle::Get()
{
	check(StyleInstance.IsValid());
	return *StyleInstance;
}

FString FMetadataOfAssetsStyle::GetPluginResourcesDir(const FString& ModuleName)
{
	// 查找插件
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(ModuleName);
	if (!Plugin.IsValid())
	{
		UE_LOG(LogMetaDataPlugin, Error, TEXT("[%hs] : 找不到插件 %s"), __FUNCTION__, *ModuleName);
		return TEXT("");
	}

	// 返回插件的 Resources 目录
	return Plugin->GetBaseDir() / TEXT("Resources");
}

TSharedRef< FSlateStyleSet > FMetadataOfAssetsStyle::CreateStyle()
{
	// 创建一个新的 Style 实例
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	// TODO: 模块重命名
	// 获得插件的 Resources 目录的路径
	FString PluginResourcesDirectoryPath = GetPluginResourcesDir(TEXT("test"));
	
	// 设置资源根目录为 Resources 目录
	Style->SetContentRoot(PluginResourcesDirectoryPath);

	// 设置图标，命令集名称 + FUICommandInfo 变量名
	Style->Set(FName(TEXT("MetadataOfAssets.Command_OpenMain")), new FSlateImageBrush(PluginResourcesDirectoryPath + TEXT("/Icon_MetadataOfAssets.png"), Icon128x128));

	return Style;
}
